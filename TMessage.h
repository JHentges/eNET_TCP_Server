#pragma once
/* // TODO:
Upcoming change to centralize the construction of serialized data items:

1) renaming all derived classes' .AsBytes() to eg ".CalcPayload()" thus removing .AsBytes from all derived classes
2) adding ".CalcPayload()" to the root class for zero-length DataItem convenience
3) removing this->pushDId() and this->pushLen() related code from all the derived classes .CalcPayload() implementations
4) write .AsBytes in the root class that pushes the DId, Len(Data), and Data
*/

/* // TODO:
Upcoming change to DataItems that "read one register and return the value":

1) use a mezzanine class related to REG_Read1 with a virtual offset field
2) each "read1return" type DataItem derives from that mezzanine, and consists of just a protected offset field

*/


/*
A Message is received in a standardized format as an array of bytes (generally across TCP).

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

The MId in the Response to Messages that encounter no errors is "R", for "Response".
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
#include "DataItems/TDataItem.h"
#include "DataItems/CFG_.h"


extern int apci; // global handle to device file for DAQ circuit on which to perform reads/writes

#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength ((__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum)))
#define maxDataLength (std::numeric_limits<TDataItemLength>::max())
#define maxPayloadLength ((__u32)(sizeof(TDataItemHeader) + maxDataLength) * 16)


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



namespace log__ {
	const int aioDEBUG = 1;
	const int aioINFO = 2;
	const int aioWARN = 4;
	const int aioERROR = 8;
	const int aioTRACE = 16;
}
