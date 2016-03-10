#include "DownloadManager.h"

Torrent::DownloadManager::DownloadManager(TorrentInfo* t)
{
	torrent = t;
	myProgress.piecesCount = scheduledProgress.piecesCount = torrent->pieces.size();
	pieces.resize(myProgress.piecesCount);
}

int Torrent::DownloadManager::getNextPieceDownload(PiecesProgress& source)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	int scheduledFallback = -1;

	for (int i = 0; i < myProgress.piecesCount; i++)
	{
		if (source.hasPiece(i))
		{
			if (!scheduledProgress.hasPiece(i))
			{
				scheduledProgress.addPiece(i);
				return i;
			}
			else if(!myProgress.hasPiece(i))
				scheduledFallback = i;
		}
		
		if (scheduledFallback > 0)
			return i;
	}

	return -1;
}

void Torrent::DownloadManager::addPiece(DownloadedPiece piece)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	pieces[piece.index] = piece;
}

bool Torrent::DownloadManager::finished()
{
	return myProgress.getPercentage() == 1.0f;
}

