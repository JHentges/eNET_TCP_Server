#pragma once
/*
	Provides the TError stuff
*/

#include <linux/types.h>
// TError (__u32) is intended to be replaced buy a class someday
typedef __u32 TError;

#include "eNET-types.h"

/* TError stuff */

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

// const TError ErrorList[] = {
// 	{-1, "ERR: Message Too Short"},
// 	{-2, "ERR: Checksum Incorrect"},
// 	{-3, "ERR: Parsing Failed"},
// 	{-4, "ERR: Bad Start Character (Deprecated)"},
// 	{-5, "ERR: Unrecognized MId"},
// 	{-6, "ERR: Received MSG Len != Reported Len"},
// 	// ... etc
// };

