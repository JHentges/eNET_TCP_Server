//------------------- Configuration Files -------------
#include <fcntl.h>
#include <unistd.h>

#include "eNET-types.h"
#include "logging.h"
#include "config.h"


int WriteConfigSetting(std::string key, std::string value, std::string file ){
	// validate key, value, and filename are valid/safe to use
	// if file == "config.current" then key must already exist
	// ConfigWrite(file, key, value);
	std::string filename = CONFIG_PATH+file+"/" + key;
	auto f = open(filename.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_DIRECT | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	int result = write(f, value.c_str(), strlen(value.c_str()));
	close(f);

	return result;
};

int ReadConfigSetting(std::string key, std::string &value, std::string file )
{
	std::string filename = CONFIG_PATH+file+"/" + key;
	auto f = open(filename.c_str(), O_RDONLY | O_CLOEXEC | O_SYNC);
	if (f<0)
	{
		Error("ReadConfigSetting() failed to open config file "+filename+", status "+std::to_string(errno));
		perror("ReadConfigSetting() file open failed ");
	}
	__u8 buf[65536];
	int result = read(f, buf, 256);
	if (result < 0)
	{
		Error("ReadConfigSetting() "+filename+" failed, result="+std::to_string(result)+", status "+std::to_string(errno));
		perror("ReadConfigSetting() failed ");
	}
	Log("Config "+file+key+value+"="+std::string((char*)buf));
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
}

//-------------------- end Configuration Files --------
