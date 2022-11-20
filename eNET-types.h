#pragma once

/*

This file is intended to provide standardized type names for use in ACCES' eNET- Series devices.
This improves portability and flexibility; changing the underlying type of a thing is easier if
has a unique type name.
(Frex, we changed from 2-byte to 4-byte TMessage Payload Lengths with only one line in this file)
	(well, plus some bug-fixing, because *that* goal was aspirational.)

This file also has some other crap in it, like some utility stuff and the enum for
TDataItem IDs, but that should change.

*/

#ifdef __linux__
#include <linux/types.h>
#else // define our base types for compiling on Windows
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
#endif

#include <cstring>
#include <string>
#include <iomanip>
#include <memory>
#include <vector>
#include <thread>

#include "safe_queue.h"
#include "TError.h"

/* type definitions */
typedef std::vector<__u8> TBytes;
typedef __u8 TMessageId;
typedef __u32 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u16 TDataId;
typedef __u16 TDataItemLength;

class TDataItem;

typedef std::shared_ptr<TDataItem> PTDataItem;
typedef std::vector<PTDataItem> TPayload;

template <typename T>
inline std::string to_hex(T i)
{
	// Ensure this function is called with a template parameter that makes sense. Note: static_assert is only available in C++11 and higher.
	static_assert(std::is_integral<T>::value, "Template argument 'T' must be a fundamental integer type (e.g. int, short, etc..).");

	std::stringstream stream;
	stream << /*std::string("0x") <<*/ std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex;

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

#pragma pack(push, 1)
typedef struct {
	__u8 offset;
	__u8 width;
	__u32 value;
} REG_Write;

typedef std::vector<REG_Write> REG_WriteList;

typedef struct
{
	TMessageId type;
	TMessagePayloadSize payload_size;
} TMessageHeader;


typedef struct TSendQueueItemClass
{
	// which TCP-per-client-read thread put this item into the Action Queue
	pthread_t &receiver;
	// which thread is responsible for sending results of the action to the client
	pthread_t &sender;
	// which queue is the sender-thread popping from
	SafeQueue<TSendQueueItemClass> &sendQueue;
	// which client is all this from/for
	int clientref;
	// what TCP port# was this received on
	int portReceive;
	// what TCP port# is this sending out on
	int portSend;
} TSendQueueItem;

#pragma pack(pop)

// throw exception if conditional is false
inline void
GUARD(bool allGood, TError resultcode, int intInfo,
	  int Line = __builtin_LINE(), const char *File = __builtin_FILE(), const char *Func = __builtin_FUNCTION())
{
	if (!(allGood))
		throw std::logic_error(std::string(File) + ": " + std::string(Func) + "(" + std::to_string(Line) + "): " + std::to_string(-resultcode) + " = " + std::to_string(intInfo));
}

