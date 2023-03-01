#include "Interface.h"
#include "utils/SHA.h"

mtt::SelectedFiles::SelectedFiles(const mtt::TorrentInfo info, const mtt::DownloadSelection& selection)
{
	std::vector<bool> selectedPieces(info.pieces.size(), false);
	for (size_t i = 0; i < info.files.size(); i++)
		if (selection[i].selected)
		{
			for (auto p = info.files[i].startPieceIndex; p <= info.files[i].endPieceIndex; p++)
			{
				selectedPieces[p] = true;
			}
		}

	for (size_t i = 0; i < info.files.size(); i++)
	{
		const auto& file = info.files[i];
		uint64_t sz = 0;

		if (selection[i].selected)
		{
			sz = file.size;

			files.push_back({ (uint32_t)i, sz });
		}
		else
		{
			//ending piece
			if (selectedPieces[file.startPieceIndex])
			{
				sz = info.pieceSize - file.startPiecePos;

				if (sz > file.size)
					sz = file.size;
			}
			//starting piece
			if (selectedPieces[file.endPieceIndex])
			{
				sz += file.endPiecePos;

				if (sz > file.size)
					sz = file.size;
			}

			if (sz)
				files.push_back({ (uint32_t)i, sz });
		}
	}
}

mtt::SelectedFiles::SelectedFiles(const mtt::TorrentInfo info, uint32_t idx)
{
	auto& targetFile = info.files[idx];
	uint32_t start = idx;
	while (start > 0)
	{
		const auto& prevFile = info.files[start - 1];
		if (prevFile.endPieceIndex != targetFile.startPieceIndex)
			break;
		start--;
	}

	for (size_t i = start; i < info.files.size(); i++)
	{
		const auto& file = info.files[i];
		if (file.startPieceIndex > targetFile.endPieceIndex)
			break;

		uint64_t sz = 0;

		if (i == idx)
		{
			sz = file.size;

			files.push_back({ idx, sz });
		}
		else
		{
			//end piece padding
			if (file.startPieceIndex == targetFile.endPieceIndex)
			{
				sz = info.pieceSize - file.startPiecePos;

				if (sz > file.size)
					sz = file.size;
			}
			//start piece padding
			if (file.endPieceIndex == targetFile.startPieceIndex)
			{
				sz += file.endPiecePos;

				if (sz > file.size)
					sz = file.size;
			}

			if (sz)
				files.push_back({ (uint32_t)i, sz });
		}
	}
}

void mtt::PieceState::init(uint32_t idx, uint32_t blocksCount)
{
	remainingBlocks = blocksCount;
	blocksState.assign(remainingBlocks, 0);
	index = idx;
}

bool mtt::PieceState::addBlock(const mtt::PieceBlock & block)
{
	auto blockIdx = (block.info.begin + 1) / BlockMaxSize;

	if (blockIdx < blocksState.size() && blocksState[blockIdx] == 0)
	{
		blocksState[blockIdx] = 1;
		remainingBlocks--;
		downloadedSize += block.info.length;

		return true;
	}

	return false;
}

mtt::Timestamp mtt::CurrentTimestamp()
{
	return (Timestamp)::time(nullptr);
}
