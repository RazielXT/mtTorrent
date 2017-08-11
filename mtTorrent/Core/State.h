#pragma once
#include <string>
#include <vector>

namespace mtt
{
	struct TorrentState
	{
		std::string torrentFilePath;
		std::string downloadPath;

		struct SelectedFile
		{
			uint32_t id;
			bool active;
		};
		std::vector<SelectedFile> selection;

		std::vector<uint32_t> progress;
	};

	struct TorrentsState
	{
		std::vector<TorrentState> torrents;

		void saveState();
		void loadState();
	};
}
