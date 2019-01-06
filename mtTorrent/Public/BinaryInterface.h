#pragma once

#include <vector>

namespace mtBI
{
	struct string
	{
		string();
		~string();

		void set(const std::string& str);
		void add(const std::string& str);
		char* data = nullptr;

		struct basic_allocator
		{
			void* alloc(size_t size);
			void dealloc(void* data);
		};
	private:
		basic_allocator* allocator;
	};

	enum class MessageId
	{
		Init,
		AddFromFile,	//char*, uint8_t[20]
		AddFromMetadata, //char*, uint8_t[20]
		Start,	//uint8_t[20], null
		Stop,	//uint8_t[20], null
		GetTorrents,	//null,TorrentsList
		GetTorrentInfo,			//uint8_t[20], TorrentInfo
		GetTorrentStateInfo,	//uint8_t[20], TorrentStateInfo
		GetPeersInfo,	//uint8_t[20], TorrentPeersInfo
		GetSourcesInfo,	//uint8_t[20], SourcesInfo
		GetMagnetLinkProgress,	//uint8_t[20],MagnetLinkProgress
		GetSettings, //null, SettingsInfo
		SetSettings //SettingsInfo, null
	};

	struct SettingsInfo
	{
		uint32_t udpPort;
		uint32_t tcpPort;
		bool dhtEnabled;
		string directory;
		uint32_t maxConnections;
	};

	struct MagnetLinkProgress
	{
		float progress;
		bool finished;
	};

	struct TorrentsList
	{
		uint32_t count;

		struct TorrentBasicInfo
		{
			uint8_t hash[20];
			bool active;
		};
		std::vector<TorrentBasicInfo> list;
	};

	struct TorrentInfo
	{
		uint32_t filesCount;
		std::vector<string> filenames;
		std::vector<size_t> filesizes;
		size_t fullsize;
		string name;
	};

	struct TorrentStateInfo
	{
		string name;
		float progress;
		size_t downloaded;
		size_t downloadSpeed;
		size_t uploaded;
		size_t uploadSpeed;
		uint32_t foundPeers;
		uint32_t connectedPeers;
		bool checking;
		float checkingProgress;
	};

	struct PeerInfo
	{
		uint8_t id[20];
		float progress;
		size_t dlSpeed;
		size_t upSpeed;
		char source[10];
		string addr;
	};

	struct TorrentPeersInfo
	{
		uint32_t count;
		std::vector<PeerInfo> peers;
	};

	struct SourceInfo
	{
		string name;
		char status[11];
		uint32_t peers;
		uint32_t seeds;
		uint32_t nextCheck;
		uint32_t interval;
	};

	struct SourcesInfo
	{
		uint32_t count;
		std::vector<SourceInfo> sources;
	};
};
