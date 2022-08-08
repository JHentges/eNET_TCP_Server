#include <ctime>
#include <random>
#include <iostream>
#include <string>
#include "eNET-types.h"
#include "TMessage.h"
using namespace std;
int apci;

#define LOGGING_DEFINE_EXTENDED_OUTPUT_TYPE

#include <experimental/source_location>
using namespace std::experimental;

#include "logging/logging.h"
using namespace ::logging;
#include "logging/logColors.h"

LOGGING_DEFINE_OUTPUT(CC<LoggingType>)

// logging levels can be disabled at compile time
// LOGGING_DISABLE_LEVEL(Error);
// LOGGING_DISABLE_LEVEL(Trace);
// LOGGING_DISABLE_LEVEL(Warning);
// LOGGING_DISABLE_LEVEL(Info);
// LOGGING_DISABLE_LEVEL(Debug);


int logtest(const source_location &loc = source_location::current())
{
    log::emit<Warning>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << ", here= " << source_location::current().function_name() << log::endl;
    log::emit() << "Hello World! with the logging framework" << log::endl;

    log::emit<Trace>() << "Logging a Trace" << log::endl;
    log::emit<Warning>() << "Logging a Warning" << log::endl;
    log::emit<Error>() << "Logging an Error" << log::endl;
    log::emit<Info>() << "Logging an Info" << log::endl;
    log::emit() << "Hello World! with the logging framework"
                << log::endl
                << log::endl;

    // using the extension to colorize the output
    log::emit<Error>() << "Combining console codes"
                       << LogC::reset
                       << log::endl
                       << log::endl;

    log::emit() << "and back to normal color mode"
                << log::endl;

    return 0;
}

TError test(TBytes Msg, const char *TestDescription, TError expectedResult = ERR_SUCCESS)
{

    printf("TEST %s", TestDescription);
    if (expectedResult != ERR_SUCCESS)
        printf(" (testing error code %d, %s), ", expectedResult, err_msg[-expectedResult]);
    printf("\n");
    TError result;
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
             << "if the exception is " << expectedResult << " then this is PASS" << endl;
    }
    printf("----------------------\n");
    return result;
}

int main(void) // "TEST"
{
    logtest();
    return 0;

    try
    {

        __u16 len;
        printf("Testing ");
        printf("\n-----------------------------------\n");

        TMessageId MId_Q = 'Q';
        TMessage M1 = TMessage(MId_Q);
        cout << M1.AsString() << endl;
        test(M1.AsBytes(), "TMessage(Q)");

        TMessageId MId_R = 'R';
        TPayload Payload;
        PTDataItem d2 = unique_ptr<TDataItem>(new TDataItem((DataItemIds)0));
        Payload.push_back(d2);
        TMessage M2 = TMessage(MId_R, Payload);
        cout << M2.AsString() << endl;
        test(M2.AsBytes(), "buildMessage() with Payload");

        DataItemIds DId = (DataItemIds)0;
        PTDataItem d1 = unique_ptr<TDataItem>(new TDataItem(DId, d2->AsBytes()));
        Payload.push_back(d1);
        TMessage M3 = TMessage(MId_Q, Payload);
        cout << M3.AsString() << endl;
        test(M3.AsBytes(), "buildMessage() with two DataItems, one with Data");

        M3.addDataItem(d1).addDataItem(d2);
        cout << M3.AsString() << endl;
        test(M3.AsBytes(), "addDataItem(TDataItem)");

        TMessageId MId_C = 'C';
        TMessage Message = TMessage(MId_C);
        PTDataItem item(new TDataItem(DataItemIds::BRD_Reset));
        Message.addDataItem(item);
        cout << Message.AsString() << endl;
        test(Message.AsBytes(), "TMessage(BRD_Reset())", ERR_SUCCESS);

        cout << "TEST addDataItem(TDIdReadRegister) with offset +20 (that's a 32-bit wide register)" << endl;
        std::shared_ptr<TREG_Read1> read32(new TREG_Read1(DataItemIds::REG_Read1, 0x20));
        Message.addDataItem(read32);
        cout << Message.AsString() << endl;

        cout << "----------------------" << endl;
        cout << "TEST addDataItem(TDIdReadRegister) (from +1, 8-bit)" << endl;
        std::shared_ptr<TREG_Read1> read8(new TREG_Read1(DataItemIds::REG_Read1, +0x01));
        Message.addDataItem(read8);
        cout << Message.AsString() << endl;

        TError res;
        cout << "----------------------" << endl;
        cout << "TEST FromBytes() [serdes round-trip test] of above " << endl;
        TMessage aMsg = TMessage::FromBytes(Message.AsBytes(), res);

        cout << "res = " << res << res << ", built aMsg without excepting" << endl
             << "---- confirm the following text is identical to the prior:" << endl;
        cout << aMsg.AsString() << endl;

        cout << "----------------------" << endl;
        item->addData(7).addData(8);

        cout << Message.AsString() << endl;
        test(Message.AsBytes(), "TMessage(BRD_Reset(7,8) (should be (void)))", ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH);
    }
    catch (logic_error e)
    {
        cout << endl
             << "!! Exception caught: " << e.what() << endl;
    }

    printf("\nDone.\n");
    return 0;
}
