#pragma once

#include "Interface.h"
#include "Storage.h"
#include "PiecesProgress.h"

namespace mtt
{
	class Files
	{
	public:

		void init(TorrentInfo&);
		void select(DownloadSelection&);
		Status prepareSelection();

		PiecesProgress progress;
		DownloadSelection selection;
		Storage storage;
	};
}
