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
		size_t hashLen = strlen(MT_HASH_NAME);
		memcpy(client.hashId, MT_HASH_NAME, hashLen);

		for (size_t i = hashLen; i < 20; i++)
		{
			client.hashId[i] = static_cast<uint8_t>(rand() % 255);
		}
	}

	client.key = static_cast<uint32_t>(rand());
}

bool containsPeer(std::vector<std::unique_ptr<PeerCommunication>>& peers, PeerInfo& info)
{
	bool added = false;
	for (auto& comm : peers)
	{
		if (comm->peerInfo == info)
			added = true;
	}

	return added;
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
	size_t trackerReannounceId = 0;

	//peers.push_back({ "127.0.0.1" , 6881});
	size_t startPeersCount = 40;
	size_t maxActivePeers = 30;

	if (peers.size())
	{
		size_t peersCount = std::min<size_t>(startPeersCount, peers.size());
		size_t addingPeerId = peersCount;

		std::vector<std::unique_ptr<PeerCommunication>> peerComms;
		peerComms.resize(peersCount);

		for (size_t i = 0; i < peersCount; i++)
		{
			peerComms[i] = std::make_unique<PeerCommunication>(&client);
			peerComms[i]->start(&torrentInfo, peers[i]);
		}

		std::thread service1([&io_service]() { io_service.run(); });

		bool actives = true;
		while (actives)
		{
			Sleep(50);

			std::vector<PeerInfo> pexAdd;

			actives = false;
			for (auto it = peerComms.begin(); it != peerComms.end();)
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
						it = peerComms.erase(it);
						continue;
					}					
				}
				else
				{
					actives = true;		
				}

				it++;
			}

			if (peerComms.size() < maxActivePeers && addingPeerId<peers.size())
			{
				if (!containsPeer(peerComms, peers[addingPeerId]))
				{
					auto p = std::make_unique<PeerCommunication>(&client);
					p->start(&torrentInfo, peers[addingPeerId]);
					peerComms.push_back(std::move(p));
				}

				addingPeerId++;
			}

			if (addingPeerId >= peers.size())
			{
				peers.clear();
				addingPeerId = 0;

				peers = trackers.announce(trackerReannounceId);
				trackerReannounceId = (trackerReannounceId + 1) % trackers.count;
			}

			for (auto& peer : pexAdd)
			{
				if (!containsPeer(peerComms, peer))
				{
					auto p = std::make_unique<PeerCommunication>(&client);
					p->start(&torrentInfo, peer);
					peerComms.push_back(std::move(p));
				}
			}
		}

		for (auto& peer : peerComms)
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
