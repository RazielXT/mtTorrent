#pragma once

#include "Interface.h"
#include "Storage.h"
#include "PiecesProgress.h"

namespace mtt
{
	class Files
	{
	public:

		Files(const TorrentInfo&);

		void setDefaults(const TorrentInfo& info);
		void initialize(DownloadSelection selection, const std::string& location);

		bool select(const TorrentInfo& info, const std::vector<bool>&);
		bool select(const TorrentInfo& info, uint32_t idx, bool selected);
		Status prepareSelection();

		PiecesProgress progress;
		DownloadSelection selection;
		Storage storage;
	};
}
