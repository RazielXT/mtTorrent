#pragma once
#include <string>
#include <vector>
#include "Network.h"
#include "Logging.h"

#define MT_NAME "mtTorrent 0.5"
#define MT_HASH_NAME "MT-0-5-"

namespace mtt
{
	struct File
	{
		int id;
		std::vector<std::string> path;
		size_t size;
		uint32_t startPieceIndex;
		size_t startPiecePos;
		uint32_t endPieceIndex;
		size_t endPiecePos;
	};

	struct PieceInfo
	{
		uint8_t hash[20];
	};

	struct TorrentInfo
	{
		uint8_t hash[20];

		std::vector<PieceInfo> pieces;
		size_t pieceSize;
		size_t expectedBitfieldSize;

		std::vector<File> files;
		std::string directory;
		size_t fullSize;
	};

	struct TorrentFileInfo
	{
		std::string announce;
		std::vector<std::string> announceList;

		TorrentInfo info;

		bool parseMagnetLink(std::string link);
	};

	struct SelectedFile
	{
		File file;
	};

	struct DownloadSelection
	{
		std::vector<SelectedFile> files;
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