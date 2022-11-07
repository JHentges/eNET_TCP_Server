#pragma once
#define CONFIG_PATH "/etc/opt/aioenet/"
#define CONFIG_CURRENT "config.current"

int WriteConfigSetting(std::string key, std::string value, std::string file = CONFIG_CURRENT);

int ReadConfigSetting(std::string key, std::string &value, std::string file = CONFIG_CURRENT);

int ReadConfigU8(std::string key, __u8 &value, std::string file = CONFIG_CURRENT);

int ReadConfigU32(std::string key, __u32 &value, std::string file = CONFIG_CURRENT);

void LoadConfig();