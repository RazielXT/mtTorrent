#include "Communicator.h"
#include "PacketHelper.h"
#include "Network.h"

#include <sstream>
#include <future>
#include <iostream>

using namespace Torrent;

void Communicator::initIds()
{
	srand((unsigned int)time(NULL));

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
	if (!torrentParser.parseFile("D:\\best.torrent"))
		return;

	torrentInfo = torrentParser.parseTorrentInfo();

	ProgressScheduler progress(&torrentInfo);
	client.scheduler = &progress;

	boost::asio::io_service io_service;
	//tcp::resolver resolver(io_service);

	client.network.io_service = &io_service;
	//client.network.resolver = &resolver;

	std::vector<PeerInfo> peers;
	peers = trackers.announceAll();

	PeerInfo add;
	add.port = 6881;
	add.ipStr = "127.0.0.1";
	//peers.push_back(add);

	if (peers.size())
	{
		size_t peersCount = std::min<size_t>(40, peers.size());

		std::vector<std::unique_ptr<PeerCommunication>> peerComm;
		peerComm.resize(peersCount);

		for (size_t i = 0; i < peersCount; i++)
		{
			peerComm[i] = std::make_unique<PeerCommunication>(&client);
			peerComm[i]->start(&torrentInfo, peers[i]);
		}

		std::thread service1([&io_service]() { io_service.run(); });

		bool actives = true;
		while (actives)
		{
			Sleep(50);

			actives = false;
			for (auto it = peerComm.begin(); it != peerComm.end();)
			{
				if (!(*it)->active || progress.finished())
				{
					if ((*it)->active)
						(*it)->stop();
				}
				else
				{
					actives = true;		
				}

				it++;
			}
		}

		service1.join();
	}

	if (progress.finished())
	{
		progress.exportFiles("D:\\");
	}
}

Torrent::Communicator::Communicator() : trackers(&client, &torrentInfo), scheduler(&torrentInfo)
{
	client.scheduler = &scheduler;
}

std::string getTimestamp()
{
	time_t timer;
	struct tm y2k = { 0 };
	double seconds;

	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time(&timer);  /* get current time; same as: timer = time(NULL)  */

	seconds = difftime(timer, mktime(&y2k));

	return std::to_string(static_cast<int64_t>(seconds));
}