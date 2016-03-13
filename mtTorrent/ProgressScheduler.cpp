#include "ProgressScheduler.h"
#include <fstream>

Torrent::ProgressScheduler::ProgressScheduler(TorrentInfo* t)
{
	torrent = t;
	myProgress.init(torrent->pieces.size());
	scheduledProgress.init(torrent->pieces.size());

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
					block.length = std::min<uint32_t>(pieceSize - block.begin, blockRequestSize);

					info.blocks.push_back(block);
				}

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
					block.length = std::min<uint32_t>(pieceSize - block.begin, blockRequestSize);

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
	DataBuffer file;

	DataBuffer pieceTemp;
	pieceTemp.resize(torrent->pieceSize);

	for (auto& p : pieces)
	{
		size_t writtenSize = 0;

		for (auto& b : p.blocks)
		{
			writtenSize += b.info.length;
			memcpy(&pieceTemp[0] + b.info.begin, b.data.data(), b.info.length);
		}

		file.insert(file.end(), pieceTemp.begin(), pieceTemp.begin() + writtenSize);
	}

	std::ofstream fileOut(path + "ttt.pdf", std::ios_base::binary);
	fileOut.write(file.data(), file.size());
}

