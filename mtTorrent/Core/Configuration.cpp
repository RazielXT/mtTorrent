#include "Configuration.h"
#include <map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "utils/HexEncoding.h"

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

		static void fromJson(rapidjson::Value& externalSettings)
		{
			auto conn = externalSettings.FindMember("connection");
			if (conn != externalSettings.MemberEnd())
			{
				if (conn->value.HasMember("tcpPort"))
					external.connection.tcpPort = (uint16_t)conn->value["tcpPort"].GetUint();
				if (conn->value.HasMember("udpPort"))
					external.connection.udpPort = (uint16_t)conn->value["udpPort"].GetUint();
				if (conn->value.HasMember("maxConn"))
					external.connection.maxTorrentConnections = conn->value["maxConn"].GetUint();
				if (conn->value.HasMember("upnp"))
					external.connection.upnpPortMapping = conn->value["upnp"].GetBool();
			}

			auto dht = externalSettings.FindMember("dht");
			if (dht != externalSettings.MemberEnd())
			{
				if (dht->value.HasMember("enabled"))
					external.dht.enable = dht->value["enabled"].GetBool();
			}

			auto files = externalSettings.FindMember("files");
			if (files != externalSettings.MemberEnd())
			{
				if (files->value.HasMember("directory"))
					external.files.defaultDirectory = files->value["directory"].GetString();
			}
		}

		bool fromJson(const char* js)
		{
			rapidjson::Document doc;
			doc.Parse(js);

			if(doc.IsObject())
				fromJson(doc);

			return doc.IsObject();
		}

		void fromInternalJson(rapidjson::Value& internalSettings)
		{
			auto item = internalSettings.FindMember("hashId");
			if (item != internalSettings.MemberEnd() && item->value.GetStringLength() == 40)
				decodeHexa(item->value.GetString(), internal_.hashId);

			item = internalSettings.FindMember("maxPeersPerTrackerRequest");
			if (item != internalSettings.MemberEnd())
				internal_.maxPeersPerTrackerRequest = item->value.GetUint();

			auto dhtSettings = internalSettings.FindMember("dht");
			if (dhtSettings != internalSettings.MemberEnd())
			{
				item = dhtSettings->value.FindMember("peersCheckInterval");
				if (item != dhtSettings->value.MemberEnd())
					internal_.dht.peersCheckInterval = item->value.GetUint();

				item = dhtSettings->value.FindMember("maxStoredAnnouncedPeers");
				if (item != dhtSettings->value.MemberEnd())
					internal_.dht.maxStoredAnnouncedPeers = item->value.GetUint();

				item = dhtSettings->value.FindMember("maxPeerValuesResponse");
				if (item != dhtSettings->value.MemberEnd())
					internal_.dht.maxPeerValuesResponse = item->value.GetUint();

				auto rootHosts = dhtSettings->value.FindMember("defaultRootHosts");
				if (rootHosts != dhtSettings->value.MemberEnd() && rootHosts->value.IsArray())
				{
					internal_.dht.defaultRootHosts.clear();
					for (auto& host : rootHosts->value.GetArray())
					{
						size_t portStart = host.GetStringLength();
						auto str = host.GetString();
						while (portStart > 0)
							if (str[portStart] == ':')
								break;
							else
								portStart--;

						if(portStart && portStart < host.GetStringLength())
							internal_.dht.defaultRootHosts.push_back({ std::string(str, portStart) , std::string(str + portStart + 1) });
					}
				}
			}

			item = internalSettings.FindMember("programFolderPath");
			if (item != internalSettings.MemberEnd())
				internal_.programFolderPath = item->value.GetString();

			item = internalSettings.FindMember("stateFolder");
			if (item != internalSettings.MemberEnd())
				internal_.stateFolder = item->value.GetString();
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

				rapidjson::Document doc;
				doc.Parse(data.data(), data.length());

				if (!doc.IsObject())
					return;

				auto internalSettings = doc.FindMember("internal");
				if (internalSettings != doc.MemberEnd())
				{
					fromInternalJson(internalSettings->value);
				}

				auto externalSettings = doc.FindMember("external");
				if (externalSettings != doc.MemberEnd())
				{
					fromJson(externalSettings->value);
				}
			}
		}

		void save()
		{
			std::ofstream file(mtt::config::getInternal().programFolderPath + "\\cfg", std::ios::binary);

			if (file)
			{
				rapidjson::StringBuffer s;
				rapidjson::Writer<rapidjson::StringBuffer> writer(s);

				writer.StartObject();

				writer.Key("internal");
				{
					writer.StartObject();
					writer.Key("hashId"); writer.String(hexToString(internal_.hashId, 20).data());
					writer.EndObject();
				}

				writer.Key("external");
				{
					auto extJson = external.toJson();
					writer.RawValue(extJson.data(), extJson.length(), rapidjson::Type::kObjectType);
				}

				writer.EndObject();

				file << s.GetString();
			}
		}

		External::External()
		{
			files.defaultDirectory = "E:\\";
		}

		std::string External::toJson() const
		{
			rapidjson::StringBuffer s;
			rapidjson::Writer<rapidjson::StringBuffer> writer(s);

			writer.StartObject();

			writer.Key("connection");
			{
				writer.StartObject();
				writer.Key("tcpPort"); writer.Uint(connection.tcpPort);
				writer.Key("udpPort"); writer.Uint(connection.udpPort);
				writer.Key("maxConn"); writer.Uint(connection.maxTorrentConnections);
				writer.Key("upnp"); writer.Bool(connection.upnpPortMapping);
				writer.EndObject();
			}

			writer.Key("dht");
			{
				writer.StartObject();
				writer.Key("enabled"); writer.Bool(dht.enable);
				writer.EndObject();
			}

			writer.Key("files");
			{
				writer.StartObject();
				writer.Key("directory"); writer.String(files.defaultDirectory.data());
				writer.EndObject();
			}

			writer.EndObject();

			return s.GetString();
		}

		Internal::Internal()
		{
			programFolderPath = ".\\data\\";
			stateFolder = programFolderPath + "state";

			srand(0);
			for (size_t i = 0; i < 20; i++)
			{
				hashId[i] = (uint8_t)rand();
			}
			trackerKey = (uint32_t)rand();

			dht.defaultRootHosts = { { "dht.transmissionbt.com", "6881" },{ "router.bittorrent.com" , "6881" } };
		}

		void Internal::fromJson(const char* js)
		{

		}

	}
}
