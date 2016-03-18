#include "ProgressScheduler.h"

Torrent::ProgressScheduler::ProgressScheduler(TorrentInfo* t) : storage(t->pieceSize)
{
	torrent = t;
	piecesTodo.init(torrent->pieces.size());
	scheduleTodo.init(torrent->pieces.size());
}

void Torrent::ProgressScheduler::selectFiles(std::vector<Torrent::File> dlSelection)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	piecesTodo.fromSelection(dlSelection);
	scheduleTodo.fromSelection(dlSelection);

	storage.selectFiles(dlSelection);
}

std::vector<Torrent::PieceBlockInfo> makePieceBlocks(uint32_t index, Torrent::TorrentInfo* torrent)
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

	if(scheduleTodo.empty() && !piecesTodo.empty())
		scheduleTodo.reset(piecesTodo);

	for (auto& it : piecesTodo.get())
	{
		uint32_t id = it.first;

		if (source.hasPiece(id))
		{
			bool needsSchedule = scheduleTodo.hasPiece(id);

			if (needsSchedule)
			{
				scheduleTodo.removePiece(id);
				
				info.blocksLeft = makePieceBlocks(id, torrent);
				info.hash = torrent->pieces[id].hash;
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

	piecesTodo.removePiece(piece.index);
	scheduleTodo.removePiece(piece.index);

	storage.storePiece(piece);
}

bool Torrent::ProgressScheduler::finished()
{
	return piecesTodo.empty();
}

float Torrent::ProgressScheduler::getPercentage()
{
	return 1.0f - piecesTodo.getPercentage();
}

void Torrent::ProgressScheduler::exportFiles(std::string path)
{
	storage.exportFiles(path);
}

