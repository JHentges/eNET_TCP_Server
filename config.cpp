//------------------- Configuration Files -------------
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include "eNET-types.h"
#include "logging.h"
#include "config.h"


int WriteConfigSetting(std::string key, std::string value, std::string file ){
	// validate key, value, and filename are valid/safe to use
	// if file == "config.current" then key must already exist
	// ConfigWrite(file, key, value);
	std::string filename = CONFIG_PATH + file + "/" + key + ".conf";
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
			Error("failed open() on " + filename + " code: " + std::to_string(errno));
			perror("WriteConfigSetting() open() failed");
		}	close(f);
	}

	return result;
};

int ReadConfigSetting(std::string key, std::string &value, std::string file )
{
	std::string filename = CONFIG_PATH + file + "/" + key + ".conf";
	auto f = open(filename.c_str(), O_RDONLY | O_CLOEXEC | O_SYNC);
	int result = -1;
	if (f < 0)
	{
		result = errno;
		Error("failed to open() on " + filename + ", code " + std::to_string(errno));
		perror("ReadConfigSetting() file open failed ");
		return result;
	}
	__u8 buf[65536];
	result = read(f, buf, 256);
	if (result < 0)
	{
		Error("ReadConfigSetting() "+filename+" failed, result="+std::to_string(result)+", status "+std::to_string(errno));
		perror("ReadConfigSetting() failed ");
	}
	int l = strlen((char *) buf)-1;
	if(buf[l]=0x10)
		buf[l] = 0;

	value = std::string((char *)buf);
	return result;
}

int ReadConfigU8(std::string key, __u8 &value, std::string file)
{
	std::string v;
	int result = ReadConfigSetting(key, v);
	value = std::stoi(v, nullptr, 16);
	return result;
}

int ReadConfigU32(std::string key, __u32 &value, std::string file)
{
	std::string v;
	int result = ReadConfigSetting(key, v);
	value = std::stoi(v, nullptr, 16);
	return result;
}

// Reads the /etc/aioenetd.d/config.current/configuration data into the Config structure
void LoadConfig()
{

	if (0 > ReadConfigU32("DAC_RangeCh0", Config.dacRanges[0])) {
		Error("Failed to read DAC_RangeCh0 Config");
		perror("Failed to read DAC_RangeCh0 Config");
	}
	if (0 > ReadConfigU32("DAC_RangeCh1", Config.dacRanges[1])) {
		Error("Failed to read DAC_RangeCh1 Config");
		perror("Failed to read DAC_RangeCh1 Config");
	}
	if (0 > ReadConfigU32("DAC_RangeCh2", Config.dacRanges[2])) {
		Error("Failed to read DAC_RangeCh2 Config");
		perror("Failed to read DAC_RangeCh2 Config");
	}
	if (0 > ReadConfigU32("DAC_RangeCh3", Config.dacRanges[3])) {
		Error("Failed to read DAC_RangeCh3 Config");
		perror("Failed to read DAC_RangeCh3 Config");
	}
	{
	// read /etc/hostname into Config.Hostname
		std::ifstream in("/etc/hostname");
		in >> Config.Hostname;
		in.close();
		Log("Hostname == " + Config.Hostname);
	}
}

//-------------------- end Configuration Files --------
