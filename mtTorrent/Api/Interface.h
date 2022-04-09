#pragma once

#include "../Public/Status.h"
#include "../Public/Alerts.h"
#include "Configuration.h"
#include <string>
#include <vector>
#include <cstring>
#include <memory>

namespace mttApi
{
	class Torrent;
	using TorrentPtr = std::shared_ptr<Torrent>;
}

namespace mtt
{
	struct File
	{
		std::vector<std::string> path;
		uint64_t size;
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
		uint64_t fullSize = 0;

		API_EXPORT std::vector<PieceBlockInfo> makePieceBlocksInfo(uint32_t idx) const;
		API_EXPORT PieceBlockInfo getPieceBlockInfo(uint32_t idx, uint32_t blockIdx) const;
		API_EXPORT uint32_t getPieceSize(uint32_t idx) const;
		API_EXPORT uint32_t getPieceBlocksCount(uint32_t idx) const;

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
			uint64_t creationDate = 0;
		}
		about;

		API_EXPORT Status parseMagnetLink(std::string link);
		API_EXPORT std::string createTorrentFileData(const uint8_t* info = nullptr, size_t infoSize = 0);
	};

	enum class Priority : uint8_t
	{
		Low = 10,
		Normal = 50,
		High = 90
	};

	struct FileSelection
	{
		bool selected;
		Priority priority;
	};

	enum class TrackerState { Clear, Initialized, Offline, Alive, Connecting, Connected, Announcing, Announced, Reannouncing };

	struct TrackerInfo
	{
		std::string hostname;
		std::string port;
		std::string path;

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
		bool active = false;
		bool finished = false;
		uint32_t receivedParts = 0;
		uint32_t partsCount = 0;	//0 until first peer connection
	};

	namespace PeerFlags
	{
		enum Flag
		{
			Tcp = 1,
			Utp = 2,
			RemoteConnection = 4,
			Encrypted = 8
		};
	}

	struct ActivePeerInfo
	{
		std::string address;
		std::string client;
		uint32_t uploadSpeed = 0;
		uint32_t downloadSpeed = 0;
		float percentage = 0;
		bool connected = false;
		bool choking = false;
		bool requesting = false;
		std::string country;
		uint32_t flags = 0;
	};

	struct AlertMessage
	{
		AlertId id;

		template <class T> const T* getAs() const
		{
			if ((int)id & (int)T::category) return static_cast<const T*>(this);
			return nullptr;
		}
	};

	struct TorrentAlert : public AlertMessage
	{
		static const mtt::AlertCategory category = mtt::AlertCategory::Torrent;

		mttApi::TorrentPtr torrent;
	};

	struct MetadataAlert : public TorrentAlert
	{
		static const mtt::AlertCategory category = mtt::AlertCategory::Metadata;
	};

	struct ConfigAlert : public AlertMessage
	{
		static const mtt::AlertCategory category = mtt::AlertCategory::Config;

		mtt::config::ValueType configType;
	};
}
