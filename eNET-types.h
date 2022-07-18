#pragma once

#ifdef __linux__
#include <linux/types.h>
#else
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
#endif
#include <string.h>
#include <stdio.h>
#include <memory>
#include <vector>
using std::vector, std::begin, std::end;


/* type definitions */
typedef vector<__u8> TBytes;
typedef __u8 TMessageId;
typedef __u16 TMessagePayloadSize;
typedef __u8 TCheckSum;
typedef __u16 TDataId;
typedef __u8 TDataItemLength;

class TDataItem;

typedef std::shared_ptr<TDataItem> PTDataItem;
typedef std::vector<PTDataItem> TPayload;

#define hexbyte(x, w) "0x" << std::hex << setfill('0') << std::setw(w) << std::uppercase << static_cast<int>(x)
#pragma region TDataItem DId enum

/* DId.h
   This header is where all the DIds are defined, including what classes handle each
*/

#define _INVALID_DATAITEMID_ ((TDataId)-1)

enum DataItemIds : TDataId // specific numbering, ordering, and grouping are preliminary
{
	INVALID = _INVALID_DATAITEMID_,

	BRD_ = 0x0000, // Query Only. Note 1: the "TLA_" DIds return human-readable text of all BRD_ category DIds and what they do & why
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
	REG_Read1, REG_ReadAll, REG_ReadSome,
	REG_ReadBuf, // like draining a FIFO, TODO: improve name
	REG_Write1, REG_WriteSome,
	REG_WriteBuf, // like filling a FIFO, TODO: improve name

	REG_ClearBits,
	REG_SetBits,
	REG_ToggleBits,

	DAC_ = 0x200, // Query Only. *1
	DAC_Output1, DAC_OutputAll, DAC_OutputSome,
	DAC_Configure1, DAC_ConfigureAll, DAC_ConfigureSome,
	DAC_ConfigAndOutput1, DAC_ConfigAndOutputAll, DAC_ConfigAndOutputSome,
	DAC_ReadbackAll,

	DIO_ = 0x300, // Query Only. *1
	DIO_Configure1, DIO_ConfigureAll, DIO_ConfigureSome,
	DIO_Input1, DIO_InputAll, DIO_InputSome,
	DIO_InputBuf1, DIO_InputBufAll, DIO_InputBufSome, // repeated unpaced reads of Digital Inputs; NOTE: not sure this is useful
	DIO_Output1, DIO_OutputAll, DIO_OutputSome,
	DIO_OutputBuf, // like unpaced waveform output; NOTE: not sure this is useful
	DIO_ConfigureReadWriteReadSome,
	DIO_Clear1, DIO_ClearAll, DIO_ClearSome,
	DIO_Set1, DIO_SetAll, DIO_SetSome,
	DIO_Toggle1, DIO_ToggleAll, DIO_ToggleSome,
	DIO_Pulse1, DIO_PulseAll, DIO_PulseSome,

	PWM_ = 0x400, // Query Only. *1
	PWM_Configure1, PWM_ConfigureAll, PWM_ConfigureSome,
	PWM_Input1, PWM_InputAll, PWM_InputSome,
	PWM_Output1, PWM_OutputAll, PWM_OutputSome,

	ADC_ = 0x1000, // Query Only. *1
	ADC_ConfigurationOfEverything, // i.e., MId "C" to do ADC_ConfigureEverything; MId "Q" to do ADC_QueryEverythingsConfiguration
	ADC_Range1, ADC_RangeAll, ADC_RangeSome,
	ADC_Span1, ADC_SpanAll, ADC_SpanSome,
	ADC_Offset1, ADC_OffsetAll, ADC_OffsetSome,
	ADC_Calibration1, ADC_CalibrationAll, ADC_CalibrationSome,
	ADC_Volts1, ADC_VoltsAll, ADC_VoltsSome,
	ADC_Counts1, ADC_CountsAll, ADC_CountsSome,
	ADC_Raw1, ADC_RawAll, ADC_RawSome,

	ADC_Streaming_stuff_including_Hz_config, // TODO: finish

// TODO: DIds below this point are TBD/notional
	SCRIPT_Pause, // insert a pause in execution of TDataItems

// broken out from "BRD_stuff_needed_for_control_and_diagnostics_of_Linux_TCPIP_WDG_DEF_ETC" mentioned above
	WDG_ = 0x4000, // Watchdog related
	DEF_ = 0x5000, // power-on default state related
	SERVICE_, // tech support stuff
	TCP_, // TCP-IP stuff broken out from the
	PNP_, // distinct from BRD_?
	CFG_, // "Other" Configuration stuff; Linux, IIoT protocol selection, etc?
};
#pragma endregion

#pragma pack(push, 1)
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
#pragma pack(pop)
