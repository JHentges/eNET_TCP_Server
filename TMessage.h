#pragma once
/*
The TCP Server Message is received in a standardized format.
Write functions to
) Build DataItems with zero or more bytes as the DataItem's `Data`
) Build Messages with zero or more DataItems as the Message `Payload`
) Append DataItems to already built Messages
) Validate a Message is well-formed (which also means all DataItems are well-formed)
) convert an array of bytes into a Message
) Parse the Message into a queue of Actions
) Execute the queue

A Message is a sequence of bytes received by the TCP Server (across TCP/IP) in this format:
	MLL_C
where
	M       is a Message ID# ("MId"), which may be mnemonic/logical
	LL      is the Length of an optional payload
	_       is zero or more bytes, the optional payload's data
	C       is the checksum for all bytes in MMMM, LL, and _

Given: checksum is calculated on all bytes in Message; the sum, including the checksum, should be zero

Further, the payload is an ordered list of zero or more Data Items, which are themselves a sequence of bytes, in this format:
	DDDL_
where
	DDD     is the Data ID# ("DId"), which identifies the *type* of data encapsulated in this data item
	L       is the length of the data encapsulated in this data item (in bytes) (excluding L and Did)
	_       is the Data Item's payload, or "the data"; `byte[L] Data`



TODO: start writing the descendants of TDataItem.  See validateDataItemPayload()'s comments below
*/
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

#include "eNET-types.h"

extern const char *err_msg[];
enum DataItemIds
{
	DIdRead8 = 1,
	DIdRead16,
	DIdRead32
};


int validateDataItemPayload(TDataId DataItemID, TBytes Data);

#pragma region utility functions / templates < >

// Template class to slice a vector from range Start to End
template <typename T>
vector<T> slicing(vector<T> const &v, int Start, int End)
{

	// Begin and End iterator
	auto first = v.begin() + Start;
	auto last = v.begin() + End + 1;

	// Copy the element
	vector<T> vector(first, last);

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
constexpr void GUARD(bool allGood, TError resultcode, int i)
{
	if (!(allGood))
		throw logic_error(string(__FILE__) + "(" + to_string(__LINE__) + "): " + string(err_msg[-(resultcode)]) + " = " + to_hex(i));
}

// return register width for given offset as defined for eNET-AIO registers
// returns 0 if offset is invalid
int widthFromOffset(int ofs);
#pragma endregion utility functions and templates

#pragma region class TDataItem declaration

// Base class for all TDataItems, descendants of which will handle payloads specific to the DId
class TDataItem
{
public:
	// NYI - validate the payload of a Data Item based on the Data Item ID
	// e.g, if the Message is equivalent to ADC_SetRangeAll(__u8 ranges[16]) then the Data
	//      should be 16 bytes, each of which is a valid Range Code
	// e.g, if the Message is ADC_SetRange(__u16 Channel, __u8 RangeCode) then the Data
	//      should be 3 bytes, being a __u16 Channel which must be valid for this device, and
	//      a valid RangeCode byte
	// NOTE:
	//   This should be implemented OOP-style: each TDataItem ID should be a descendant-class that provides the
	//   specific validate and parse appropriate to that DataItemID
	static int validateDataItemPayload(TDataId DataItemID, TBytes Data);
	static int isValidDataItemID(TDataId DataItemID);
	static int validateDataItem(TBytes msg);
	static shared_ptr<TDataItem> parseDataItem(TBytes msg, TError &result);

public:
	TDataItem(TDataId DId);
	TDataItem(TBytes bytes);
	TDataItem(TDataId DId, TBytes bytes);
	TDataItem();

public:
	TDataItem &addData(__u8 aByte);
	TDataItem &setDId(TDataId DId);
	TDataId getDId();

	virtual TBytes AsBytes();
	virtual string AsString();

	TBytes Data;

private:
	TDataId Id{0};
};
#pragma endregion TDataItem declaration

#pragma region TDataItem descendant for DId "Read Register Value"
class DIdReadRegister : public TDataItem
{
public:
	static TError validateDataItemPayload(TDataId DataItemID, TBytes Data);

public:
	DIdReadRegister() = default;
	DIdReadRegister(TDataId DId, int ofs);
	DIdReadRegister(TBytes bytes);

public:
	DIdReadRegister & setOffset(int ofs);
	virtual TBytes AsBytes();
	virtual string AsString();

public:
	int offset{0};
	int width{0};
};

#pragma endregion

#pragma region class TMessage declaration
class TMessage
{
public:
	/* A Message consists of a Header (TMessageHeader) and optional payload plus a checksum byte
	 * This function parses an array of bytes that is supposed to be an entire Message
	 * ...then returns a TMessage and sets result to indicate error/success.
	 * It returns NULL on errors
	 */
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
	static vector<shared_ptr<TDataItem>> parsePayload(TBytes Payload, __u32 payload_length, TError &result);
	static TMessage FromBytes(TBytes buf, TError &result);

public:
	TMessage() = default;
	TMessage(TMessageId MId);
	TMessage(TMessageId MId, vector<std::shared_ptr<TDataItem>> Payload);
	TMessage(TBytes Msg);
	TMessageId getMId();
	TCheckSum getChecksum();
	TMessage &setMId(TMessageId MId);
	TMessage &addDataItem(std::shared_ptr<TDataItem> item);
	TBytes AsBytes();
	string AsString();

protected:
	TMessageId Id;
	// vector<TDataItem*> DataItems;
	vector<std::shared_ptr<TDataItem>> DataItems;
};
#pragma endregion TMessage declaration

TMessage FromBytes(TBytes buf, TError &result);
