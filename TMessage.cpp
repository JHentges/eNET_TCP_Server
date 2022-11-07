/* // TODO:
Upcoming change to centralize the construction of serialized data items:

1) renaming all derived classes' .AsBytes() to eg ".CalcPayload()" thus removing .AsBytes from all derived classes
2) adding ".CalcPayload()" to the root class for zero-length DataItem convenience
3) removing this->pushDId() and this->pushLen() related code from all the derived classes .CalcPayload() implementations
4) write .AsBytes in the root class that pushes the DId, Len(Data), and Data
*/

/* // TODO:
Upcoming change to DataItems that "read one register and return the value":

1) use a mezzanine class related to REG_Read1 with a virtual offset field
2) each "read1return" type DataItem derives from that mezzanine, and consists of just a protected offset field

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

#include "DataItems/TDataItem.h"
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



/*
// TODO:
	Upgrade the TDIdListEntry struct to include pointers to functions for calcPayload() and Go(), plus
	void * arguments for each.

	Then create a wrapper TDataItem descendant that calls those function pointers (if they aren't NULL) inside
	its .Go() and .calcPayload(), passing in the void * argument to each.

	This would, for example, allow all of the Data Items that are effectively REG_Read1() to
	have no classes explicitly in the source: just entries in the DIdList[].

	Similarly for all the Data Items that amount to REG_Write1(), like setting most (individual) ADC options.

	More complex but still re-usable calcPayload or Go can be implemented with new structs as the void * args.

	Perhaps a set of DIds that amount to "run this system command line" and they are simply passed a string.

	?Note: make sure that .AsString() has enough information to do its job "well"; otherwise add another field
	to TDIdListEntry as needed?  Either the string for .AsString() or another function pointer and void * arg.

*/



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
			Error("TMessage::parsePayload: DataItemLength > payload_length returned error " + std::to_string(result) + ", " + err_msg[-result]);
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
	//TBytes raw = this->AsBytes(bAsReply);
	//Trace("TMessage, Raw Bytes: ", raw);
	dest << "Message = MId:" << to_hex<__u8>(this->getMId()) << ", # DataItems: " << DataItems.size();
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
