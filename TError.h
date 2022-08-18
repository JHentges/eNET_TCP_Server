#pragma once
#include <linux/types.h>


#include "eNET-types.h"

#define LOGGING_DEFINE_EXTENDED_OUTPUT_TYPE

#include <experimental/source_location>
using namespace std::experimental;

#include "logging/logging.h"
//using namespace ::logging;
#include "logging/logColors.h"

LOGGING_DEFINE_OUTPUT(CC<LoggingType>)

// logging levels can be disabled at compile time
// LOGGING_DISABLE_LEVEL(logging::Error);
// LOGGING_DISABLE_LEVEL(Trace);
// LOGGING_DISABLE_LEVEL(Warning);
// LOGGING_DISABLE_LEVEL(Info);
// LOGGING_DISABLE_LEVEL(Debug);

	// log::emit<Warning>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << ", here= " << source_location::current().function_name() << log::endl;
	// log::emit() << "Hello World! with the logging framework" << log::endl;

	// log::emit<logging::Trace>() << "Logging a Trace" << log::endl;
	// log::emit<Warning>() << "Logging a Warning" << log::endl;
	// log::emit<logging::Error>() << "Logging an Error" << log::endl;
	// log::emit<Info>() << "Logging an Info" << log::endl;
	// log::emit() << "Hello World! with the logging framework" << log::endl << log::endl;
int Log(const std::string message, bool crlf = false);
int Trace(std::string message, const source_location &loc = source_location::current());
int Error(std::string message, const source_location &loc = source_location::current());
int LogBytes(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
int TraceBytes(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
int ErrorBytes(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());


#define _INVALID_MESSAGEID_ ((TMessageId)-1)

// TODO: Change the #defined errors, and the const char *err_msg[], to an error class/struct/array
#define ERR_SUCCESS 0
#define ERR_MSG_TOO_SHORT -1
#define ERR_MSG_CHECKSUM -2
#define ERR_MSG_PARSE -3
#define ERR_MSG_START -4
#define ERR_MSG_ID_UNKNOWN -5
#define ERR_MSG_LEN_MISMATCH -6
#define ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH -7
#define ERR_MSG_DATAITEM_ID_UNKNOWN -8
#define ERR_MSG_DATAITEM_TOO_SHORT -9
#define ERR_DId_BAD_PARAM -10
#define ERR_DId_BAD_OFFSET -11
#define ERR_DId_INVALID -12
#define ERR_NYI -13
#define ERR_ADC_BUSY -14
#define ERR_ADC_FATAL -15
extern const char *err_msg[];
typedef __u32 TError;

// // --------------------- New, classy, TError, below ------------------
// typedef __s32 TStatusCode;
// typedef std::string TStatusDescription;

// class TError {
// 	public:
// 		TStatusCode Code;
// 		TStatusDescription Message;
// 		TError(TStatusCode Code = ERR_SUCCESS, TStatusDescription description = ""){}
// 		//TStatusCode operator=(TError anError) { return Code; }
// 		// operator int() const { return Code; }
// 		// operator std::string() const { return Message.c_str(); }
// 		// friend std::ostream& operator<<(std::ostream& os, const TError& err);
// };

// // std::ostream& operator<<(std::ostream& os, const TError& err)
// // {
// //	 os << err.Code << ": " << err.Message;
// //	 return os;
// // }

// class TErrorDetailed : public TError
// {
// 	public:
// 		// per-Error type pointer to struct with fields providing details
// 		std::unique_ptr<void> Details;
// 		time_t Timestamp;
// };

// class TError_Parse : public TErrorDetailed
// {
// 	struct TError_Parse_Details
// 	{
// 		size_t NumBytesReceived;
// 		TBytes ReceivedBytes;
// 	};

// 	public:
// 		std::unique_ptr<struct TError_Parse_Details> Details;
// };

// const TError ErrorList[] = {
// 	{-1, "ERR: Message Too Short"},
// 	{-2, "ERR: Checksum Incorrect"},
// 	{-3, "ERR: Parsing Failed"},
// 	{-4, "ERR: Bad Start Character (Deprecated)"},
// 	{-5, "ERR: Unrecognized MId"},
// 	{-6, "ERR: Received MSG Len != Reported Len"},
// 	// ... etc
// };