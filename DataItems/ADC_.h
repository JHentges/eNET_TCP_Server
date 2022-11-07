#pragma once

#include "TDataItem.h"

class TADC_BaseClock : public TDataItem
{
public:
	TADC_BaseClock(){ setDId(ADC_BaseClock);}
	TADC_BaseClock(TBytes buf);
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual TADC_BaseClock &Go();
	virtual std::string AsString(bool bAsReply = false);
protected:
	__u32 baseClock = 25000000;
};

class TADC_StreamStart : public TDataItem
{
public:
	TADC_StreamStart(TBytes buf);
	TADC_StreamStart(){ setDId(ADC_StreamStart);};
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual TADC_StreamStart &Go();
	virtual std::string AsString(bool bAsReply = false);
protected:
	int argConnectionID = -1;
};

class TADC_StreamStop : public TDataItem
{
public:
	TADC_StreamStop(){ setDId(ADC_StreamStop);}
	TADC_StreamStop(TBytes buf);
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual TADC_StreamStop &Go();
	virtual std::string AsString(bool bAsReply = false);
};
