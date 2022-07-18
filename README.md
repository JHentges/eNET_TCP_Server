# eNET_TCP_Server
## Protocol 2.0 library for eNET- devices from ACCES.

This library code is intended to be used by a TCP Listener, aioenetd, running on an Intel Sitara AM64x processor's Linux image (Ubuntu, buildroot, Yocto, whatever).  As a secondary goal building this C++ Library into a .so or .dll for use from other OSes, including Windows, VxWorks, QNX, etc., consider portability concerns.

The code requires C++17, and perhaps even C++20 (I can't recall; it is currently *built* in C++20 but I'm not certain the current implementation *requires* more than C++17; this source has been iterating often prior to initial release.)

It provides a bridge between packets on Ethernet and FPGA-based DAQ resources on the eNET-device.

The library is responsible for 
* turning byte streams / buffers (typically as received via TCP) into TMessage Objects
* constructing valid TMessage Objects from source code
* turning TMessage Objects into byte streams / buffers (typically for sending "Responses" across TCP to clients)

Each Message consists of 
* a one-byte MId ("MessageId", or "Message Type Identifier")
* a two-byte Length Field, which is the number of bytes (between 0 and 32767) in the Payload.  A Length field with bit 15 set is reserved for future expansion.
* zero or more bytes, up to 32767, of Payload data
* a one-byte Checksum chosen so it causes the sum of all bytes in a Message (modulus 256) to equal zero

3 MId (Message IDs) are (currently) defined: 
* 'Q', Query, a message from Client to Server asking for one or more DataItem(s)' values
* 'C', Configure, a message from Client to Server to set the value of one or more DataItems' underlying truths (register value, Linux configuration objects, etc)
* 'R', Response, a message from Server to Client to ACK, NACK, or provide fuller responses to 'C' or 'Q' Messages, as appropriate.

The payload is defined as zero, or one or more, DataItems.
Each DataItem consists of
* a two-byte DId ("DataId", or "Data Type Identifier")
* a one-byte Length Field, which is the number of bytes (between 0 and 255) in the DataItem's Data
* zero or more bytes, up to 255, of DataItem Data
```
A Message is a sequence of bytes received by the TCP Server (across TCP/IP) in this format:
 	MLL_C
where
	M       is a Message ID# ("MId"), which may be mnemonic/logical
	LL      is the Length of an optional payload
	_        is zero or more bytes, the optional payload's data
	C       is the checksum for all bytes in MMMM, LL, and _

Given: checksum is calculated on all bytes in Message; the sum, including the checksum, should be zero

Further, the payload is an ordered list of zero or more Data Items, which are themselves a sequence of bytes, in this format:
	DDL_
where
	DD      is the Data ID# ("DId"), which identifies the *type* of data encapsulated in this data item
	L       is the length of the data encapsulated in this data item (in bytes) (excluding L and Did)
	_       is the Data Item's payload, or "the data"; `byte[L] Data`
```
Towards these goals this Protocol Library source provides serializers, deserializes, constructors, and validators for TMessages and TDataItems.
* To deserialize, to validate and create a TMessage or TDataItem from a byte array use the .fromBytes() factory methods
* To serialize, to convert TMessages and TDataItems into byte arrays use the .AsBytes() methods
* To get human-readable versions of TMessages or TDataItems, use .AsString() methods

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
TMessage.h / TMessage.cpp - declares / defines the class for Messages and DataItems, including all derived classes
TError.h / TError.cpp - declares / defines the TError type and functions to operate on it.  NYI (current error handling is interim)
eNET-types.h - declares/defines low-level types all modules should use; the primary goal is to promote cross-platform by jargon-rectification

apci_ioctl.h/apcilib.cpp/apcilib.h/eNET-AIO.h - these files are from the apci-eNET fork of APCI and are intended to be included by reference; I don't know how to do that, so I just copied them into this repo for now

test.cpp - a simple program I've been using to test the various functionality of the Protocol library; the source has evolved over time as the Protocol implementation moved from "C" to "C++" to "Classy C++", and will adapt again as we continue moving forward (to "Exceptional Classy", etc)




