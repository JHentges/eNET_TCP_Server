#include "parser.h"
#define DEBUG_PRINT_RAW 0

void print_dataitem(__u8 *dataitem)
{
       
    TDataItemLength DataItemSize = dataitem[3];
    TDataItemID dataItemID;
    dataItemID.Did[0] = dataitem[0];
    dataItemID.Did[1] = dataitem[1];
    dataItemID.Did[2] = dataitem[2];
   
    printf("    DataItem: Len: %02x  Did: %06X ", DataItemSize, dataItemID.DataItemID);
    
    __u8 *data = dataitem + sizeof(TDataItemID::Did) + sizeof(TDataItemLength);

  //  int dataLen = len - sizeof(TDataItemID::Did) - sizeof(TDataItemLength);
    printf("Data: ");
    for (int i = 0; i < DataItemSize; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");
}


void print_payload(__u8 *payload, __u32 payload_size) 
{
    printf(" Payload:");
    for (__u32 i = 0; i < payload_size; i++)
        printf("%02x ", payload[i]);
}


void print_message(__u8 Message[], __u32 MessageLength) 
{
    printf("  Message (%d bytes):", MessageLength);
#if DEBUG_PRINT_RAW != 0   
    for (__u32 i = 0; i < MessageLength; i++)
        printf(" %02X", Message[i]);
    printf("\n");
#endif

#if DEBUG_PRINT_RAW != 0
    for (__u32 i = 0; i < sizeof(TMessageID); i++)
        printf("  %02X ", Message[i]);
#endif

    TMessageID MessageID = *(TMessageID *)&Message[0];
    printf("(%02X)-- PayloadSize:", MessageID);

#if DEBUG_PRINT_RAW != 0  
    for (__u32 i = sizeof(TMessageID); i < sizeof(TMessageID) + sizeof(TMessagePayloadSize); i++)
        printf("%02X ", Message[i]);
#endif

    TMessagePayloadSize PayloadSize = *(TMessagePayloadSize *)&Message[sizeof(TMessageID)];
    printf("(%04d)--", PayloadSize);
    if (PayloadSize > 0)
        print_payload(Message + sizeof(TMessageID) + sizeof(TMessagePayloadSize), PayloadSize);
    else
        printf(" No Payload");

    printf(" -- Checksum:");
    for (__u32 i = MessageLength - sizeof(TCheckSum); i < MessageLength; i++)
        printf("%02X ", Message[i]);
    printf("\n");
}
