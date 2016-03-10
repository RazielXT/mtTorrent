#pragma once

#include "Interface.h"
#include <mutex>

namespace Torrent
{
	class DownloadManager
	{
		DownloadManager(TorrentInfo* torrent);

		int getNextPieceDownload(PiecesProgress& source);
		void addPiece(DownloadedPiece piece);
		bool finished();

		std::vector<DownloadedPiece> pieces;
		PiecesProgress scheduledProgress;
		std::mutex schedule_mutex;

		PiecesProgress myProgress;
		TorrentInfo* torrent;
	};
}
