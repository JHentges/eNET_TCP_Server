#include <string>
// #include <memory>
// #include <sstream>
// #include <iostream>
// #include <iomanip>
#include <stdio.h>
// #include <linux/types.h>

#include "logging.h"

timespec firsttime;

std::string elapsedms()
{
	struct timespec up;
	clock_gettime(CLOCK_BOOTTIME, &up);
	if(firsttime.tv_nsec == 0)
		firsttime = up;
	double now = double(up.tv_sec-firsttime.tv_sec) + double(up.tv_nsec-firsttime.tv_nsec) / 1.0e9;
	std::string str = std::to_string(now)+"s";
	return str;
}

// "\033[33m[Warning]\033[0m ";


#ifndef LOG_DISABLE_INFO
int Log(const std::string message, const source_location &loc)
{
	printf("\033[32m[ INFO  ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	return 0;
}

int Log(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[32m[ INFO  ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	return 0;
}
#endif

#ifndef LOG_DISABLE_TRACE
int Trace(const std::string message, const source_location &loc)
{
	printf("\033[36m[ TRACE ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	return 0;
}

int Trace(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[36m[ TRACE ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s%s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	return 0;
}
#endif

#ifndef LOG_DISABLE_DEBUG
int Debug(const std::string message, const source_location &loc)
{
	printf("[ DEBUG ] %14s \033[32m%13s %s\033[0m (%5d) %s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	return 0;
}

int Debug(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("[ DEBUG ] %14s \033[32m%13s %s\033[0m (%5d) %s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");
	return 0;
}
#endif

int Error(const std::string message, const source_location &loc)
{
	printf("\033[31m[ ERROR ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s\n", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), message.c_str());
	return 0;
}

int Error(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	printf("\033[31m[ ERROR ]\033[0m %14s \033[32m%13s %s\033[0m (%5d) %s%s", elapsedms().c_str(), loc.file_name(), loc.function_name(), loc.line(), msg.str().c_str(), crlf?"\n":"");

	return 0;
}
