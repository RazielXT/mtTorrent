#pragma once

#include "Storage.h"
#include <mutex>

namespace mtt
{
	class ProgressScheduler
	{
	public:

		ProgressScheduler(TorrentFileInfo* torrent);

		void selectFiles(std::vector<File> selection);

		PieceDownloadInfo getNextPieceDownload(PiecesProgress& source);
		void addDownloadedPiece(DownloadedPiece& piece);

		bool finished();
		float getPercentage();

		void saveProgress();
		void loadProgress();
		void exportFiles();

	private:

		Storage storage;

		PiecesProgress scheduleTodo;
		std::mutex schedule_mutex;

		PiecesProgress piecesTodo;
		TorrentFileInfo* torrent;
	};
}
