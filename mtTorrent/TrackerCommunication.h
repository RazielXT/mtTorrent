#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"

namespace mtt
{
	struct TrackerCollector
	{
		TrackerCollector(TorrentFileInfo* tInfo);
		size_t trackersCount;

		void announceAsync();
		void waitForAnyResults();
		std::vector<PeerInfo> getResults();

		std::vector<PeerInfo> announce(size_t id);

	private:

		uint32_t collectingTodo = 0;
		std::vector<PeerInfo> asyncResults;
		std::mutex resultsMutex;

		ClientInfo* client;
		TorrentFileInfo* torrent;
	};
}