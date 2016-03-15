#pragma once

#include "BencodeParser.h"
#include "TrackerCommunication.h"
#include "Interface.h"
#include "ProgressScheduler.h"

namespace Torrent
{
	class Communicator
	{
	public:

		Communicator();

		void test();

		ClientInfo client;
		BencodeParser torrentParser;
		TorrentInfo torrentInfo;
		TrackerCollector trackers;	
		ProgressScheduler scheduler;

		void initIds();
	};
}
