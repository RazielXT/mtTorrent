#pragma once
#include <string>
#include <vector>
#include "Interface.h"

namespace mtt
{
	struct TorrentState
	{
		TorrentState(std::vector<uint8_t>&);

		std::string downloadPath;

		struct File
		{
			bool selected;
			Priority priority = Priority::Normal;
		};
		std::vector<File> files;

		std::vector<uint8_t>& pieces;
		int64_t lastStateTime = 0;
		bool started = false;

		std::vector<DownloadedPieceState> unfinishedPieces;

		uint64_t uploaded = 0;

		void save(const std::string& name);
		bool load(const std::string& name);
		static void remove(const std::string& name);
	};

	struct TorrentsList
	{
		struct TorrentInfo
		{
			std::string name;
		};
		std::vector<TorrentInfo> torrents;

		void save();
		bool load();
	};
}
