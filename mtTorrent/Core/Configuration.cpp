#include "Configuration.h"
#include "utils/HexEncoding.h"
#include "AlertsManager.h"
#include "Api/Configuration.h"
#include "utils/Filesystem.h"
#include "utils/Xml.h"
#include <map>
#include <mutex>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <Shlobj.h>
#pragma comment (lib, "Shell32.lib")
#pragma comment (lib, "ole32.lib")
#endif

std::string getDefaultDirectory()
{
#ifdef _WIN32
	// this will work for X86 and X64 if the matching 'bitness' client has been installed
	PWSTR pszPath = NULL;
	if (SHGetKnownFolderPath(FOLDERID_Downloads, KF_FLAG_CREATE, NULL, &pszPath) != S_OK)
		return "";

	std::wstring path = pszPath;
	CoTaskMemFree(pszPath);

	if (!path.empty())
	{
		int requiredSize = WideCharToMultiByte(CP_UTF8, 0, path.data(), (int)path.size(), NULL, 0, NULL, NULL);

		if (requiredSize > 0)
		{
			std::string utf8String;
			utf8String.resize(requiredSize);
			WideCharToMultiByte(CP_UTF8, 0, path.data(), (int)path.size(), utf8String.data(), requiredSize, NULL, NULL);
			utf8String += "\\mtTorrent\\";
			return utf8String;
		}
	}
#endif

	return std::filesystem::current_path().string();
}

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
			save();
			AlertsManager::Get().configAlert(Alerts::Id::ConfigChanged, type);

			std::lock_guard<std::mutex> guard(cbMutex);

			for (const auto& cb : callbacks)
			{
				if (cb.second.first == type)
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
			changed |= val.enableTcpIn != external.connection.enableTcpIn;
			changed |= val.enableTcpOut != external.connection.enableTcpOut;
			changed |= val.enableUtpIn != external.connection.enableUtpIn;
			changed |= val.enableUtpOut != external.connection.enableUtpOut;
			changed |= val.encryption != external.connection.encryption;

			if (changed)
			{
				external.connection = val;
				triggerChange(ValueType::Connection);
			}
		}

		void setValues(const External::Dht& val)
		{
			bool changed = val.enabled != external.dht.enabled;

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

		void setValues(const External::Transfer& val)
		{
			bool changed = val.maxDownloadSpeed != external.transfer.maxDownloadSpeed;
			changed |= val.maxUploadSpeed != external.transfer.maxUploadSpeed;

			if (changed)
			{
				external.transfer = val;
				triggerChange(ValueType::Transfer);
			}
		}

		void setValues(const External& val)
		{
			setValues(val.files);
			setValues(val.connection);
			setValues(val.dht);
			setValues(val.transfer);
		}

		void setInternalValues(const Internal& val)
		{
			internal_ = val;
			memcpy(internal_.hashId, val.hashId, 20);
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

			std::ifstream file(mtt::config::getInternal().programFolderPath + pathSeparator + "config.xml", std::ios::binary);

			if (file)
			{
				std::string data((std::istreambuf_iterator<char>(file)),
					std::istreambuf_iterator<char>());

				mtt::XmlParser::Document parser;
				if (parser.parse(data.data(), data.size()))
				{
					dirty = false;

					auto root = parser.getRoot();

					if (auto internalSettings = root->firstNode("internal"))
					{
						auto id = internalSettings->value("hashId");
						if (id.length() == 20)
							memcpy(internal_.hashId, id.data(), 20);
					}

					if (auto externalSettings = root->firstNode("external"))
					{
						if (auto cSettings = externalSettings->firstNode("connection"))
						{
							if (auto i = cSettings->firstNode("tcpPort"))
								external.connection.tcpPort = (uint16_t)i->valueNumber();
							if (auto i = cSettings->firstNode("udpPort"))
								external.connection.udpPort = (uint16_t)i->valueNumber();
							if (auto i = cSettings->firstNode("maxTorrentConnections"))
								external.connection.maxTorrentConnections = (uint32_t)i->valueNumber();
							if (auto i = cSettings->firstNode("upnpPortMapping"))
								external.connection.upnpPortMapping = (bool)i->valueNumber();
							if (auto i = cSettings->firstNode("enableTcpIn"))
								external.connection.enableTcpIn = (bool)i->valueNumber();
							if (auto i = cSettings->firstNode("enableTcpOut"))
								external.connection.enableTcpOut = (bool)i->valueNumber();
							if (auto i = cSettings->firstNode("enableUtpIn"))
								external.connection.enableUtpIn = (bool)i->valueNumber();
							if (auto i = cSettings->firstNode("enableUtpOut"))
								external.connection.enableUtpOut = (bool)i->valueNumber();
						}
						if (auto dhtSettings = externalSettings->firstNode("dht"))
						{
							if (auto i = dhtSettings->firstNode("enabled"))
								external.dht.enabled = (bool)i->valueNumber();
						}
						if (auto tSettings = externalSettings->firstNode("transfer"))
						{
							if (auto i = tSettings->firstNode("maxDownloadSpeed"))
								external.transfer.maxDownloadSpeed = (uint32_t)i->valueNumber();
							if (auto i = tSettings->firstNode("maxUploadSpeed"))
								external.transfer.maxUploadSpeed = (uint32_t)i->valueNumber();
						}
						if (auto fSettings = externalSettings->firstNode("files"))
						{
							if (auto i = fSettings->firstNode("directory"))
								external.files.defaultDirectory = i->value();
						}
					}
				}
			}

			if (external.files.defaultDirectory.empty())
				external.files.defaultDirectory = getDefaultDirectory();
		}

		void save()
		{
			if (!dirty)
				return;

			std::ofstream file(mtt::config::getInternal().programFolderPath + pathSeparator + "config.xml", std::ios::binary);

			if (file)
			{
				std::string buffer;
				mtt::XmlWriter::Element writer(buffer, "config");

				{
					auto e = writer.createChild("internal");
					e.addValueCData("hashId", std::string_view((const char*)internal_.hashId, 20));
					e.close();
				}

				{
					auto e = writer.createChild("external");

					auto c = e.createChild("connection");
					c.addValue("tcpPort", external.connection.tcpPort);
					c.addValue("udpPort", external.connection.udpPort);
					c.addValue("maxTorrentConnections", external.connection.maxTorrentConnections);
					c.addValue("upnpPortMapping", external.connection.upnpPortMapping);
					c.addValue("enableTcpIn", external.connection.enableTcpIn);
					c.addValue("enableTcpOut", external.connection.enableTcpOut);
					c.addValue("enableUtpIn", external.connection.enableUtpIn);
					c.addValue("enableUtpOut", external.connection.enableUtpOut);
					c.close();

					auto d = e.createChild("dht");
					d.addValue("enabled", external.dht.enabled);
					d.close();

					auto t = e.createChild("transfer");
					t.addValue("maxDownloadSpeed", external.transfer.maxDownloadSpeed);
					t.addValue("maxUploadSpeed", external.transfer.maxUploadSpeed);
					t.close();

					auto f = e.createChild("files");
					f.addValueCData("directory", external.files.defaultDirectory);
					f.close();

					e.close();
				}

				writer.close();

				file.write(buffer.data(), buffer.size());
			}
		}

		External::External()
		{
		}

		Internal::Internal()
		{
			programFolderPath = std::string(".") + pathSeparator + "data" + pathSeparator;
			stateFolder = programFolderPath + "state";

			memcpy(hashId, MT_HASH_NAME, std::size(MT_HASH_NAME));

			static char const printable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz";

			for (size_t i = std::size(MT_HASH_NAME) - 1; i < 20; i++)
			{
				hashId[i] = (uint8_t)printable[Random::Number() % std::size(printable)];
			}
			trackerKey = Random::Number();

			dht.defaultRootHosts = { { "dht.transmissionbt.com", "6881" },{ "router.bittorrent.com" , "6881" } };
		}
	}
}
