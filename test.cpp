#include "parser.h"
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
#if defined(TEST_TMessage)
    __u16 len;
    printf("Testing ");
#ifdef TEST_TMessage
    printf("TMessage validates");
#endif
    printf("\n-----------------------------------\n");
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
