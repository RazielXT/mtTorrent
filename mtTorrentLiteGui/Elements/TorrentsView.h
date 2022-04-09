#pragma once
#include "FileProgress.h"
#include "SpeedChart.h"
#include <vector>
#include <map>

class AppCore;

class TorrentsView
{
public:
	TorrentsView(AppCore& core);

	void update();

	//update selection info based on torrents grid state
	void refreshSelection();

	void updateList();
	void update(const uint8_t* hash);

	struct SelectedTorrent
	{
		uint8_t hash[20];
	};
	std::vector<SelectedTorrent> getAllSelectedTorrents();

	FileProgress piecesProgress;

private:

	SpeedChart speedChart;

	AppCore& core;

	bool lastInfoIncomplete = false;

	//text info about selected torrent
	void refreshTorrentInfo(uint8_t* hash);
	void refreshTorrentsGrid();
	void refreshPeers();
	void refreshSources();

	std::map<int, int> torrentRows;
	struct TorrentState
	{
		bool active = true;
		uint64_t tm;
	};
	std::map<int, TorrentState> torrentState;

	bool listChanged = true;
};
