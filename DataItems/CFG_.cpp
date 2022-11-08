#include <fstream>
#include <iostream>
#include <exception>

#include "CFG_.h"

TCFG_Hostname::TCFG_Hostname(TBytes buf)
{
	if (buf.size() >= 64) throw std::logic_error("invalid hostname");
	std::string name(buf.begin(), buf.end());

	// if > 0
	// 	convert to lowercase
	// 	confirm buf is a valid hostname string; a-z and 0-9 plus "-" only; cannot start with -, cannot be >63 characters
	// 	save to this->hostname
}

TBytes TCFG_Hostname::calcPayload(bool bAsReply)
{
	TBytes bytes;
	stuff(bytes, this->hostname);
	return bytes;
}

std::string TCFG_Hostname::AsString(bool bAsReply)
{
	return "CFG_Hostname() = "+this->hostname;
}

TCFG_Hostname &TCFG_Hostname::Go()
{
	// exit if current /etc/hostname is the same as this->hostname
	if (Config.Hostname != this->hostname)
	{
		// write this->hostname to /etc/hostname
		std::ofstream out("/etc/hostname");
		out<<this->hostname;
		out.close();
	}
	return *this;
}