#pragma once

#include <string>
#include <vector>
#include "Interface.h"

namespace mtt
{
	struct TorrentState
	{
		TorrentState(std::vector<uint8_t>&);

		struct 
		{
			std::string name;
			uint32_t pieceSize = 0;
			uint64_t fullSize = 0;
		}
		info;

		std::string downloadPath;

		DownloadSelection selection;

		std::vector<uint8_t>& pieces;
		int64_t lastStateTime = 0;
		Timestamp addedTime = 0;
		bool started = false;

		std::vector<PieceState> unfinishedPieces;

		uint64_t downloaded = 0;
		uint64_t uploaded = 0;

		uint32_t version = 1;

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
