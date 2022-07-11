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
	DDL_
where
	DD      is the Data ID# ("DId"), which identifies the *type* of data encapsulated in this data item
	L       is the length of the data encapsulated in this data item (in bytes) (excluding L and Did)
	_       is the Data Item's payload, or "the data"; `byte[L] Data`

TODO: finish writing the descendants of TDataItem.  See validateDataItemPayload()'s comments below
*/
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "eNET-types.h"
using namespace std;

#pragma region TError stuff
#define _INVALID_MESSAGEID_ ((TMessageId)-1)

// TODO: Change the #defined errors, and the const char *err_msg[], to an error class/struct/array
#define ERR_SUCCESS 0
#define ERR_MSG_TOO_SHORT -1
#define ERR_MSG_CHECKSUM -2
#define ERR_MSG_PARSE -3
#define ERR_MSG_START -4
#define ERR_MSG_ID_UNKNOWN -5
#define ERR_MSG_LEN_MISMATCH -6
#define ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH -7
#define ERR_MSG_DATAITEM_ID_UNKNOWN -8
#define ERR_MSG_DATAITEM_TOO_SHORT -9

#define ERR_DId_BAD_PARAM -10
#define ERR_DId_BAD_OFFSET -11
#define ERR_DId_INVALID -12

extern const char *err_msg[];

#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength (__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum))
#pragma endregion
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
constexpr inline void
GUARD(	bool allGood, TError resultcode, int intInfo,
		int Line = __builtin_LINE(), const char *File = __builtin_FILE(), const char *Func = __builtin_FUNCTION() )
{
	if (!(allGood))
		throw logic_error(string(File)
		 + ": " + string(Func)
         + "(" + to_string(Line) + "): "
         + string(err_msg[-(resultcode)]) + " = " + to_hex(intInfo));
}

// throw exception if conditional is false; "i" must be a defined TError
// #define GUARD(allGood, resultCode, i) \
// 		{ \
// 			if (!(allGood)) \
// 				throw logic_error(string(__FILE__) \
//  					+ "(" + to_string(__LINE__) + "): " \
// 					+ string(err_msg[-(resultCode)]) + " = " + to_hex((int)i));\
// 		 }

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
	static int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	static int isValidDataItemID(DataItemIds DataItemID);
	static int validateDataItem(TBytes msg);
	static int getDIdIndex(DataItemIds DId);
	static PTDataItem fromBytes(TBytes msg, TError &result);
	static string getDIdDesc(DataItemIds DId);
	static TDataItemLength getMinLength(DataItemIds DId);
	static TDataItemLength getTargetLength(DataItemIds DId);
	static TDataItemLength getMaxLength(DataItemIds DId);

public:
	TDataItem(DataItemIds DId);
	TDataItem(TBytes bytes);
	TDataItem(DataItemIds DId, TBytes bytes);
	TDataItem();

public:
	virtual TDataItem &addData(__u8 aByte);
	virtual TDataItem &setDId(DataItemIds DId);
	virtual DataItemIds getDId();
	virtual bool isValidDataLength();
	virtual TBytes AsBytes();
	string getDIdDesc();
	virtual string AsString();

	TBytes Data;

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
#pragma region class TDIdReadRegister : TDataItem for DataItemIds::REG_Read1 "Read Register Value"
class TDIdReadRegister : public TDataItem
{
public:
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);

public:
	TDIdReadRegister() = default;
	TDIdReadRegister(DataItemIds DId, int ofs);
	TDIdReadRegister(TBytes bytes);

public:
	TDIdReadRegister & setOffset(int ofs);
	virtual TBytes AsBytes();
	virtual string AsString();

public:
	int offset{0};
	int width{0};
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
	static TPayload parsePayload(TBytes Payload, __u32 payload_length, TError &result);
	// factory method but might not be as good as TDataItem::fromBytes(), and figure out F or f
	static TMessage FromBytes(TBytes buf, TError &result);

public:
	TMessage() = default;
	TMessage(TMessageId MId);
	TMessage(TMessageId MId,TPayload Payload);
	TMessage(TBytes Msg);
	TMessageId getMId();
	TCheckSum getChecksum();
	TMessage &setMId(TMessageId MId);
	TMessage &addDataItem(PTDataItem item);
	TBytes AsBytes();
	string AsString();

protected:
	TMessageId Id;
	TPayload DataItems;
};
#pragma endregion TMessage declaration

// utility template to turn class into base-class-pointer-to-instance-on-heap
template <class X> std::unique_ptr<TDataItem> construct() { return std::unique_ptr<TDataItem>(new X); }

// template <class X>
// std::unique_ptr<TDataItem> construct(Arg...) { return std::unique_ptr<TDataItem>(new X(Arg...)); }

typedef std::unique_ptr<TDataItem> DIdConstructor();

typedef struct
{
	TDataId DId;
	TDataItemLength minLen;
	TDataItemLength expectedLen;
	TDataItemLength maxLen;
	DIdConstructor *Construct;
	const char *desc;
} TDIdList;

#define DIdNYI(d)	{d, 0, 0, 0, construct<TDataItemNYI>, #d "(NYI)"}

TDIdList const DIdList[] = {
	{_INVALID_DATAITEMID_, 0, 0, 0, construct<TDataItem>, "Invalid DId"},
	{BRD_, 0, 0, 255, construct<TDataItem>, "TDataItem Base (BRD_)"},
	{BRD_Reset, 0, 0, 0, construct<TDataItem>, "BRD_Reset(void)"},
	{REG_Read1, 0, 1, 1, construct<TDIdReadRegister>, "REG_Read1(__u8 offset)"},
	{REG_ReadAll, 0, 0, 0, construct<TDataItemNYI>, "REG_ReadAll(void)"},
	{REG_ReadSome, 0, 0, 0, construct<TDataItemNYI>, "REG_ReadSome"},
	{REG_ReadBuf, 0, 0, 0, construct<TDataItemNYI>, "REG_ReadBuf"},
	{REG_Write1, 2, 5, 5, construct<TDIdWriteRegister>, "REG_Write1(__u8 offset, auto value)"},
	{REG_WriteSome, 0, 0, 0, construct<TDataItemNYI>, "REG_WriteSome"},
	{REG_WriteBuf, 0, 0, 0, construct<TDataItemNYI>, "REG_WriteBuf"},
	{REG_ClearBits, 2, 5, 5, construct<TDataItemNYI>, "REG_ClearBits(__u8 offset, auto bitMaskToClear)"},
	{REG_SetBits, 2, 5, 5, construct<TDataItemNYI>, "Reg_SetBits(__u8 offset, auto bitMaskToSet)"},
	{REG_ToggleBits, 2, 5, 5, construct<TDataItemNYI>, "Reg_ToggleBits(__u8 offset, auto bitMaskToToggle)"},

	{DAC_, 0, 0, 0, construct<TDataItem>, "TDataItemBase (DAC_)"},
	{DAC_Output1, 5, 5, 5, construct<TDIdDacOutput>, "DAC_Output1(__u8 iDAC, single Volts)"},
	DIdNYI(DAC_OutputAll),DIdNYI(DAC_OutputSome),
	DIdNYI(DAC_Configure1),DIdNYI(DAC_ConfigureAll),DIdNYI(DAC_ConfigureSome),
	DIdNYI(DAC_ConfigAndOutput1), DIdNYI(DAC_ConfigAndOutputAll), DIdNYI(DAC_ConfigAndOutputSome),
	DIdNYI(DAC_ReadbackAll),

	DIdNYI(DIO_),
	DIdNYI(DIO_Configure1), DIdNYI(DIO_ConfigureAll), DIdNYI(DIO_ConfigureSome),
	DIdNYI(DIO_Input1), DIdNYI(DIO_InputAll), DIdNYI(DIO_InputSome),
	DIdNYI(DIO_InputBuf1), DIdNYI(DIO_InputBufAll), DIdNYI(DIO_InputBufSome), // repeated unpaced reads of Digital Inputs; NOTE: not sure this is useful
	DIdNYI(DIO_Output1), DIdNYI(DIO_OutputAll), DIdNYI(DIO_OutputSome),
	DIdNYI(DIO_OutputBuf),
	DIdNYI(DIO_ConfigureReadWriteReadSome),
	DIdNYI(DIO_Clear1), DIdNYI(DIO_ClearAll), DIdNYI(DIO_ClearSome),
	DIdNYI(DIO_Set1), DIdNYI(DIO_SetAll), DIdNYI(DIO_SetSome),
	DIdNYI(DIO_Toggle1), DIdNYI(DIO_ToggleAll), DIdNYI(DIO_ToggleSome),
	DIdNYI(DIO_Pulse1), DIdNYI(DIO_PulseAll), DIdNYI(DIO_PulseSome),

	DIdNYI(PWM_),
	DIdNYI(PWM_Configure1), DIdNYI(PWM_ConfigureAll), DIdNYI(PWM_ConfigureSome),
	DIdNYI(PWM_Input1), DIdNYI(PWM_InputAll), DIdNYI(PWM_InputSome),
	DIdNYI(PWM_Output1), DIdNYI(PWM_OutputAll), DIdNYI(PWM_OutputSome),

	DIdNYI(ADC_),
	DIdNYI(ADC_ConfigurationOfEverything),
	DIdNYI(ADC_Range1), DIdNYI(ADC_RangeAll), DIdNYI(ADC_RangeSome),
	DIdNYI(ADC_Span1), DIdNYI(ADC_SpanAll), DIdNYI(ADC_SpanSome),
	DIdNYI(ADC_Offset1), DIdNYI(ADC_OffsetAll), DIdNYI(ADC_OffsetSome),
	DIdNYI(ADC_Calibration1), DIdNYI(ADC_CalibrationAll), DIdNYI(ADC_CalibrationSome),
	DIdNYI(ADC_Volts1), DIdNYI(ADC_VoltsAll), DIdNYI(ADC_VoltsSome),
	DIdNYI(ADC_Counts1), DIdNYI(ADC_CountsAll), DIdNYI(ADC_CountsSome),
	DIdNYI(ADC_Raw1), DIdNYI(ADC_RawAll), DIdNYI(ADC_RawSome),

	DIdNYI(ADC_Streaming_stuff_including_Hz_config),

	DIdNYI(SCRIPT_Pause), // SCRIPT_Pause(__u8 delay ms)

	DIdNYI(WDG_),
	DIdNYI(DEF_),
	DIdNYI(SERVICE_),
	DIdNYI(TCP_),
	DIdNYI(PNP_),
	DIdNYI(CFG_),
	// etc.  Need to list all the ones we care about soon, and all (of the ones we keep), eventually.
	// TODO: return `NYI` for expected but not implemented DIds ... somehow.  C++ doesn't have introspection of enums ...
	//		perhaps make a TDataItem derivative that is hard-coded NYI
};
