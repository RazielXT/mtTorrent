#pragma once

#include "Storage.h"
#include "PiecesProgress.h"


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
		uint32_t getDownloadedSize();
		float getSpeed();

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
