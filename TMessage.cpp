/* // TODO:
Upcoming change to centralize the construction of serialized data items:

1) renaming all derived classes' .AsBytes() to i.e.,  ".CalcPayload()" thus removing .AsBytes from all derived classes
2) adding ".CalcPayload()" to the root class for zero-length DataItem convenience
3) removing this->pushDId() and this->pushLen() related code from all the derived classes .CalcPayload() implementations
4) write .AsBytes in the root class that pushes the DId, Len(Data), and Data
*/


#include <unistd.h>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <ctime>
using namespace std;

#include "logging.h"
#include "TError.h"
#include "TMessage.h"

#include "apcilib.h"
#include "eNET-AIO16-16F.h"
#include "adc.h"

const vector<TMessageId> ValidMessageIDs{
// to server
	'Q', // query/read
	'C', // config/write
	'M', // generic Message; bundle of Actions
// to client
	'R', // Response, no errors
	'X', // response, error, syntaX
	'E', // response, Error, semantic (e.g., "out of range" in an argument ), or operational (e.g., hardware timeout)
	'H', // Hello
};

// crap function returns 8 or 32 for valid offsets into eNET-AIO's register map, or 0 for invalid
// specific to eNET-AIO register map
int widthFromOffset(int ofs)
{
	if (ofs < 0x18)
		return 8;
	else if ((ofs <= 0xDC) && (ofs % 4 == 0))
		return 32;
    return 0;
}

#define DIdNYI(d)	{d, 0, 0, 0, construct<TDataItemNYI>, #d " (NYI)"}
// DId Enum, minLen,tarLen,maxLen,class-constructor,human-readable-doc
TDIdListEntry const DIdList[] = {
	{INVALID, 0, 0, 0, construct<TDataItem>, "Invalid DId"},
	{BRD_, 0, 0, 255, construct<TDataItem>, "TDataItem Base (BRD_)"},
	{BRD_Reset, 0, 0, 0, construct<TDataItem>, "BRD_Reset(void)"},
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
	{DAC_Output1, 5, 5, 5, construct<TDIdDacOutput>, "DAC_Output1(u8 iDAC, single Volts)"},
	DIdNYI(DAC_OutputAll),
	DIdNYI(DAC_OutputSome),
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
	DIdNYI(ADC_ConfigurationOfEverything),
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
	Trace("ENTER, DId: "+ std::to_string(DId), Data);
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
string TDataItem::getDIdDesc(DataItemIds DId)
{
	return string(DIdList[TDataItem::getDIdIndex(DId)].desc);
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
	Trace("Received = ", msg);

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
	this->pushLen(bytes, this->Data.size());

	bytes.insert(end(bytes), begin(Data), end(Data));
	Trace("Built: ", bytes);
	return bytes;
}

string TDataItem::getDIdDesc()
{
	return string(DIdList[TDataItem::getDIdIndex(this->getDId())].desc);
}

// returns human-readable, formatted (multi-line) string version of this TDataItem
string TDataItem::AsString(bool bAsReply)
{
	stringstream dest;

	dest << this->getDIdDesc() << ", Data bytes: " << this->Data.size() << ": ";

	if (this->Data.size() != 0)
	{
		for (auto byt : this->Data)
		{
			dest << "0x" << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(byt) << " ";
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

#pragma region TDIdReadRegister

TREG_Read1 &TREG_Read1::Go()
{
	this->Value = 0;
	switch (this->width)
	{
	case 8:
		this->resultCode = apci_read8(apci, 0, BAR_REGISTER, this->offset, (__u8*)&this->Value);
		Trace("apci_read8(" + to_hex<__u8>((__u8)this->offset) + ") → " + to_hex<__u32>((__u8)this->Value));
		break;
	case 32:
		this->resultCode = apci_read32(apci, 0, BAR_REGISTER, this->offset, &this->Value);
		Trace("apci_read32(" + to_hex<__u8>((__u8)this->offset) + ") → " + to_hex<__u32>((__u32)this->Value));
		break;
	}
	return *this;
}



std::shared_ptr<void> TREG_Read1::getResultValue()
{
	Trace("returning " + (this->width==8?to_hex<__u8>((__u8)this->Value):to_hex<__u32>((__u32)this->Value)));
	return this->width == 8
					? (std::shared_ptr<void>) std::shared_ptr<__u8>(new __u8(this->Value))
					: (std::shared_ptr<void>) std::shared_ptr<__u32>(new __u32(this->Value));
}

TError TREG_Read1::validateDataItemPayload(DataItemIds DataItemID, TBytes Data)
{
	TError result = ERR_SUCCESS;
	if (Data.size() != 1){
		Error(err_msg[-ERR_DId_BAD_PARAM]);
		return ERR_DId_BAD_PARAM;
	}
	int offset = Data[0];
	int w = widthFromOffset(offset);
	if (w != 0){
		Error(err_msg[-ERR_DId_BAD_OFFSET]);
		return ERR_DId_BAD_OFFSET;
	}

	return result;
};



TREG_Read1::TREG_Read1(DataItemIds DId, int ofs)
{
	Trace("ENTER. DId: "+to_hex<TDataId>(DId)+", offset: "+to_hex<__u8>(ofs));
	this->setDId(REG_Read1);
	this->setOffset(ofs);
}

TREG_Read1::TREG_Read1()
{
	this->setDId(REG_Read1);
}

TREG_Read1::TREG_Read1(TBytes data)
{
	Trace("ENTER. TBytes: ", data);
	this->setDId(REG_Read1);

	GUARD(data.size() == 1, ERR_DId_BAD_PARAM, data.size());
	this->offset = data[0];
	int w = widthFromOffset(offset);
	GUARD(w != 0, ERR_DId_BAD_PARAM, REG_Read1);
	this->width = w;
}

TREG_Read1 &TREG_Read1::setOffset(int ofs)
{
	int w = widthFromOffset(ofs);
	if (w == 0){
		Error("Invalid offset");
		throw std::logic_error("Invalid offset passed to TREG_Read1::setOffset");
	}
	this->offset = ofs;
	this->width = w;
	return *this;
}

TBytes TREG_Read1::AsBytes(bool bAsReply)
{
	TBytes bytes;
	this->pushDId(bytes);
	int w = 1;
	if (bAsReply)
		w += this->width / 8;
	Trace("Payload len: " + std::to_string(w));
	this->pushLen(bytes, w);
	bytes.push_back(this->offset);

	if (bAsReply){
		auto value = this->getResultValue();
		__u32 v = *((__u32 *)value.get());
		for (int i=0; i<this->width/8; i++)
		{
			bytes.push_back(v & 0x000000FF);
			v >>= 8;
		}
	}

	Trace("TREG_Read1::AsBytes built: ", bytes);
	return bytes;
}

string TREG_Read1::AsString(bool bAsReply)
{
	stringstream dest;

	dest << "REG_Read1(" << hex << setw(2) << setfill('0') << static_cast<int>(this->offset) << ")";
	if (bAsReply)
	{
		dest << " -> ";
		auto value = this->getResultValue();
		__u32 v = *((__u32 *)value.get());
		if (this->width == 8){
			dest << hex << setw(2) << (v & 0x000000FF);
		}
		else
		{
			dest << hex << setw(8) << v;
		}
	}
	Trace("Built: " + dest.str());
	return dest.str();
}
#pragma endregion

#pragma region TREG_Writes implementation
TREG_Writes::~TREG_Writes()
{
	Trace("Attempting to free the memory in this->Writes vector");
	this->Writes.clear();
}

// TODO: write WaitUntilBitsMatch(__u8 offset, __u32 bmMask, __u32 bmPattern);
int WaitUntilRegisterBitIsLow(__u8 offset, __u32 bitMask)  // TODO: move into utility source file
{
	__u32 value = 0;
	int attempt = 0;
	do {
		int status = apci_read32(apci, 1, BAR_REGISTER, offset, &value);
		Trace("SPI Busy Bit at " + std::string(to_hex<__u8>(offset)) + " is " +( (value & bitMask) ? "1" : "0"));
		if (status < 0)
			return -errno;
		if (++attempt > 100)
		{
			Error("Timeout waiting for SPI to be not busy, at offset: "+to_hex<__u8>(offset));
			return -ETIMEDOUT; // TODO: swap "attempt" with "timeout" RTC if benchmark proves RTC is not too slow
		}
	} while ((value & bitMask));

	return 0;
}

TREG_Writes &TREG_Writes::addWrite(__u8 w, int ofs, __u32 value)
{
	Trace("ENTER, w:" + std::to_string(w) + ", offset: " + to_hex<__u8>(ofs) + ", value: " + to_hex<__u32>(value));
	REG_Write aWrite;
	aWrite.width = w;
	aWrite.offset = ofs;
	aWrite.value = value;
	this->Writes.emplace_back(aWrite);
	return *this;
}

#define SPI_DELAY_DAC 160000 // 160 µsec in ns
#define SPI_DELAY_DIO 160000 // 160 µsec in ns

__s64 now() // in nanoseconds
{
	timespec Now;
	clock_gettime(CLOCK_BOOTTIME, &Now);
	return Now.tv_sec * 1E9 + Now.tv_nsec;
}

TREG_Writes &TREG_Writes::Go()
{
	static __s64 nextAllowedTimeDioSpi = now(), nextAllowedTimeDacSpi = now();
	__s64 diffTime = 0.0;

	this->resultCode = 0;
	for (auto action : this->Writes)
		switch (action.width)
		{
		case 8:
			this->resultCode |= apci_write8(apci, 0, BAR_REGISTER, action.offset, (__u8)(action.value & 0x000000FF));
			Trace("apci_write8(" + to_hex<__u8>(action.offset) + ") → " + to_hex<__u8>((action.value & 0xFF)));
			break;
		case 32:
			// DAC (at offset +30) and DIO (at offsets +3C → +44) are SPI based and must not write while the respective SPI bus is busy
			switch(action.offset)
			{
				case ofsDac: // DAC SPI busy handling
					// do{
					// 	diffTime = nextAllowedTimeDacSpi - now();
					// } while (diffTime > 0);
					//Log("calling wait for spi on DAC register");
					this->resultCode = WaitUntilRegisterBitIsLow(ofsDacSpiBusy, bmDacSpiBusy);
					//Trace("DAC SPI Busy Wait returned " + to_string(this->resultCode));
					//usleep(160);
					break;
				case ofsDioDirections:
				case ofsDioOutputs:
				case ofsDioInputs: // DIO SPI busy handling
					// do{
					// 	diffTime = nextAllowedTimeDioSpi - now();
					// 	printf("delta %lld\n", diffTime);
					// } while (diffTime > 0);
					//Log("calling wait for spi on DIO register");
					this->resultCode = WaitUntilRegisterBitIsLow(ofsDioSpiBusy, bmDioSpiBusy);
					//Trace("DIO SPI Busy Wait returned "+to_string(this->resultCode));
					//usleep(160);
					break;
				default:
					break;
			}

			if (0 == this->resultCode)
			{
				this->resultCode |= apci_write32(apci, 0, BAR_REGISTER, action.offset, action.value);
				switch(action.offset)
			{
				case ofsDac: // DAC SPI busy handling
					nextAllowedTimeDacSpi = now() + SPI_DELAY_DAC;
					break;
				case ofsDioDirections:
				case ofsDioOutputs:
				case ofsDioInputs: // DIO SPI busy handling
					nextAllowedTimeDioSpi = now() + SPI_DELAY_DIO;
					break;
				default:
					break;
			}
				if (0 == this->resultCode)
				{
					Trace("apci_write32(" + to_hex<__u8>(action.offset) + ") → " + to_hex<__u32>(action.value));
				}
				else
				{
					Error("apci_write32(" + to_hex<__u8>(action.offset) + ") → " + to_hex<__u32>(action.value) + " returned "+std::to_string(this->resultCode));
					return *this;
				}
			}
			else // TODO: FIX: should change TREG_Writes' REG_Write struct to include a resultCode and this else to set both the individual action.resultCode and the TREG_Writes.resultCode
			{
				Error("WaitUntilRegisterBitIsLow() returned "+std::to_string(this->resultCode));
				return *this;
			}
			break;
		}
	return *this;
}
string TREG_Writes::AsString(bool bAsReply)
{
	stringstream dest;
	for (auto aWrite : this->Writes)
	{
		__u32 v = aWrite.value;
		dest << "REG_Write1(" << hex << setw(2) << setfill('0') << static_cast<int>(aWrite.offset) << ", " << hex << setw(aWrite.width / 8 * 2) << v << ");";
	}
	return dest.str();
}
#pragma endregion TREG_Writes

#pragma region TREG_Write1 implementation
TREG_Write1::TREG_Write1()
{
	this->setDId(REG_Write1);
}
TREG_Write1::~TREG_Write1()
{
	this->Writes.clear();
}

TREG_Write1::TREG_Write1(TBytes buf)
{
	this->setDId(REG_Write1);
	GUARD(buf.size() > 0, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);
	__u8 ofs = buf[0];
	int w = widthFromOffset(ofs);
	GUARD(w != 0, ERR_DId_BAD_OFFSET, ofs);
	GUARD(w == 8 ? (buf.size() == 2) : (buf.size() == 5), ERR_DId_BAD_PARAM, buf.size());

	__u32 value = 0;
	if (w == 8)
		value = *(__u8 *)&buf[1];
	else
		value = *(__u32 *)&buf[1];

	this->addWrite(w, ofs, value);
}

TBytes TREG_Write1::AsBytes(bool bAsReply)
{
	TBytes bytes;
	this->pushDId(bytes);
	int w = 1 + this->Writes[0].width / 8;
	this->pushLen(bytes, w);
	if (this->Writes.size() > 0 )
		bytes.push_back(this->Writes[0].offset);
	else
		Error("ERROR: nothing in Write[] queue");

			__u32 v = this->Writes[0].value;
	for (int i = 0; i < this->Writes[0].width / 8; i++)
	{
		bytes.push_back(v & 0x000000FF);
		v >>= 8;
	}
	return bytes;
}

#pragma endregion

#pragma region "ADC Stuff"
TADC_StreamStart::TADC_StreamStart() {
	setDId(ADC_StreamStart);
}

TADC_StreamStart::TADC_StreamStart(TBytes buf)
{
	this->setDId(ADC_StreamStart);
	GUARD((buf.size() == 0) || (buf.size() == 4), ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);

	if (buf.size() == 4)
	{
		if (-1 == AdcStreamingConnection)
		{
			this->argConnectionID = (int)*(__u32 *)buf.data();
			AdcStreamingConnection = this->argConnectionID;
		}
		else
		{
			Error("ADC Busy");
			throw std::logic_error("ADC Busy already, on Connection: "+std::to_string(AdcStreamingConnection));
		}
	}
	Trace("AdcStreamingConnection: "+std::to_string(AdcStreamingConnection));
}

TBytes TADC_StreamStart::AsBytes(bool bAsReply)
{
	this->setDId(ADC_StreamStart);
	TBytes bytes;
	this->pushDId(bytes);
	this->pushLen(bytes, 4);
	auto con = this->argConnectionID;
	for (int i = 0; i < sizeof(con); i++)
	{
		bytes.push_back(con & 0x000000FF);
		con >>= 8;
	}
	Trace("TADC_StreamStart::AsBytes built: ", bytes);
	return bytes;
};

TADC_StreamStart &TADC_StreamStart::Go()
{
	Trace("ADC_StreamStart::Go(), ADC Streaming Data will be sent on ConnectionID: "+std::to_string(AdcStreamingConnection));
	auto status = apci_dma_transfer_size(apci, 1, RING_BUFFER_SLOTS, BYTES_PER_TRANSFER);
	if (status)
	{
		Error("Error setting apci_dma_transfer_size: "+std::to_string(status));
		throw std::logic_error(err_msg[-status]);
	}

	AdcStreamTerminate = 0;
	if (AdcWorkerThreadID == -1)
	{
		AdcWorkerThreadID = pthread_create(&worker_thread, NULL, &worker_main, &AdcStreamingConnection);
	}
	apci_start_dma(apci);
	return *this;
};

std::string TADC_StreamStart::AsString(bool bAsReply)
{
	std::string msg = this->getDIdDesc();
	if (bAsReply)
	{
		msg += ", ConnectionID = " + to_hex<int>(this->argConnectionID);
	}
	return msg;
};



TADC_StreamStop::TADC_StreamStop(TBytes buf)
{
	this->setDId(ADC_StreamStop);
	GUARD(buf.size() == 0, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, 0);
}

TBytes TADC_StreamStop::AsBytes(bool bAsReply)
{

	this->setDId(ADC_StreamStop);
	TBytes bytes;
	this->pushDId(bytes);
	int w = 0;
	this->pushLen(bytes, w);
	Trace("TADC_StreamStop::AsBytes built: ", bytes);
	return bytes;
};

TADC_StreamStop &TADC_StreamStop::Go()
{
	Trace("ADC_StreamStop::Go(): terminating ADC Streaming");
	AdcStreamTerminate = 1;
	apci_cancel_irq(apci, 1);
	AdcStreamingConnection = -1;
	AdcWorkerThreadID = -1;
	// pthread_cancel(worker_thread);
	// pthread_join(worker_thread, NULL);
	Trace("ADC_StreamStop::Go() exiting");
	return *this;
};

std::string TADC_StreamStop::AsString(bool bAsReply)
{
	return this->getDIdDesc();
}

#pragma endregion "ADC"



#pragma region TMessage implementation

TCheckSum TMessage::calculateChecksum(TBytes Message)
{

	TCheckSum checksum = 0;
	for (__u8 aByte : Message)
		checksum += aByte;
	return checksum;
}

bool TMessage::isValidMessageID(TMessageId MessageId)
{

	bool result = false;
	for (int i = 0; i < sizeof(ValidMessageIDs) / sizeof(TMessageId); i++)
	{
		if (ValidMessageIDs[i] == MessageId)
		{
			result = true;
			break;
		}
	}
	return result;
}

// Checks the Payload for well-formedness
// returns 0 if the Payload is well-formed
// this means that all the Data Items are well formed and the total size matches the expectation
// "A Message has an optional Payload, which is a sequence of zero or more Data Items"
TError TMessage::validatePayload(TBytes Payload)
{

	// WARN: Does not seem to work as expected when parsing multiple DataItems
	TError result = ERR_SUCCESS;
	if (Payload.size() == 1) // one-byte Payload is "the checksum byte".
		return result;

	if (Payload.size() == 0) // zero-length Payload size is a valid payload
		return ERR_MSG_DATAITEM_TOO_SHORT;
	if (Payload.size() < sizeof(TDataItemHeader))
		return ERR_MSG_DATAITEM_TOO_SHORT;

	TDataItemHeader *head = (TDataItemHeader *)Payload.data();

	int DataItemSize = sizeof(TDataItemHeader) + head->dataLength;
	if (DataItemSize > Payload.size())
	{
		Error("--ERR: data item thinks it is longer than payload, disize: " + std::to_string(DataItemSize) +", psize: " + std::to_string(Payload.size()));
		result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
	}
	else
	{
		TBytes DataItem = slicing(Payload, 0, DataItemSize);
		result = TDataItem::validateDataItem(DataItem);
		if (result == 0) // if the Data Item is well-formed, check the next one
		{
			Payload.erase(Payload.cbegin(), Payload.cbegin() + DataItemSize);
			if (Payload.size() > 0)
				result = validatePayload(Payload);
		}
	}

	return result;
}

// Checks the Message for well-formedness
// returns 0 if Message is well-formed
TError TMessage::validateMessage(TBytes buf) // "NAK()" is shorthand for return error condition etc
{
	Trace("ENTER: RAW Message: ", buf);
	if (buf.size() < minimumMessageLength)
		return ERR_MSG_TOO_SHORT; // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?

	TMessageHeader *head = (TMessageHeader *)buf.data();

	if (!isValidMessageID(head->type))
		return ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageId Category byte)

	__u32 statedMessageLength = minimumMessageLength + head->payload_size;
	if (buf.size() < statedMessageLength)
		return ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data

	TCheckSum checksum = TMessage::calculateChecksum(buf);

	if (__valid_checksum__ != checksum){
		Error("calculated csum: "+std::to_string(checksum)+" ERROR should be zero\n");
		return ERR_MSG_CHECKSUM; // NAK(invalid checksum)
}
	TBytes payload = buf;
	payload.erase(payload.cbegin(), payload.cbegin() + sizeof(TMessageHeader));

	TError validPayload = validatePayload(payload);
	if (validPayload != 0)
		return validPayload;

	return 0; // valid message
}
/* A Payload consists of zero or more DataItems
 * This function parses an array of bytes that is supposed to be a Payload
 * ...then returns a vector of those TDataItems and sets result to indicate error/success
 */
TPayload TMessage::parsePayload(TBytes Payload, __u32 payload_length, TError &result)
{

	TPayload dataItems; // an empty vector<>
	result = ERR_SUCCESS;
	if (payload_length == 0){ // zero-length payload size is a valid payload
		return dataItems;
	}

	TBytes DataItemBytes = Payload; // pointer to start of byte[] Payload
	while (payload_length >= sizeof(TDataItemHeader))
	{
		TDataItemHeader *head = (TDataItemHeader *)DataItemBytes.data();
		// DataItemLength is the size of the Data Item, including the size of the Data Item Length
		// + Data Item ID, and the Data Item's payload's bytelength
		int DataItemLength = sizeof(TDataItemHeader) + head->dataLength; // DataItem[3] is payload length
		if (DataItemLength > payload_length)
		{
			result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
			Error("TMessage::parsePayload: DIAG::fromBytes DataItemLength > payload_length returned error " + std::to_string(result) + ", " + err_msg[-result]);
			break;
		}

		PTDataItem item = TDataItem::fromBytes(DataItemBytes, result);
		if (result != ERR_SUCCESS)
		{
			Error("TMessage::parsePayload: DIAG::fromBytes returned error " + std::to_string(result) + ", " + err_msg[-result]);
			break;
		}
		dataItems.push_back(item);

		// remove the bytes from the Payload that were parsed into 'item'
		DataItemBytes.erase(DataItemBytes.cbegin(), DataItemBytes.cbegin() + DataItemLength);
		payload_length -= DataItemLength;
	}
	return dataItems;
}

TMessage TMessage::FromBytes(TBytes buf, TError &result)
{

	result = ERR_SUCCESS;
	Trace("Received: ", buf);

	auto siz = buf.size();
	if (siz < minimumMessageLength)
	{
		result = ERR_MSG_TOO_SHORT;
		Error("Message Size < minimumMessageLength ("+std::to_string(siz)+" < " + std::to_string(minimumMessageLength));
		return TMessage(_INVALID_MESSAGEID_); // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?
	}
	TMessageHeader *head = (TMessageHeader *)buf.data();

	if (!isValidMessageID(head->type))
	{
		result = ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageID Category byte)
		Error("TMessage::FromBytes: detected invalid MId: "+ std::to_string(result) + ", " + err_msg[-result]);
		return TMessage();
	}
	__u32 statedMessageLength = minimumMessageLength + head->payload_size;
	if (siz < statedMessageLength)
	{
		result = ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data
		return TMessage();
	}

	TPayload dataItems;
	if (head->payload_size > 0)
	{
		Trace("TMessage::FromBytes: Payload is " + std::to_string(head->payload_size) + " bytes");
		TBytes payload = buf;
		payload.erase(payload.cbegin(), payload.cbegin() + sizeof(TMessageHeader));
		Trace("TMessage::FromBytes generated payload: ", payload);
		dataItems = parsePayload(payload, head->payload_size, result);
		Trace("parsePayload returned " + std::to_string(dataItems.size()) + " with resultCode " + std::to_string(result));
	}

	TCheckSum checksum = calculateChecksum(buf);
	if (__valid_checksum__ != checksum)
	{
		result = ERR_MSG_CHECKSUM; // NAK(invalid checksum)
		Error("TMessage::FromBytes: invalid checksum "+std::to_string(checksum)+" ERROR should be zero\n");
		return TMessage();
	}

	TMessage message = TMessage(head->type, dataItems);
	Trace("TMessage::FromBytes: TMessage constructed...Payload DataItem Count: "+ std::to_string(message.DataItems.size()));
	return message;
}

TMessage::TMessage(TMessageId MId)
{

	this->setMId(MId);
}

TMessage::TMessage(TMessageId MId, TPayload Payload)
{

	this->setMId(MId);
	for (auto one : Payload)
	{
		DataItems.push_back(one);
	}
	// DataItems = Payload;
}

TMessage::TMessage(TBytes Msg)
{

	TError result = ERR_SUCCESS;
	*this = TMessage::FromBytes(Msg, result); // CODE SMELL: this technique makes me question my existence
	if (result != ERR_SUCCESS)
		throw std::logic_error(err_msg[-result]);
};

TMessageId TMessage::getMId()
{
	Trace("TMessage");
	return this->Id;
}

TCheckSum TMessage::getChecksum(bool bAsReply)
{
	Trace("TMessage");
	return TMessage::calculateChecksum(this->AsBytes(bAsReply));
}

TMessage &TMessage::setMId(TMessageId ID)
{
	Trace("TMessage MId=" + std::to_string(ID));
	if (!isValidMessageID(ID))
		throw std::logic_error("ERR_MSG_ID_UNKNOWN"); // TODO: FIX using TError
	this->Id = ID;

	return *this;
}

TMessage &TMessage::addDataItem(PTDataItem item)
{

	this->DataItems.push_back(item);
	return *this;
}

TBytes TMessage::AsBytes(bool bAsReply)
{

	TMessagePayloadSize payloadLength = 0;
	for (auto item : this->DataItems)
	{
		payloadLength += item->AsBytes(bAsReply).size();
	}
	TBytes bytes;

	bytes.push_back(this->Id);

	for (int i=0; i < (sizeof(TMessagePayloadSize)); i++){ // push message payload length
		__u8 lsb = payloadLength & 0xFF;
		bytes.push_back(lsb);
		payloadLength >>= 8;
	}

	for (auto item : this->DataItems)
	{
		TBytes byt = item->AsBytes(bAsReply);
		bytes.insert(end(bytes), begin(byt), end(byt));
	}

	TCheckSum csum = -calculateChecksum(bytes);
	bytes.push_back(csum); // WARN: only works because TCheckSum == __u8
	Trace("Built: ", bytes);

	return bytes;
}

string TMessage::AsString(bool bAsReply)
{

	stringstream dest;
	TBytes raw = this->AsBytes(bAsReply);
	Trace("TMessage, Raw Bytes: ", raw);
	dest << "Message = MId:" << to_hex<__u8>(this->getMId()) << ", csum: " << to_hex<__u8>(raw.back())<< ", DataItems: " << DataItems.size();
	if (DataItems.size() != 0)
	{
		for (int itemNumber = 0; itemNumber < DataItems.size(); itemNumber++)
		{
			PTDataItem item = this->DataItems[itemNumber];
			dest << endl
				 << "           " << setw(2) << itemNumber+1 << ": " << item->AsString(bAsReply);
		}
	}
	return dest.str();
}

#pragma endregion
