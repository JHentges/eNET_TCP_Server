#pragma once

#include "../eNET-types.h"
#include "../TError.h"


#pragma region TDataItem DId enum


#define _INVALID_DATAITEMID_ ((TDataId)-1)

enum DataItemIds : TDataId // specific numbering, ordering, and grouping are preliminary
{
	INVALID = _INVALID_DATAITEMID_,
	// Note 1: the "TLA_" DIds (e.g., `BRD_` and `REG_` et al; i.e., those DId names that don't have anything after the `_`)
	//         return human-readable text of all TLA_ category DIds and what they do & why
	BRD_ = 0x0000, // Query Only.
	BRD_Reset,
	BRD_DeviceID,
	BRD_Features,
	BRD_FpgaID,
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
	DAC_Range1 = 0x204, // Query Only.
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
	ADC_Claim,
	ADC_Release,
	ADC_BaseClock = 0x1003,
	ADC_StartHz,
	ADC_StartDivisor,
	ADC_ConfigurationOfEverything,
	ADC_Differential1,
	ADC_DifferentialAll,
	ADC_DifferentialSome,
	ADC_Range1,
	ADC_RangeAll,
	ADC_RangeSome,
	ADC_Calibration1,
	ADC_CalibrationAll,
	ADC_CalibrationSome,
	ADC_Span1,
	ADC_SpanAll,
	ADC_SpanSome,
	ADC_Offset1,
	ADC_OffsetAll,
	ADC_OffsetSome,
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
	WDG_ = 0x4000,	// Watchdog related
	DEF_ = 0x5000,	// power-on default state related
	SERVICE_,		// tech support stuff
	TCP_ = 0x7000,	// TCP-IP stuff broken out from the
	TCP_ConnectionID = 0x7001,
	PNP_,			// distinct from BRD_?
	CFG_ = 0x9000,	// "Other" Configuration stuff; Linux, IIoT protocol selection, etc?
	CFG_Hostname,
};
#pragma endregion

#pragma pack(push, 1)
typedef struct
{
	DataItemIds DId;
	TDataItemLength dataLength;
} TDataItemHeader;
#pragma pack(pop)

int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);

#define printBytes(dest, intro, buf, crlf)                                                       \
	{                                                                                            \
		dest << intro;                                                                           \
		for (auto byt : buf)                                                                     \
			dest << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " "; \
		if (crlf)                                                                                \
			dest << std::endl;                                                                        \
	}

// return register width for given offset as defined for eNET-AIO registers
// returns 0 if offset is invalid
int widthFromOffset(int ofs);

template <typename T> void stuff(TBytes & buf, const T v)
{
	auto value = v;
	for (int i = 0; i < sizeof(T); i++)
	{
		buf.push_back(value & 0xFF);
		value >>= 8;
	}
}

template <typename T=std::string> void stuff(TBytes & buf, const std::string v)
{
	for (char c : v)
		buf.push_back(c);
}

// void stuff32(TBytes & buf, const __u32 v)
// {
// 	auto value = v;
// 	for (int i = 0; i < sizeof(value); i++)
// 	{
// 		buf.push_back(value & 0x000000FF);
// 		value >>= 8;
// 	}
// }


// utility template to turn class into (base-class)-pointer-to-instance-on-heap, so derived class gets called
template <class X> std::unique_ptr<TDataItem> construct(TBytes FromBytes) { return std::unique_ptr<TDataItem>(new X(FromBytes)); }
typedef std::unique_ptr<TDataItem> DIdConstructor(TBytes FromBytes);

typedef struct
{
	DataItemIds DId;
	TDataItemLength minLen;
	TDataItemLength expectedLen;
	TDataItemLength maxLen;
	DIdConstructor *Construct;
	std::string desc;
} TDIdListEntry;

extern TDIdListEntry const DIdList[];


#pragma region "class TDataItem" declaration

/*	The following classes (TDataItem and its descendants) try to use a consistent convention
	for the order in which Methods are listed:
		1) Deserialization stuff, used while converting bytes (received via TCP) into Objects
		2) Serialization stuff, used to construct and convert Objects into byte vectors (for sending over TCP)
		3) Verbs: stuff associated with using Objects as Actions, and the results thereof
		4) diagnostic, debug, or otherwise "Rare" stuff, like .AsString()
	Fields, if any, come last.
*/

// Base class for all TDataItems, descendants of which will handle payloads specific to the DId
class TDataItem
{
public:
	// 1) Deserialization: methods used when converting byte vectors into objects

	// factory fromBytes() instantiates appropriate (sub-)class of TDataItem via DIdList[]
	// .fromBytes() would typically be called by TMessage::fromBytes();
	static PTDataItem fromBytes(TBytes msg, TError &result);

	// this block of methods are typically used by ::fromBytes() to syntax-check the byte vector
	static int validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	static int isValidDataItemID(DataItemIds DataItemID);
	static int validateDataItem(TBytes msg);
	static TDataItemLength getMinLength(DataItemIds DId);
	static TDataItemLength getTargetLength(DataItemIds DId);
	static TDataItemLength getMaxLength(DataItemIds DId);

	// index into DIdList; TODO: kinda belongs in a DIdList class method...
	static int getDIdIndex(DataItemIds DId);
	// serialize the Payload portion of the Data Item; calling this->calcPayload is done by TDataItem.AsBytes(), only
	virtual TBytes calcPayload(bool bAsReply = false) {return Data;}
	// serialize for sending via TCP; calling TDataItem.AsBytes() is normally done by TMessage::AsBytes()
	virtual TBytes AsBytes(bool bAsReply=false);
	// push DId into buf; utility for AsBytes()
	void pushDId(TBytes &buf);
	// push Length of Data into buf; utility for AsBytes();
	static void pushLen(TBytes &buf, TDataItemLength len);
	// 2) Serialization: methods for source to generate TDataItems, typically for "Response Messages"

	// zero-"Data" data item constructor
	TDataItem(DataItemIds DId);
	// some-"Data" constructor for specific DId; *RARE*, *DEBUG mainly, to test round-trip conversion implementation*
	// any DId that is supposed to have data would use its own constructor that takes the correct data types, not a
	// simple TBytes, for the Data Payload
	TDataItem(DataItemIds DId, TBytes bytes);

	// TODO: WARN: Why would this *ever* be used?
	TDataItem();

	// TDataItem anItem(DId_Read1).addData(offset) kind of thing.  no idea if it will be useful other than debugging
	virtual TDataItem &addData(__u8 aByte);
	// TDataItem anItem().setDId(DId_Read1) kind of thing.  no idea why it exists
	virtual TDataItem &setDId(DataItemIds DId);
	// encapsulates reading the Id for the DataItem
	virtual DataItemIds getDId();

	virtual bool isValidDataLength();

	// parse byte array into TDataItem; *RARE*, *DEBUG mainly, to test round-trip conversion implementation*
	// this is an explicit class-specific .fromBytes(), which the class method .fromBytes() will invoke for NYI DIds etc
	TDataItem(TBytes bytes);

	// 3) Verbs -- things used when *executing* the TDataItem Object
public:
	// intended to be overriden by descendants it performs the query/config operation and sets instance state as appropriate
	virtual TDataItem &Go();
	// encapsulates the result code of .Go()'s operation
	virtual TError getResultCode();
	// encapsulates the Value that results from .Go()'s operation; DIO_Read1() might have a bool Value;
	// ADC_GetImmediateScanV() might be an array of single precision floating point Volts
	virtual std::shared_ptr<void> getResultValue(); // TODO: fix; think this through

	// 4) Diagnostic / Debug - methods typically used for implementation debugging
public:
	// returns human-readable string representing the DataItem and its payload; normally used by TMessage.AsString(bAsReply)
	virtual std::string AsString(bool bAsReply=false);
	// used by .AsString(bAsReply) to fetch the human-readable name/description of the DId (from DIdList[].Description)
	virtual std::string getDIdDesc();
	// class method to get the human-readable name/description of any known DId; TODO: should maybe be a method of DIdList[]
	static std::string getDIdDesc(DataItemIds DId);

protected:
	TBytes Data;
	TError resultCode;
	int conn;

protected:
	DataItemIds Id{0};
	bool bWrite = false;
};
#pragma endregion TDataItem declaration

#pragma region "class TDataItemNYI" declaration
class TDataItemNYI : public TDataItem
{
public:
	TDataItemNYI() = default;
	TDataItemNYI(TBytes buf) : TDataItem::TDataItem{buf}{};
};
#pragma endregion
