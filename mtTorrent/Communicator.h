#pragma once

#include "BencodeParser.h"
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
		BencodeParser torrentParser;
		TorrentInfo torrentInfo;
		TrackerCollector trackers;	
		
		void initIds();
	};
}
