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
			char hashId[20];
			uint32_t key;
		};

		struct Cfg
		{
			External external;
			Internal internal;
		}

		extern Cfg* get();
	}
}