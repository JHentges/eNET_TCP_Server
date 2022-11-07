#pragma once
#include <string>
// #include <memory>
// #include <sstream>
// #include <iostream>
// #include <iomanip>
// #include <stdio.h>
// #include <linux/types.h>
#include "eNET-types.h"
/* Logging Stuff */

#define LOG_DISABLE_TRACE
// #define LOG_DISABLE_INFO
#define LOG_DISABLE_DEBUG
// #define LOG_DISABLE_ERROR

#include <experimental/source_location>
using namespace std::experimental;

// logging levels can be disabled at compile time
#ifdef LOG_DISABLE_TRACE
#define Trace(...) {}
#else
int Trace(std::string message, const source_location &loc = source_location::current());
int Trace(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
#endif

#ifdef LOG_DISABLE_WARNING
#define Warn(...) {}
#endif

#ifdef LOG_DISABLE_INFO
#define Log(...) {}
#else
int Log(  std::string message, const source_location &loc = source_location::current());
int Log(  std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
#endif

#ifdef LOG_DISABLE_DEBUG
#define Debug(...) {}
#else
int Debug(std::string message, const source_location &loc = source_location::current());
int Debug(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
#endif

int Error(std::string message, const source_location &loc = source_location::current());
int Error(std::string intro, TBytes bytes, bool crlf = true, const source_location &loc = source_location::current());
