#include "ProgressScheduler.h"

Torrent::ProgressScheduler::ProgressScheduler(TorrentInfo* t)
{
	torrent = t;
	myProgress.piecesCount = scheduledProgress.piecesCount = torrent->pieces.size();
	pieces.resize(myProgress.piecesCount);
}

Torrent::PieceDownloadInfo Torrent::ProgressScheduler::getNextPieceDownload(PiecesProgress& source)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	Torrent::PieceDownloadInfo info;

	for (int i = 0; i < myProgress.piecesCount; i++)
	{
		if (source.hasPiece(i))
		{
			if (!scheduledProgress.hasPiece(i))
			{
				scheduledProgress.addPiece(i);
				Torrent::PieceDownloadInfo info;

				const size_t blockRequestSize = 16 * 1024;
				size_t pieceSize = torrent->pieceSize;

				if (i == myProgress.piecesCount - 1)
					pieceSize = torrent->files.back().endPiecePos;

				for (int j = 0; j*blockRequestSize < pieceSize; j++)
				{
					PieceBlockInfo block;
					block.begin = j*blockRequestSize;
					block.index = i;
					block.length = std::min(pieceSize - block.begin, blockRequestSize);

					info.blocks.push_back(block);
				}

				return info;
			}
		}
	}

	return info;
}

void Torrent::ProgressScheduler::addDownloadedPiece(DownloadedPiece piece)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	pieces[piece.index] = piece;
}

bool Torrent::ProgressScheduler::finished()
{
	return myProgress.getPercentage() == 1.0f;
}

