#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Scheduler.h"
#include "PeerManager.h"

namespace mtt
{
	struct TorrentCore
	{
		TorrentFileInfo torrent;
		DownloadSelection selection;

		ServiceThreadpool service;
		Scheduler scheduler;
		PeerManager peerMgr;
	};

	class TorrentsCollection
	{
	public:

		std::vector<CorePtr> torrents;

		static TorrentsCollection& Get();

		CorePtr GetTorrent(TorrentFileInfo& info);
		CorePtr GetTorrent(uint8_t* id);
	};
}
