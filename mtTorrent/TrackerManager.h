#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"

namespace mtt
{
	struct TrackerManager
	{
		TrackerManager();

		void setTorrentInfo(TorrentFileInfo* info);
		
		void start();
		void end();

		std::function<void()> onAnnounceResult;
	};
}