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
	if (!torrent.parse("D:\\pdf.torrent"))
		return;

		auto peers = trackers.announceAll();

		if (peers.size())
		{
			PeerCommunication peer[100];

			std::future<void> futures[100];

			for (size_t i = 0; i < peers.size(); i++)
			{
				futures[i] = std::async(&PeerCommunication::start, &peer[i], &torrent.info, &client, peers[i]);
			}			

			for (size_t i = 0; i < peers.size(); i++)
			{
				futures[i].get();
			}
		}
	
}

Torrent::Communicator::Communicator() : trackers(&client, &torrent.info)
{

}
