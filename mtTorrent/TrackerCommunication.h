#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"

namespace Torrent
{
	struct TrackerCollector
	{
		TrackerCollector(ClientInfo* client, TorrentInfo* tInfo);

		std::vector<PeerInfo> announceAll();
		std::vector<PeerInfo> announce(size_t id);

		size_t count;

	private:

		ClientInfo* client;
		TorrentInfo* torrent;
	};
}