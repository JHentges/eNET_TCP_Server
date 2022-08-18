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

int Log(const std::string message, bool crlf)
{
	logging::log::emit() << message.c_str();
	if (crlf)
		logging::log::emit() << logging::log::endl;
	return 0;
}

int Trace(const std::string message, const source_location &loc)
{
	logging::log::emit<logging::Trace>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << message.c_str() << logging::log::endl;
	return 0;
}


int Error(const std::string message, const source_location &loc)
{
	logging::log::emit<logging::Error>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << message.c_str() << logging::log::endl;
	return 0;
}

int LogBytes(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		logging::log::emit() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}

int TraceBytes(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		logging::log::emit<logging::Trace>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Trace>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")— " << msg.str().c_str();
	return 0;
}

int ErrorBytes(const std::string intro, const TBytes bytes, bool crlf, const source_location &loc)
{
	std::stringstream msg;
	msg << intro;
	for (auto byt : bytes)
		msg << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
	if (crlf)
		msg ;

	if (crlf)
		logging::log::emit<logging::Error>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str() << logging::log::endl;
	else
		logging::log::emit<logging::Error>() << loc.file_name() << " : " << loc.function_name() << "(" << loc.line() << ")" << msg.str().c_str();
	return 0;
}