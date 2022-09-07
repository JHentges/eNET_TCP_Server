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


TODO: implement a good TError replacement
TODO: finish writing the descendants of TDataItem.  See validateDataItemPayload()'s comments below
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
#define minimumMessageLength ((__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum)))
#define maxDataLength (std::numeric_limits<TDataItemLength>::max())
#define maxPayloadLength ((__u32)(sizeof(TDataItemHeader) + maxDataLength) * 16)

#pragma region utility functions / templates < >

#define s_print(s, ...)              \
	{                                \
		if (s)                       \
			sprintf(s, __VA_ARGS__); \
		else                         \
			printf(__VA_ARGS__);     \
	}

int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);

#define printBytes(dest, intro, buf, crlf)                                                       \
	{                                                                                            \
		dest << intro;                                                                           \
		for (auto byt : buf)                                                                     \
			dest << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " "; \
		if (crlf)                                                                                \
			dest << std::endl;                                                                        \
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


// throw exception if conditional is false
inline void
GUARD(bool allGood, TError resultcode, int intInfo,
	  int Line = __builtin_LINE(), const char *File = __builtin_FILE(), const char *Func = __builtin_FUNCTION())
{
	if (!(allGood))
		throw std::logic_error(std::string(File) + ": " + std::string(Func) + "(" + std::to_string(Line) + "): " + std::to_string(resultcode) + " = " + to_hex(intInfo));
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

#pragma region "class TDataItem" declaration

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
	virtual TBytes AsBytes(bool bAsReply=false);
	// push DId into buf; utility for AsBytes()
	void pushDId(TBytes &buf);
	// push Length of Data into buf; utility for AsBytes();
	static void pushLen(TBytes &buf, TDataItemLength len);
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
	// returns human-readable string representing the DataItem and its payload; normally used by TMessage.AsString(bAsReply)
	virtual std::string AsString(bool bAsReply=false);
	// used by .AsString(bAsReply) to fetch the human-readable name/description of the DId (from DIdList[].Description)
	virtual std::string getDIdDesc();
	// class method to get the human-readable name/description of any known DId; TODO: should maybe be a method of DIdList[]
	static std::string getDIdDesc(DataItemIds DId);

protected:
	TBytes Data;
	TError resultCode;
	int conn;

private:
	DataItemIds Id{0};
};
#pragma endregion TDataItem declaration
#pragma region "class TDataItemNYI" declaration
class TDataItemNYI : public TDataItem
{
public:
	TDataItemNYI() = default;
	TDataItemNYI(TBytes buf) : TDataItem::TDataItem{buf}{};
};
#pragma endregion

#pragma region "class TREG_Read1 : TDataItem" for DataItemIds::REG_Read1 "Read Register Value"
class TREG_Read1 : public TDataItem
{
	// 1) Deserialization
public:
	// called by TDataItem::fromBytes() via DIdList association with DId
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	TREG_Read1(TBytes data);

	// 2) Serialization: For creating Objects to be turned into bytes
public:
	TREG_Read1();
	// constructor of choice for source; all parameters included. TODO: ? make overloadable
	TREG_Read1(DataItemIds DId, int ofs);
	TREG_Read1 &setOffset(int ofs);
	virtual TBytes AsBytes(bool bAsReply=false);

	// 3) Verbs
public:
	virtual TREG_Read1 &Go();
	virtual std::shared_ptr<void> getResultValue(); // TODO: fix; think this through

	// 4) Diagnostic
	virtual std::string AsString(bool bAsReply = false);

public:
	int offset{0};
	int width{0};

private:
	__u32 Value;
};

#pragma endregion

#pragma region "class TREG_Writes declaration"
// ABSTRACT base class for TREG_Write and related functionality (eg ADC_SetConfig(), DAC_WriteAll())
class TREG_Writes : public TDataItem
{
	public:
		TREG_Writes() = default;
		~TREG_Writes();
		TREG_Writes(TBytes buf);
		virtual TREG_Writes &Go();
		TREG_Writes &addWrite(__u8 w, int ofs, __u32 value);
		virtual std::string AsString(bool bAsReply);

	protected:
		REG_WriteList Writes;
};
#pragma endregion

#pragma region "class TREG_Write1 : TREG_Writes" for REG_Write1 "Write Register Value"
class TREG_Write1 : public TREG_Writes
{
public:
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	TREG_Write1();
	~TREG_Write1();
	TREG_Write1(TBytes buf);
	virtual TBytes AsBytes(bool bAsReply=false);
	//virtual std::string AsString(bool bAsReply=false);
};
#pragma endregion

#pragma region "DAC Output"
class TDIdDacOutput : public TDataItem
{
public:
	TDIdDacOutput(TBytes buf) : TDataItem::TDataItem{buf}{};
};
#pragma endregion

#pragma region "ADC Stuff"
class TADC_StreamStart : public TDataItem
{
	// 1) Deserialization
public:
	TADC_StreamStart(TBytes buf);

	// 2) Serialization: For creating Objects to be turned into bytes
public:
	TADC_StreamStart();
	virtual TBytes AsBytes(bool bAsReply=false);

	// 3) Verbs
	virtual TADC_StreamStart &Go();

	// 4) Diagnostic
	virtual std::string AsString(bool bAsReply = false);

protected:
	int argConnectionID = -1;
};

class TADC_StreamStop : public TDataItem
{
	// 1) Deserialization
public:
	TADC_StreamStop(){ setDId(ADC_StreamStop);}
	TADC_StreamStop(TBytes buf);

	// 2) Serialization: For creating Objects to be turned into bytes
public:
	virtual TBytes AsBytes(bool bAsReply=false);

	// 3) Verbs
	virtual TADC_StreamStop &Go();

	// 4) Diagnostic
	virtual std::string AsString(bool bAsReply = false);
};

#pragma endregion "ADC"


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

	static void pushLen(TBytes & buf, TMessagePayloadSize len)
	{
		for (int i = 0; i < (sizeof(len)); i++)
		{
			__u8 lsb = len & 0xFF;
			buf.push_back(lsb);
			len >>= 8;
		}
	}

public:
	TMessage() = default;
	TMessage(TMessageId MId);
	TMessage(TMessageId MId, TPayload Payload);
	/*
	 * This function parses a vector<byte> that is supposed to be an entire Message
	 * and returns a TMessage Object if no Syntax Errors were encountered.
	 * Similar to the factory method TMessage::fromBytes(TBytes Bytes, TError Result).
	 * It throws exceptions on errors
	 */
	TMessage(TBytes Msg);

public:
	TMessageId getMId();
	TCheckSum getChecksum(bool bAsReply = false);
	// set the MId ("MessageId")
	TMessage &setMId(TMessageId MId);
	// append a Data Item onto this Message's Payload
	TMessage &addDataItem(PTDataItem item);

	// returns this Message serialized into TBytes suitable for TCP send()
	TBytes AsBytes(bool bAsReply = false);
	// returns this Message as a human-readable std::string
	std::string AsString(bool bAsReply = false);

public:
	// vector of Data Items (std::vector<std::shared_ptr<TDataItem>>)
	TPayload DataItems;

protected:
	TMessageId Id;
	int conn;
};
#pragma endregion TMessage declaration

// utility template to turn class into (base-class)-pointer-to-instance-on-heap, so derived class gets called
template <class X> std::unique_ptr<TDataItem> construct_old() { return std::unique_ptr<TDataItem>(new X); }
//typedef std::unique_ptr<TDataItem> DIdConstructor();
// typedef struct {
// 	DIdConstructor *Construct; // = construct<TREG_Read1> or = construct<TDataItem> or whatnot
// } TDIdListEntry;

template <class X> std::unique_ptr<TDataItem> construct(TBytes FromBytes) { return std::unique_ptr<TDataItem>(new X(FromBytes)); }
typedef std::unique_ptr<TDataItem> DIdConstructor(TBytes FromBytes);

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


namespace log__ {
	const int aioDEBUG = 1;
	const int aioINFO = 2;
	const int aioWARN = 4;
	const int aioERROR = 8;
	const int aioTRACE = 16;
}
