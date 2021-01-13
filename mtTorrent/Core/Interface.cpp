#include "Interface.h"

mtt::SelectedIntervals::SelectedIntervals(const mtt::DownloadSelection& selection)
{
	for (const auto& f : selection.files)
		if (f.selected)
		{
			if (!selectedIntervals.empty() && f.info.startPieceIndex == selectedIntervals.back().endIdx)
				selectedIntervals.back().endIdx = f.info.endPieceIndex;
			else
				selectedIntervals.push_back({ f.info.startPieceIndex, f.info.endPieceIndex });
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
