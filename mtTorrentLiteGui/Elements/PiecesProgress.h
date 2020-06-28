#pragma once
#include "../../mtTorrent/Public/BinaryInterface.h"

class AppCore;

/*
Pieces chart + files progress grid
*/
class PiecesProgress
{
public:
	PiecesProgress(AppCore& core);

	void update();

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

	struct
	{
		uint32_t from = -1;
		uint32_t to;
	}
	piecesHighlight;

	//temporarily change progress highlighting to highlight chart interval (specific file) 
	void highlightChartPieces(uint32_t from, uint32_t to);

	AppCore& core;
};
