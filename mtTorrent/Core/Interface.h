#pragma once

#include "Api/Interface.h"
#include "utils/Network.h"
#include "Public/Status.h"
#include "Logging.h"
#include <atomic>

#define MT_NAME "mtTorrent 0.9"
#define MT_HASH_NAME "-mt0900-"
#define MT_NAME_SHORT "mt09"

const uint32_t BlockMaxSize = 16 * 1024;

namespace mtt
{
	class Torrent;
	using TorrentPtr = std::shared_ptr<Torrent>;

	struct PiecesProgress;

	struct PiecesCheck
	{
		PiecesCheck(PiecesProgress& p) : progress(p) {}

		PiecesProgress& progress;
		uint32_t piecesCount = 0;
		uint32_t piecesChecked = 0;
		bool rejected = false;
	};

	enum class PeerSource
	{
		Tracker,
		Pex,
		Dht,
		Manual,
		Remote
	};

	struct PieceBlock
	{
		PieceBlockInfo info;
		BufferView buffer;
	};

	struct PieceState
	{
		std::vector<uint8_t> blocksState;
		uint32_t remainingBlocks = 0;
		uint32_t downloadedSize = 0;
		uint32_t index = {};

		void init(uint32_t idx, uint32_t blocksCount);
		bool addBlock(const PieceBlock& block);
	};

	using DownloadSelection = std::vector<FileSelection>;

	struct SelectedFiles
	{
		SelectedFiles(const mtt::TorrentInfo info, const mtt::DownloadSelection& selection);
		SelectedFiles(const mtt::TorrentInfo info, uint32_t idx);

		struct Selection
		{
			uint32_t idx;
			uint64_t size;
		};
		std::vector<Selection> files;
	};

	struct AnnounceResponse
	{
		uint32_t interval = 5 * 60;

		uint32_t leechCount = 0;
		uint32_t seedCount = 0;

		std::vector<Addr> peers;
	};

	Timestamp CurrentTimestamp();
}
