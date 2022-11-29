#pragma once

#include "TDataItem.h"
#include "../eNET-types.h"

class TDAC_Output : public TDataItem
{
public:
	TDAC_Output(TBytes buf);
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual std::string AsString(bool bAsReply = false);
	virtual TDAC_Output &Go();
protected:
	__u8 dacChannel = 0;
	__u16 dacCounts = 0;
};



class TDAC_Range1 : public TDataItem
{
public:
	TDAC_Range1(TBytes buf);
	TDAC_Range1(){ setDId(DAC_Range1);};
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual std::string AsString(bool bAsReply = false);
	virtual TDAC_Range1 &Go();
protected:
	__u8 dacChannel = 0;
	__u32 dacRange = 0;
};