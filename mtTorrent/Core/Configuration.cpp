#include "Configuration.h"
#include <map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include "utils/HexEncoding.h"
#include "AlertsManager.h"
#include "Api/Configuration.h"
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"

namespace mtt
{
	namespace config
	{
		External external;
		Internal internal_;

		bool dirty = true;
		std::mutex cbMutex;
		std::map<int, std::pair<ValueType, std::function<void()>>> callbacks;
		int registerCounter = 1;

		static void triggerChange(ValueType type)
		{
			dirty = true;
			AlertsManager::Get().configAlert(AlertId::ConfigChanged, type);

			std::lock_guard<std::mutex> guard(cbMutex);

			for (auto cb : callbacks)
			{
				if(cb.second.first == type)
					cb.second.second();
			}
		}

		const mtt::config::External& getExternal()
		{
			return external;
		}

		const mtt::config::Internal& getInternal()
		{
			return internal_;
		}

		void setValues(const External::Connection& val)
		{
			bool changed = val.tcpPort != external.connection.tcpPort;
			changed |= val.udpPort != external.connection.udpPort;
			changed |= val.maxTorrentConnections != external.connection.maxTorrentConnections;
			changed |= val.upnpPortMapping != external.connection.upnpPortMapping;

			if (changed)
			{
				external.connection = val;
				triggerChange(ValueType::Connection);
			}
		}

		void setValues(const External::Dht& val)
		{
			bool changed = val.enable != external.dht.enable;

			if (changed)
			{
				external.dht = val;
				triggerChange(ValueType::Dht);
			}
		}

		void setValues(const External::Files& val)
		{
			bool changed = val.defaultDirectory != external.files.defaultDirectory;

			if (changed)
			{
				external.files = val;
				triggerChange(ValueType::Files);
			}
		}

		API_EXPORT void setValues(const External::Transfer& val)
		{
			bool changed = val.maxDownloadSpeed != external.transfer.maxDownloadSpeed;
			changed |= val.maxUploadSpeed != external.transfer.maxUploadSpeed;

			if (changed)
			{
				external.transfer = val;
				triggerChange(ValueType::Transfer);
			}
		}

		int registerOnChangeCallback(ValueType v, std::function<void()> cb)
		{
			std::lock_guard<std::mutex> guard(cbMutex);
			callbacks[++registerCounter] = { v, cb };

			return registerCounter;
		}

		void unregisterOnChangeCallback(int id)
		{
			std::lock_guard<std::mutex> guard(cbMutex);
			auto it = callbacks.find(id);

			if (it != callbacks.end())
				callbacks.erase(it);
		}

		void load()
		{
			std::filesystem::path dir(internal_.stateFolder);
			if (!std::filesystem::exists(dir))
			{
				std::error_code ec;

				std::filesystem::create_directory(internal_.programFolderPath, ec);
				std::filesystem::create_directory(dir, ec);
			}

			std::ifstream file(mtt::config::getInternal().programFolderPath + "\\cfg", std::ios::binary);

			if (file)
			{
				std::string data((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());

				mtt::BencodeParser parser;
				if (!parser.parse((const uint8_t*)data.data(), data.size()))
					return;

				dirty = false;
				auto root = parser.getRoot();

				if (auto internalSettings = root->getDictItem("internal"))
				{
					auto id = internalSettings->getTxt("hashId");
					if (id.length() == 40)
						decodeHexa(id, internal_.hashId);

					if (auto i = internalSettings->getIntItem("maxPeersPerTrackerRequest"))
						internal_.maxPeersPerTrackerRequest = (uint32_t)i->getBigInt();

					if (auto dhtSettings = internalSettings->getDictItem("dht"))
					{
						if (auto i = dhtSettings->getIntItem("peersCheckInterval"))
							internal_.dht.peersCheckInterval = (uint32_t)i->getBigInt();
						if (auto i = dhtSettings->getIntItem("maxStoredAnnouncedPeers"))
							internal_.dht.maxStoredAnnouncedPeers = (uint32_t)i->getBigInt();
						if (auto i = dhtSettings->getIntItem("maxPeerValuesResponse"))
							internal_.dht.maxPeerValuesResponse = (uint32_t)i->getBigInt();

						if (auto rootHosts = dhtSettings->getListItem("defaultRootHosts"))
						{
							internal_.dht.defaultRootHosts.clear();
							for (auto& host : *rootHosts)
							{
								auto str = host.getTxt();
								size_t portStart = str.find_last_of(':');
								if (portStart != std::string::npos)
									internal_.dht.defaultRootHosts.push_back({ str.substr(0, portStart) , str.substr(portStart + 1) });
							}
						}
					}
				}

				if (auto externalSettings = root->getDictItem("external"))
				{
					if (auto cSettings = externalSettings->getDictItem("connection"))
					{
						if (auto i = cSettings->getIntItem("tcpPort"))
							external.connection.tcpPort = (uint16_t)i->getBigInt();
						if (auto i = cSettings->getIntItem("udpPort"))
							external.connection.udpPort = (uint16_t)i->getBigInt();
						if (auto i = cSettings->getIntItem("maxTorrentConnections"))
							external.connection.maxTorrentConnections = (uint32_t)i->getBigInt();
						if (auto i = cSettings->getIntItem("upnpPortMapping"))
							external.connection.upnpPortMapping = (bool)i->getBigInt();
					}
					if (auto dhtSettings = externalSettings->getDictItem("dht"))
					{
						if (auto i = dhtSettings->getIntItem("enabled"))
							external.dht.enable = (bool)i->getBigInt();
					}
					if (auto tSettings = externalSettings->getDictItem("transfer"))
					{
						if (auto i = tSettings->getIntItem("maxDownloadSpeed"))
							external.transfer.maxDownloadSpeed = (uint32_t)i->getBigInt();
						if (auto i = tSettings->getIntItem("maxUploadSpeed"))
							external.transfer.maxUploadSpeed = (uint32_t)i->getBigInt();
					}
					if (auto fSettings = externalSettings->getDictItem("files"))
					{
						if (auto i = fSettings->getTxtItem("directory"))
							external.files.defaultDirectory = std::string(i->data, i->size);
					}
				}
			}
		}

		void save()
		{
			if (!dirty)
				return;

			std::ofstream file(mtt::config::getInternal().programFolderPath + "\\cfg", std::ios::binary);

			if (file)
			{
				mtt::BencodeWriter writer;
				writer.startMap();

				{
					writer.startMap("internal");
					writer.addItem("hashId", hexToString(internal_.hashId, 20));
					writer.endMap();
				}

				{
					writer.startMap("external");

					writer.startMap("connection");
					writer.addItem("tcpPort", external.connection.tcpPort);
					writer.addItem("udpPort", external.connection.udpPort);
					writer.addItem("maxTorrentConnections", external.connection.maxTorrentConnections);
					writer.addItem("upnpPortMapping", external.connection.upnpPortMapping);
					writer.endMap();

					writer.startMap("dht");
					writer.addItem("enabled", (uint32_t)external.dht.enable);
					writer.endMap();

					writer.startMap("transfer");
					writer.addItem("maxDownloadSpeed", external.transfer.maxDownloadSpeed);
					writer.addItem("maxUploadSpeed", external.transfer.maxUploadSpeed);
					writer.endMap();

					writer.startMap("files");
					writer.addItem("directory", external.files.defaultDirectory);
					writer.endMap();

					writer.endMap();
				}

				writer.endMap();

				file << writer.data;
			}
		}

		External::External()
		{
			files.defaultDirectory = ".\\";
		}

		Internal::Internal()
		{
			programFolderPath = ".\\data\\";
			stateFolder = programFolderPath + "state";

			srand((int)::time(NULL));
			
			memcpy(hashId, MT_HASH_NAME, std::size(MT_HASH_NAME));

			static char const printable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz-_.!~*()";

			for (size_t i = std::size(MT_HASH_NAME) - 1; i < 20; i++)
			{
				hashId[i] = (uint8_t)printable[rand() % strlen(printable)];
			}
			trackerKey = (uint32_t)rand();

			dht.defaultRootHosts = { { "dht.transmissionbt.com", "6881" },{ "router.bittorrent.com" , "6881" } };
		}
	}
}
