#include "TDataItem.h"
#include "BRD_.h"
#include "../apcilib.h"
#include "../eNET-AIO16-16F.h"

extern int apci;

std::string TBRD_FpgaID::AsString(bool bAsReply){
	if (bAsReply)
		return "BRD_FpgaID() → " + to_hex<__u32>(this->fpgaID);
	else
		return "BRD_FpgaID()";
}
TBRD_FpgaID &TBRD_FpgaID::Go() {
	apci_read32(apci, 1, BAR_REGISTER, ofsFpgaID, &this->fpgaID);
	return *this;
}
TBytes TBRD_FpgaID::calcPayload(bool bAsReply) {
	TBytes bytes;
	if (bAsReply)
		stuff<__u32>(bytes, this->fpgaID);
	return bytes;
}


std::string TBRD_DeviceID::AsString(bool bAsReply) {
	if (bAsReply)
		return "BRD_DeviceID() → " + to_hex<__u16>(this->deviceID);
	else
		return "BRD_DeviceID()";
}
TBRD_DeviceID &TBRD_DeviceID::Go() {
	__u32 value;
	apci_read32(apci, 1, BAR_REGISTER, ofsDeviceID, &value);
	this->deviceID = value & 0xFFFF;
	return *this;
}
TBytes TBRD_DeviceID::calcPayload(bool bAsReply) {
	TBytes bytes;
	if (bAsReply)
		stuff(bytes, this->deviceID);
	return bytes;
}


std::string TBRD_Features::AsString(bool bAsReply) {
	if (bAsReply)
		return "BRD_Features() → " + to_hex<__u16>(this->features);
	else
		return "BRD_Features()";
}
TBRD_Features &TBRD_Features::Go() {
	__u32 value;
	apci_read32(apci, 1, BAR_REGISTER, ofsFeatures, &value);
	this->features = value & 0xFF;
	return *this;
}
TBytes TBRD_Features::calcPayload(bool bAsReply){
	TBytes bytes;
	if (bAsReply)
		stuff(bytes, this->features);
	return bytes;
}

#pragma endregion
