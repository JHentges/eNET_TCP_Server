#include "../logging.h"
#include "TDataItem.h"
#include "DAC_.h"
#include "../config.h"

TDAC_Range1::TDAC_Range1(TBytes bytes)
{
	Debug("Received: ", bytes);
	setDId(DAC_Range1);
	TError result = ERR_SUCCESS;
	this->Data = bytes;

	if (bytes.size() >= 1)
	{
		GUARD(this->Data[0] < 4, ERR_DId_BAD_PARAM, this->Data[0]);
		this->dacChannel = this->Data[0];
		this->dacRange = 0xFFFFFFFF;
		this->bWrite = false;
	}
	if (bytes.size() == 5)
	{
		__u32 rangeCode = this->Data[1] | this->Data[2] << 8 | this->Data[3] << 16 | this->Data[4] << 24;
		if((rangeCode < 4)
		|| (rangeCode==0x30313055)
		|| (rangeCode==0x35303055)
		|| (rangeCode==0x3530E142)
		|| (rangeCode==0x3031E142))
		{
			this->bWrite = true;
			this->dacRange = rangeCode;
			Log("DAC "+std::to_string(this->dacChannel)+" range set to "+to_hex<__u32>(this->dacRange));
		}
	}
	return;
}

TBytes TDAC_Range1::calcPayload(bool bAsReply)
{
	TBytes bytes;
	bytes.push_back(this->dacChannel);
	bytes.push_back(this->dacRange & 0xFF);
	bytes.push_back((this->dacRange >> 8) & 0xFF);
	bytes.push_back((this->dacRange >> 16) & 0xFF);
	bytes.push_back((this->dacRange >> 24) & 0xFF);
	return bytes;
};

TDAC_Range1 &TDAC_Range1::Go()
{
	if (this->bWrite)
	{
		// if (permissions(this->permissions, bmConfigFileWrite)) // TODO: something here
		// if (this->dacRange != Config.dacRanges[this->dacChannel])
		{
			// write DAC_Range config file data
			if (0 > WriteConfigSetting("DAC_RangeCh"+std::to_string(this->dacChannel), to_hex<__u32>(this->dacRange)))
			{
				// handle write error
				this->dacRange = Config.dacRanges[this->dacChannel];
				Error("Failed to write config setting DAC_RangeCh");
			}
			else
				Config.dacRanges[this->dacChannel] = this->dacRange;
			;

			//   ?OR?   issue registered callback into aioenetd so it will
			// if (this->OnWrite != nullptr)
				// this->OnWrite(this->dacChannel, this->dacRange);
		}
	}
	else
	{
		// read DAC_Range1; handled by .AsBytes/.AsString
		// if (this->OnRead != nullptr)
			// this->OnRead(dacChannel);
	}
	return *this;
};

std::string TDAC_Range1::AsString(bool bAsReply)
{
	// NYI
	return "d";
};