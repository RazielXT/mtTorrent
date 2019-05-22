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
			External();

			struct Connection
			{
				uint16_t tcpPort = 55125;
				uint16_t udpPort = 55125;

				uint32_t maxTorrentConnections = 50;

				bool upnpPortMapping = false;
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

			std::string toJson() const;
		};

		struct Internal
		{
			Internal();

			uint8_t hashId[20];

			uint32_t trackerKey;
			uint32_t maxPeersPerTrackerRequest = 100;

			struct
			{
				std::vector<std::pair<std::string, std::string>> defaultRootHosts;
				uint32_t peersCheckInterval = 60;

				uint32_t maxStoredAnnouncedPeers = 32;
				uint32_t maxPeerValuesResponse = 32;
			}
			dht;

			std::string programFolderPath;
			std::string stateFolder;

			void fromJson(const char* js);
		};

		const External& getExternal();
		Internal& getInternal();

		enum class ValueType { Connection, Dht, Files };
		void setValues(const External::Connection& val);
		void setValues(const External::Dht& val);
		void setValues(const External::Files& val);
		bool fromJson(const char* js);

		int registerOnChangeCallback(ValueType, std::function<void()>);
		void unregisterOnChangeCallback(int);

		void load();
		void save();
	}
}