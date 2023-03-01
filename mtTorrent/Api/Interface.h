#pragma once

#include "../Public/Status.h"
#include "../Public/Alerts.h"
#include "../utils/DataBuffer.h"
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
	using Timestamp = uint32_t;

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

		DataBuffer data;
	};

	struct TorrentFileMetadata
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;

		struct
		{
			std::string createdBy;
			Timestamp creationDate = 0;
		}
		about;

		API_EXPORT Status parseMagnetLink(std::string link);
		API_EXPORT DataBuffer createTorrentFileData();
	};

	using Priority = uint8_t;
	constexpr Priority PriorityLow = 10;
	constexpr Priority PriorityNormal = 50;
	constexpr Priority PriorityHigh = 90;

	struct FileSelection
	{
		bool selected;
		Priority priority;
	};

	enum class TrackerState { Clear, Initialized, Offline, Alive, Connecting, Connected, Announcing, Announced };

	struct TrackerInfo
	{
		std::string hostname;
		std::string port;
		std::string path;

		uint32_t peers = 0;
		uint32_t seeds = 0;
		uint32_t leeches = 0;
		uint32_t announceInterval = 0;
		Timestamp lastAnnounce = 0;
		Timestamp nextAnnounce = 0;

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
			Encrypted = 8,
			Holepunch = 16
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

	struct PathValidation
	{
		mtt::Status status = mtt::Status::E_InvalidPath;

		uint64_t fullSpace = 0;
		uint64_t availableSpace = 0;

		uint64_t requiredSize = 0;
		uint64_t missingSize = 0;	//not yet allocated
	};

	struct AlertMessage
	{
		virtual ~AlertMessage() = default;

		Alerts::Id id;

		template <class T> const T* getAs() const
		{
			if (id & T::category) return static_cast<const T*>(this);
			return nullptr;
		}
	};

	struct TorrentAlert : public AlertMessage
	{
		static const mtt::Alerts::Category category = mtt::Alerts::Category::Torrent;

		mttApi::TorrentPtr torrent;
	};

	struct MetadataAlert : public AlertMessage
	{
		static const mtt::Alerts::Category category = mtt::Alerts::Category::Metadata;

		mttApi::TorrentPtr torrent;
	};

	struct ConfigAlert : public AlertMessage
	{
		static const mtt::Alerts::Category category = mtt::Alerts::Category::Config;

		mtt::config::ValueType configType;
	};
}
