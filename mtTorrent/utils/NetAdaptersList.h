#pragma once
#include <string>
#include <vector>

namespace NetAdapters
{
	struct NetAdapter
	{
		std::string name;
		std::string gateway;
		std::string clientIp;
	};

	std::vector<NetAdapter> getActiveNetAdapters();
};