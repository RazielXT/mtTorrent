#pragma once

#include "Interface.h"
#include <mutex>

namespace Torrent
{
	class ProgressScheduler
	{
		ProgressScheduler(TorrentInfo* torrent);

		PieceDownloadInfo getNextPieceDownload(PiecesProgress& source);
		void addDownloadedPiece(DownloadedPiece piece);
		bool finished();

		std::vector<DownloadedPiece> pieces;
		PiecesProgress scheduledProgress;
		std::mutex schedule_mutex;

		PiecesProgress myProgress;
		TorrentInfo* torrent;
	};
}
