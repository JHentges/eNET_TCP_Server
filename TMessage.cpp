#include <cstdlib>
#include <stdexcept>

#include "TMessage.h"
using namespace std;

const char *err_msg[] = {
	/*  0 */ "VALID",
	/* -1 */ "Message too short",
	/* -2 */ "Checksum error",
	/* -3 */ "Parse error",
	/* -4 */ "Invalid Start symbol found",
	/* -5 */ "Unknown Message ID",
	/* -6 */ "Length mismatch",
	/* -7 */ "DataItem length mismatch",
	/* -8 */ "DataItem ID unknown",
	/* -9 */ "DataItem too short",
};

#define hexbyte(x, w) "0x" << std::hex << setfill('0') << std::setw(w) << std::uppercase << static_cast<int>(x)

const vector<TMessageId> ValidMessages{
	'Q',
	'R',
	'C',
};
const vector<TDataId> ValidDataItemIDs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0A, 0x0B};

int widthFromOffset(int ofs)
{
	int w = 0;
	if (ofs < 0x18)
		w = 8;
	else if (ofs <= 0xDC)
		w = 32;
	return w;
}

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
int TDataItem::validateDataItemPayload(TDataId DataItemID, TBytes Data)
{
	int result = ERR_SUCCESS;
	return result;
}

int TDataItem::isValidDataItemID(TDataId DataItemID)
{
	int result = false;
	for (TDataId aValidId : ValidDataItemIDs)
	{
		if (DataItemID == aValidId)
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

	TDataItemLength DataItemSize = head->dataLength; // WARN: only works if length field is one byte
	TDataId Id = head->DId;
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

shared_ptr<TDataItem> TDataItem::parseDataItem(TBytes msg, TError &result)
{
	result = ERR_SUCCESS;
	printf("parseDataItem received = ");
	for (auto byt : msg)
		printf("%02X ", byt);
	printf("\n");
	// if (! isValidDataItemId(Id))
	// {
	//     result = ERR_MSG_DATAITEM_ID_UNKNOWN;
	//     return TDataItem();
	// }

	if (msg.size() < DataItemHeaderSize) // todo: suck, fix
		throw std::underflow_error("message");

	TDataItemHeader *head = (TDataItemHeader *)msg.data();
	TDataItemLength DataSize = head->dataLength; // MessageLength

	shared_ptr<TDataItem> anItem;
	if (DataSize == 0)
		return shared_ptr<TDataItem>(new TDataItem(head->DId)); // no data in this data item is valid
	TBytes data;
	data.insert(data.end(), msg.cbegin() + sizeof(TDataItemHeader), msg.cbegin() + sizeof(TDataItemHeader) + DataSize);

	result = validateDataItemPayload(head->DId, data); // TODO: change to parseData that returns vector of classes-of-data-types
	if (result != ERR_SUCCESS)
		return shared_ptr<TDataItem>(new TDataItem());

	printf("parseDataItem generated DId = %04X, data = ", head->DId);
	for (auto byt : data)
		printf("%02X ", byt);
	printf("\n");
	return shared_ptr<TDataItem>(new TDataItem(head->DId, data));
}
#pragma endregion

TDataItem::TDataItem() : TDataItem{0} {};

TDataItem::TDataItem(TDataId DId)
{
	this->setDId(DId);
};

// parses vector of bytes (presumably received across TCP socket) into a TDataItem
TDataItem::TDataItem(TBytes bytes) : TDataItem()
{
	TError result = ERR_SUCCESS;
	GUARD(bytes.size() < DataItemHeaderSize, ERR_MSG_DATAITEM_TOO_SHORT, bytes.size());

	TDataItemHeader *head = (TDataItemHeader *)bytes.data();
	GUARD(isValidDataItemID(head->DId), ERR_MSG_DATAITEM_ID_UNKNOWN, head->DId);

	TDataItemLength DataSize = head->dataLength; // TODO, FIX: only works whle TDataId is one byte; change to TDataItemHeader casting
	GUARD(bytes.size() != DataItemHeaderSize + DataSize, ERR_MSG_PAYLOAD_DATAITEM_LEN_MISMATCH, DataSize);

	this->Data = TBytes(bytes.begin() + sizeof(TDataItemHeader), bytes.end()); // extract the Data from the DataItem bytes
	if (DataSize > 0)
		TError result = validateDataItemPayload(this->Id, Data); // TODO: change to parseData that returns vector of classes-of-data-types
	return;
}

TDataItem::TDataItem(TDataId DId, TBytes bytes)
{
	this->setDId(DId);
	this->Data = bytes;
	printf("TDataItem Constructor (%04hx, bytes) = ", DId);
	for (auto byt : this->Data)
		printf("%02X ", byt);
	printf("\n");
}

TDataItem &TDataItem::addData(__u8 aByte)
{
	Data.push_back(aByte);
	return *this;
}

TDataItem &TDataItem::setDId(TDataId DId)
{
	GUARD(isValidDataItemID(DId), ERR_MSG_DATAITEM_ID_UNKNOWN, DId);
	this->Id = DId;
	return *this;
}

TDataId TDataItem::getDId()
{
	return this->Id;
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

/* sets dest to human readable string. */

string TDataItem::AsString()
{
	stringstream dest;

	dest << "DataItem = DId:" << setfill('0') << hex << uppercase << setw(sizeof(TDataId) * 2) << this->getDId()
		 << ", Data bytes: " << dec << setw(0) << this->Data.size() << ", ";
	if (this->Data.size() != 0)
	{
		for (auto byt : this->Data)
		{
			dest << "0x" << hex << setfill('0') << setw(2) << uppercase << static_cast<int>(byt) << " ";
		}
	}
	return dest.str();
}

#pragma endregion TDataItem implementation

#pragma region DIdReadRegister
TError DIdReadRegister::validateDataItemPayload(TDataId DataItemID, TBytes Data)
{
	TError result = ERR_SUCCESS;
	if (Data.size() != 1)
		return ERR_DId_BAD_PARAM;

	int offset = Data[0];
	switch (DataItemID)
	{
	case DIdRead8: // on eNET-AIO the only 8-bit registers are 0x00 through 0x17
		if (offset > 0x17)
			result = ERR_DId_BAD_OFFSET;
		break;

	case DIdRead16: // eNET-AIO has zero 16-bit registers
		return ERR_DId_BAD_PARAM;
		// if ((offset % 2) != 0)
		// 	return ERR_DId_BAD_OFFSET;
		break;

	case DIdRead32: // eNET-AIO has 32-bit registers from 0x18 through 0xC0, but 0xA8->0xBC are undefined
		if ((offset < 0x18) || (offset > 0xDC) || ((offset >= 0xA8) && (offset < 0xC0)))
			return ERR_DId_BAD_OFFSET;
		if ((offset % 4) != 0)
			return ERR_DId_BAD_OFFSET;
		break;
	}
	return result;
};

DIdReadRegister::DIdReadRegister(TDataId DId, int ofs)
{
	this->setDId(DId);
	this->setOffset(ofs);
}

DIdReadRegister::DIdReadRegister(TBytes bytes)
{
	TDataItemHeader *head = (TDataItemHeader *)bytes.data();
	TDataId DId = head->DId;
	GUARD(((DId != DIdRead8) && (DId != DIdRead16) && (DId != DIdRead32)), ERR_DId_INVALID, DId);
	TBytes data(bytes.cbegin() + sizeof(TDataItemHeader), bytes.cend());
	TError result = ERR_SUCCESS;
	GUARD(data.size() == 1, ERR_DId_BAD_PARAM, data.size());
	this->offset = data[0];
	int w = widthFromOffset(offset);
	GUARD(w != 0, ERR_DId_BAD_PARAM, DId);
	this->width = w;
}

DIdReadRegister &DIdReadRegister::setOffset(int ofs)
{
	int w = widthFromOffset(ofs);
	if (w == 0)
		throw logic_error("Invalid offset passed to DIdReadRegister::setOffset");
	this->offset = ofs;
	this->width = w;
	return *this;
}

TBytes DIdReadRegister::AsBytes()
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

string DIdReadRegister::AsString()
{
	stringstream dest;
	dest << "DataItem = ReadRegister" << this->width << " from Offset +0x" << hex << setw(2) << setfill('0') << static_cast<int>(this->offset);
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
	TError result = ERR_SUCCESS;
	if (Payload.size() == 1) // one-byte Payload is the checksum byte.
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
	payload.pop_back();
	payload.erase(payload.cbegin(), payload.cbegin() + sizeof(TMessageHeader));

	int validPayload = validatePayload(payload);
	if (validPayload != 0)
		return validPayload;

	return 0; // valid message
}
/* A Payload consists of zero or more DataItems
 * This function parses an array of bytes that is supposed to be a Payload
 * ...then returns a vector of those TDataItems and sets result to indicate error/success
 */
vector<shared_ptr<TDataItem>> TMessage::parsePayload(TBytes Payload, __u32 payload_length, TError &result)
{
	vector<shared_ptr<TDataItem>> dataItems; // an empty vector<>
	result = ERR_SUCCESS;
	if (payload_length == 0) // zero-length payload size is a valid payload
		return dataItems;

	TBytes DataItemBytes = Payload; // pointer to start of byte[] Payload
	printf("parsePayload received = ");
	for (auto byt : DataItemBytes)
		printf("%02X ", byt);
	printf("\n");
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
			cout << "DIAG::parseDataItem DataItemLength > payload_length returned error " << result << endl;
			break;
		}
		shared_ptr<TDataItem> item = TDataItem::parseDataItem(DataItemBytes, result);
		if (result != ERR_SUCCESS)
		{
			cout << "DIAG::parseDataItem returned error " << result << endl;
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

	printf("FromBytes received = ");
	for (auto byt : buf)
		printf("%02X ", byt);
	printf("\n");

	result = validateMessage(buf);
	if (result != ERR_SUCCESS)
		printf("DIAG::validateMessage failed with error %d\n", result);

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
		return TMessage(result);
	}
	__u32 statedMessageLength = minimumMessageLength + head->payload_size;
	if (siz < statedMessageLength)
	{
		result = ERR_MSG_LEN_MISMATCH; // NAK(received insufficient data, yet) until more data
		return TMessage(result);
	}

	vector<std::shared_ptr<TDataItem>> dataItems;
	if (head->payload_size > 0)
	{
		TBytes payload = buf;
		payload.erase(payload.cbegin(), payload.cbegin() + sizeof(TMessageHeader));
		printf("FromBytes generated a payload to parse of = ");
		for (auto byt : payload)
			printf("%02X ", byt);
		printf("\n");
		dataItems = parsePayload(payload, head->payload_size, result);
	}

	printf("FromBytes: payload parsed ... data items has %ld items\n", dataItems.size());

	TCheckSum checksum = calculateChecksum(buf);
	if (__valid_checksum__ != checksum)
	{
		result = ERR_MSG_CHECKSUM; // NAK(invalid checksum)
		printf("FromBytes: invalid checksum\n");
		return TMessage(result);
	}

	TMessage message = TMessage(head->type, dataItems);
	// printf("FromBytes: TMessage constructed...data items has %ld items\n", message.DataItems.size());
	return message;
}

TMessage::TMessage(TMessageId MId)
{
	this->setMId(MId);
}

TMessage::TMessage(TMessageId MId, vector<std::shared_ptr<TDataItem>> Payload)
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
		throw logic_error(err_msg[-ERR_MSG_ID_UNKNOWN]);
	this->Id = ID;

	return *this;
}

TMessage &TMessage::addDataItem(std::shared_ptr<TDataItem> item)
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
	printf("Message ID == %02X, payloadLength == %hd, LSB(PLen)=%hhd MSB=%hhd\n", Id, payloadLength, lsb, msb);
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
			shared_ptr<TDataItem> item = this->DataItems[itemNumber];
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
