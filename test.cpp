#include "parser.h"

int test(__u8 *Msg, int len, const char *TestDescription, int expectedResult)
{

    printf("TEST %s", TestDescription);
    if (expectedResult != ERR_SUCCESS)
        printf(" (testing error code %d, %s), ", expectedResult, err_msg[-expectedResult]);
    printf("\n");
    int result = validateMessage(Msg,len);
    if (result != expectedResult)
        printf("FAIL: result (%d, %s) is not expected result (%d, %s)\n",
               result, err_msg[-result], expectedResult, err_msg[-expectedResult]);
    else
        printf("PASS: result is as expected [%d, %s]\n", result, err_msg[-result]);
    printf("\n");
    return result;
}



int main(void) // "TEST"
{
    __u16 len;
    //          MessageID# | MSG_PAYLOAD_SIZE | MSG_PAYLOAD | CHECKSUM

    __u8 msg1[] = {"Q"       "\4\0"             "\1\0\0\0"    "\xAA"};
    test(msg1, sizeof(msg1)-1, "Message with payload (no data)", ERR_SUCCESS);


    __u8 msg2[] = {"Q"       "\0\0"               ""          "\xAF"};
    test(msg2, sizeof(msg2)-1, "Message with no payload", ERR_SUCCESS); 


     __u8 err1[] = {"Q"      "\0\0"             ""            ""    };
    test(err1, sizeof(err1)-1, "truncated Message", ERR_MSG_TOO_SHORT);
    
    __u8 err2[] = {"Q"       "\0\0"             ""            "\xFE"};
    test(err2, sizeof(err2)-1, "Message with invalid checksum", ERR_MSG_CHECKSUM);
    
     __u8 err3[] = {"Z"   "\2\0"             "\0\1"        "\xFB"}; // NYI
    test(err3, sizeof(err3)-1, "Message to provoke Parse Error -3 (NYI)", ERR_MSG_PARSE);

    __u8 err5[] = { "Z"   "\2\0"             "\0\1"        "\xAC"};
    test(err5, sizeof(err5)-1, "Message with invalid message type", ERR_MSG_ID_UNKNOWN);

    __u8 err6[] = { "Q"   "\7\0"             ""            "\xFF"};
    test(err6, sizeof(err6)-1, "Message with invalid payload size", ERR_MSG_LEN_MISMATCH);

    TMessageID msgID2 = 'R';
    __u8 payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    __u8 *msg4 = buildMessage(msgID2, payload, sizeof(payload), len);
    test(msg4, len, "buildMessage with invalid payload", ERR_MSG_DATAITEM_ID_UNKNOWN);
    free(msg4);


    __u8 err8[] = {"C"    "\4\0"             "\1\2\1""\0"    "\xB5"};
    test(err8, sizeof(err8)-1, "Message with invalid Data Item ID", ERR_MSG_DATAITEM_ID_UNKNOWN);

    TMessageID msgID = 'Q';
    __u8 *msg3 = buildMessage(msgID, NULL, 0, len);
    test(msg3, len, "buildMessage with no payload", ERR_SUCCESS);
    free(msg3);

    TDataItemID payloadID;
    TMessageID msgID4 = 'Q';
    payloadID.DataItemID = 0x00000A;
    __u8 payload4[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    __u8 paylen;
    __u8 *payload2 = buildPayload(payloadID, payload4, sizeof(payload4), paylen);
    __u8 *msg5 = buildMessage(msgID4, payload2, paylen, len);
    test(msg5, len, "buildMessage with payload", ERR_SUCCESS);
    free(payload2);
    free(msg5);


    return 0;
}
