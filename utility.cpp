#include "parser.h"

const TMessageID ValidMessages[] = {
   'Q','R','C',
    // NOTE: FOURCC("QDIn") type of constants are expected to be used for the Message IDs, eventually.
};

const TDataItemID ValidDataItemIDs[] = {
	0x000001, 0x000002, 0x000003, 0x000004, 0x000005,
	0x000006, 0x000007, 0x000008, 0x000009, 0x00000a
	// NOTE: FOURCC("QDIn") type of constants are expected to be used for the DataItem IDs, eventually.
};

bool isValidMessageID(TMessageID MessageID) 
{
    bool result = false;
    for (int i = 0; i < sizeof(ValidMessages) / sizeof(TMessageID) ; i++) {
        if (ValidMessages[i] == MessageID) {
            result = true;
            break;
        }
    }
    return result;
}

// Sums all bytes in Message except the start sentinel, specifically including the checksum byte
// WARNING: only works if TChecksum is a byte
TCheckSum calculateChecksum(__u8 Message[], __u32 MessageLength) 
{
    TCheckSum checksum = 0;
    for (__u32 i = 0; i < MessageLength-1; i++)        
        checksum += Message[i];

    if (__diag_print_csum != 0)
     printf("                                 calculated checksum: %02X\n", (-checksum & 0xFF));

    checksum += Message[MessageLength-1];
    return checksum;
}

int isValidDataItemID(TDataItemID DataItemID) 
{
    int result = false;   
    for (int i = 0; i < sizeof(ValidDataItemIDs) / sizeof(TDataItemID); i++) {
        if (ValidDataItemIDs[i].DataItemID == DataItemID.DataItemID) {
            result = true;
            break;
        }
    }
    return result;
}