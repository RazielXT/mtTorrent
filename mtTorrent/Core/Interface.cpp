#include "Interface.h"
#include "utils/SHA.h"

mtt::SelectedIntervals::SelectedIntervals(const mtt::TorrentInfo info, const mtt::DownloadSelection& selection)
{
	for (size_t i = 0; i < info.files.size(); i++)
		if (selection[i].selected)
		{
			if (!selectedIntervals.empty() && info.files[i].startPieceIndex == selectedIntervals.back().endIdx)
				selectedIntervals.back().endIdx = info.files[i].endPieceIndex;
			else
				selectedIntervals.push_back({ info.files[i].startPieceIndex, info.files[i].endPieceIndex });
		}
}

bool mtt::SelectedIntervals::isSelected(const mtt::File& f)
{
	for (const auto& i : selectedIntervals)
	{
		if (f.startPieceIndex <= i.endIdx && f.endPieceIndex >= i.beginIdx)
			return true;
	}
	return false;
}

bool mtt::SelectedIntervals::isSelected(uint32_t idx)
{
	for (const auto& i : selectedIntervals)
	{
		if (idx <= i.endIdx && idx >= i.beginIdx)
			return true;
	}
	return false;
}

void mtt::PieceState::init(uint32_t idx, uint32_t blocksCount)
{
	remainingBlocks = blocksCount;
	blocksState.assign(remainingBlocks, 0);
	index = idx;
}

bool mtt::PieceState::addBlock(const mtt::PieceBlock & block)
{
	auto blockIdx = (block.info.begin + 1) / BlockRequestMaxSize;

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
