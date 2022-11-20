//------------------- Configuration Files -------------
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include "eNET-types.h"
#include "logging.h"
#include "config.h"


void initConfigStruct(TConfig &config) {
	config.Hostname = "enetaio000000000000000";
	config.Model = "eNET-untested";
	config.Description = "eNET-untested Description";
	config.SerialNumber = "eNET-unset Serial Number";
	config.FpgaVersionCode = 0xDEADBA57;
	config.numberOfSubmuxes = 0;
	config.adcDifferential = 0b00000000;
	for (int i = 0; i < 16; i++) {
		if (i < 4) {
			config.submuxBarcodes[i]="";
			config.submuxTypes[i]="";
			for(int j=0;j<4;j++) {
				config.submuxScaleFactors[i][j] = 1.0;
				config.submuxOffsets[i][j] = 0.0;
			}
			config.dacRanges[i] = 1;
			config.dacScaleCoefficients[i] = 1.0;
			config.dacOffsetCoefficients[i] = 0.0;
		}
		if (i < 8) {
			config.adcScaleCoefficients[i] = 1.0;
			config.adcOffsetCoefficients[i] = 0.0;

		}
		if (i < 16) {
			config.adcRangeCodes[i]=1;
		}
	}
}

#define filename (CONFIG_PATH+which+key+".conf")


int ReadConfigString(std::string key, std::string &value, std::string which )
{
	//std::string filename = CONFIG_PATH + file + "/" + key + ".conf";

	auto f = open(filename.c_str(), O_RDONLY | O_CLOEXEC | O_SYNC);
	int result = -1;
	if (f < 0)
	{
		result = errno;
		Error("failed to open() on " + filename + ", code " + std::to_string(result));
		perror("ReadConfigString() file open failed ");
		return result;
	}
	__u8 buf[65536];
	result = read(f, buf, 256);
	if (result < 0)
	{
		result = errno;
		Error("ReadConfigString() " + filename + " failed, result=" + std::to_string(result) + ", status " + std::to_string(result));
		perror("ReadConfigString() failed ");
		return result;
	}
	int L = strlen((char *) buf)-1;
	if(buf[L] = 0x10)
		buf[L] = 0;

	value = std::string((char *)buf);
	return result;
}

int ReadConfigU8(std::string key, __u8 &value, std::string which )
{
	std::string v;
	int result = ReadConfigString(key, v);
	if (result == 0)
		value = std::stoi(v, nullptr, 16);
	return result;
}

int ReadConfigU32(std::string key, __u32 &value, std::string which )
{
	std::string v;
	int result = ReadConfigString(key, v);
	if (result == 0)
		value = std::stoi(v, nullptr, 16);
	return result;
}

int ReadConfigFloat(std::string key, float &value, std::string which )
{
	__u32 v;
	int result = ReadConfigU32(key, v);
	if (result == 0)
		value = *((float *)&v);
	return result;
}

#define HandleError(x) {if (x<0)	\
		Error("Error");				\
}

int WriteConfigString(std::string key, std::string value, std::string which ){
	// validate key, value, and filename are valid/safe to use
	// if file == "config.current" then key must already exist
	// ConfigWrite(file, key, value);
	//std::string filename = CONFIG_PATH + file + "/" + key + ".conf";
	auto f = open(filename.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	int result = -1;
	if (f < 0)
	{
		result = errno;
		Error("failed open() on " + filename + " code: " + std::to_string(result));
		perror("open() failed");
	}
	else
	{
		result = write(f, value.c_str(), strlen(value.c_str()));
		if (result < 0)
		{
			result = errno;
			Error("failed open() on " + filename + " code: " + std::to_string(result));
			perror("WriteConfigString() open() failed");
		}	close(f);
	}

	return result;
};

int WriteConfigU8(std::string key, __u8 value, std::string which )
{
	std::string v = to_hex<__u8>(value);
	return WriteConfigString(key, v, which);
}

int WriteConfigU32(std::string key, __u32 value, std::string which )
{
	std::string v = to_hex<__u32>(value);
	return WriteConfigString(key, v, which);
}

int WriteConfigFloat(std::string key, float value, std::string which )
{
	__u32 v = *((__u32 *)&value);
	return WriteConfigU32(key, v, which);
}

// Reads the /etc/aioenetd.d/config.current/configuration data into the Config structure
void LoadConfig(std::string which)
{

	{
	// read /etc/hostname into Config.Hostname
		std::ifstream in("/etc/hostname");
		in >> Config.Hostname;
		in.close();
		Log("Hostname == " + Config.Hostname);
	}

	HandleError(ReadConfigString("BRD_Model", Config.Model, which));
	HandleError(ReadConfigString("BRD_Description", Config.Description, which));
	HandleError(ReadConfigString("BRD_SerialNumber", Config.SerialNumber, which));

	HandleError(ReadConfigU8("BRD_NumberOfSubmuxes", Config.numberOfSubmuxes, which));

	HandleError(ReadConfigString("BRD_SubmuxBarcode0", Config.submuxBarcodes[0], which));
	HandleError(ReadConfigString("BRD_SubmuxBarcode1", Config.submuxBarcodes[1], which));
	HandleError(ReadConfigString("BRD_SubmuxBarcode2", Config.submuxBarcodes[2], which));
	HandleError(ReadConfigString("BRD_SubmuxBarcode3", Config.submuxBarcodes[3], which));

	HandleError(ReadConfigString("BRD_SubmuxType0", Config.submuxTypes[0], which));
	HandleError(ReadConfigString("BRD_SubmuxType1", Config.submuxTypes[1], which));
	HandleError(ReadConfigString("BRD_SubmuxType2", Config.submuxTypes[2], which));
	HandleError(ReadConfigString("BRD_SubmuxType3", Config.submuxTypes[3], which));

	HandleError(ReadConfigFloat("BRD_Submux0Range0", Config.submuxScaleFactors[0][0], which));
	HandleError(ReadConfigFloat("BRD_Submux0Range1", Config.submuxScaleFactors[0][1], which));
	HandleError(ReadConfigFloat("BRD_Submux0Range2", Config.submuxScaleFactors[0][2], which));
	HandleError(ReadConfigFloat("BRD_Submux0Range3", Config.submuxScaleFactors[0][3], which));
	HandleError(ReadConfigFloat("BRD_Submux1Range0", Config.submuxScaleFactors[1][0], which));
	HandleError(ReadConfigFloat("BRD_Submux1Range1", Config.submuxScaleFactors[1][1], which));
	HandleError(ReadConfigFloat("BRD_Submux1Range2", Config.submuxScaleFactors[1][2], which));
	HandleError(ReadConfigFloat("BRD_Submux1Range3", Config.submuxScaleFactors[1][3], which));
	HandleError(ReadConfigFloat("BRD_Submux2Range0", Config.submuxScaleFactors[2][0], which));
	HandleError(ReadConfigFloat("BRD_Submux2Range1", Config.submuxScaleFactors[2][1], which));
	HandleError(ReadConfigFloat("BRD_Submux2Range2", Config.submuxScaleFactors[2][2], which));
	HandleError(ReadConfigFloat("BRD_Submux2Range3", Config.submuxScaleFactors[2][3], which));
	HandleError(ReadConfigFloat("BRD_Submux3Range0", Config.submuxScaleFactors[3][0], which));
	HandleError(ReadConfigFloat("BRD_Submux3Range1", Config.submuxScaleFactors[3][1], which));
	HandleError(ReadConfigFloat("BRD_Submux3Range2", Config.submuxScaleFactors[3][2], which));
	HandleError(ReadConfigFloat("BRD_Submux3Range3", Config.submuxScaleFactors[3][3], which));

	HandleError(ReadConfigFloat("BRD_Submux0Offset0", Config.submuxOffsets[0][0], which));
	HandleError(ReadConfigFloat("BRD_Submux0Offset1", Config.submuxOffsets[0][1], which));
	HandleError(ReadConfigFloat("BRD_Submux0Offset2", Config.submuxOffsets[0][2], which));
	HandleError(ReadConfigFloat("BRD_Submux0Offset3", Config.submuxOffsets[0][3], which));
	HandleError(ReadConfigFloat("BRD_Submux1Offset0", Config.submuxOffsets[1][0], which));
	HandleError(ReadConfigFloat("BRD_Submux1Offset1", Config.submuxOffsets[1][1], which));
	HandleError(ReadConfigFloat("BRD_Submux1Offset2", Config.submuxOffsets[1][2], which));
	HandleError(ReadConfigFloat("BRD_Submux1Offset3", Config.submuxOffsets[1][3], which));
	HandleError(ReadConfigFloat("BRD_Submux2Offset0", Config.submuxOffsets[2][0], which));
	HandleError(ReadConfigFloat("BRD_Submux2Offset1", Config.submuxOffsets[2][1], which));
	HandleError(ReadConfigFloat("BRD_Submux2Offset2", Config.submuxOffsets[2][2], which));
	HandleError(ReadConfigFloat("BRD_Submux2Offset3", Config.submuxOffsets[2][3], which));
	HandleError(ReadConfigFloat("BRD_Submux3Offset0", Config.submuxOffsets[3][0], which));
	HandleError(ReadConfigFloat("BRD_Submux3Offset1", Config.submuxOffsets[3][1], which));
	HandleError(ReadConfigFloat("BRD_Submux3Offset2", Config.submuxOffsets[3][2], which));
	HandleError(ReadConfigFloat("BRD_Submux3Offset3", Config.submuxOffsets[3][3], which));

	HandleError(ReadConfigU8("ADC_Differential", Config.adcDifferential, which));

	HandleError(ReadConfigU32("ADC_RangeCodeCh00", Config.adcRangeCodes[0], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh01", Config.adcRangeCodes[1], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh02", Config.adcRangeCodes[2], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh03", Config.adcRangeCodes[3], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh04", Config.adcRangeCodes[4], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh05", Config.adcRangeCodes[5], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh06", Config.adcRangeCodes[6], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh07", Config.adcRangeCodes[7], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh08", Config.adcRangeCodes[8], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh09", Config.adcRangeCodes[9], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh10", Config.adcRangeCodes[10], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh11", Config.adcRangeCodes[11], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh12", Config.adcRangeCodes[12], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh13", Config.adcRangeCodes[13], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh14", Config.adcRangeCodes[14], which));
	HandleError(ReadConfigU32("ADC_RangeCodeCh15", Config.adcRangeCodes[15], which));

	HandleError(ReadConfigFloat("ADC_ScaleRange0", Config.adcScaleCoefficients[0], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange1", Config.adcScaleCoefficients[1], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange2", Config.adcScaleCoefficients[2], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange3", Config.adcScaleCoefficients[3], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange4", Config.adcScaleCoefficients[4], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange5", Config.adcScaleCoefficients[5], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange6", Config.adcScaleCoefficients[6], which));
	HandleError(ReadConfigFloat("ADC_ScaleRange7", Config.adcScaleCoefficients[7], which));

	HandleError(ReadConfigFloat("ADC_OffsetRange0", Config.adcOffsetCoefficients[0], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange1", Config.adcOffsetCoefficients[1], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange2", Config.adcOffsetCoefficients[2], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange3", Config.adcOffsetCoefficients[3], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange4", Config.adcOffsetCoefficients[4], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange5", Config.adcOffsetCoefficients[5], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange6", Config.adcOffsetCoefficients[6], which));
	HandleError(ReadConfigFloat("ADC_OffsetRange7", Config.adcOffsetCoefficients[7], which));

	HandleError(ReadConfigU32("DAC_RangeCh0", Config.dacRanges[0], which));
	HandleError(ReadConfigU32("DAC_RangeCh1", Config.dacRanges[1], which));
	HandleError(ReadConfigU32("DAC_RangeCh2", Config.dacRanges[2], which));
	HandleError(ReadConfigU32("DAC_RangeCh3", Config.dacRanges[3], which));

	HandleError(ReadConfigFloat("DAC_ScaleCh0", Config.dacScaleCoefficients[0], which));
	HandleError(ReadConfigFloat("DAC_ScaleCh1", Config.dacScaleCoefficients[1], which));
	HandleError(ReadConfigFloat("DAC_ScaleCh2", Config.dacScaleCoefficients[2], which));
	HandleError(ReadConfigFloat("DAC_ScaleCh3", Config.dacScaleCoefficients[3], which));

	HandleError(ReadConfigFloat("DAC_OffsetCh0", Config.dacOffsetCoefficients[0], which));
	HandleError(ReadConfigFloat("DAC_OffsetCh1", Config.dacOffsetCoefficients[1], which));
	HandleError(ReadConfigFloat("DAC_OffsetCh2", Config.dacOffsetCoefficients[2], which));
	HandleError(ReadConfigFloat("DAC_OffsetCh3", Config.dacOffsetCoefficients[3], which));
}
