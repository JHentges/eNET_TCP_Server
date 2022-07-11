#include <ctime>
#include <random>
#include <iostream>
#include "eNET-types.h"
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


#ifdef TEST_ParseMessage
    TMessageId MId_C = 'C';
    TMessage Message = TMessage(MId_C);

    PTDataItem item(new TDataItem(DataItemIds::BRD_Reset));

    Message.addDataItem(item);
    test(Message.AsBytes(), "TMessage(BRD_Reset())", ERR_SUCCESS);

    cout << "TEST addDataItem(TDIdReadRegister) with offset that's a 32-bit wide register " << endl;
    std::shared_ptr<TDIdReadRegister> read32(new TDIdReadRegister(DataItemIds::REG_Read1, 0x20));
    Message.addDataItem(read32);
    cout << Message.AsString() << endl;

    cout << "----------------------" << endl;
    cout << "TEST addDataItem(TDIdReadRegister)" << endl;
    std::shared_ptr<TDIdReadRegister> read8(new TDIdReadRegister(DataItemIds::REG_Read1, +0x01));
    Message.addDataItem(read8);
    cout << Message.AsString() << endl;

    TError res;
    cout << "----------------------" << endl;
    cout << "TEST FromBytes() of above " << endl;
    TMessage aMsg = TMessage::FromBytes(Message.AsBytes(), res);

    cout << "res = " << (int)res << ", built aMsg without excepting" <<endl;

    string str = aMsg.AsString();
    cout << str << endl;

    cout << "----------------------" << endl;
    item->addData(7).addData(8);

    cout << Message.AsString() << endl;
    test(Message.AsBytes(), "TMessage(BRD_Reset(7,8) (should be (void)))", ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH);

#endif
    }catch (logic_error e){
        cout << endl
             << "!! Exception caught: " << e.what() << endl;
    }

    printf("\nDone.\n");
    return 0;
}
