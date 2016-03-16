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
	if (!torrentParser.parseFile("D:\\punch.torrent"))
		return;

	torrentInfo = torrentParser.parseTorrentInfo();

	ProgressScheduler progress(&torrentInfo);
	//progress.selectFiles({ torrentInfo.files[3], torrentInfo.files[10], torrentInfo.files[12] });
	progress.selectFiles(torrentInfo.files);

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
		int addedPeersId = 40;

		std::vector<PeerCommunication*> peerComm;
		peerComm.resize(peersCount);

		for (size_t i = 0; i < peersCount; i++)
		{
			peerComm[i] =new PeerCommunication(&client);
			peerComm[i]->start(&torrentInfo, peers[i]);
		}

		std::thread service1([&io_service]() { io_service.run(); });

		bool actives = true;
		while (actives)
		{
			Sleep(50);

			std::vector<PeerInfo> pexAdd;

			actives = false;
			for (auto it = peerComm.begin(); it != peerComm.end();)
			{
				auto& pexPeers = (*it)->ext.pex.addedPeers;

				if (pexAdd.empty() && !pexPeers.empty())
				{
					pexAdd = pexPeers;
					pexPeers.clear();
				}		

				if (!(*it)->active || progress.finished())
				{
					if ((*it)->active)
						(*it)->stop();
					else
					{
						it = peerComm.erase(it);
						continue;
					}					
				}
				else
				{
					actives = true;		
				}

				it++;
			}

			if (peerComm.size() < 30 && addedPeersId<peers.size())
			{
				auto p = new PeerCommunication(&client);
				p->start(&torrentInfo, peers[addedPeersId]);
				peerComm.push_back(p);

				addedPeersId++;
			}

			for (auto& peer : pexAdd)
			{
				bool added = false;
				for (auto& comm : peerComm)
				{
					if (comm->peerInfo == peer)
						added = true;
				}

				if (!added)
				{
					auto p = new PeerCommunication(&client);
					p->start(&torrentInfo, peer);
					peerComm.push_back(p);
				}
			}
		}


		for (auto& peer : peerComm)
		{
			peer->stop();
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
