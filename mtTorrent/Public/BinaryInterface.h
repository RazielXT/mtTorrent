#pragma once

#include <vector>
#include <string>
#include "Status.h"

namespace mtBI
{
	class allocator;

	struct string
	{
		string();
		~string();

		char* data = nullptr;

		string& operator=(const std::string& str);
		string& operator=(const char* str);

		void assign(const char* str, size_t length);

	private:
		const allocator * const allocator;
	};

	enum class MessageId
	{
		Init,
		Deinit,
		AddFromFile,	//char*, uint8_t[20]
		AddFromMetadata, //char*, uint8_t[20]
		Start,	//uint8_t[20], null
		Stop,	//uint8_t[20], null
		Remove,	//RemoveTorrentRequest, null
		GetTorrents,	//null,TorrentsList
		GetTorrentInfo,			//uint8_t[20], TorrentInfo
		GetTorrentStateInfo,	//uint8_t[20], TorrentStateInfo
		GetPeersInfo,	//uint8_t[20], TorrentPeersInfo
		GetSourcesInfo,	//uint8_t[20], SourcesInfo
		GetMagnetLinkProgress,	//uint8_t[20],MagnetLinkProgress
		GetMagnetLinkProgressLogs,	//uint8_t[20],MagnetLinkProgressLogs
		GetSettings, //null, SettingsInfo
		SetSettings, //SettingsInfo, null
		RefreshSource, //SourceId, null
		GetTorrentFilesSelection, //SourceId, TorrentFilesSelection
		SetTorrentFilesSelection, //TorrentFilesSelectionRequest, null
		AddPeer,	//AddPeerRequest, null
		GetPiecesProgress, //uint8_t[20], PiecesProgress
	};

	struct SourceId
	{
		uint8_t hash[20];
		string name;
	};

	struct SettingsInfo
	{
		uint32_t udpPort;
		uint32_t tcpPort;
		bool dhtEnabled;
		string directory;
		uint32_t maxConnections;
		bool upnpEnabled;
	};

	struct PiecesProgress
	{
		uint32_t bitfieldSize;
		uint32_t piecesCount;
		std::vector<uint8_t> bitfield;
	};

	struct MagnetLinkProgress
	{
		float progress;
		bool finished;
	};

	struct MagnetLinkProgressLogs
	{
		uint32_t count;
		uint32_t start;
		std::vector<string> logs;
	};

	struct RemoveTorrentRequest
	{
		uint8_t hash[20];
		bool deleteFiles;
	};

	struct FileSelection
	{
		string name;
		bool selected;
		size_t size;
	};

	struct TorrentFilesSelection
	{
		uint32_t count;
		std::vector<FileSelection> selection;
	};

	struct FileSelectionRequest
	{
		bool selected;
	};

	struct AddPeerRequest
	{
		uint8_t hash[20];
		string addr;
	};

	struct TorrentFilesSelectionRequest
	{
		uint8_t hash[20];
		std::vector<FileSelectionRequest> selection;
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
		float selectionProgress;
		size_t downloaded;
		size_t downloadSpeed;
		size_t uploaded;
		size_t uploadSpeed;
		uint32_t foundPeers;
		uint32_t connectedPeers;
		bool utmActive;
		bool checking;
		float checkingProgress;
		mtt::Status activeStatus;
	};

	struct PeerInfo
	{
		uint8_t id[20];
		float progress;
		size_t dlSpeed;
		size_t upSpeed;
		string addr;
		string client;
		string country;
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
