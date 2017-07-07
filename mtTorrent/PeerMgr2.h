#pragma once

#include "BencodeParser.h"
#include "TrackerCommunication.h"
#include "TorrentDefines.h"
#include "ProgressScheduler.h"

namespace mtt
{
	class PeerMgr
	{
	public:

		PeerMgr(TorrentFileInfo* torrent, ProgressScheduler& scheduler);

		void start();

		TorrentFileInfo* torrentInfo;
		TrackerCollector trackers;
		ProgressScheduler& scheduler;
	};
}
