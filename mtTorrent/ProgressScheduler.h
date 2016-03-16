#pragma once

#include "Storage.h"
#include <mutex>

namespace Torrent
{
	class ProgressScheduler
	{
	public:

		ProgressScheduler(TorrentInfo* torrent);

		void selectFiles(std::vector<File> selection);

		PieceDownloadInfo getNextPieceDownload(PiecesProgress& source);
		void addDownloadedPiece(DownloadedPiece& piece);

		bool finished();
		float getPercentage();

		void exportFiles(std::string path);

	private:

		Storage storage;

		PiecesProgress scheduledProgress;
		std::mutex schedule_mutex;

		PiecesProgress myProgress;
		TorrentInfo* torrent;
	};
}
