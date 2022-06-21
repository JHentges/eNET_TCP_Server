#define __diag_print_csum 0

#ifndef parserh
#define parserh

#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
using std::vector, std::begin, std::end;

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


/* type definitions */
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


// CLASSY ----------------------------

class TDataItem {
	protected:
		TDataItemID Id;
	public:
		TDataItemID getID() { return Id; }
		TDataItem & setID(__u32 ID) { Id.DataItemID = ID & 0x00FFFFFF; return *this;}
		TDataItem & setID(__u8 ID[3]) { Id.DataItemID = 0; Id.Did[0] = ID[0]; Id.Did[1] = ID[1];  Id.Did[2] = ID[2]; return *this; }
	/* constructors that set the initial DataItemID */
	TDataItem(){ Id.DataItemID = 0; }
	TDataItem(TDataItemID ID) { Id = ID; }
	TDataItem(__u32 ID){ this->setID(ID); }
	TDataItem(__u8 ID[3]) { this->setID(ID); }

	public:
		std::vector<__u8> Data;
	/* constructors that set the initial ID and Data */
	TDataItem(TDataItemID ID, vector<__u8> DATA) { Id = ID; Data = DATA; }

	public:
		std::vector<__u8> AsBytes(){
			vector<__u8> bytes;
			bytes.push_back(Id.Did[0]);
			bytes.push_back(Id.Did[1]);
			bytes.push_back(Id.Did[2]);
			__u8 len = Data.size();
			bytes.push_back(len);
			bytes.insert(end(bytes), begin(Data), end(Data));
			return bytes;
		}
};

class TMessage
{
	protected:
		TMessageID Id;
		TCheckSum Csum;
	public:
		TMessageID getID() { return Id; }
		TMessage &setID(TMessageID ID) { Id = ID; return *this; }
	/* constructors that set the initial MessageID */
	TMessage(TMessageID ID) { Id = ID; }
	
	public:
		std::vector<TDataItem> DataItems;
	/* constructors that set the initial MessageID and Payload */
		TMessage(TMessageID ID, vector<TDataItem> Payload){ Id = ID; DataItems = Payload; }

	public:
		vector<__u8> AsBytes(){
			__u16 payloadLength = 0;
			for (TDataItem item : DataItems){
				payloadLength += item.AsBytes().size();
				//printf("item.AsBytes().size() == %ld, payloadLength == %d\n", item.AsBytes().size(), payloadLength);
			}
			vector<__u8> bytes;

			__u8 msb = (payloadLength >> 8) & 0xFF;
			__u8 lsb = payloadLength & 0xFF;
			bytes.push_back(Id); /// WARN: only works because TMessageID == __u8
			//printf("Message ID == %02X, payloadLength == %hd, LSB(PLen)=%hhd MSB=%hhd\n", Id, payloadLength, lsb, msb);
			bytes.push_back(lsb);
			bytes.push_back(msb);
			for (TDataItem item : DataItems)
			{
				vector<__u8> byt = item.AsBytes();
				bytes.insert(end(bytes), begin(byt), end(byt));
			}

			__u8 csum = -calculateChecksum(bytes.data(), bytes.size());
			bytes.push_back(csum);
#if __diag_print_csum == 1
			printf("RAW TMessage.AsBytes(): ");
			for (__u8 byt : bytes)
			{
				printf("%02X ", byt);
			}
			printf("\n");
#endif
			return bytes;
		}
};

#endif