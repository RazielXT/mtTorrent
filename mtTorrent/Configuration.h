#pragma once
#include <string>

namespace mtt
{
	namespace config
	{
		struct External
		{
			uint32_t listenPort = 80;
			uint32_t maxPeersPerRequest = 100;

			std::string defaultDirectory;
		};

		struct Internal
		{
			uint8_t hashId[20];
			uint32_t key;
		};

		extern External external;
		extern Internal internal;
	}
}