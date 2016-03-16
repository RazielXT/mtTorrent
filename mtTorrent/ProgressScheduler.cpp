#include "ProgressScheduler.h"

Torrent::ProgressScheduler::ProgressScheduler(TorrentInfo* t) : storage(t->pieceSize)
{
	torrent = t;
	myProgress.init(torrent->pieces.size());
	scheduledProgress.init(torrent->pieces.size());
}

void Torrent::ProgressScheduler::selectFiles(std::vector<Torrent::File> dlSelection)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	myProgress.setSelection(dlSelection);
	scheduledProgress.setSelection(dlSelection);

	storage.selectFiles(dlSelection);
}

std::vector<Torrent::PieceBlockInfo> makePieceBlocks(int index, Torrent::TorrentInfo* torrent)
{
	std::vector<Torrent::PieceBlockInfo> out;
	const size_t blockRequestSize = 16 * 1024;
	size_t pieceSize = torrent->pieceSize;

	if (index == torrent->pieces.size() - 1)
		pieceSize = torrent->files.back().endPiecePos;

	for (int j = 0; j*blockRequestSize < pieceSize; j++)
	{
		Torrent::PieceBlockInfo block;
		block.begin = j*blockRequestSize;
		block.index = index;
		block.length = static_cast<uint32_t>(std::min(pieceSize - block.begin, blockRequestSize));

		out.push_back(block);
	}

	return out;
}

Torrent::PieceDownloadInfo Torrent::ProgressScheduler::getNextPieceDownload(PiecesProgress& source)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	Torrent::PieceDownloadInfo info;

	for (int i = 0; i < myProgress.piecesCount; i++)
	{
		if (source.hasPiece(i))
		{
			bool needsSchedule = scheduledProgress.wantsPiece(i);
			bool fullRetry = scheduledProgress.finished() && myProgress.wantsPiece(i);

			if (needsSchedule || fullRetry)
			{
				scheduledProgress.addPiece(i);
				
				info.blocksLeft = makePieceBlocks(i, torrent);
				info.hash = torrent->pieces[i].hash;
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

