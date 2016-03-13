#pragma once

#include "Interface.h"
#include <mutex>

namespace Torrent
{
	class ProgressScheduler
	{
	public:

		ProgressScheduler(TorrentInfo* torrent);

		PieceDownloadInfo getNextPieceDownload(PiecesProgress& source);
		void addDownloadedPiece(DownloadedPiece piece);
		bool finished();
		float getPercentage();
		void exportFiles(std::string path);

	private:

		std::vector<DownloadedPiece> pieces;
		PiecesProgress scheduledProgress;
		std::mutex schedule_mutex;

		PiecesProgress myProgress;
		TorrentInfo* torrent;
	};
}
