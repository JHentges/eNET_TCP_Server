#pragma once

#include <string.h>
#include <stdio.h>
#include <vector>
using std::vector, std::begin, std::end;

#ifdef __linux__
#include <linux/types.h>
#else
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
#endif

/* type definitions */
typedef vector<__u8> TBytes;
typedef __u8 TMessageId;
typedef __u16 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u16 TDataId;
typedef __u8 TDataItemLength;
typedef __u32 TError;
#define _INVALID_DATAITEMID_ ((TDataId)-1)
#define _INVALID_MESSAGEID_ ((TMessageId)-1)

#define s_print(s, ...)              \
	{                                \
		if (s)                       \
			sprintf(s, __VA_ARGS__); \
		else                         \
			printf(__VA_ARGS__);     \
	}

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

#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength (__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum))
#define DataItemHeaderSize (sizeof(TDataId) + sizeof(TDataItemLength))



#pragma pack(push, 1)

typedef struct
{
	TMessageId type;
	TMessagePayloadSize payload_size;
} TMessageHeader;

typedef struct {
	TDataId DId;
	TDataItemLength dataLength;
} TDataItemHeader;

#pragma pack(pop)

extern const vector<TDataId> ValidDataItemIDs;
extern const vector<TMessageId> ValidMessages;
