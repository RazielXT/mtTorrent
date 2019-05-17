#include "Configuration.h"
#include <map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"

namespace mtt
{
	namespace config
	{
		External external;
		Internal internal_;

		std::mutex cbMutex;
		std::map<int, std::pair<ValueType, std::function<void()>>> callbacks;
		int registerCounter = 1;

		static void triggerChange(ValueType type)
		{
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

		mtt::config::Internal& getInternal()
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

		static void setDefaultValues()
		{
			internal_.programFolderPath = ".\\data\\";
			internal_.stateFolder = "state";

			std::filesystem::path dir(internal_.programFolderPath + internal_.stateFolder);
			if (!std::filesystem::exists(dir))
			{
				std::error_code ec;

				std::filesystem::create_directory(internal_.programFolderPath, ec);
				std::filesystem::create_directory(dir, ec);
			}

			srand(0);
			for (size_t i = 0; i < 20; i++)
			{
				internal_.hashId[i] = (uint8_t)rand();
			}
			internal_.trackerKey = (uint32_t)rand();

			internal_.defaultRootHosts = { { "dht.transmissionbt.com", "6881" },{ "router.bittorrent.com" , "6881" } };

			external.files.defaultDirectory = "E:\\";
		}

		void load()
		{
			setDefaultValues();

			std::ifstream file(mtt::config::getInternal().programFolderPath + "cfg", std::ios::binary);

			if (file)
			{
				std::string data((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());

				BencodeParser parser;
				if (parser.parse((uint8_t*)data.data(), data.length()))
				{
					if (auto root = parser.getRoot())
					{
						if (auto internalSettings = root->getDictItem("internal"))
						{
							auto hash = internalSettings->getTxt("hashId");
							if (hash.length() == 20)
							{
								memcpy(internal_.hashId, hash.data(), 20);
							}
						}
						if (auto externalSettings = root->getDictItem("external"))
						{
							if (auto conn = externalSettings->getDictItem("connection"))
							{
								external.connection.tcpPort = conn->getValueOr("tcpPort", external.connection.tcpPort);
								external.connection.udpPort = conn->getValueOr("udpPort", external.connection.udpPort);
								external.connection.maxTorrentConnections = conn->getValueOr("maxConn", external.connection.maxTorrentConnections);
								external.connection.upnpPortMapping = conn->getValueOr("upnp", external.connection.upnpPortMapping);
							}
							if (auto dht = externalSettings->getDictItem("dht"))
							{
								external.dht.enable = dht->getValueOr("enabled", external.dht.enable);
							}
							if (auto files = externalSettings->getDictItem("files"))
							{
								external.files.defaultDirectory = files->getValueOr("directory", external.files.defaultDirectory);
							}
						}
					}
				}
			}
		}

		void save()
		{
			std::ofstream file(mtt::config::getInternal().programFolderPath + "cfg", std::ios::binary);

			if (file)
			{
				BencodeWriter writer;

				writer.startMap();

				{
					writer.startRawMapItem("8:internal");
					writer.addRawItemFromBuffer("6:hashId", (const char*)internal_.hashId, 20);
					writer.endMap();
				}

				{
					writer.startRawMapItem("8:external");

					writer.startRawMapItem("10:connection");
					writer.addRawItem("7:tcpPort", external.connection.tcpPort);
					writer.addRawItem("7:udpPort", external.connection.udpPort);
					writer.addRawItem("7:maxConn", external.connection.maxTorrentConnections);
					writer.addRawItem("4:upnp", external.connection.upnpPortMapping);
					writer.endMap();

					writer.startRawMapItem("3:dht");
					writer.addRawItem("7:enabled", external.dht.enable);
					writer.endMap();

					writer.startRawMapItem("5:files");
					writer.addRawItem("9:directory", external.files.defaultDirectory);
					writer.endMap();

					writer.endMap();
				}

				writer.endMap();

				file << writer.data;
			}
		}
	}
}
