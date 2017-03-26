#include "ProgressScheduler.h"

mtt::ProgressScheduler::ProgressScheduler(TorrentFileInfo* t) : storage(t->pieceSize)
{
	torrent = t;
	piecesTodo.init(torrent->pieces.size());
	scheduleTodo.init(torrent->pieces.size());
}

void mtt::ProgressScheduler::selectFiles(std::vector<mtt::File> dlSelection)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	piecesTodo.fromSelection(dlSelection);
	scheduleTodo.fromSelection(dlSelection);

	storage.selectFiles(dlSelection);
}

std::vector<mtt::PieceBlockInfo> makePieceBlocks(uint32_t index, mtt::TorrentFileInfo* torrent)
{
	std::vector<mtt::PieceBlockInfo> out;
	const size_t blockRequestSize = 16 * 1024;
	size_t pieceSize = torrent->pieceSize;

	if (index == torrent->pieces.size() - 1)
		pieceSize = torrent->files.back().endPiecePos;

	for (int j = 0; j*blockRequestSize < pieceSize; j++)
	{
		mtt::PieceBlockInfo block;
		block.begin = j*blockRequestSize;
		block.index = index;
		block.length = static_cast<uint32_t>(std::min(pieceSize - block.begin, blockRequestSize));

		out.push_back(block);
	}

	return out;
}

mtt::PieceDownloadInfo mtt::ProgressScheduler::getNextPieceDownload(PiecesProgress& source)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	mtt::PieceDownloadInfo info;

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

void mtt::ProgressScheduler::addDownloadedPiece(DownloadedPiece& piece)
{
	std::lock_guard<std::mutex> guard(schedule_mutex);

	piecesTodo.removePiece(piece.index);
	scheduleTodo.removePiece(piece.index);

	storage.storePiece(piece);
}

bool mtt::ProgressScheduler::finished()
{
	return piecesTodo.empty();
}

float mtt::ProgressScheduler::getPercentage()
{
	return 1.0f - piecesTodo.getPercentage();
}

void mtt::ProgressScheduler::saveProgress()
{
	storage.saveProgress();
}

void mtt::ProgressScheduler::loadProgress()
{
	storage.loadProgress();
}

void mtt::ProgressScheduler::exportFiles()
{
	storage.exportFiles();
}

