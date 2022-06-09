
void print_payload(__u8 *payload, __u32 payload_size) 
{
    for (__u32 i = 0; i < payload_size; i++)
        printf("%02x ", payload[i]);
}


void print_message(__u8 Message[], __u32 MessageLength) 
{
    printf("Message (%d bytes):", MessageLength);
#ifdef DEBUG_PRINT_RAW    
    for (__u32 i = 0; i < MessageLength; i++)
        printf(" %02X", Message[i]);
    printf("\n");
#endif

    printf("Start:%c -- MID:", Message[0]);

#ifdef DEBUG_PRINT_RAW    
    for (__u32 i = 1; i <= sizeof(TMessageID); i++)
        printf("%02X ", Message[i]);
#endif

    TMessageID MessageID = *(TMessageID *)&Message[1];
    printf("(%08X)-- PayloadSize:", MessageID);

#ifdef DEBUG_PRINT_RAW    
    for (__u32 i = sizeof(TMessageID) + 1; i <= sizeof(TMessageID) + sizeof(TMessagePayloadSize); i++)
        printf("%02X ", Message[i]);
#endif

    TMessagePayloadSize PayloadSize = *(TMessagePayloadSize *)&Message[sizeof(TMessageID) + 1];
    printf("(%04d)-- Payload:", PayloadSize);

    print_payload(Message + sizeof(TMessageID) + sizeof(TMessagePayloadSize) + 1, PayloadSize);

    printf(" -- Checksum:");
    for (__u32 i = MessageLength - sizeof(TCheckSum); i < MessageLength; i++)
        printf("%02X ", Message[i]);
    printf("\n");
}
