#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "TError.h"

const char *err_msg[] = {
	/*   0 */ "VALID",
	/*  -1 */ "Message too short",
	/*  -2 */ "Checksum error",
	/*  -3 */ "Parse error",
	/*  -4 */ "Invalid Start symbol found",
	/*  -5 */ "Unknown Message ID",
	/*  -6 */ "Length mismatch",
	/*  -7 */ "DataItem length mismatch",
	/*  -8 */ "DataItem ID unknown",
	/*  -9 */ "DataItem too short",
	/* -10 */ "DataItem Payload error",
	/* -11 */ "DId Offset Arg error",
	/* -12 */ "DId Invalid",
	/* -13 */ "Not Yet Implemented",
	/* -14 */ "ADC Busy",
	/* -15 */ "ADC FATAL",
};

timespec firsttime;

std::string elapsedms()
{
	struct timespec up;
	clock_gettime(CLOCK_BOOTTIME, &up);
	if(firsttime.tv_nsec == 0)
		firsttime = up;
	double now = double(up.tv_sec-firsttime.tv_sec) + double(up.tv_nsec-firsttime.tv_nsec) / 1.0e9;
	return std::to_string(now) + "sec] ";
}

// "\033[33m[Warning]\033[0m ";


#ifndef LOG_DISABLE_INFO
int Log(const std::string message, const source_location &loc)
{
	printf("\033[32m[  INFO ]\033[0m %s%s:%s(%d)%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	//logging::log::emit<logging::Info>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << message.c_str() << logging::log::endl;
	return 0;
}
// [  Info ] with TBytes dump
int Log(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[32m[  INFO ]\033[0m %s%s:%s(%d)%s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	// if (crlf)
	// 	logging::log::emit<logging::Info>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	// else
	// 	logging::log::emit<logging::Info>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}
#endif

#ifndef LOG_DISABLE_TRACE
int Trace(const std::string message, const source_location &loc)
{
	printf("\033[36m[ TRACE ]\033[0m %s%s:%s(%d)%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	// logging::log::emit<logging::Trace>() << elapsedms().c_str()<< loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}

int Trace(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[36m[ TRACE ]\033[0m %s%s:%s(%d)%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	// if (crlf)
	// 	logging::log::emit<logging::Trace>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	// else
	// 	logging::log::emit<logging::Trace>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}
#endif

#ifndef LOG_DISABLE_DEBUG
int Debug(const std::string message, const source_location &loc)
{
	printf("[ DEBUG ] %s%s:%s(%d)%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	//logging::log::emit<logging::Debug>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}

int Debug(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("[ DEBUG ] %s%s:%s(%d)%s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	// if (crlf)
	// 	logging::log::emit<logging::Debug>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	// else
	// 	logging::log::emit<logging::Debug>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}
#endif

int Error(const std::string message, const source_location &loc)
{
	printf("\033[31m[ ERROR ]\033[0m %s%s:%s(%d)%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	//logging::log::emit<logging::Error>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}

int Error(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[31m[ ERROR ]\033[0m %s%s:%s(%d)%s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");

	// if (crlf)
	// 	logging::log::emit<logging::Error>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str() << logging::log::endl;
	// else
	// 	logging::log::emit<logging::Error>() << elapsedms().c_str() << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str();
	return 0;
}
