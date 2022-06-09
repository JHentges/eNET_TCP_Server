#define DEBUG_PRINT_RAW

/*
The TCP Server Message is received in a standardized format.
Write functions to
1) Validate the Message received is well-formed (valid Checksum for example)
2) Parse the Message into a queue of Actions
3) Execute the queue

A Message is a sequence of bytes received by the TCP Server (across TCP/IP) in this format:
	SCMidLL_C
where
	S       is the START symbol
	C       is the category of the Message
	Mid     is a Message ID#, which may be mnemonic/logical
	LL      is the Length of an optional payload
	_       is zero or more bytes, the optional payload's data
	C       is the checksum for all bytes in MMMM, LL, and _

Given: checksum is calculated on all bytes in Message except the "start sentinel"; the sum, including the checksum, should be zero

Further, the payload is an ordered list of zero or more Data Items, which are themselves a sequence of bytes, in this format:
	LDid_
where
	L       is the length of the data encapsulated in this data item
	Did     is the Data ID#, which identifies the *type* of data encapsulated in this data item
	_       is the Data Item's payload, or "the data"
*/
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "diag.h"

bool isValidMessageID(TMessageID MessageID) 
{
    bool result = false;
    for (int i = 0; i < sizeof(ValidMessages) / sizeof(TMessageID); i++) {
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
    for (__u32 i = sizeof(TMessageStart); i < MessageLength; i++)        
        checksum += Message[i];
    return checksum;
}

// Checks the Message for well-formedness
// returns 0 if Message is well-formed
int validateMessage(__u8 *buf, __u32 received_bytes) // "NAK()" is shorthand for return error condition etc
{
    print_message(buf, received_bytes);

    if (received_bytes < minimumMessageLength)
        return ERR_MSG_TOO_SHORT; // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?

    TMessageHeader *head = (TMessageHeader *) buf;

    if (head->start != __charStart__)
        return ERR_MSG_START; // NAK(invalid start byte)

    if (! isValidMessageID(head->type))
        return ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageID Category byte)

    __u32 statedMessageLength = minimumMessageLength + head->payload_size;
    if (received_bytes < statedMessageLength)
        return ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data

    TCheckSum checksum = calculateChecksum(buf, statedMessageLength);

    if (__valid_checksum__ !=checksum)
        return ERR_MSG_CHECKSUM; // NAK(invalid checksum)

    return 0; // valid message
}

__u8 * buildMessage(TMessageID MessageID, __u8 *Payload, TMessagePayloadSize PayloadLen, __u16 &messageSize)
{
    messageSize = sizeof(TMessageHeader) + PayloadLen + sizeof(TCheckSum);
    __u8 *Message = (__u8 *) malloc(messageSize);
    Message[0] = __charStart__;
    memcpy(Message + sizeof(TMessageStart), &MessageID, sizeof(TMessageID));
    memcpy(Message + sizeof(TMessageStart) + sizeof(TMessageID), &PayloadLen, sizeof(TMessagePayloadSize));
    memcpy(Message + sizeof(TMessageStart) + sizeof(TMessageID) + sizeof(TMessagePayloadSize), Payload, PayloadLen);
    TCheckSum checksum = -calculateChecksum(Message, sizeof(TMessageHeader) + PayloadLen);
    memcpy(Message + sizeof(TMessageHeader) + PayloadLen, &checksum, sizeof(TCheckSum));
    return Message;
}


int main(void) // "TEST"
{
    //     MSG_START_BYTE | MessageID# | MSG_PAYLOAD_SIZE | MSG_PAYLOAD | CHECKSUM
    __u8 msg1[] = {"$"      "\1\0\0\0"   "\4\0"             "\2\0\1\2"    "\xF6"};
    __u8 msg2[] = {"$"      "\1\0\0\0"   "\0\0"             ""            "\xFF"};


    int result1 = validateMessage(msg1, sizeof(msg1)-1 );
    if (result1) {
        printf("Error: %d, '%s'\n\n", result1, err_msg[-result1]);
    }
    else
        printf("Message with payload is valid\n\n");

    int result2 = validateMessage(msg2, sizeof(msg2)-1 );
    if (result2) {
        printf("Error: %d, '%s'\n\n", result2, err_msg[-result2]);
    }
    else
        printf("Message without payload is valid\n\n");

    __u8 err1[] = {"$"      "\1\0\0\0"   "\0\0"             ""            ""    };

    printf("testing Error -1\n");
    int result4 = validateMessage(err1, sizeof(err1)-1 );
    if (result4) {
        printf("Error: %d, '%s'\n\n", result4, err_msg[-result4]);
    }
    else
        printf("Message is valid\n\n");
    
    printf("testing Error -2\n");
    __u8 err2[] = {"$"      "\1\0\0\0"   "\0\0"             ""            "\xFE"};
    int result5 = validateMessage(err2, sizeof(err2)-1 );
    if (result5) {
        printf("Error: %d, '%s'\n\n", result5, err_msg[-result5]);
    }
    else
        printf("Message is valid\n\n");
    
    printf("testing Error -3 (NYI)\n");
    __u8 err3[] = {"$"      "\2\0\0\0"   "\2\0"             "\0\1"        "\xFB"}; // NYI
    int result6 = validateMessage(err3, sizeof(err3)-1 );
    if (result6) {
        printf("Error: %d, '%s'\n\n", result6, err_msg[-result6]);
    }
    else
        printf("Message is valid\n\n");

    printf("testing Error -4\n");
    __u8 err4[] = {"#"      "\1\0\0\0"   "\0\0"             ""            "\xFF"};
    int result7 = validateMessage(err4, sizeof(err4)-1 );
    if (result7) {
        printf("Error: %d, '%s'\n\n", result7, err_msg[-result7]);
    }
    else
        printf("Message is valid\n\n");

    printf("testing Error -5\n");
    __u8 err5[] = {"$"      "\2\0\1\0"   "\2\0"             "\0\1"        "\xFB"};
    int result8 = validateMessage(err5, sizeof(err5)-1 );
    if (result8) {
        printf("Error: %d, '%s'\n\n", result8, err_msg[-result8]);
    }
    else
        printf("Message is valid\n\n");

    printf("testing Error -6\n");
    __u8 err6[] = {"$"      "\1\0\0\0"   "\7\0"             ""            "\xFF"};
    int result9 = validateMessage(err6, sizeof(err6)-1 );
    if (result9) {
        printf("Error: %d, '%s'\n\n", result9, err_msg[-result9]);
    }
    else
        printf("Message is valid\n\n");

    TMessageID msgID = 0x00000009;
    __u16 len;
    __u8 *msg3 = buildMessage(msgID, NULL, 0, len);
    printf("testing buildMessage, no payload\n");
    int result10 = validateMessage(msg3,len );
    if (result10) {
        printf("Error: %d, '%s'\n\n", result10, err_msg[-result10]);
    }
    else
        printf("Message is valid\n\n");

    TMessageID msgID2 = 0x0000000A;
    __u8 payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    __u8 *msg4 = buildMessage(msgID2, payload, sizeof(payload), len);
    printf("testing buildMessage, with payload\n");
    int result11 = validateMessage(msg4,len );
    if (result11) {
        printf("Error: %d, '%s'\n\n", result11, err_msg[-result11]);
    }
    else
        printf("Message is valid\n\n");

    return 0;
}
