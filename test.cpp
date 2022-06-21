#include "parser.h"
//#define TEST_Validators
//#define TEST_buildMessage
//#define TEST_buildPayload
#define TEST_TMessage

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
    printf("----------------------\n");
    return result;
}
int test(TMessage Msg, const char *TestDescription, int expectedResult)
{
    vector<__u8> byts = Msg.AsBytes();
    return test(byts.data(), byts.size(), TestDescription, expectedResult);
}

int main(void) // "TEST"
{
#if defined(TEST_Validators) || defined(TEST_buildMessage) || defined(TEST_buildPayload) || defined(TEST_TMessage)
    __u16 len;
    printf("Testing ");
#ifdef TEST_Validators
    printf("validateMessage(byte[]), ");
#endif
#ifdef TEST_buildMessage
    printf("buildMessage() sans Payload, ");
#endif
#ifdef TEST_buildPayload
    printf("validateMessage() using buildPayload(), ");
#endif
#ifdef TEST_TMessage
    printf("TMessage validates");
#endif
    printf("\n-----------------------------------\n");
#endif
#ifdef TEST_Validators
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

    __u8 err8[] = {"C"    "\4\0"             "\1\2\1""\0"    "\xB5"};
    test(err8, sizeof(err8)-1, "Message with invalid Data Item ID", ERR_MSG_DATAITEM_ID_UNKNOWN);
#endif
#ifdef TEST_buildMessage
    TMessageID msgID = 'Q';
    __u8 *msg3 = buildMessage(msgID, NULL, 0, len);
    test(msg3, len, "buildMessage with no payload", ERR_SUCCESS);
    free(msg3);
    
    TMessageID msgID2 = 'R';
    __u8 payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    __u8 *msg4 = buildMessage(msgID2, payload, sizeof(payload), len);
    test(msg4, len, "buildMessage with invalid payload", ERR_MSG_DATAITEM_ID_UNKNOWN);
    free(msg4);
#endif
#ifdef TEST_buildPayload
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
#endif

#ifdef TEST_TMessage
    TMessageID Id = 'Q';
    TMessage M1 = TMessage(Id);
    vector<__u8> B1 = M1.AsBytes();
    test(B1.data(), B1.size(), "buildMessage()", ERR_SUCCESS);

    vector<TDataItem> Payload;
    TDataItem d2 = TDataItem(2);
    Payload.push_back(d2);

    TMessage M2 = TMessage(Id, Payload);
    vector<__u8> B2 = M2.AsBytes();
    test(B2.data(), B2.size(), "buildMessage() with Payload", ERR_SUCCESS);

    TDataItemID DId;
    DId.DataItemID = 1;
    TDataItem d1 = TDataItem(DId, B1);
    Payload.push_back(d1);
    TMessage M3 = TMessage(Id, Payload);
    vector<__u8> B3 = M3.AsBytes();
    test(B3.data(), B3.size(), "buildMessage() with two DataItems, one with Data", ERR_SUCCESS);

    test(M3, "buildMessage using test(TMessage)", ERR_SUCCESS);

#endif

    return 0;
}
