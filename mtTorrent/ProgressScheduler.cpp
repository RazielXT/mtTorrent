#include "ProgressScheduler.h"

Torrent::ProgressScheduler::ProgressScheduler(TorrentInfo* t) : storage(selection, t->pieceSize)
{
	torrent = t;
	myProgress.init(torrent->pieces.size());
	scheduledProgress.init(torrent->pieces.size());
}

void Torrent::ProgressScheduler::selectFiles(std::vector<Torrent::File> dlSelection)
{
	selection.files.clear();

	for (auto& f : dlSelection)
	{
		selection.files.push_back({ f, StorageType::Memory });
	}

	storage.selectionChanged();
}

Torrent::PieceDownloadInfo Torrent::ProgressScheduler::getNextPieceDownload(PiecesProgress& source)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	Torrent::PieceDownloadInfo info;
	info.blocksCount = 0;

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
					block.length = static_cast<uint32_t>(std::min(pieceSize - block.begin, blockRequestSize));

					info.blocksLeft.push_back(block);
				}

				info.blocksCount = info.blocksLeft.size();
				return info;
			}
		}
	}

	for (int i = 0; i < myProgress.piecesCount; i++)
	{
		if (source.hasPiece(i))
		{
			if (!myProgress.hasPiece(i))
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
					block.length = static_cast<uint32_t>(std::min(pieceSize - block.begin, blockRequestSize));

					info.blocksLeft.push_back(block);
				}

				info.blocksCount = info.blocksLeft.size();
				return info;
			}
		}
	}

	return info;
}

void Torrent::ProgressScheduler::addDownloadedPiece(DownloadedPiece& piece)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	if (myProgress.hasPiece(piece.index))
		return;

	storage.storePiece(piece);

	myProgress.addPiece(piece.index);
}

bool Torrent::ProgressScheduler::finished()
{
	return myProgress.getPercentage() == 1.0f;
}

float Torrent::ProgressScheduler::getPercentage()
{
	return myProgress.getPercentage();
}

void Torrent::ProgressScheduler::exportFiles(std::string path)
{
	storage.exportFiles(path);
}

