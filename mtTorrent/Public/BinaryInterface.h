#pragma once

#include "Status.h"
#include "ModuleString.h"
#include <vector>

namespace mtBI
{
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
		GetPiecesInfo, //uint8_t[20], PiecesInfo
		GetUpnpInfo	//null, string
	};

	struct SourceId
	{
		uint8_t hash[20];
		mtt::string name;
	};

	struct SettingsInfo
	{
		uint32_t udpPort;
		uint32_t tcpPort;
		bool dhtEnabled;
		mtt::string directory;
		uint32_t maxConnections;
		bool upnpEnabled;
	};

	struct PiecesInfo
	{
		uint32_t piecesCount;

		uint32_t bitfieldSize;
		std::vector<uint8_t> bitfield;

		uint32_t requestSize;
		std::vector<uint32_t> requests;
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
		std::vector<mtt::string> logs;
	};

	struct RemoveTorrentRequest
	{
		uint8_t hash[20];
		bool deleteFiles;
	};

	struct FileSelection
	{
		mtt::string name;
		bool selected;
		size_t size;
		uint32_t pieceStart;
		uint32_t pieceEnd;
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
		mtt::string addr;
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
		std::vector<mtt::string> filenames;
		std::vector<size_t> filesizes;
		size_t fullsize;
		mtt::string name;
	};

	struct TorrentStateInfo
	{
		mtt::string name;
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
		mtt::string addr;
		mtt::string client;
		mtt::string country;
	};

	struct TorrentPeersInfo
	{
		uint32_t count;
		std::vector<PeerInfo> peers;
	};

	struct SourceInfo
	{
		mtt::string name;
		uint32_t peers;
		uint32_t seeds;
		uint32_t leechers;
		uint32_t nextCheck;
		uint32_t interval;
		enum : char { Stopped, Ready, Offline, Connecting, Announcing, Announced } status;
	};

	struct SourcesInfo
	{
		uint32_t count;
		std::vector<SourceInfo> sources;
	};
};
