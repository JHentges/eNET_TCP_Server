#ifndef parserh
#define parserh

#define ERR_MSG_TOO_SHORT -1
#define ERR_MSG_CHECKSUM -2
#define ERR_MSG_PARSE -3
#define ERR_MSG_START -4
#define ERR_MSG_ID_UNKNOWN -5
#define ERR_MSG_LEN_MISMATCH -6

const char *err_msg[] = {
    "VALID",
    "Message too short",
    "Checksum error",
    "Parse error",
    "Invalid Start symbol found",
    "Unknown message ID",
    "Length mismatch"
};

#define __charStart__ (TMessageStart)('$')  // for illustrative purposes (i.e., "for now", TBD, subject to change, et al)
#define __valid_checksum__ (TCheckSum)(0)
#define minimumMessageLength (__u32)(sizeof(TMessageHeader) + sizeof(TCheckSum))

typedef __u8 TMessageStart;
typedef __u32 TMessageID;
typedef __u16 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u8 TDataItemLength;

#pragma pack(push, 1)
	typedef struct {
		TMessageStart start;
		TMessageID type;
		TMessagePayloadSize payload_size;
	} TMessageHeader;

typedef struct  {
	TDataItemLength length;
	__u8 ID[3]; // might change protocol definition from __u24 to __u32 just for this
} TDataItemHeader;

typedef struct  {
	TDataItemHeader head;
	__u32 datalen;
	void *data; // alloc array of __u8
} TDataItem;

typedef struct  {
	TMessageID Type;
	__u32 PayloadCount;
	void *Payloads; // alloc array of TDataItems;
} TParsedMessage;
#pragma pack(pop)

TMessageID ValidMessages[] = {
    0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005,
    0x00000006, 0x00000007, 0x00000008, 0x00000009, 0x0000000a
    // NOTE: FOURCC("QDIn") type of constants are expected to be used for the Message IDs, eventually.
};


#endif