#include "parser.h"

__u8 * buildPayload(TDataItemID ID, __u8 *DataItem, __u8 dataLength, __u8 &payloadLen)
{
    payloadLen = sizeof(TDataItemLength) + sizeof(TDataItemID) + dataLength;
    __u8 *payload = (__u8 *) malloc(payloadLen);
    memcpy(payload, ID.Did, 3); // 3 bytes
    payload[3] = dataLength+1; // only works if TDataItemLength is a byte
    memcpy(payload + sizeof(TDataItemLength) + sizeof(TDataItemID), DataItem, dataLength);
    return payload;
}


__u8 * buildMessage(TMessageID MessageID, __u8 *Payload, TMessagePayloadSize PayloadLen, __u16 &messageSize)
{
    messageSize = sizeof(TMessageHeader) + PayloadLen + sizeof(TCheckSum);
    __u8 *Message = (__u8 *) malloc(messageSize);
    memcpy(Message, &MessageID, sizeof(TMessageID));
    memcpy(Message + sizeof(TMessageID), &PayloadLen, sizeof(TMessagePayloadSize));
    memcpy(Message + sizeof(TMessageID) + sizeof(TMessagePayloadSize), Payload, PayloadLen);
    TCheckSum checksum = -calculateChecksum(Message, sizeof(TMessageHeader) + PayloadLen);
    memcpy(Message + sizeof(TMessageHeader) + PayloadLen, &checksum, sizeof(TCheckSum));
    return Message;
}

