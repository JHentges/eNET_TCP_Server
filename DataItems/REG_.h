#pragma once
#include "../eNET-types.h"
#include "TDataItem.h"

#pragma region "class TREG_Read1 : TDataItem" for DataItemIds::REG_Read1 "Read Register Value"
class TREG_Read1 : public TDataItem
{
public:
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	TREG_Read1(TBytes data);
	TREG_Read1();
	TREG_Read1(DataItemIds DId, int ofs);
	TREG_Read1 &setOffset(int ofs);
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual TREG_Read1 &Go();
	virtual std::shared_ptr<void> getResultValue(); // TODO: fix; think this through
	virtual std::string AsString(bool bAsReply = false);
	int offset{0};
	int width{0};
private:
	__u32 Value;
};
#pragma endregion

#pragma region "class TREG_Writes declaration"
// ABSTRACT base class for TREG_Write and related functionality (eg ADC_SetConfig(), DAC_WriteAll())
class TREG_Writes : public TDataItem
{
	public:
		TREG_Writes() = default;
		~TREG_Writes();
		TREG_Writes(TBytes buf);
		virtual TREG_Writes &Go();
		TREG_Writes &addWrite(__u8 w, int ofs, __u32 value);
		virtual std::string AsString(bool bAsReply);

	protected:
		REG_WriteList Writes;
};
#pragma endregion

#pragma region "class TREG_Write1 : TREG_Writes" for REG_Write1 "Write Register Value"
class TREG_Write1 : public TREG_Writes
{
public:
	static TError validateDataItemPayload(DataItemIds DataItemID, TBytes Data);
	TREG_Write1();
	~TREG_Write1();
	TREG_Write1(TBytes buf);
	virtual TBytes calcPayload(bool bAsReply=false);
	//virtual std::string AsString(bool bAsReply=false);
};
#pragma endregion
