#pragma once
#include <string>

namespace mtt
{
	namespace config
	{
		struct External
		{
			uint32_t listenPort = 55125;
			uint32_t listenPortUdp = 55126;

			std::string defaultDirectory;
			bool enableDht = false;
		};

		struct Internal
		{
			uint8_t hashId[20];
			uint32_t key = 1111;

			uint32_t maxPeersPerTrackerRequest = 100;
		};

		extern External external;
		extern Internal internal;
	}
}