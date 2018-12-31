#pragma once
#include "Torrent.h"

namespace mtt
{
	class Core
	{
	public:

		TorrentPtr torrent;

		void init();

		void start();
	};
}
