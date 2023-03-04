#pragma once

#include <string>
#include <vector>

#if defined(_WIN32) && defined(MTT_IS_DLL)
#	ifdef ASIO_STANDALONE
#		define API_EXPORT __declspec(dllexport)
#	else
#		define API_EXPORT __declspec(dllimport)
#	endif
#else
#  define API_EXPORT
#endif

namespace mtt
{
	namespace config
	{
		enum class Encryption { Refuse, Allow, Require };

		/*
			External settings available usually to user
		*/
		struct External
		{
			External();

			struct Connection
			{
				uint16_t tcpPort = 55125;
				uint16_t udpPort = 55125;

				uint32_t maxTorrentConnections = 50;

				bool upnpPortMapping = false;

				bool enableTcpIn = true;
				bool enableTcpOut = true;
				bool enableUtpIn = true;
				bool enableUtpOut = true;

				bool preferUtp = false;

				Encryption encryption = Encryption::Allow;
			}
			connection;

			struct Dht
			{
				bool enabled = true;
			}
			dht;

			struct Transfer
			{
				uint32_t maxDownloadSpeed = 0;
				uint32_t maxUploadSpeed = 0;
			}
			transfer;

			struct Files
			{
				std::string defaultDirectory;
			}
			files;
		};

		/*
			Internal settings used by mtTorrent library
		*/
		struct Internal
		{
			Internal();

			uint8_t hashId[20];

			uint32_t trackerKey;
			uint32_t maxPeersPerTrackerRequest = 200;

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

			uint32_t bandwidthUpdatePeriodMs = 250;

			uint32_t downloadCachePerGroup = 2 * 1024 * 1024;
		};

		const API_EXPORT External& getExternal();
		const API_EXPORT Internal& getInternal();

		enum class ValueType { Connection, Dht, Files, Transfer };
		API_EXPORT void setValues(const External& val);
		API_EXPORT void setValues(const External::Connection& val);
		API_EXPORT void setValues(const External::Dht& val);
		API_EXPORT void setValues(const External::Files& val);
		API_EXPORT void setValues(const External::Transfer& val);

		API_EXPORT void setInternalValues(const Internal& val);
	}
}
