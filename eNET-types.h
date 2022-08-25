#pragma once

/*

This file is intended to provide standardized type names for use in ACCES' eNET- Series devices.
This improves portability and flexibility; changing the underlying type of a thing is easier if
has a unique type name.
(Frex, we changed from 2-byte to 4-byte TMessage Payload Lengths with only one line in this file)
	(well, plus some bug-fixing, because *that* goal was aspirational.)

This file also has some other crap in it, like some utility stuff (hexbyte()) and the enum for
TDataItem IDs, but that should change.

*/

#ifdef __linux__
#include <linux/types.h>
#else // define our base types for compiling on Windows
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
#endif

#include <cstring>
#include <string>
#include <iomanip>
#include <memory>
#include <vector>
#include <thread>

#include "safe_queue.h"

/* type definitions */
typedef std::vector<__u8> TBytes;
typedef __u8 TMessageId;
typedef __u32 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u16 TDataId;
typedef __u16 TDataItemLength;

class TDataItem;

typedef std::shared_ptr<TDataItem> PTDataItem;
typedef std::vector<PTDataItem> TPayload;

#define hexbyte(x, w) "0x" << std::hex << setfill('0') << std::setw(w) << std::uppercase << static_cast<int>(x)
/// Convert integer value `val` to text in hexadecimal format. The minimum width is padded with leading zeros; if not
/// specified, this `width` is derived from the type of the argument. Function suitable from char to long long.
/// Pointers, floating point values, etc. are not supported; passing them will result in an (intentional!) compiler error.
/// Basics from: http://stackoverflow.com/a/5100745/2932052
// template <typename T>
// inline std::string to_hex(T val, size_t width=sizeof(T)*2)
// {
//     std::stringstream ss;
//     ss << std::setfill('0') << std::setw(width) << std::hex << (val|0);
//     return ss.str();
// }

template <typename T>
inline std::string to_hex(T i)
{
	// Ensure this function is called with a template parameter that makes sense. Note: static_assert is only available in C++11 and higher.
	static_assert(std::is_integral<T>::value, "Template argument 'T' must be a fundamental integer type (e.g. int, short, etc..).");

	std::stringstream stream;
	stream << /*std::string("0x") <<*/ std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex;

	// If T is an 8-bit integer type (e.g. uint8_t or int8_t) it will be
	// treated as an ASCII code, giving the wrong result. So we use C++17's
	// "if constexpr" to have the compiler decides at compile-time if it's
	// converting an 8-bit int or not.
	if constexpr (std::is_same_v<std::uint8_t, T>)
	{
		// Unsigned 8-bit unsigned int type. Cast to int (thanks Lincoln) to
		// avoid ASCII code interpretation of the int. The number of hex digits
		// in the  returned string will still be two, which is correct for 8 bits,
		// because of the 'sizeof(T)' above.
		stream << static_cast<int>(i);
	}
	else if (std::is_same_v<std::int8_t, T>)
	{
		// For 8-bit signed int, same as above, except we must first cast to unsigned
		// int, because values above 127d (0x7f) in the int will cause further issues.
		// if we cast directly to int.
		stream << static_cast<int>(static_cast<uint8_t>(i));
	}
	else
	{
		// No cast needed for ints wider than 8 bits.
		stream << i;
	}

	return stream.str();
}
#pragma region TDataItem DId enum

/* DId.h
   This header is where all the DIds are defined, including what classes handle each
*/

#define _INVALID_DATAITEMID_ ((TDataId)-1)

enum DataItemIds : TDataId // specific numbering, ordering, and grouping are preliminary
{
	INVALID = _INVALID_DATAITEMID_,
	// Note 1: the "TLA_" DIds (e.g., `BRD_` and `REG_` et al; i.e., those DId names that don't have anything after the `_`)
	//         return human-readable text of all TLA_ category DIds and what they do & why
	BRD_ = 0x0000, // Query Only.
	BRD_Reset,

	BRD_stuff_needed_for_control_and_diagnostics_of_Linux_TCPIP_WDG_DEF_ETC, // TBD, long list

	REG_ = 0x100, // Query Only.
	// NOTE: REG_ister access functionality does not allow (at this time) specifying 8-. 16-, or 32-bit access width.
	//       Instead, the width is determined automatically from the register offset, because none of the eNET-AIO registers are flexible
	//    Additionally, the TDataItem::fromBytes() factory method will throw an error if an invalid offset is passed,
	//       like offset=0x41 is invalid because 0x40 is a 32-bit register
	// NOTE: int widthFromOffset(int ofs) is used to determine the register width but it is hard-coded, specific to eNET-AIO, by ranges
	//       of offsets. We'll want the aioenetd to eventually support OTHER (non-eNET-AIO16-128A Family) eNET- boards so this will
	//       need to be a configurable, preferably read off the hardware (although "from eMMC" is probably sufficient)
	REG_Read1,
	REG_ReadAll,
	REG_ReadSome,
	REG_ReadBuf, // like draining a FIFO, TODO: improve name
	REG_Write1,
	REG_WriteSome,
	REG_WriteBuf, // like filling a FIFO, TODO: improve name

	REG_ClearBits,
	REG_SetBits,
	REG_ToggleBits,

	DAC_ = 0x200, // Query Only. *1
	DAC_Output1,
	DAC_OutputAll,
	DAC_OutputSome,
	DAC_Configure1,
	DAC_ConfigureAll,
	DAC_ConfigureSome,
	DAC_ConfigAndOutput1,
	DAC_ConfigAndOutputAll,
	DAC_ConfigAndOutputSome,
	DAC_ReadbackAll,

	DIO_ = 0x300, // Query Only. *1
	DIO_Configure1,
	DIO_ConfigureAll,
	DIO_ConfigureSome,
	DIO_Input1,
	DIO_InputAll,
	DIO_InputSome,
	DIO_InputBuf1,
	DIO_InputBufAll,
	DIO_InputBufSome, // repeated unpaced reads of Digital Inputs; NOTE: not sure this is useful
	DIO_Output1,
	DIO_OutputAll,
	DIO_OutputSome,
	DIO_OutputBuf, // like unpaced waveform output; NOTE: not sure this is useful
	DIO_ConfigureReadWriteReadSome,
	DIO_Clear1,
	DIO_ClearAll,
	DIO_ClearSome,
	DIO_Set1,
	DIO_SetAll,
	DIO_SetSome,
	DIO_Toggle1,
	DIO_ToggleAll,
	DIO_ToggleSome,
	DIO_Pulse1,
	DIO_PulseAll,
	DIO_PulseSome,

	PWM_ = 0x400, // Query Only. *1
	PWM_Configure1,
	PWM_ConfigureAll,
	PWM_ConfigureSome,
	PWM_Input1,
	PWM_InputAll,
	PWM_InputSome,
	PWM_Output1,
	PWM_OutputAll,
	PWM_OutputSome,

	ADC_ = 0x1000,				   // Query Only. *1
	ADC_ConfigurationOfEverything, // i.e., MId "C" to do ADC_ConfigureEverything; MId "Q" to do ADC_QueryEverythingsConfiguration
	ADC_Range1,
	ADC_RangeAll,
	ADC_RangeSome,
	ADC_Span1,
	ADC_SpanAll,
	ADC_SpanSome,
	ADC_Offset1,
	ADC_OffsetAll,
	ADC_OffsetSome,
	ADC_Calibration1,
	ADC_CalibrationAll,
	ADC_CalibrationSome,
	ADC_Volts1,
	ADC_VoltsAll,
	ADC_VoltsSome,
	ADC_Counts1,
	ADC_CountsAll,
	ADC_CountsSome,
	ADC_Raw1,
	ADC_RawAll,
	ADC_RawSome,

	ADC_Stream = 0x1100,
	ADC_StreamStart,
	ADC_StreamStop,

	ADC_Streaming_stuff_including_Hz_config, // TODO: finish

	// TODO: DIds below this point are TBD/notional
	SCRIPT_ = 0x3000,
	SCRIPT_Pause, // insert a pause in execution of TDataItems

	// broken out from "BRD_stuff_needed_for_control_and_diagnostics_of_Linux_TCPIP_WDG_DEF_ETC" mentioned above
	WDG_ = 0x4000, // Watchdog related
	DEF_ = 0x5000, // power-on default state related
	SERVICE_,	   // tech support stuff
	TCP_ = 0x7000,		   // TCP-IP stuff broken out from the
	TCP_Hello = 0x7001,
	PNP_,		   // distinct from BRD_?
	CFG_,		   // "Other" Configuration stuff; Linux, IIoT protocol selection, etc?
};
#pragma endregion

#pragma pack(push, 1)
typedef struct {
	__u8 offset;
	__u8 width;
	__u32 value;
} REG_Write;

typedef std::vector<REG_Write> REG_WriteList;

typedef struct
{
	TMessageId type;
	TMessagePayloadSize payload_size;
} TMessageHeader;

typedef struct
{
	DataItemIds DId;
	TDataItemLength dataLength;
} TDataItemHeader;


typedef struct TSendQueueItemClass
{
	// which TCP-per-client-read thread put this item into the Action Queue
	pthread_t &receiver;
	// which thread is responsible for sending results of the action to the client
	pthread_t &sender;
	// which queue is the sender-thread popping from
	SafeQueue<TSendQueueItemClass> &sendQueue;
	// which client is all this from/for
	int clientref;
	// what TCP port# was this received on
	int portReceive;
	// what TCP port# is this sending out on
	int portSend;
} TSendQueueItem;

// TODO: FIX: the below is a guess at all plausible needs, not what is actually needed
// I strongly suspect I don't need portReceive/portSend because clientref is the socketID which knows all those specifics
// I shouldn't need sendQueue because thread sender should know it; either because there's only one sender and one sendqueue
//   or because the per-client send-thread/-queue was created by a per-client receive-thread which would inject sendQueue
//   or because sender.queue is queryable
//   or whatever other reasons.
// OTOH: it'll be easy to figure out what I need and clean this up later
typedef struct TActionQueueItemClass
{
	// which TCP-per-client-read thread put this item into the Action Queue
	pthread_t &receiver;
	// which thread is responsible for sending results of the action to the client
	pthread_t &sender;
	// which queue should the reply generated by this Action be pushed into
	SafeQueue<TSendQueueItem> &sendQueue;
	// which client is all this from/for
	int clientref;
	// what TCP port# was this received on
	int portReceive;
	// what TCP port# is this sending out on
	int portSend;
} TActionQueueItem;

#pragma pack(pop)
