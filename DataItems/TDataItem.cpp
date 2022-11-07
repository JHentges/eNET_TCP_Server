#include "../logging.h"
#include "../eNET-types.h"
#include "TDataItem.h"
#include "ADC_.h"
#include "BRD_.h"
#include "CFG_.h"
#include "DAC_.h"
#include "REG_.h"
#include "../apcilib.h"
#include "../eNET-AIO16-16F.h"

extern int apci;

#define DIdNYI(d)	{d, 0, 0, 0, construct<TDataItemNYI>, #d " (NYI)"}
// DId Enum, minLen,tarLen,maxLen,class-constructor,human-readable-doc
TDIdListEntry const DIdList[] = {
	{INVALID, 0, 0, 0, construct<TDataItem>, "Invalid DId"},
	{BRD_, 0, 0, 255, construct<TDataItem>, "TDataItem Base (BRD_)"},
	{BRD_Reset, 0, 0, 0, construct<TDataItem>, "BRD_Reset(void)"},
	{BRD_DeviceID, 0, 4, 255, construct<TDataItem>, "BRD_DeviceID() → u16"},
	{BRD_Features, 0, 4, 255, construct<TDataItem>, "BRD_Features() → u8"},
	{BRD_FpgaID, 0, 4, 255, construct<TDataItem>, "BRD_FpgaID() → u32"},

	{REG_Read1, 1, 1, 1, construct<TREG_Read1>, "REG_Read1(u8 offset) → [u8|u32]"},
	DIdNYI(REG_ReadAll),
	DIdNYI(REG_ReadSome),
	DIdNYI(REG_ReadBuf),
	{REG_Write1, 2, 5, 5, construct<TREG_Write1>, "REG_Write1(u8 ofs, [u8|u32] data)"},
	DIdNYI(REG_WriteSome),
	DIdNYI(REG_WriteBuf),
	DIdNYI(REG_ClearBits),
	DIdNYI(REG_SetBits),
	DIdNYI(REG_ToggleBits),

	{DAC_, 0, 0, 0, construct<TDataItem>, "TDataItemBase (DAC_)"},
	{DAC_Output1, 5, 5, 5, construct<TDAC_Output>, "DAC_Output1(u8 iDAC, single Volts)"},
	DIdNYI(DAC_OutputAll),
	DIdNYI(DAC_OutputSome),
	{DAC_Range1, 5, 5, 5, construct<TDAC_Range1>, "DAC_Range1(u8 iDAC, u32 RangeCode)"},
	DIdNYI(DAC_Configure1),
	DIdNYI(DAC_ConfigureAll),
	DIdNYI(DAC_ConfigureSome),
	DIdNYI(DAC_ConfigAndOutput1),
	DIdNYI(DAC_ConfigAndOutputAll),
	DIdNYI(DAC_ConfigAndOutputSome),
	DIdNYI(DAC_ReadbackAll),

	DIdNYI(DIO_),
	DIdNYI(DIO_Configure1),
	DIdNYI(DIO_ConfigureAll),
	DIdNYI(DIO_ConfigureSome),
	DIdNYI(DIO_Input1),
	DIdNYI(DIO_InputAll),
	DIdNYI(DIO_InputSome),
	DIdNYI(DIO_InputBuf1),
	DIdNYI(DIO_InputBufAll),
	DIdNYI(DIO_InputBufSome), // repeated unpaced reads of Digital Inputs; NOTE: not sure this is useful
	DIdNYI(DIO_Output1),
	DIdNYI(DIO_OutputAll),
	DIdNYI(DIO_OutputSome),
	DIdNYI(DIO_OutputBuf),
	DIdNYI(DIO_ConfigureReadWriteReadSome),
	DIdNYI(DIO_Clear1),
	DIdNYI(DIO_ClearAll),
	DIdNYI(DIO_ClearSome),
	DIdNYI(DIO_Set1),
	DIdNYI(DIO_SetAll),
	DIdNYI(DIO_SetSome),
	DIdNYI(DIO_Toggle1),
	DIdNYI(DIO_ToggleAll),
	DIdNYI(DIO_ToggleSome),
	DIdNYI(DIO_Pulse1),
	DIdNYI(DIO_PulseAll),
	DIdNYI(DIO_PulseSome),

	DIdNYI(PWM_),
	DIdNYI(PWM_Configure1),
	DIdNYI(PWM_ConfigureAll),
	DIdNYI(PWM_ConfigureSome),
	DIdNYI(PWM_Input1),
	DIdNYI(PWM_InputAll),
	DIdNYI(PWM_InputSome),
	DIdNYI(PWM_Output1),
	DIdNYI(PWM_OutputAll),
	DIdNYI(PWM_OutputSome),

	DIdNYI(ADC_),
	DIdNYI(ADC_Claim),
	DIdNYI(ADC_Release),

	{ADC_BaseClock, 0, 0, 4, construct<TDataItem>, "ADC_BaseClock() → u32"},
	{ADC_StartHz, 4, 4, 4, construct<TDataItem>, "ADC_StartHz(f32)"},
	{ADC_StartDivisor, 4, 4, 4, construct<TDataItem>, "ADC_StartDivisor(u32)"},
	DIdNYI(ADC_ConfigurationOfEverything),
	DIdNYI(ADC_Differential1),
	DIdNYI(ADC_DifferentialAll),
	DIdNYI(ADC_DifferentialSome),
	DIdNYI(ADC_Range1),
	DIdNYI(ADC_RangeAll),
	DIdNYI(ADC_RangeSome),
	DIdNYI(ADC_Span1),
	DIdNYI(ADC_SpanAll),
	DIdNYI(ADC_SpanSome),
	DIdNYI(ADC_Offset1),
	DIdNYI(ADC_OffsetAll),
	DIdNYI(ADC_OffsetSome),
	DIdNYI(ADC_Calibration1),
	DIdNYI(ADC_CalibrationAll),
	DIdNYI(ADC_CalibrationSome),
	DIdNYI(ADC_Volts1),
	DIdNYI(ADC_VoltsAll),
	DIdNYI(ADC_VoltsSome),
	DIdNYI(ADC_Counts1),
	DIdNYI(ADC_CountsAll),
	DIdNYI(ADC_CountsSome),
	DIdNYI(ADC_Raw1),
	DIdNYI(ADC_RawAll),
	DIdNYI(ADC_RawSome),

	{ADC_StreamStart, 4, 4, 4, construct<TADC_StreamStart>, "ADC_StreamStart((u32)AdcConnectionId)"},
	{ADC_StreamStop, 0, 0, 0, construct<TADC_StreamStop>, "ADC_StreamStop()"},

	DIdNYI(ADC_Streaming_stuff_including_Hz_config),

	DIdNYI(SCRIPT_Pause), // SCRIPT_Pause(__u8 delay ms)

	DIdNYI(WDG_),
	DIdNYI(DEF_),
	DIdNYI(SERVICE_),
	DIdNYI(TCP_),
	{TCP_ConnectionID, 0, 4, 255, construct<TDataItem>, "TDataItem TCP_ConnectionID"},
	DIdNYI(PNP_),
	DIdNYI(CFG_),
	{CFG_Hostname, 5, 5, 5, construct<TCFG_Hostname>, "CFG_Hostname({valid Hostname})"},

	// etc.  Need to list all the ones we care about soon, and all (of the ones we keep), eventually.
	// TODO: return `NYI` for expected but not implemented DIds ... somehow.  C++ doesn't have introspection of enums ...
	//		perhaps make a TDataItem derivative that is hard-coded NYI
};



#pragma region TDataItem implementation
/*	TDataItem
	Base Class, provides basics for handling TDataItem payloads.
	Descendant classes should handle stuff specific to the TDataItem's DataItemId ("DId")
	e.g, if the Message is equivalent to ADC_SetRangeAll(__u8 ranges[16]) then the Data
		should be 16 bytes, each of which is a valid Range Code
	e.g, if the Message is ADC_SetRange(__u16 Channel, __u8 RangeCode) then the Data
		should be 3 bytes, being a __u16 Channel which must be valid for this device, and
		a valid RangeCode byte
	NOTE:
		I suspect I need more ABCs: TWriteableData() and TReadOnlyData(), perhaps...
*/
#pragma region TDataItem Class Methods(static)
// NYI - validate the payload of a Data Item based on the Data Item ID
// e.g, if the Message is equivalent to ADC_SetRangeAll(__u8 ranges[16]) then the Data
//      should be 16 bytes, each of which is a valid Range Code
// e.g, if the Message is ADC_SetRange(__u16 Channel, __u8 RangeCode) then the Data
//      should be 3 bytes, being a __u16 Channel which must be valid for this device, and
//      a valid RangeCode byte
// NOTE:
//   This should be implemented OOP-style: each TDataItem ID should be a descendant-class that provides the
//   specific validate and parse appropriate to that DataItemID
int TDataItem::validateDataItemPayload(DataItemIds DId, TBytes Data)
{
	Trace("ENTER, DId: "+ to_hex<TDataId>(DId)+": ", Data);
	int result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
	int index = TDataItem::getDIdIndex(DId);
	TDataItemLength len = Data.size();
	Trace(std::to_string(getMinLength(DId)) + " <= " + std::to_string(len) + " <= " + std::to_string(getMaxLength(DId)));
	if ((TDataItem::getMinLength(DId) <= len) && (len <= TDataItem::getMaxLength(DId)))
	{
		result = ERR_SUCCESS;
		Trace("Valid");
	}else {
		Error("INVALID");
	}
	return result;
}

int TDataItem::getDIdIndex(DataItemIds DId)
{
	int count = sizeof(DIdList) / sizeof(TDIdListEntry);
	for (int index = 0; index < count; index++)
	{
		if (DId == DIdList[index].DId){
			return index;
	}}
	throw std::logic_error("DId not found in list");
}

// returns human-readable description of this TDataItem
std::string TDataItem::getDIdDesc(DataItemIds DId)
{
	return std::string(DIdList[TDataItem::getDIdIndex(DId)].desc);
}

TDataItemLength TDataItem::getMinLength(DataItemIds DId)
{
	return DIdList[getDIdIndex(DId)].minLen;
}

TDataItemLength TDataItem::getTargetLength(DataItemIds DId)
{
	return DIdList[getDIdIndex(DId)].expectedLen;
}

TDataItemLength TDataItem::getMaxLength(DataItemIds DId)
{
	return DIdList[getDIdIndex(DId)].maxLen;
}

void TDataItem::pushDId(TBytes & buf)
{
	TDataId DId = this->Id;
	for (int i = 0; i < sizeof(DId); i++)
	{
		buf.push_back(DId & 0x000000FF);
		DId >>= 8;
	}
}

void TDataItem::pushLen(TBytes & buf, TDataItemLength len)
{
	for (int i = 0; i < sizeof(len); i++)
	{
		buf.push_back(len & 0x000000FF);
		len >>= 8;
	}
}

int TDataItem::isValidDataItemID(DataItemIds DataItemID)
{
	int result = false;
	for (auto aDIdEntry : DIdList)
	{
		if (DataItemID == aDIdEntry.DId)
		{
			result = true;
			break;
		}
	}
	return result;
}

int TDataItem::validateDataItem(TBytes msg)
{
	int result = 0;
	if (msg.size() < sizeof(TDataItemHeader)){
		Error(err_msg[-ERR_MSG_DATAITEM_TOO_SHORT]);
		return ERR_MSG_DATAITEM_TOO_SHORT;
	}
	TDataItemHeader *head = (TDataItemHeader *)msg.data();

	TDataItemLength DataItemSize = head->dataLength; // WARN: only works if TDataItemHeader length field is one byte
	DataItemIds Id = head->DId;
	if (!isValidDataItemID(Id))
	{
		Error(err_msg[ERR_MSG_DATAITEM_ID_UNKNOWN]);
		return ERR_MSG_DATAITEM_ID_UNKNOWN;
	}

	if (DataItemSize == 0) // no data in this data item so no need to check the payload
		return result;
	else
	{
		TBytes Data = msg;
		Data.erase(Data.cbegin(), Data.cbegin() + sizeof(TDataItemHeader));
		result = validateDataItemPayload(Id, Data);
	}

	Trace("validateDataItem status: " + std::to_string(result) + ", " + err_msg[-result]);
	return result;
}

// factory method
PTDataItem TDataItem::fromBytes(TBytes msg, TError &result)
{
	result = ERR_SUCCESS;
	Debug("Received = ", msg);

	GUARD((msg.size() >= sizeof(TDataItemHeader)), ERR_MSG_DATAITEM_TOO_SHORT, msg.size());

	TDataItemHeader *head = (TDataItemHeader *)msg.data();
	GUARD(isValidDataItemID(head->DId), ERR_DId_INVALID, head->DId);
	// Trace("Got DId: " + to_hex<TDataId>(head->DId));

	PTDataItem anItem;
	TDataItemLength DataSize = head->dataLength; // MessageLength
	TBytes data;
	if (DataSize == 0) {

	}
	else
	{
		data.insert(data.end(), msg.cbegin() + sizeof(TDataItemHeader), msg.cbegin() + sizeof(TDataItemHeader) + DataSize);

		result = validateDataItemPayload(head->DId, data);
		if (result != ERR_SUCCESS){
			Error("TDataItem::fromBytes() failed validateDataItemPayload with status: " + std::to_string(result) + ", " + err_msg[-result]);
			return PTDataItem(new TDataItem());
		}
	}
	Log("TDataItem::fromBytes sending to constructor: ", data);
	for (auto entry : DIdList)
		if (entry.DId == head->DId){
			auto item = entry.Construct(data);
			return item;
		}
	return PTDataItem(new TDataItem(head->DId, data));
}
#pragma endregion

TDataItem::TDataItem() : TDataItem{DataItemIds(_INVALID_DATAITEMID_)} {
};

TDataItem::TDataItem(DataItemIds DId)
{
	Trace("ENTER, DId: " + to_hex<TDataId>(DId));
	this->setDId(DId);
};


// parses vector of bytes (presumably received across TCP socket) into a TDataItem
TDataItem::TDataItem(TBytes bytes) : TDataItem()
{
	Trace("bytes = ", bytes);
	TError result = ERR_SUCCESS;
	GUARD(bytes.size() < sizeof(TDataItemHeader), ERR_MSG_DATAITEM_TOO_SHORT, bytes.size());

	TDataItemHeader *head = (TDataItemHeader *)bytes.data();
	GUARD(isValidDataItemID(head->DId), ERR_MSG_DATAITEM_ID_UNKNOWN, head->DId);

	TDataItemLength DataSize = head->dataLength;
	GUARD(bytes.size() != sizeof(TDataItemHeader) + DataSize, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, DataSize);

	this->Data = TBytes(bytes.begin() + sizeof(TDataItemHeader), bytes.end()); // extract the Data from the DataItem bytes
	if (DataSize > 0)
		TError result = validateDataItemPayload(this->Id, Data); // TODO: change to parseData that returns vector of classes-of-data-types
	return;
}

TDataItem::TDataItem(DataItemIds DId, TBytes bytes)
{
	this->setDId(DId);
	this->Data = bytes;
}

TDataItem &TDataItem::addData(__u8 aByte)
{
	Data.push_back(aByte);
	return *this;
}

TDataItem &TDataItem::setDId(DataItemIds DId)
{
	GUARD(isValidDataItemID(DId), ERR_MSG_DATAITEM_ID_UNKNOWN, DId);
	this->Id = DId;
	Trace("Set DId to " + to_hex<TDataId>(this->Id));
	return *this;
}

DataItemIds TDataItem::getDId()
{
	return this->Id;
}

bool TDataItem::isValidDataLength()
{
	bool result = false;
	DataItemIds DId = this->getDId();
	int index = TDataItem::getDIdIndex(DId);
	TDataItemLength len = this->Data.size();
	if ((this->getMinLength(DId) <= len) && (this->getMaxLength(DId) >= len))
	{
		result = true;
	}
	return result;
}

TBytes TDataItem::AsBytes(bool bAsReply)
{
	Trace("ENTER, bAsReply = " + std::to_string(bAsReply));
	TBytes bytes;
	this->pushDId(bytes);
	this->Data = this->calcPayload(bAsReply);
	this->pushLen(bytes, this->Data.size());

	bytes.insert(end(bytes), begin(Data), end(Data));
	Trace("Built: ", bytes);
	return bytes;
}

std::string TDataItem::getDIdDesc()
{
	return std::string(DIdList[TDataItem::getDIdIndex(this->getDId())].desc);
}

// returns human-readable, formatted (multi-line) string version of this TDataItem
std::string TDataItem::AsString(bool bAsReply)
{
	std::stringstream dest;

	dest << this->getDIdDesc() << ", Data bytes: " << this->Data.size() << ": ";

	if (this->Data.size() != 0)
	{
		for (auto byt : this->Data)
		{
			dest << "0x" << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << static_cast<int>(byt) << " ";
		}
	}
	Trace(dest.str());
	return dest.str();
}

#pragma region Verbs-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- // TODO: fix; think this through
TDataItem &TDataItem::Go()
{
	Trace("ENTER - NYI!!!!");
	Error("Not Yet Implemented!");
	this->resultCode = ERR_NYI;
	return *this;
}

TError TDataItem::getResultCode()
{
	Trace("resultCode: " + std::to_string(this->resultCode));
	return this->resultCode;
}

std::shared_ptr<void> TDataItem::getResultValue()
{
	Trace("ENTER - TDataItem doesn't have a resultValue... returning 0");
	return std::shared_ptr<void>(0);
}

#pragma endregion
#pragma endregion TDataItem implementation

#pragma region TDataItemNYI implementation
// NYI (lol)
#pragma endregion

int WriteConfigSetting(std::string key, std::string value, std::string file="config.current");
