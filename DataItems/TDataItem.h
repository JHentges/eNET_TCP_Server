#pragma once

#include "../eNET-types.h"
#include "../TError.h"


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
