#pragma once
#include <string>

struct Uri
{
	std::string path;
	std::string protocol;
	std::string host;
	std::string port;

	static Uri Parse(const std::string& uri);
};
