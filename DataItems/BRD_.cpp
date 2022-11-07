#include "TDataItem.h"
//#include "../eNET-types.h"
//#include "../logging.h"
#include "BRD_.h"
#include "../apcilib.h"
#include "../eNET-AIO16-16F.h"

extern int apci;

TBytes TBRD_FpgaID::calcPayload(bool bAsReply)
{
	TBytes bytes;
	auto id = this->fpgaID;
	if (bAsReply)
	{
		for (int i = 0; i < sizeof(id); i++)
		{
			bytes.push_back(id & 0x000000FF);
			id >>= 8;
		}
	}
	return bytes;
}

TBRD_FpgaID &TBRD_FpgaID::Go()
{
	apci_read32(apci, 1, BAR_REGISTER, ofsFpgaID, &this->fpgaID);
	return *this;
}

std::string TBRD_FpgaID::AsString(bool bAsReply)
{
	if (bAsReply)
		return "BRD_FpgaID() → " + to_hex<__u32>(this->fpgaID);
	else
		return "BRD_FpgaID()";
}


TBytes TBRD_DeviceID::calcPayload(bool bAsReply)
{
	TBytes bytes;
	auto id = this->deviceID;
	if (bAsReply)
	{
		for (int i = 0; i < sizeof(id); i++)
		{
			bytes.push_back(id & 0x000000FF);
			id >>= 8;
		}
	}
	return bytes;
}

TBRD_DeviceID &TBRD_DeviceID::Go()
{
	__u32 value;
	apci_read32(apci, 1, BAR_REGISTER, ofsDeviceID, &value);
	this->deviceID = value & 0xFFFF;
	return *this;
}

std::string TBRD_DeviceID::AsString(bool bAsReply)
{
	if (bAsReply)
		return "BRD_DeviceID() → " + to_hex<__u16>(this->deviceID);
	else
		return "BRD_DeviceID()";
}



TBytes TBRD_Features::calcPayload(bool bAsReply)
{
	TBytes bytes;
	auto id = this->features;
	if (bAsReply)
	{
		for (int i = 0; i < sizeof(id); i++)
		{
			bytes.push_back(id & 0x000000FF);
			id >>= 8;
		}
	}
	return bytes;
}

TBRD_Features &TBRD_Features::Go()
{
	__u32 value;
	apci_read32(apci, 1, BAR_REGISTER, ofsFeatures, &value);
	this->features = value & 0xFF;
	return *this;
}

std::string TBRD_Features::AsString(bool bAsReply)
{
	if (bAsReply)
		return "BRD_Features() → " + to_hex<__u16>(this->features);
	else
		return "BRD_Features()";
}

#pragma endregion
