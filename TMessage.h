#pragma once
/*
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
		MLL_C
	where
		M       is a Message ID# ("MId"), which may be mnemonic/logical; proposed MId include "C", "Q", "E", "X", "R", and "M"
		LL      is the Length of an optional payload
		_       is zero or more bytes, the optional payload's data
		C       is the checksum for all bytes in MMMM, LL, and _

	Given: checksum is calculated on all bytes in Message; the sum, including the checksum, should be zero

[PAYLOAD FORMAT]
	The payload is an ordered list of zero or more Data Items

[DATA ITEM FORMAT]
	Data Items are themselves a sequence of bytes, in this format:
		DDL_
	where
		DD      is the Data ID# ("DId"), which identifies the *type* of data encapsulated in this data item
		L       is the length of the data encapsulated in this data item (in bytes) (excluding L and Did)
		_       is the Data Item's payload, or "the data"; `byte[L] Data`


	// Messages as code [byte-packed little-endian structs]:
	struct TDataItem {
		__u16	DId;
		__u8	Length;
		__u8[Length] Data; // conceptually, a union with per-DId "parameter structs"
	}

	struct TMessage {
		__u8	MId;
		__u16	Length;
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


TODO: implement a good TError replacement
TODO: finish writing the descendants of TDataItem.  See validateDataItemPayload()'s comments below
*/

/*[aioenetd Protocol 2 TCP-Listener/Server Daemon/Service implementation and concept notes]
from discord code-review conversation with Daria; these do not belong in this source file:
	your "the main loop" == "my worker thread that dequeues Actions";
	your "exploding" == "my Object factory construction";
	You've moved "my Object construction" into a single-threaded spot, "the main loop", from where I have it, in an
	individual socket's receive-thread.
	Because I have multiple receive threads it isn't nearly as necessary to "be fast": the TCP Stack will queue bytes for me.
	Because my error-checking location (the parser, the exploder, .FromBytes()) is in a socket-specific (ie client-specific)
	thread it is harder for fuzzed bytes sent over TCP to affect other clients or the device as a whole.
	Because my worker and all receive threads share a thread-safe std::queue<> everything is serialized nicely.

	By putting 66% of the error checking, syntax AND semantic (but not operational errors, eg hardware timeouts) into the
	receive threads — and in fact, into the TMessage constructor, I am guaranteed all TMessage Objects are valid and safe
	to submit to the worker thread, thus less likely to cause errors in that single-thread / single-point-of-failure
	(and make the worker execution faster, as it is "my single thread" and thus bottleneck)
-----
	A TMessage is constructed from received bytes by `auto aMessage = TMessage::fromBytes(buf);`, or an "X" Response TMessage with
	syntax error details gets returned, instead.
	Either way, the constructed TMessage is pushed into the Action Queue.
	The asynchronous worker thread pops a TMessage off the Action Queue, does a `for(aDataItem : aMessage.Payload){aDataItem.Go()}`,
	and either modifies-in-place and sends the TMessage as a reply or constructs and sends a new TMessage as the reply,
	the TMessage(s) then goes out of scope and deallocates
-----
	The TMessage library needs to handle syntax errors, mistakes in the *format* of a bytestream, and semantic errors, mistakes
	in the *content* of the received bytes.  Some categoriese of semantic errors, however, are specific to a particular model of eNET-
	device, let alone specific to a model Family.

	Consider ADC_GetChannelV(iChannel): iChannel is valid if (0 <= iChannel <= 15), right?  Nope, not "generally": this is only true
	for the ~12 models in the base Family, eNET-AIO16-16F, eNET-AI12-16E, etc.  But eNET- devices, and therefore Protocol 2 devices
	intended to operate via this TMessage library and the aioenetd implementation, include the DPK and DPK M Families; this means:
	iChannel is valid if (0 <= iChannel <= highestAdcChan), and highestAdcChan is 15, 31, 63, 96, or 127; depending on the specific
	model running aioenetd/using this library.

	So, to catch Semantic errors of this type (invalid channel parameter in an ADC_GetChannelV() TDataItem) the parser must "know"
	the value of "highestAdcChan" for the model it is running on.

	The sum of all things that the parser needs to know to handle semantic error checking, specific to the model running the library,
	are encapsulated as "getters" in a HAL.  The list is quite long as there are a LOT of variations built out of the eNET-AIO design.

	These "getters" *could* be implemented in a static, compile-time, manner.  Consider a device_specific.h file that has a bunch of
	eg `#define highestAdcChan 31`-type constants defined.  However, this is a "write it for 1" approach, and would require loading a
	different TMessage library binary for every model, as the binary is effectively hard-coded for a specific device's needs.

	The USB FWE2.0 firmwares are almost this simplistic in their approach to a HAL: there is a device_specific.h, but there is also a
	run-time operation that introspects the DeviceID and tweaks some constants, like the friendly_model_name string, to a model-specific
	value.  This run-time operation is a hard-coded switch(DeviceId){}, and thus is implemented per Firmware (but allows one Firmware
	to run, as a binary, on any models built from the same or compatible PCB).

	This only works because there are very few variants per firmware source: the USB designs are sufficiently different that reusing
	firmware is implausible: it would require a *real* HAL implementation (one that abstracted every pin on the FX2, and every
	register that could ever exist on the external address/data bus).

	We're going for a better approach to the HAL in this library.

	TBD, LOL

	The library implementation, today (2022-07-21 @ 10:55am Pacific), only checks for Semantic errors in the one DId that's been
	implemented; REG_Read1(offset).  This DId uses a helper function hardcoded in the source called `WidthFromOffset(offset)`,
	which returns one of 0, 8, or 32, indicating "invalid offset", "offset is a valid 8-bit register", or "offset is a valid
	32-bit register", respectively.  In theory it should be able to respond "16" as well, but the eNET-AIO, in all of its models,
	has no 16-bit registers.  This is an instance where the HAL is weak; "WidthFromOffset()" shouldn't be hard-coded; it should
	be able to respond accurately about whatever device it is running on (or perhaps even whatever device it is asked about).

	One way to accomplish this would be loading configuration information tables from nonvolatile on-device storage.  "tables",
	here, is plural not to refer to both a hypothetical single table needed for WidthFromOffset() and all the other tables for
	supporting other HAL-queriables, but to express that WidthFromOffset() *alone* needs several tables, if it is to support
	the general case.  Sure, eNET-AIO only has registers that can each only be correctly accessed at a specific bit width (ie it is
	unsafe or impossible to successfully read or write 8 bits from any eNET-AIO 32-bit register), but many ACCES devices do not
	have this limitation; most devices support 8, 16, or 32-bit access to any register or group thereof (as long as offsets are
	width-aligned; eg 32-bit operations require that offset % 4 == 0).  WidthFromOffset() therefore becomes complex, and
	declaratively describing each device's capabilities and restrictions is also complex.

	Another approach would be to implement a "HAL interface" (C++ calls an interface an Abstract Base Class or ABC) which the library
	would use to access the needed polymorphisms via a device-specific TDeviceHAL object it is provided ("Dependency Injection").
	Every TDeviceHAL descendant would provide not-less-than a set of device-specific constants like `highestAdcChan` from the
	introduction example.  Better would be to also include "verbs" that implement generic operations as needed for the specific
	device, like an ADC_SetRange1(iChannel, rangeSpan), but this level of interface is incredibly complex, in any generic form.

	Consider the RA1216 which expects the ADC input range to be specified as a voltage span code plus an offset in ±16-bit counts;

	This is an extreme example, an outlier; an example that forces "the most general case" to be handled ... but this is merely the
	outlier in the "ADC RangeCode" axis.  Consider the RAD242 which has a 24-bit ADC, the AD8-16 which has an 8-bit ADC, the
	USB-DIO-32I which has 1 bit per I/O Group instead of the typical 8 or 4 bits — there are *many* axes of outliers, and they all
	become necessary to handle if you try to make a truly generic library/HAL interface.

	This is why "Universal Libraries" (like National provides) are so big and difficult to code against.

	Thus, "Protocol 2" is designed to be slightly generic, but more importantly, extensible.


	Similar to
	the simplistic #include "device_specific.h" "#define" approach, but supporting run-time selection of which "{modelUID}.h" is
	used.




---
	TMessages and TDataItems and such all use "smart pointers", in many cases unique_ptr<> (and I'm wanting to move more of the
	shared_ptr<> to unique_ptr<> in the future but lack the experience to simply grok the impact on my code/algorithm/structure)

 */

#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "eNET-types.h"
#include "TError.h"

extern int apci; // global handle to device file for DAQ circuit on which to perform reads/writes

#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength (__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum))
#pragma region utility functions / templates < >


#define s_print(s, ...)              \
	{                                \
		if (s)                       \
			sprintf(s, __VA_ARGS__); \
		else                         \
			printf(__VA_ARGS__);     \
	}


int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);

#define printBytes(dest, intro, buf, crlf)                               \
	{                                                                    \
		dest << intro;                                                   \
		for (auto byt : buf)                                             \
			dest << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(byt) << " ";  \
		if (crlf)                                                        \
			dest << endl;                                                \
	}

// Template class to slice a vector from range Start to End
template <typename T>
std::vector<T> slicing(std::vector<T> const &v, int Start, int End)
{

	// Begin and End iterator
	auto first = v.begin() + Start;
	auto last = v.begin() + End + 1;

	// Copy the element
	std::vector<T> vector(first, last);

	// Return the results
	return vector;
}

template <typename T>
std::string to_hex(T i)
{
	// Ensure this function is called with a template parameter that makes sense. Note: static_assert is only available in C++11 and higher.
	static_assert(std::is_integral<T>::value, "Template argument 'T' must be a fundamental integer type (e.g. int, short, etc..).");

	std::stringstream stream;
	stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex;

	// If T is an 8-bit integer type (e.g. uint8_t or int8_t) it will be
	// treated as an ASCII code, giving the wrong result. So we use C++17's
	// "if constexpr" to have the compiler decides at compile-time if it's
	// converting an 8-bit int or not.
	if constexpr (std::is_same_v<std::uint8_t, T>)
	{
		// Unsigned 8-bit unsigned int type. Cast to int (thanks Lincoln) to
		// avoid ASCII code interpretation of the int. The number of hex digits
		// in the  returned string will still be two, which is correct for 8 bits,
		// because of the 'sizeof(T)' above.
		stream << static_cast<int>(i);
	}
	else if (std::is_same_v<std::int8_t, T>)
	{
		// For 8-bit signed int, same as above, except we must first cast to unsigned
		// int, because values above 127d (0x7f) in the int will cause further issues.
		// if we cast directly to int.
		stream << static_cast<int>(static_cast<uint8_t>(i));
	}
	else
	{
		// No cast needed for ints wider than 8 bits.
		stream << i;
	}

	return stream.str();
}

// throw exception if conditional is false
inline void
GUARD(	bool allGood, TError resultcode, int intInfo,
		int Line = __builtin_LINE(), const char *File = __builtin_FILE(), const char *Func = __builtin_FUNCTION() )
{
	if (!(allGood))
		throw std::logic_error(std::string(File)
		 + ": " + std::string(Func)
         + "(" + std::to_string(Line) + "): "
         + std::to_string(resultcode) + " = " + to_hex(intInfo));
}

// return register width for given offset as defined for eNET-AIO registers
// returns 0 if offset is invalid
int widthFromOffset(int ofs);
#pragma endregion utility functions and templates
/*	The following classes (TDataItem and its descendants) try to use a consistent convention
	for the order Methods are listed

	1) Deserialization stuff, used while converting bytes (received via TCP) into Objects
	2) Serialization stuff, used to construct and convert Objects into byte vectors (for sending over TCP)
	3) Verbs: stuff associated with using Objects as Actions, and the results thereof
	4) diagnostic, debug, or otherwise "Rare" stuff, like .AsString()
*/
#pragma region class TDataItem declaration

// Base class for all TDataItems, descendants of which will handle payloads specific to the DId
class TDataItem
{
public:
// 1) Deserialization: methods used when converting byte vectors into objects

	// factory fromBytes() instantiates appropriate (sub-)class of TDataItem via DIdList[]
	// .fromBytes() would typically be called by TMessage::fromBytes();
	static PTDataItem fromBytes(TBytes msg, TError &result);

	// this block of methods are typically used by ::fromBytes() to syntax-check the byte vector
	static int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	static int isValidDataItemID(DataItemIds DataItemID);
	static int validateDataItem(TBytes msg);
	static TDataItemLength getMinLength(DataItemIds DId);
	static TDataItemLength getTargetLength(DataItemIds DId);
	static TDataItemLength getMaxLength(DataItemIds DId);

	// index into DIdList; TODO: kinda belongs in a DIdList class method...
	static int getDIdIndex(DataItemIds DId);

	// serialize for sending via TCP; calling TDataItem.AsBytes() is normally done by TMessage::AsBytes()
	virtual TBytes AsBytes();

// 2) Serialization: methods for source to generate TDataItems, typically for "Response Messages"

	// zero-"Data" data item constructor
	TDataItem(DataItemIds DId);
	// some-"Data" constructor for specific DId; *RARE*, *DEBUG mainly, to test round-trip conversion implementation*
	// any DId that is supposed to have data would use its own constructor that takes the correct data types, not a
	// simple TBytes, for the Data Payload
	TDataItem(DataItemIds DId, TBytes bytes);

	// TODO: WARN: Why would this *ever* be used?
	TDataItem();

	// TDataItem anItem(DId_Read1).addData(offset) kind of thing.  no idea if it will be useful other than debugging
	virtual TDataItem &addData(__u8 aByte);
	// TDataItem anItem().setDId(DId_Read1) kind of thing.  no idea why it exists
	virtual TDataItem &setDId(DataItemIds DId);
	// encapsulates reading the Id for the DataItem
	virtual DataItemIds getDId();

	virtual bool isValidDataLength();

	// parse byte array into TDataItem; *RARE*, *DEBUG mainly, to test round-trip conversion implementation*
	// this is an explicit class-specific .fromBytes(), which the class method .fromBytes() will invoke for NYI DIds etc
	TDataItem(TBytes bytes);

// 3) Verbs -- things used when *executing* the TDataItem Object
public:
	// intended to be overriden by descendants it performs the query/config operation and sets instance state as appropriate
	virtual TDataItem &Go();
	// encapsulates the result code of .Go()'s operation
	virtual TError getResultCode();
	// encapsulates the Value that results from .Go()'s operation; DIO_Read1() might have a bool Value;
	// ADC_GetImmediateScanV() might be an array of single precision floating point Volts
	virtual std::shared_ptr<void> getResultValue(); // TODO: fix; think this through


// 4) Diagnostic / Debug - methods typically used for implementation debugging
public:
	// returns human-readable string representing the DataItem and its payload; normally used by TMessage.AsString()
	virtual std::string AsString();
	// used by .AsString() to fetch the human-readable name/description of the DId (from DIdList[].Description)
	virtual std::string getDIdDesc();
	// class method to get the human-readable name/description of any known DId; TODO: should maybe be a method of DIdList[]
	static std::string getDIdDesc(DataItemIds DId);

protected:
	TBytes Data;
	TError resultCode;

private:
	DataItemIds Id{0};
};
#pragma endregion TDataItem declaration
#pragma region class TDataItemNYI declaration
class TDataItemNYI : public TDataItem
{
public:
	TDataItemNYI() = default;
};
#pragma endregion
#pragma region class TREG_Read1 : TDataItem for DataItemIds::REG_Read1 "Read Register Value"
class TREG_Read1 : public TDataItem
{
// 1) Deserialization
public:
	// called by TDataItem::fromBytes() via DIdList association with DId
	TREG_Read1(TBytes bytes);

// 2) Serialization: For creating Objects to be turned into bytes
public:
	// constructor of choice for source; all parameters included. TODO: ? make overloadable
	TREG_Read1(DataItemIds DId, int ofs);
	TREG_Read1() = default;
	virtual TBytes AsBytes();
	TREG_Read1 & setOffset(int ofs);

// 3) Verbs
public:
	virtual TREG_Read1 &Go();
	virtual TError getResultCode();
	virtual std::shared_ptr<void> getResultValue(); // TODO: fix; think this through
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);

// 4) Diagnostic
	virtual std::string AsString();

public:
	int offset{0};
	int width{0};

private:
	__u32 Value;
};

#pragma endregion
#pragma region class TDIdWriteRegister : TDataItem for REG_Write1 "Write Register Value"
class TDIdWriteRegister : public TDataItem
{
public:

};
#pragma endregion
#pragma region "DAC Output"
class TDIdDacOutput : public TDataItem
{
	public:
};
#pragma endregion



#pragma region class TMessage declaration
class TMessage
{
public:
	// Sums all bytes in Message, specifically including the checksum byte
	// WARNING: this algorithm only works if TChecksum is a byte
	static TCheckSum calculateChecksum(TBytes Message);
	static bool isValidMessageID(TMessageId MessageId);
	// Checks the Payload for well-formedness
	// returns 0 if the Payload is well-formed
	// this means that all the Data Items are well formed and the total size matches the expectation
	// "A Message has an optional Payload, which is a sequence of zero or more Data Items"
	static TError validatePayload(TBytes Payload);
	// Checks the Message for well-formedness
	// returns 0 if Message is well-formed
	static TError validateMessage(TBytes buf);
	/* A Payload consists of zero or more DataItems
	 * This function parses an array of bytes that is supposed to be a Payload
	 * ...then returns a vector of those TDataItems and sets result to indicate error/success
	 */
	static TPayload parsePayload(TBytes Payload, __u32 payload_length, TError &result);
	// factory method but might not be as good as TDataItem::fromBytes()
	// TODO: figure out F or f for the name
	static TMessage FromBytes(TBytes buf, TError &result);

public:
	TMessage() = default;
	TMessage(TMessageId MId);
	TMessage(TMessageId MId,TPayload Payload);
	/*
	 * This function parses a vector<byte> that is supposed to be an entire Message
	 * and returns a TMessage Object if no Syntax Errors were encountered.
	 * It throws exceptions on errors
	 */
	TMessage(TBytes Msg);

public:
	TMessageId getMId();
	TCheckSum getChecksum();
	TMessage &setMId(TMessageId MId);
	TMessage &addDataItem(PTDataItem item);

	TBytes AsBytes();
	std::string AsString();

protected:
	TMessageId Id;
	TPayload DataItems;
};
#pragma endregion TMessage declaration

// utility template to turn class into base-class-pointer-to-instance-on-heap
template <class X> std::unique_ptr<TDataItem> construct() { return std::unique_ptr<TDataItem>(new X); }

typedef std::unique_ptr<TDataItem> DIdConstructor();

typedef struct
{
	DataItemIds DId;
	TDataItemLength minLen;
	TDataItemLength expectedLen;
	TDataItemLength maxLen;
	DIdConstructor *Construct;
	std::string desc;
} TDIdListEntry;

extern TDIdListEntry const DIdList[];