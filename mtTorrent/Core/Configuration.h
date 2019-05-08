#pragma once
#include <string>
#include <vector>
#include <functional>

namespace mtt
{
	namespace config
	{
		struct External
		{
			struct Connection
			{
				uint16_t tcpPort;
				uint16_t udpPort;

				uint32_t maxTorrentConnections = 30;
			}
			connection;

			struct Dht
			{
				bool enable = false;
			}
			dht;

			struct Files
			{
				std::string defaultDirectory;
			}
			files;
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

			uint32_t dhtPeersCheckInterval = 60;
			std::string programFolderPath;
			std::string stateFolder;
		};

		const External& getExternal();
		Internal& getInternal();

		enum class ValueType { Connection, Dht, Files };
		void setValues(const External::Connection& val);
		void setValues(const External::Dht& val);
		void setValues(const External::Files& val);

		int registerOnChangeCallback(ValueType, std::function<void(ValueType)>);
		void unregisterOnChangeCallback(int);

		void load();
		void save();
	}
}