#include <ctime>
#include <random>
#include <iostream>
#include "TMessage.h"

using namespace std;

//#define TEST_TMessage
#define TEST_ParseMessage

int test(TBytes Msg, const char *TestDescription, int expectedResult=ERR_SUCCESS)
{

    printf("TEST %s", TestDescription);
    if (expectedResult != ERR_SUCCESS)
        printf(" (testing error code %d, %s), ", expectedResult, err_msg[-expectedResult]);
    printf("\n");
    int result = 0;
    try
    {
        result = TMessage::validateMessage(Msg);
        if (result != expectedResult)
            printf("FAIL: result (%d, %s) is not expected result (%d, %s)\n",
                result, err_msg[-result], expectedResult, err_msg[-expectedResult]);
        else
            printf("PASS: result is as expected [%d, %s]\n", result, err_msg[-result]);
    }
    catch (logic_error e)
    {
        cout << endl
             << "Exception caught: " << e.what() << endl
             << "if the exception is " << err_msg[-expectedResult] << " then this is PASS" << endl;
    }
    printf("----------------------\n");
    return result;
}

int main(void) // "TEST"
{
    try{
#if defined(TEST_TMessage)
    __u16 len;
    printf("Testing ");
#ifdef TEST_TMessage
    printf("TMessage validates");
#endif
#endif
    printf("\n-----------------------------------\n");

#ifdef TEST_TMessage
    TMessageId MId_Q = 'Q';
    TMessage M1 = TMessage(MId_Q);
    cout << M1.AsString() << endl;

    test(M1.AsBytes(), "buildMessage()");

    TMessageId MId_R = 'R';
    vector<TDataItem*> Payload;
    TDataItem d2 = TDataItem(2);
    Payload.push_back(&d2);
    TMessage M2 = TMessage(MId_R, Payload);
    cout << M2.AsString() << endl;
    test(M2.AsBytes(), "buildMessage() with Payload");

    TDataId DId = 1;
    TDataItem d1 = TDataItem(DId, M1.AsBytes());
    Payload.push_back(&d1);
    TMessage M3 = TMessage(MId_Q, Payload);
    cout << M3.AsString() << endl;
    test(M3.AsBytes(), "buildMessage() with two DataItems, one with Data");

    M3.addDataItem(&d1).addDataItem(&d2);
    cout << M3.AsString() << endl;
    test(M3.AsBytes(), "addDataItem(TDataItem)");
#endif

    // srand(time(0));

#ifdef TEST_ParseMessage
    TMessageId MId_C = 'C';
    TMessage Message = TMessage(MId_C);
   
    TDataItem item = TDataItem(6).addData(7).addData(8);
    Message.addDataItem(&item);
    
    cout << Message.AsString() << endl;
    test(Message.AsBytes(), "TMessage(MId) constructor, one DataItem");

    // cout << "TEST addDataItem(DIdReadRegister)" << endl;
    // DIdReadRegister read8 = DIdReadRegister(DIdRead8, 1);
    // TBytes thebytes = read8.AsBytes();
    // Message.addDataItem(&read8);
    // cout << Message.AsString() << endl;

    TError res;
    cout <<endl<< "TEST FromBytes() of above" << endl;
    try
    {
        TBytes buf = Message.AsBytes();
        TMessage aMsg = TMessage::FromBytes(buf, res);
        cout << "aMsg created using TMessage::FromBytes()" << endl;

        TBytes buf2 = aMsg.AsBytes();
        printf("buf2 from aMsg.AsBytes() = ");
        for (auto byt : buf)
            printf("%02hhX ", byt);
        printf("\n");

        string str = aMsg.AsString();
        cout << str << endl;
        printf("passed because exception would've skipped this if it was going to fail.\n");
    }
    catch (logic_error e)
    {
        printf("!! Exception: %s\n", e.what());
        printf("FAILED, FromBytes returned %d [%s]\n\n", res, err_msg[-res]);
    }
    catch (bad_alloc e)
    {
        printf("!! Exception: %s\n", e.what());
        printf("FAILED, FromBytes returned %d [%s]\n\n", res, err_msg[-res]);
    }

#endif
    }catch (logic_error e){
        cout << endl
             << "!! Exception caught: " << e.what() << endl;
    }
    cout << endl
         << "WARNING: TMessage's Payload field is a vector of pointers to TDataItems.  I'm concerned about scope..." << endl;
    printf("\nDone.\n");
    return 0;
}
