#pragma once
/*
eNET-AIO configuration structure and support API.

	All configuration that should be stored non-volatile is held in TConfig structures.
	The "currently in-use or about to be" configuration is TConfig Config; global.

	There are Factory, Current, and User configurations in nonvolatile storage on eNET-AIO.

	The different configuration types are stored in directories per type under /etc/opt/aioenet/,
	config.factory/, config.current/, and config.user/

	NOTE: Future revisions may support multiple user configs

	aioenetd (or other users of this config.h/.cpp) should:
		call initConfigStruct(Config) to set "useful" defaults in the global Config struct.
		call InitializeConfigFiles() to create the config directories and files if they are not found
		call LoadConfig(CONFIG_FACTORY), which will update Config with any found .conf values
		call LoadConfig(CONFIG_CURRENT), which will update the Config with the customer's desired values
*/

#include "eNET-types.h"
#include "eNET-AIO16-16F.h"

#define CONFIG_PATH "/etc/opt/aioenet/"
#define CONFIG_CURRENT "config.current/"
#define CONFIG_FACTORY "config.factory/"
#define CONFIG_USER "config.user/"

/*
RangeCode 0 = 0-10 V
RangeCode 1 = ±10 V
RangeCode 2 = 0-5 V
RangeCode 3 = ±5 V
RangeCode 4 = 0-2 V
RangeCode 5 = ±2 V
RangeCode 6 = 0-1 V
RangeCode 7 = ±1 V
*/
typedef struct TConfigStruct {
	std::string Hostname;
	std::string Model;
	std::string Description;
	std::string SerialNumber;
	__u32 FpgaVersionCode;
	__u8 numberOfSubmuxes;
	std::string submuxBarcodes[4];
	std::string submuxTypes[maxSubmuxes];
	float submuxScaleFactors[maxSubmuxes][gainGroupsPerSubmux];
	float submuxOffsets[maxSubmuxes][gainGroupsPerSubmux];
	__u8 adcDifferential; // bit map bit0=adcCh0, set == differential // J2H: TODO: may want to invert this; may want 16 of them, may want 128 of them, may want 16+128 of them
	__u32 adcRangeCodes[16];
	float adcScaleCoefficients[8];
	float adcOffsetCoefficients[8];
	__u32 dacRanges[4];
	float dacScaleCoefficients[4];
	float dacOffsetCoefficients[4];
} TConfig;
extern TConfig Config;

void initConfigStruct(TConfig &config);
void LoadConfig(std::string which = CONFIG_CURRENT);

// On success: set value to config string from disk and return 0
// On error: leave value unchanged and return errno
// Config is stored in /etc/opt/aioenet/ in directories named config.factory/ config.current/ and config.user/
// in files named key.conf
int ReadConfigString(std::string key, std::string &value, std::string which = CONFIG_CURRENT);

// On success: set value to config byte from disk and return 0
// On error: leave value unchanged and return errno
// Config is stored in /etc/opt/aioenet/ in directories named config.factory/ config.current/ and config.user/
// in files named key.conf, in two-digit HEX form; intel nybble order, padded with leading zeroes
int ReadConfigU8(std::string key, __u8 &value, std::string which = CONFIG_CURRENT);

// On success: set value to config __u32 from disk and return 0
// On error: leave value unchanged and return errno
// Config is stored in /etc/opt/aioenet/ in directories named config.factory/ config.current/ and config.user/
// in files named key.conf, in 8-digit HEX form; intel order, padded with leading zeroes
int ReadConfigU32(std::string key, __u32 &value, std::string which = CONFIG_CURRENT);

// On success: set value to config float from disk and return 0
// On error: leave value unchanged and return errno
// Config is stored in /etc/opt/aioenet/ in directories named config.factory/ config.current/ and config.user/
// in files named key.conf, in 8-digit HEX form; intel order, padded with leading zeroes
// (the single-precision 4-byte float is stored as a __u32 for perfect round-trip serdes)
int ReadConfigFloat(std::string key, float &value, std::string which = CONFIG_CURRENT);

int WriteConfigString(std::string key, std::string value, std::string which = CONFIG_CURRENT);
int WriteConfigFloat(std::string key, float value, std::string which = CONFIG_CURRENT);
int WriteConfigU8(std::string key, __u8 value, std::string which = CONFIG_CURRENT);
int WriteConfigU32(std::string key, __u32 value, std::string which = CONFIG_CURRENT);

// void SaveConfig(TConfig config, std::string which = CONFIG_CURRENT);

// create /etc/opt/aioenet/ if missing.
// create /etc/opt/aioenet/config.factory/ if missing.
// create /etc/opt/aioenet/config.current/ if missing.
// create /etc/opt/aioenet/config.user/ if missing.
// create individual /etc/opt/aioenet/config.factory/foo.conf files for all config. fields that do not already have foo.conf.
// if any individual foo.conf were created copy it to config.current/foo.conf if missing there.
// if any individual foo.conf were created copy it to config.user/foo.conf if missing there.
void InitializeConfigFiles(TConfig &config); // should only run if /etc/opt/aioenet/config.factory/ is missing or empty
