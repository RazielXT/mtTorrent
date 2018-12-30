#pragma once
#include <string>
#include <vector>
#include <atomic>
#include "utils\Network.h"
#include "Status.h"
#include "Logging.h"

#define MT_NAME "mtTorrent 0.5"
#define MT_HASH_NAME "MT-0-5-"
#define MT_NAME_SHORT "mt05"

const uint32_t BlockRequestMaxSize = 16 * 1024;

namespace mtt
{
	class Torrent;
	using TorrentPtr = std::shared_ptr<Torrent>;

	struct File
	{
		int id;
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

		std::vector<PieceBlockInfo> makePieceBlocksInfo(uint32_t idx);
		PieceBlockInfo getPieceBlockInfo(uint32_t idx, uint32_t blockIdx);
		uint32_t getPieceSize(uint32_t idx);
		uint32_t getPieceBlocksCount(uint32_t idx);

		uint32_t lastPieceIndex = 0;
		uint32_t lastPieceSize = 0;
		uint32_t lastPieceLastBlockIndex = 0;
		uint32_t lastPieceLastBlockSize = 0;
	};

	struct TorrentFileInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;

		Status parseMagnetLink(std::string link);
		DataBuffer createTorrentFileData();
	};

	struct FileSelectionInfo
	{
		bool selected;
		File info;
	};

	struct DownloadSelection
	{
		std::vector<FileSelectionInfo> files;
	};

	struct PieceBlock
	{
		PieceBlockInfo info;
		DataBuffer data;
	};

	struct PieceDownloadInfo
	{
		std::vector<PieceBlockInfo> blocksLeft;
		size_t blocksCount = 0;
		uint32_t index;
	};

	struct DownloadedPiece
	{
		DataBuffer data;
		uint32_t index = -1;
		uint32_t remainingBlocks = 0;
		std::vector<uint8_t> blocksTodo;

		void init(uint32_t idx, uint32_t pieceSize, uint32_t blocksCount);
		void addBlock(PieceBlock& block);
		bool isValid(const uint8_t* expectedHash);
	};

	struct AnnounceResponse
	{
		uint32_t interval = 5 * 60;

		uint32_t leechCount = 0;
		uint32_t seedCount = 0;

		std::vector<Addr> peers;
	};

	enum TrackerState { Clear, Initialized, Alive, Connecting, Connected, Announcing, Announced, Reannouncing };

	struct TrackerInfo
	{
		std::string hostname;

		uint32_t peers = 0;
		uint32_t seeds = 0;
		uint32_t leechers = 0;
		uint32_t announceInterval = 0;
		uint32_t lastAnnounce = 0;

		TrackerState state = Clear;
	};

	struct MetadataDownloadState
	{
		bool finished = false;
		uint32_t receivedParts = 0;
		uint32_t partsCount = 0;
		uint8_t source[20];
	};

	struct PiecesCheck
	{
		uint32_t piecesCount = 0;
		std::atomic<uint32_t> piecesChecked = 0;
		bool rejected = false;
		std::vector<uint8_t> pieces;
	};
}