#pragma once

#include "TorrentFileParser.h"
#include "TcpStream.h"
#include "TrackerCommunication.h"
#include "Interface.h"

namespace Torrent
{
	class Communicator
	{
	public:

		Communicator();

		void test();

		ClientInfo client;
		TorrentFileParser torrent;
		TrackerCollector trackers;	
		
		void initIds();
	};
}
