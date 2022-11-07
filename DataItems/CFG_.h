#pragma once
#include "../eNET-types.h"
#include "TDataItem.h"

class TCFG_Hostname : public TDataItem
{
public:
	TCFG_Hostname(TBytes buf);
	TCFG_Hostname(){ setDId(CFG_Hostname); }
	virtual TBytes calcPayload(bool bAsReply=false);
	virtual std::string AsString(bool bAsReply = false);
	virtual TCFG_Hostname &Go();
protected:
	std::string hostname;
};