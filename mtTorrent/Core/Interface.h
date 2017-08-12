#pragma once
#include <string>
#include <vector>
#include "utils\Network.h"
#include "Logging.h"

#define MT_NAME "mtTorrent 0.5"
#define MT_HASH_NAME "MT-0-5-"
#define MT_NAME_SHORT "mt05"

namespace mtt
{
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

	struct TorrentInfo
	{
		uint8_t hash[20];

		std::vector<PieceInfo> pieces;
		uint32_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string name;
		size_t fullSize;
	};

	struct TorrentFileInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;

		bool parseMagnetLink(std::string link);
	};

	struct LoadedTorrent
	{
		TorrentInfo info;
	};

	using TorrentPtr = std::shared_ptr<LoadedTorrent>;

	struct FileSelectionInfo
	{
		bool selected;
		File info;
	};

	struct DownloadSelection
	{
		std::vector<FileSelectionInfo> files;
	};

	struct PieceBlockInfo
	{
		uint32_t index;
		uint32_t begin;
		uint32_t length;
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
		size_t dataSize = 0;

		bool isValid(char* expectedHash);
		void reset(size_t maxPieceSize);
		void addBlock(PieceBlock& block);
		size_t receivedBlocks = 0;
	};

	struct AnnounceResponse
	{
		uint32_t interval = 5 * 60;

		uint32_t leechCount = 0;
		uint32_t seedCount = 0;

		std::vector<Addr> peers;
	};
}