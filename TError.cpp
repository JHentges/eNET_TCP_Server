#include <memory>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
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

// fetches nanoseconds elapsed since booting
__u64 get_boottimens()
{
	static __u64 first_time = 0;
	struct timespec up;
	clock_gettime(CLOCK_BOOTTIME, &up);
	if (first_time=0)
	{
		first_time = up.tv_sec * 1000000000 + up.tv_nsec;
		Log("first="+std::to_string(first_time));
	}
	return up.tv_sec * 1000000000 + up.tv_nsec - first_time;
}

int Log(const std::string message, bool crlf)
{
	logging::log::emit<logging::Info>() <<  message.c_str();
	if (crlf)
		logging::log::emit<logging::Info>() << logging::log::endl;
	return 0;
}

int Trace(const std::string message, const source_location &loc)
{
	logging::log::emit<logging::Trace>() << get_boottimens() << "] " << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}

int Debug(const std::string message, const source_location &loc)
{
	logging::log::emit<logging::Debug>() << get_boottimens() << "] "  << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}


int Error(const std::string message, const source_location &loc)
{
	logging::log::emit<logging::Error>() << get_boottimens() << "] "  << loc.file_name() << ":" << loc.function_name() << "(" << loc.line() << ") — " << message.c_str() << logging::log::endl;
	return 0;
}

// [  Info ] with TBytes dump
int Log(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		logging::log::emit<logging::Info>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Info>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}

int Trace(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		logging::log::emit<logging::Trace>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Trace>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}

int Debug(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		logging::log::emit<logging::Debug>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Debug>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}


int Error(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		msg ;

	if (crlf)
		logging::log::emit<logging::Error>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Error>() << get_boottimens() << "] "  << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str();
	return 0;
}
