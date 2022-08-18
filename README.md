# eNET_TCP_Server "aioenetd"
## Protocol 2.0 library for eNET- devices from ACCES.

This library code is intended to be used by a TCP Listener, aioenetd, running on an Intel Sitara AM64x processor's Linux image (Ubuntu, buildroot, Yocto, whatever).  As a secondary goal building this C++ Library into a .so or .dll for use from other OSes, including Windows, VxWorks, QNX, etc., consider portability concerns.

The code requires C++17, and perhaps even C++20 (I can't recall; it is currently *built* in C++20 but I'm not certain the current implementation *requires* more than C++17; this source has been iterating often prior to initial release.)

It provides a bridge between packets on Ethernet and FPGA-based DAQ resources on the eNET-device.

The library is responsible for
* turning byte streams / buffers (typically as received via TCP) in ACCES' eNET- Protocol 2.0 format into TMessage Objects
* constructing valid TMessage Objects from source code
* turning TMessage Objects into byte streams / buffers (typically for sending "Responses" across TCP to clients)

A Message is received in a standardized format as an array of bytes, generally across TCP.

This TMessage Library provides functions to:
* "deserialize" a Message: convert an array of bytes into a TMessage
* Build DataItems with zero or more bytes as the DataItem's `Data`
* Build Messages with zero or more DataItems as the Message `Payload`
* Append DataItems to already built Messages
* Validate a Message is well-formed (which also means all DataItems in its payload are well-formed)
* "serialize" a Message into an array of bytes (generally for sending across TCP)
* Produce human-readable versions of Messages (.AsString()) for diagnostic/documentation use


[MESSAGE FORMAT]
	A Message is a sequence of bytes received by the TCP Server (across TCP/IP) in this format:
		MLLLL_C
	where
		M       is a Message ID# ("MId"), which may be mnemonic/logical; proposed MId include "C", "Q", "E", "X", "R", and "M"
		LLLL    is the Length of an optional payload; the 21 LSBits are significant; bits 31:21 are reserved flag bits
				// TODO: confirm 21 bits is the minimum to hold 16*(sizeof(TDataItemHeader)+maxDataLength)
		_       is zero or more bytes, the optional payload's data
		C       is the checksum for all bytes in MMMM, LL, and _

	Given: checksum is calculated on all bytes in Message; the sum, including the checksum, should be zero

[PAYLOAD FORMAT]
	The payload is an ordered list of zero or more Data Items

[DATA ITEM FORMAT]
	Data Items are themselves a sequence of bytes, in this format:
		DDLL_
	where
		DD      is the Data ID# ("DId"), which identifies the *type* of data encapsulated in this data item
		LL      is the length of the optional payload "Data"
		_       is zero or more bytes, the optional Data Item's payload, or "the data"; `byte[L] Data`
				the bytes must conform Data Items' specific DId's requirements (see each DId's doc for details)


	// Messages as code [byte-packed little-endian structs]:
	struct TDataItem {
		__u16	DId;
		__u16	Length;
		__u8[Length] Data; // conceptually, a union with per-DId "parameter structs"
	}

	struct TMessage {
		__u8	MId;
		__u32	Length;
		__u8[Length] Payload; // conceptually a TDataItem[]
		__u8	Checksum;
	}

Each TDataItem in a Message represents an "Action", generally a read and/or a write of something as specified by the DId.

Messages serve to bundle TDataItem's Actions into atomic execution blocks.  All of the TDataItems in a Message are executed
	in order without any TDataItems from other Clients' Messages squeezing between.

The MId in the Response to Messages that encounter no errors "R" "Response".
The Response to Messages that are "well formed" (have valid syntax), even if other categories of errors are encountered,
	will be a Message with the same number of TDataItems as the Message, such that "Response.Payload.TDataItem[index]"
	provides the results (data read or written) of the corresponding "Message.Payload.TDataItem[index]".

[ERRORS]
Messages can generate three categories of Errors: "Syntax Errors", "Semantic Errors", and "Operational Errors".
*	Syntax Errors are structural mismatches. Examples include:
		Unrecognized MId or DId
		Message or DataItem Length doesn't match the purported length
		Bad checksum byte
		etc
	Syntax Errors are caught, and reported, while the TByte Message is being parsed, before any Actions are taken.
	Syntax Errors are likely to *cause* the parser to think additional Syntax Errors exist in the TBytes of the Message,
		so only the first detected Syntax Error is reported, via an MId == "E" Response Message to the socket/client, with
		a single TDataItem of type DId == SyntaxError (FFFF).
	The MId for the Response to Messages that encounter a Syntax Error (during parse) is "X" "syntaX".
	The payload in an X Response will contain one or more TDataItems with details about the detected Syntax Error.

*	Semantic Errors are invalid content in otherwise validly structured places ("bad parameter"). Examples include:
		An ADC channel higher than the number of channels on the device
		A Register offset that isn't defined on the device
		A PWM base frequency that is too fast or too slow for the device to generate
		A bitIndex that exceeds the number of bits on the device
	Semantic Errors are caught, and the offending TDataItem is skipped, during the execution of the Message's bundle
		of TDataItems, and reported *after* the entire Message has been executed.

	The MId for the Response to Messages that encounter Semantic or Execution errors is "E" "Error".
	A Response to a Message that contains one or more Semantic Errors among its TDataItems will result in an "E" Response Message,
		with one TDataItem in the E Reponse per TDataItem in the Message, but the TDataItem generated for TDataItems that had
		Semantic Errors will so indicate via a special DId.

*	Operational Errors occur from hardware faults, temporary or permanent.  Examples include:
		DAC SPI-Bus "Wait For Not Busy" Timeout duration exceeded
		Create File operation failed
		Firmware Rev xx Binary not found
		ADC "Wait For End Of Conversion" Timeout duration exceeded
	A Response to a Message that encountered one or more Operational Errors during execution will result in an "E" Response Message,
		with one TDataItem in the E Reponse per TDataItem in the Message, but the TDataItem generated for TDataItems that encountered
		Operational Errors will so indicate via a special DId.
---
	TMessages and TDataItems and such all use "smart pointers", in many cases unique_ptr<> (and I'm wanting to move more of the
	shared_ptr<> to unique_ptr<> in the future but lack the experience to simply grok the impact on my code/algorithm/structure)


Towards these goals this Protocol Library source provides serializers, deserializes, constructors, and validators for TMessages and TDataItems (and derived classes).
* To deserialize, to validate and create a TMessage or TDataItem from a byte array use the .fromBytes() factory methods or equivalent constructor(TBytes)
* To serialize, to convert TMessages and TDataItems into byte arrays use the .AsBytes() methods
* To get human-readable versions of TMessages or TDataItems, use .AsString() methods; Note TMessage.AsString() calls its Payload[].AsString()s

Typically the server will:
1) Receive bytes via TCP
2) pass the buffer as a vector<byte> (called "TBytes") to TMessage.fromBytes() to construct a TMessage, which itself optionally contains a TPayload, a vector of TDataItems, which themselves optionally contain a Payload.
3) if .fromBytes() succeeds it means the TBytes contained a valid Message, and it will be queued into a worker thread for execution (NYI)
4) if .fromBytes() fails an error Response Message will be sent, logged, printed to the console, etc (NYI)

Separately, the worker thread pops items off the queue and executes them, generally as atomic actions insofar as plausible, and Responds appropriately.

Each DataItem can be thought of as an API entry-point or function.  Some DId are provide low-level functionality, some mid-level, and some high-level.  Implementation will proceed low to high.

Each DId can equated with a hypothetical API function name, as represented in the enum DataItemIds in eNET-types.h, like:
REG_Read1, REG_Write1, BRD_Reset, DIO_ConfigureReadWriteReadSome, etc.

Currently these DId are named with a prefix, underscore, and mnemonic.  The following groups / prefixes are defined:
BRD_ - Data Items associated with the board as a whole, or the DAQ functionality as a whole, TBD
REG_ - lowest-level interface to DAQ functionality
DAC_  - DAC (D/A, Analog Output) functionality
DIO_ - Digital I/O stuff
PWM_ - PWM stuff; Note: Arduino bundles this with Analog Outputs and hides a lot of flexibility from users ...
ADC_  - A/D configuration, operations and data / results
and the following are under consideration, but might best fit under BRD_
WDG_ - Watchdog Timer stuff
DEF_ - Power On Defaults for DAQ stuff
TCP_ - IP address and other networking choices
PNP_ - Plug-and-Play stuff, device discovery, etc.
CFG_ - Linux-side device-level configurations, protocol-selection, etc.
SERVICE_ - technical support aids
SCRIPT_ - scripting related DIds, like SCRIPT_msleep(__u16 milliseconds)

## Data Item Specifics
The TDataItem class is a base class from which classes for each DId-specific functionality are derived.

The TDataItem's Data is a simple vector of bytes
Derived classes' Data should reflect the parameters to the hypothetical API function the DId represents.
* The TDataItem descendant to encapsulate DIO_Write1(bitIndex, bitLevel) would have a bitIndex field, and a bitLevel field; presumably a byte and bool, respectively.
* The class encapsulating DIO_ReadConfigureWriteReadSome(bitConfigureMask, configureBits, bitWriteMask, writeBits) would have far more fields (hypothetically)
* etc

TDataItem::fromBytes() is a class factory that instantiates the correct derived class for the DId, as configured in the DIdList[] array defined in TMessage.cpp.
Adding new DId classes requires changing or adding an entry in DIdList[].


## Files / Descriptions
This repo is not well organized.
It should hold the library code necessary to serialize and deserialize ACCES eNET-Protocol 2.0 format byte arrays; and it does.
However, the library is tightly coupled to apci-eNET (the linux kernel module and API library for register & DMA/IRQ access to eNET-AIO16-128F Family DAQ functionality)
It also contains aioenetd, the linux daemon that runs on eNET-AIO16-128F.
The eNET-Protocol 2.0 library should NOT be tightly coupled to any given eNET- Series hardware device; this code is a degenerate case because only one hardware type exists so far.

### ACCES eNET-Protocol 2.0 stuff
TMessage.h / TMessage.cpp - declares / defines the class for Messages and DataItems, including all derived classes
TError.h / TError.cpp - declares / defines the TError type and functions to operate on it.  NYI (current error handling is interim)
eNET-types.h - declares/defines low-level types all modules should use; the primary goal is to promote cross-platform by jargon-rectification
logging/** - Logging library slightly modified from Dr. Dobbs Journal Article Copyright (c) 2008, 2009 Michael Schulze <mschulze@ivs.cs.uni-magdeburg.de>

### this repo contains several projects in addition to the ACCES eNET-Protocol 2.0 TMessage library code
#### eNET-AIO16-128F hardware-related files
apci_ioctl.h / apcilib.cpp / apcilib.h / eNET-AIO.h - these files are from the apci-eNET fork of APCI and are intended to be included by reference; I don't know how to do that, so I just copied them into this repo for now

#### protocol library test program
test.cpp - a simple program I've been using to test the various functionality of the Protocol library; the source has evolved over time as the Protocol implementation moved from "C" to "C++" to "Classy C++", and will adapt again as we continue moving forward (to "Exceptional Classy", etc)

#### aioenetd server/listener daemon
aioenetd.cpp - listens on port for TCP packets in Protocol 2 format, turns them into TMessages, executes them against the device, and replies with results
adc.h / adc.cpp - declares / defines the ADC Streaming worker threads and related functionality that aioenetd uses. CAUTION: TADC_StreamStart() and relateds are tightly coupled to this


