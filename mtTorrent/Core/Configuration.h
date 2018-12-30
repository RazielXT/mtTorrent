#pragma once
#include <string>
#include <vector>

namespace mtt
{
	namespace config
	{
		struct External
		{
			uint16_t tcpPort;
			uint16_t udpPort;

			std::string defaultDirectory;
			bool enableDht = false;

			uint32_t maxTorrentConnections = 20;
		};

		struct Internal
		{
			uint8_t hashId[20];
			uint32_t trackerKey;

			uint32_t maxPeersPerTrackerRequest = 100;
			std::vector<std::pair<std::string, std::string>> defaultRootHosts;

			struct
			{
				uint32_t maxStoredAnnouncedPeers = 32;
				uint32_t maxPeerValuesResponse = 32;
			}
			dht;
		};

		extern External external;
		extern Internal internal_;
	}
}