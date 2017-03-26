#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"

namespace mtt
{
	struct TrackerCollector
	{
		TrackerCollector(TorrentFileInfo* tInfo);

		std::vector<PeerInfo> announceAll();
		std::vector<PeerInfo> announce(size_t id);

		size_t count;

	private:

		ClientInfo* client;
		TorrentFileInfo* torrent;
	};
}