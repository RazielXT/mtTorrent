#pragma once

#include "BencodeParser.h"
#include "UdpTrackerComm.h"
#include <mutex>

namespace mtt
{
	struct TrackerCollector
	{
		TrackerCollector(TorrentFileInfo* tInfo);
		size_t trackersCount;

		void announceAsync();
		void waitForAnyResults();
		std::vector<Addr> getResults();

		std::vector<Addr> announce(size_t id);

	private:

		uint32_t collectingTodo = 0;
		std::vector<Addr> asyncResults;
		std::mutex resultsMutex;

		TorrentFileInfo* torrent;
	};
}