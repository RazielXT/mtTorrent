#pragma once
#include "../../mtTorrent/Public/BinaryInterface.h"
#include <vector>

class AppCore;

/*
Pieces chart + files progress grid
*/
class FileProgress
{
public:
	FileProgress(AppCore& core);

	void update();

	void fileSelectionChanged(int row);

private:

	//setup chart for currently selected torrent
	void initPiecesChart();

	void updatePiecesChart();
	void updateFilesProgress();

	mtBI::PiecesInfo progress;
	mtt::array<uint8_t> lastBitfield;

	const uint32_t MaxProgressChartIntervalSize = 1000;
	uint32_t ProgressChartIntervalSize = 0;

	uint32_t getProgressChartIndex(uint32_t pieceIdx);

	bool refreshing = false;
	bool selectionChanged = false;
	std::vector<size_t> activePieces;
	std::vector<size_t> fileFinishedPieces;

	AppCore& core;
};
