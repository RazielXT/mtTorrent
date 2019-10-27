#pragma once
#include <string>
#include <vector>

#ifdef ASIO_STANDALONE
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT __declspec(dllimport)
#endif

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

			API_EXPORT std::string toJson() const;
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

			API_EXPORT void fromJson(const char* js);
		};

		const API_EXPORT External& getExternal();
		API_EXPORT Internal& getInternal();

		enum class ValueType { Connection, Dht, Files };
		API_EXPORT void setValues(const External::Connection& val);
		API_EXPORT void setValues(const External::Dht& val);
		API_EXPORT void setValues(const External::Files& val);
		API_EXPORT bool fromJson(const char* js);
	}
}
