#ifndef parserh
#define parserh



#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __diag_print_csum 0


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

#define __charStart__ (TMessageStart)('$')  // for illustrative purposes (i.e., "for now", TBD, subject to change, et al)
#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength (__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum))

typedef __u8 TMessageID;
typedef __u16 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u8 TDataItemLength;

#pragma pack(push, 1)
	typedef struct {
		TMessageID type;
		TMessagePayloadSize payload_size;
	} TMessageHeader;

typedef union {
	__u32 DataItemID;
	__u8 Did[3]; // might change protocol definition from __u24 to __u32 just for this
} TDataItemID;

typedef struct  {
	TMessageID Type;
	__u32 PayloadCount;
	void *Payloads; // alloc array of TDataItems;
} TParsedMessage;
#pragma pack(pop)

/* prototypes from consts.cpp */
extern const char *err_msg[];
extern const TMessageID ValidMessages[];
extern const TDataItemID ValidDataItemIDs[];



/* prototypes from utility.cpp */
bool isValidMessageID(TMessageID MessageID);
TCheckSum calculateChecksum(__u8 Message[], __u32 MessageLength);
int isValidDataItemID(TDataItemID DataItemID);

/* prototypes from parser.cpp */
int validateDataItemPayload(TDataItemID DataItemID, __u8 *Data, __u32 DataSize);
int validateDataItem(__u8 *DataItem);
int validatePayload(__u8 *Payload, __u32 payload_size);
int validateMessage(__u8 *buf, __u32 received_bytes);

/* prototypes from buildMessages.cpp */
__u8 *buildPayload(TDataItemID ID, __u8 *DataItem, __u8 dataLength, __u8 &payloadLen);
__u8 *buildMessage(TMessageID MessageID, __u8 *Payload, TMessagePayloadSize PayloadLen, __u16 &messageSize);


#endif