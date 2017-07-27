#pragma once

#include "BencodeParser.h"
#include "PeerCommunication.h"
#include "UdpTrackerComm.h"

namespace mtt
{
	struct TrackerManager
	{
	public:

		TrackerManager();

		void init(TorrentFileInfo* info);
		void addTrackers(std::vector<std::string> trackers);

		void start();
		void end();

	private:

		std::vector<std::shared_ptr<Tracker>> activeTrackers;
		std::vector<std::string> trackers
	};
}