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