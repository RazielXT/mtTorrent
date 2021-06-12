#pragma once

#include "Status.h"
#include "Alerts.h"
#include "ModuleString.h"
#include "ModuleArray.h"

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
		GetMagnetLinkProgressLogs,	//MagnetLinkProgressLogsRequest,MagnetLinkProgressLogsResponse
		GetSettings, //null, SettingsInfo
		SetSettings, //SettingsInfo, null
		RefreshSource, //SourceId, null
		SetTorrentFilesSelection, //TorrentFilesSelectionRequest, null
		SetTorrentFileSelection, //TorrentFileSelectionRequest, null
		SetTorrentFilesPriority, //TorrentFilesPriorityRequest, null
		GetTorrentFilesProgress, //hash, TorrentFilesProgress
		SetTorrentPath, //TorrentSetPathRequest, null
		AddPeer,	//AddPeerRequest, null
		GetPiecesInfo, //uint8_t[20], PiecesInfo
		GetUpnpInfo,	//null, string
		RegisterAlerts,	//RegisterAlertsRequest, null
		PopAlerts,		//null, AlertsList
		CheckFiles,		//hash, null
		GetFilesAllocation,		//hash, FilesAllocation
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
		uint32_t maxDownloadSpeed;
		uint32_t maxUploadSpeed;
	};

	struct PiecesInfo
	{
		uint32_t piecesCount;
		uint32_t receivedCount;
		mtt::array<uint8_t> bitfield;
		mtt::array<uint32_t> requests;
	};

	struct MagnetLinkProgress
	{
		float progress;
		bool finished;
	};

	struct MagnetLinkProgressLogsRequest
	{
		uint32_t start;
		uint8_t hash[20];
	};

	struct MagnetLinkProgressLogsResponse
	{
		uint32_t fullcount;
		mtt::array<mtt::string> logs;
	};

	struct RemoveTorrentRequest
	{
		uint8_t hash[20];
		bool deleteFiles;
	};

	struct AddPeerRequest
	{
		uint8_t hash[20];
		mtt::string addr;
	};

	struct FileSelectionRequest
	{
		bool selected;
	};

	struct TorrentFilesSelectionRequest
	{
		uint8_t hash[20];
		mtt::array<FileSelectionRequest> selection;
	};

	struct TorrentFileSelectionRequest
	{
		uint8_t hash[20];
		uint32_t index;
		bool selected;
	};

	struct TorrentFilesPriorityRequest
	{
		uint8_t hash[20];
		mtt::array<uint8_t> priority;
	};

	struct TorrentsList
	{
		struct TorrentBasicInfo
		{
			uint8_t hash[20];
			bool active;
		};
		mtt::array<TorrentBasicInfo> list;
	};

	struct TorrentFile
	{
		mtt::string name;
		uint64_t size;
		bool selected;
		uint8_t priority;
	};

	struct TorrentInfo
	{
		mtt::array<TorrentFile> files;
		uint64_t fullsize;
		mtt::string name;
		mtt::string downloadLocation;
		mtt::string createdBy;
		int creationDate;
	};

	struct TorrentStateInfo
	{
		mtt::string name;
		uint64_t timeAdded;
		float progress;
		float selectionProgress;
		uint64_t downloaded;
		uint32_t downloadSpeed;
		uint64_t uploaded;
		uint32_t uploadSpeed;
		uint32_t foundPeers;
		uint32_t connectedPeers;
		bool started;
		bool stopping;
		bool utmActive;
		bool checking;
		float checkingProgress;
		mtt::Status activeStatus;
	};

	struct PeerInfo
	{
		float progress;
		bool connected;
		bool choking;
		bool requesting;
		uint32_t dlSpeed;
		uint32_t upSpeed;
		mtt::string addr;
		mtt::string client;
		mtt::string country;
	};

	struct TorrentPeersInfo
	{
		mtt::array<PeerInfo> peers;
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
		mtt::array<SourceInfo> sources;
	};

	struct TorrentSetPathRequest
	{
		uint8_t hash[20];
		mtt::string path;
		bool moveFiles;
	};

	struct RegisterAlertsRequest
	{
		uint32_t categoryMask;
	};

	struct Alert
	{
		mtt::AlertId id;
		uint8_t hash[20];
		int type = 0;
	};

	struct AlertsList
	{
		mtt::array<Alert> alerts;
	};

	struct TorrentFileProgress
	{
		float progress;
		uint32_t receivedPieces;
		uint32_t pieceStart;
		uint32_t pieceEnd;
	};

	struct TorrentFilesProgress
	{
		mtt::array<TorrentFileProgress> files;
	};

	struct FilesAllocation
	{
		mtt::array<uint64_t> files;
	};
};
