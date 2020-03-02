#pragma once
#include <string>
#include <vector>
#include "Public\Status.h"
#include "Public\Alerts.h"
#include <atomic>

#ifdef ASIO_STANDALONE
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT __declspec(dllimport)
#endif

namespace mtt
{
	struct File
	{
		std::vector<std::string> path;
		size_t size;
		uint32_t startPieceIndex;
		uint32_t startPiecePos;
		uint32_t endPieceIndex;
		uint32_t endPiecePos;
	};

	struct PieceInfo
	{
		uint8_t hash[20];
	};

	struct PieceBlockInfo
	{
		uint32_t index;
		uint32_t begin;
		uint32_t length;
	};

	struct TorrentInfo
	{
		uint8_t hash[20];

		std::vector<File> files;
		std::string name;

		std::vector<PieceInfo> pieces;
		uint32_t pieceSize = 0;
		size_t expectedBitfieldSize = 0;
		size_t fullSize = 0;

		API_EXPORT std::vector<PieceBlockInfo> makePieceBlocksInfo(uint32_t idx);
		API_EXPORT PieceBlockInfo getPieceBlockInfo(uint32_t idx, uint32_t blockIdx);
		API_EXPORT uint32_t getPieceSize(uint32_t idx);
		API_EXPORT uint32_t getPieceBlocksCount(uint32_t idx);

		uint32_t lastPieceIndex = 0;
		uint32_t lastPieceSize = 0;
		uint32_t lastPieceLastBlockIndex = 0;
		uint32_t lastPieceLastBlockSize = 0;

		bool isPrivate = false;
	};

	struct TorrentFileInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;

		struct
		{
			std::string createdBy;
			int creationDate = 0;
		}
		about;

		API_EXPORT Status parseMagnetLink(std::string link);
		API_EXPORT std::string createTorrentFileData();
	};

	enum class Priority : uint8_t
	{
		Low = 10,
		Normal = 50,
		High = 90
	};

	struct FileSelectionInfo
	{
		bool selected;
		Priority priority;
		File info;
	};

	struct DownloadSelection
	{
		std::vector<FileSelectionInfo> files;
	};

	enum class TrackerState { Clear, Initialized, Offline, Alive, Connecting, Connected, Announcing, Announced, Reannouncing };

	struct TrackerInfo
	{
		std::string hostname;

		uint32_t peers = 0;
		uint32_t seeds = 0;
		uint32_t leechers = 0;
		uint32_t announceInterval = 0;
		uint32_t lastAnnounce = 0;
		uint32_t nextAnnounce = 0;

		TrackerState state = TrackerState::Clear;
	};

	struct MetadataDownloadState
	{
		bool finished = false;
		uint32_t receivedParts = 0;
		uint32_t partsCount = 0;
	};

	struct PiecesCheck
	{
		uint32_t piecesCount = 0;
		std::atomic<uint32_t> piecesChecked = 0;
		bool rejected = false;
		std::vector<uint8_t> pieces;
	};

	enum class PeerSource
	{
		Tracker,
		Pex,
		Dht,
		Manual,
		Remote
	};

	struct ActivePeerInfo
	{
		std::string address;
		std::string client;
		uint32_t uploadSpeed;
		uint32_t downloadSpeed;
		float percentage;
		std::string country;
	};

	struct AlertMessage
	{
		AlertId id;

		template <class T> T* getAs()
		{
			if ((int)id & (int)T::category) return static_cast<T*>(this);
			return nullptr;
		}
	};

	struct TorrentAlert : public AlertMessage
	{
		static const mtt::AlertCategory category = mtt::AlertCategory::Torrent;

		uint8_t hash[20];
	};

	struct MetadataAlert : public TorrentAlert
	{
		static const mtt::AlertCategory category = mtt::AlertCategory::Metadata;

		static const int alert_type = 1;
	};
}
