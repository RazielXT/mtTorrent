#include "Communicator.h"
#include "PacketHelper.h"
#include "Network.h"

#include <sstream>
#include <future>
#include <iostream>

using namespace Torrent;

void Communicator::initIds()
{
	srand(time(NULL));

	{
		client.hashId[0] = 'M';
		client.hashId[1] = 'T';
		client.hashId[2] = '0';
		client.hashId[3] = '-';
		client.hashId[4] = '1';
		client.hashId[5] = '-';
		for (size_t i = 6; i < 20; i++)
		{
			client.hashId[i] = static_cast<uint8_t>(rand() % 255);
		}
	}

	client.key = static_cast<uint32_t>(rand());
}

void Communicator::test()
{
	if (!torrentParser.parseFile("D:\\pdf.torrent"))
		return;

	torrentInfo = torrentParser.parseTorrentInfo();

		auto peers = trackers.announceAll();

		if (peers.size())
		{
			PeerCommunication peer[400];

			std::future<void> futures[400];

			for (size_t i = 0; i < peers.size(); i++)
			{
				futures[i] = std::async(&PeerCommunication::start, &peer[i], &torrentInfo, &client, peers[i]);
			}			

			for (size_t i = 0; i < peers.size(); i++)
			{
				futures[i].get();
			}
		}
	
}

Torrent::Communicator::Communicator() : trackers(&client, &torrentInfo)
{

}
