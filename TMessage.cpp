#include <cstdlib>
#include <stdexcept>

#include "TMessage.h"
#include "TError.h"

#include "apcilib.h"
#include "eNET-AIO.h"

using namespace std;

const vector<TMessageId> ValidMessages{
	'Q',
	'R',
	'C',
};

int widthFromOffset(int ofs)
{
	int w = 0;
	if (ofs < 0x18)
		w = 8;
	else if ((ofs <= 0xDC) && (ofs % 4 == 0))
		w = 32;
	return w;
}

__u8 eNET_AIO_In8(__u8 Offset){
	__u8 readValue;
	apci_read8(apci, 0, BAR_REGISTER, Offset, &readValue);
	return readValue;
};

__u32 eNET_AIO_In32(__u8 Offset){
	__u32 readValue;
	apci_read32(apci, 0, BAR_REGISTER, Offset, &readValue);
	return readValue;
}

// DId Enum, minLen,tarLen,maxLen,class-constructor,human-readable-doc
TDIdList const DIdList[] = {
	{INVALID, 0, 0, 0, construct<TDataItem>, "Invalid DId"},
	{BRD_, 0, 0, 255, construct<TDataItem>, "TDataItem Base (BRD_)"},
	{BRD_Reset, 0, 0, 0, construct<TDataItem>, "BRD_Reset(void)"},
	{REG_Read1, 0, 1, 1, construct<TDIdReadRegister>, "REG_Read1(__u8 offset)"},
	DIdNYI(REG_ReadAll),
	DIdNYI(REG_ReadSome),
	DIdNYI(REG_ReadBuf),
	DIdNYI(REG_Write1),
	DIdNYI(REG_WriteSome),
	DIdNYI(REG_WriteBuf),
	DIdNYI(REG_ClearBits),
	DIdNYI(REG_SetBits),
	DIdNYI(REG_ToggleBits),

	{DAC_, 0, 0, 0, construct<TDataItem>, "TDataItemBase (DAC_)"},
	{DAC_Output1, 5, 5, 5, construct<TDIdDacOutput>, "DAC_Output1(__u8 iDAC, single Volts)"},
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

	DIdNYI(ADC_Streaming_stuff_including_Hz_config),

	DIdNYI(SCRIPT_Pause), // SCRIPT_Pause(__u8 delay ms)

	DIdNYI(WDG_),
	DIdNYI(DEF_),
	DIdNYI(SERVICE_),
	DIdNYI(TCP_),
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
	int result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
	int index = TDataItem::getDIdIndex(DId);
	TDataItemLength len = Data.size();
	// printf("validateDataItemPayload(%04X, ", DId);
	// printBytes(cout, "", Data, 0);
	// printf(") : %d <= %d <= %d\n", getMinLength(DId), len, getMaxLength(DId));
	if ((TDataItem::getMinLength(DId) <= len) && (len <= TDataItem::getMaxLength(DId)))
	{
		result = ERR_SUCCESS;
	}

	return result;
}

int TDataItem::getDIdIndex(DataItemIds DId)
{
	int count = sizeof(DIdList) / sizeof(TDIdList);
	for (int index = 0; index < count; index++)
	{
		if (DId == DIdList[index].DId)
			return index;
	}
	throw exception();
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
	if (msg.size() < sizeof(TDataItemHeader))
		return ERR_MSG_DATAITEM_TOO_SHORT;
	TDataItemHeader *head = (TDataItemHeader *)msg.data();

	TDataItemLength DataItemSize = head->dataLength; // WARN: only works if TDataItemHeader length field is one byte
	DataItemIds Id = head->DId;
	if (!isValidDataItemID(Id))
	{
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
	return result;
}

// factory method
PTDataItem TDataItem::fromBytes(TBytes msg, TError &result)
{
	result = ERR_SUCCESS;
	// printBytes(cout, "TDataItem::fromBytes received = ", msg, true);

	GUARD((msg.size() >= sizeof(TDataItemHeader)), ERR_MSG_DATAITEM_TOO_SHORT, msg.size());

	TDataItemHeader *head = (TDataItemHeader *)msg.data();
	GUARD(isValidDataItemID(head->DId), ERR_DId_INVALID, head->DId);

	TDataItemLength DataSize = head->dataLength; // MessageLength


	PTDataItem anItem;
	if (DataSize == 0)
		return PTDataItem(new TDataItem(head->DId)); // no data in this data item is valid
	TBytes data;
	data.insert(data.end(), msg.cbegin() + sizeof(TDataItemHeader), msg.cbegin() + sizeof(TDataItemHeader) + DataSize);

	result = validateDataItemPayload(head->DId, data);
	if (result != ERR_SUCCESS)
		return PTDataItem(new TDataItem());

	for(auto entry : DIdList)
		if (entry.DId == head->DId)
			return entry.Construct();

	return PTDataItem(new TDataItem(head->DId, data));
}
#pragma endregion

TDataItem::TDataItem() : TDataItem{DataItemIds(_INVALID_DATAITEMID_)} {};

TDataItem::TDataItem(DataItemIds DId)
{
	this->setDId(DId);
};

// parses vector of bytes (presumably received across TCP socket) into a TDataItem
TDataItem::TDataItem(TBytes bytes) : TDataItem()
{
	TError result = ERR_SUCCESS;
	GUARD(bytes.size() < sizeof(TDataItemHeader), ERR_MSG_DATAITEM_TOO_SHORT, bytes.size());

	TDataItemHeader *head = (TDataItemHeader *)bytes.data();
	GUARD(isValidDataItemID(head->DId), ERR_MSG_DATAITEM_ID_UNKNOWN, head->DId);

	TDataItemLength DataSize = head->dataLength; // TODO, FIX: only works whle TDataId is one byte; change to TDataItemHeader casting
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

TBytes TDataItem::AsBytes()
{
	TBytes bytes;
	TDataId DId = this->getDId();
	for (int i = 0; i < sizeof(DId); i++)
	{
		bytes.push_back(DId & 0x000000FF);
		DId >>= 8;
	}
	int siz = this->Data.size();
	if (siz > 255)
	{
		printf("this = %p siz=%d\n", this, siz);
	}
	bytes.push_back(siz);
	bytes.insert(end(bytes), begin(Data), end(Data));
	return bytes;
}

string TDataItem::getDIdDesc()
{
	return string(DIdList[TDataItem::getDIdIndex(this->getDId())].desc);
}


// returns human-readable, formatted (multi-line) string version of this TDataItem
string TDataItem::AsString()
{
	stringstream dest;

	// dest << "DataItem = DId:" << setfill('0') << hex << uppercase << setw(sizeof(TDataId) * 2) << this->getDId()
	// 	 << ", Data bytes: " << dec << setw(0) << this->Data.size() << ", ";

	dest << "DataItem = " << this->getDIdDesc() << ", ";

	if (this->Data.size() != 0)
	{
		for (auto byt : this->Data)
		{
			dest << "0x" << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(byt) << " ";
		}
	}
	return dest.str();
}

#pragma region Verbs -------------------------------------- // TODO: fix; think this through
TDataItem &TDataItem::Go()
{
	this->result = ERR_NYI;
	return *this;
}

TError TDataItem::getResultCode()
{
	return 0;
}

std::shared_ptr<void> TDataItem::getResultValue()
{
	return std::shared_ptr<void>(0);
}

#pragma endregion
#pragma endregion TDataItem implementation

#pragma region TDataItemNYI implementation
// NYI (lol)
#pragma endregion

#pragma region TDIdReadRegister


TDIdReadRegister &TDIdReadRegister::Go()
{
	this->result = ERR_NYI;
	return *this;
}

TError TDIdReadRegister::getResultCode()
{
	return this->result;
}

std::shared_ptr<void> TDIdReadRegister::getResultValue()
{
	return std::shared_ptr<__u32>(new __u32(this->Value));
}

TError TDIdReadRegister::validateDataItemPayload(DataItemIds DataItemID, TBytes Data)
{
	TError result = ERR_SUCCESS;
	if (Data.size() != 1)
		return ERR_DId_BAD_PARAM;

	int offset = Data[0];
	int w = widthFromOffset(offset);
	if (w != 0)
		return ERR_DId_BAD_OFFSET;

	return result;
};

TDIdReadRegister::TDIdReadRegister(DataItemIds DId, int ofs)
{
	// printf("! DIAG: DId passed to TDIdReadRegister constructor was %04X, ofs=%02X\n", DId, ofs);
	this->setDId(DId);
	this->setOffset(ofs);
}

TDIdReadRegister::TDIdReadRegister(TBytes bytes)
{
	TDataItemHeader *head = (TDataItemHeader *)bytes.data();
	TDataId DId = head->DId;
	// printf("! DIAG: DId passed to TDIdReadRegister constructor(TBytes) was %04X\n", DId);
	GUARD(DId != DataItemIds::REG_Read1, ERR_DId_INVALID, DId);
	TBytes data(bytes.cbegin() + sizeof(TDataItemHeader), bytes.cend());
	TError result = ERR_SUCCESS;
	GUARD(data.size() == 1, ERR_DId_BAD_PARAM, data.size());
	this->offset = data[0];
	int w = widthFromOffset(offset);
	GUARD(w != 0, ERR_DId_BAD_PARAM, DId);
	this->width = w;
}

TDIdReadRegister &TDIdReadRegister::setOffset(int ofs)
{
	int w = widthFromOffset(ofs);
	if (w == 0)
		throw logic_error("Invalid offset passed to TDIdReadRegister::setOffset");
	this->offset = ofs;
	this->width = w;
	return *this;
}

TBytes TDIdReadRegister::AsBytes()
{
	TBytes bytes;
	TDataId DId = this->getDId();
	for (int i = 0; i < sizeof(TDataId); i++)
	{
		bytes.push_back(DId & 0x000000FF);
		DId >>= 8;
	}

	bytes.push_back(1); // payload is one byte, so push "1" for the size
	bytes.push_back(this->offset);
	return bytes;
}

string TDIdReadRegister::AsString()
{
	stringstream dest;
	dest << "DataItem = " << this->getDIdDesc() << " [";
	if (this->width == 8)
		dest << " " << this->width;
	else
		dest << this->width;
	dest << " bit] from Offset +0x" << hex << setw(2) << setfill('0') << static_cast<int>(this->offset);
	return dest.str();
}
#pragma endregion

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
	for (int i = 0; i < sizeof(ValidMessages) / sizeof(TMessageId); i++)
	{
		if (ValidMessages[i] == MessageId)
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
		printf("--ERR: data item thinks it is longer than payload, disize:%d, psize:%ld\n", DataItemSize, Payload.size());
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
	printBytes(cout, "RAW Message:", buf, 1);
	if (buf.size() < minimumMessageLength)
		return ERR_MSG_TOO_SHORT; // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?

	TMessageHeader *head = (TMessageHeader *)buf.data();

	if (!isValidMessageID(head->type))
		return ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageId Category byte)

	__u32 statedMessageLength = minimumMessageLength + head->payload_size;
	if (buf.size() < statedMessageLength)
		return ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data

	TCheckSum checksum = TMessage::calculateChecksum(buf);

	if (__valid_checksum__ != checksum)
		return ERR_MSG_CHECKSUM; // NAK(invalid checksum)

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
	if (payload_length == 0) // zero-length payload size is a valid payload
		return dataItems;

	TBytes DataItemBytes = Payload; // pointer to start of byte[] Payload
	while (payload_length > sizeof(TDataItemHeader))
	{
		TDataItemHeader *head = (TDataItemHeader *)DataItemBytes.data();
		// DataItemLength is the size of the Data Item, including the size of the Data Item Length
		// + Data Item ID, and the Data Item's payload's bytelength
		int DataItemLength = sizeof(TDataItemHeader) + head->dataLength; // DataItem[3] is payload length
		// cout << "DataItemLength is " << DataItemLength << ", payload_length is " << payload_length << endl;
		if (DataItemLength > payload_length)
		{
			result = ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH;
			cout << "DIAG::fromBytes DataItemLength > payload_length returned error " << result << ", " << result << endl;
			break;
		}

		PTDataItem item = TDataItem::fromBytes(DataItemBytes, result);
		if (result != ERR_SUCCESS)
		{
			cout << "DIAG::fromBytes returned error " << result << ", " << result << endl;
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
	// printBytes(cout, "TMessage::FromBytes received = ", buf, true);

	auto siz = buf.size();
	if (siz < minimumMessageLength)
	{
		result = ERR_MSG_TOO_SHORT;
		printf("FromBytes: Message Size < minimumMessageLength (%ld < %d\n", siz, minimumMessageLength);
		return TMessage(_INVALID_MESSAGEID_); // NAK(received insufficient data, yet) until more data (the rest of the Message/header) received?
	}
	TMessageHeader *head = (TMessageHeader *)buf.data();

	if (!isValidMessageID(head->type))
	{
		cout << "FromBytes() detected invalid MId: " << head->type << endl;
		result = ERR_MSG_ID_UNKNOWN; // NAK(invalid MessageID Category byte)
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
		TBytes payload = buf;
		payload.erase(payload.cbegin(), payload.cbegin() + sizeof(TMessageHeader));
		dataItems = parsePayload(payload, head->payload_size, result);
	}

	TCheckSum checksum = calculateChecksum(buf);
	if (__valid_checksum__ != checksum)
	{
		result = ERR_MSG_CHECKSUM; // NAK(invalid checksum)
		printf("FromBytes: invalid checksum\n");
		return TMessage();
	}

	TMessage message = TMessage(head->type, dataItems);
	// printf("FromBytes: TMessage constructed...data items has %ld items\n", message.DataItems.size());
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
		throw logic_error(err_msg[-result]);
};

TMessageId TMessage::getMId()
{
	return this->Id;
}

TCheckSum TMessage::getChecksum()
{
	return TMessage::calculateChecksum(this->AsBytes());
}

TMessage &TMessage::setMId(TMessageId ID)
{
	if (!isValidMessageID(ID))
		throw logic_error("ERR_MSG_ID_UNKNOWN"); // TODO: FIX using TError
	this->Id = ID;

	return *this;
}

TMessage &TMessage::addDataItem(PTDataItem item)
{
	this->DataItems.push_back(item);
	return *this;
}

TBytes TMessage::AsBytes()
{
	__u16 payloadLength = 0;

	for (auto item : this->DataItems)
	{
		payloadLength += item->AsBytes().size();
		// printf("item.AsBytes().size() == %ld, payloadLength == %d\n", item.AsBytes().size(), payloadLength);
	}
	TBytes bytes;

	__u8 msb = (payloadLength >> 8) & 0xFF;
	__u8 lsb = payloadLength & 0xFF;
	bytes.push_back(this->Id); /// WARN: only works because TMessageID == __u8
	//printf("Message ID == %02X, payloadLength == %hd, LSB(PLen)=%hhd MSB=%hhd\n", Id, payloadLength, lsb, msb);
	bytes.push_back(lsb);
	bytes.push_back(msb);
	for (auto item : this->DataItems)
	{
		TBytes byt = item->AsBytes();
		bytes.insert(end(bytes), begin(byt), end(byt));
	}

	TCheckSum csum = -calculateChecksum(bytes);
	bytes.push_back(csum); // WARN: only works because TCheckSum == __u8
	return bytes;
}

string TMessage::AsString()
{
	stringstream dest;
	TBytes raw = this->AsBytes();
#if 0
	dest << "Raw: ";
	for (auto byt : raw)
	{
		dest << "0x" << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(byt) << " ";
	}
	dest << endl;
#endif
	dest << "Message = MId:" << this->getMId() << ", DataItems: " << DataItems.size();
	if (DataItems.size() != 0)
	{
		for (int itemNumber = 0; itemNumber < DataItems.size(); itemNumber++)
		{
			PTDataItem item = this->DataItems[itemNumber];
			dest << endl
				 << "    " << item->AsString();
		}
		// for (TDataItem item : this->DataItems)
		// {
		// 	dest << endl
		// 			<< "  " << item.AsString();
		// }
	}
	dest << endl
		 << "    Checksum byte: " << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(raw.back());
	return dest.str();
}

#pragma endregion


