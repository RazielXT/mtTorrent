#pragma once

#include "Api\Interface.h"
#include "utils\Network.h"
#include "Public\Status.h"
#include "Logging.h"

#define MT_NAME "mtTorrent 0.9"
#define MT_HASH_NAME "MT-0-9-"
#define MT_NAME_SHORT "mt09"

const uint32_t BlockRequestMaxSize = 16 * 1024;

namespace mtt
{
	class Torrent;
	using TorrentPtr = std::shared_ptr<Torrent>;

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
}
