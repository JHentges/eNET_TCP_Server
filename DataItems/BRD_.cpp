#include "TDataItem.h"
#include "BRD_.h"
#include "../apci.h"
#include "../eNET-AIO16-16F.h"

//extern int apci;

std::string TBRD_FpgaID::AsString(bool bAsReply){
	if (bAsReply)
		return "BRD_FpgaID() → " + to_hex<__u32>(this->fpgaID);
	else
		return "BRD_FpgaID()";
}
TBRD_FpgaID &TBRD_FpgaID::Go() {
	this->fpgaID = in(ofsFpgaID);
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
	this->deviceID = in(ofsDeviceID) & 0xFFFF;
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
	this->features = in(ofsFeatures) & 0xFF;
	return *this;
}
TBytes TBRD_Features::calcPayload(bool bAsReply){
	TBytes bytes;
	if (bAsReply)
		stuff(bytes, this->features);
	return bytes;
}

#pragma endregion
