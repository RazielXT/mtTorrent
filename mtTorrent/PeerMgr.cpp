#include "PeerMgr.h"
#include "PacketHelper.h"
#include "Network.h"

#include <sstream>
#include <future>
#include <iostream>

using namespace mtt;

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

void addUniquePeers(std::vector<PeerInfo>& from, std::vector<PeerInfo>& to, size_t pos)
{
	for (auto& p : from)
	{
		bool found = false;

		for (auto& p2 : to)
		{
			if (p == p2)
			{
				found = true;
				break;
			}
		}

		if (!found)
			to.insert(to.begin() + pos, p);
	}
}

PeerMgr::PeerMgr(TorrentFileInfo* info, ProgressScheduler& sched) : torrentInfo(info), scheduler(sched), trackers(torrentInfo)
{

}

void PeerMgr::start()
{
	Checkpoint check;

	bool localTest = false;

	GENERAL_INFO_LOG("Announcing\n");

	std::vector<PeerInfo> peers;
	if(!localTest)
		peers = trackers.announceAll();
	else
		peers.push_back({ "127.0.0.1" , 55391 });

	GENERAL_INFO_LOG("Received " << peers.size() << " peers\n");

	size_t trackerReannounceId = 0;

	size_t startPeersCount = 15;
	size_t maxActivePeers = 30;

	ClientInfo* client = mtt::getClientInfo();

	if (peers.size())
	{
		size_t peersCount = std::min<size_t>(startPeersCount, peers.size());
		size_t addingPeerId = peersCount;

		std::vector<std::unique_ptr<PeerCommunication>> peerComms;
		peerComms.resize(peersCount);

		for (size_t i = 0; i < peersCount; i++)
		{
			peerComms[i] = std::make_unique<PeerCommunication>(client->network.io_service, &scheduler);
			peerComms[i]->start(torrentInfo, peers[i]);
		}

		std::thread service1([client]() { client->network.io_service->run(); });

		check.hit(0, 5 * 60 * 1000);
		check.hit(1, 1000);

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
					(*it)->ext.pex.addedPeers.clear();
				}		

				if (!(*it)->active || scheduler.finished())
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

			if(!localTest)
			if (peerComms.size() < maxActivePeers && addingPeerId<peers.size())
			{
				if (!containsPeer(peerComms, peers[addingPeerId]))
				{
					auto p = std::make_unique<PeerCommunication>(client->network.io_service, &scheduler);
					p->start(torrentInfo, peers[addingPeerId]);
					peerComms.push_back(std::move(p));
				}

				addingPeerId++;
			}

			if (!localTest)
			if (addingPeerId >= peers.size())
			{
				bool timeForReannounceRound = true;

				if (trackerReannounceId == 0)
				{
					timeForReannounceRound = check.hit(0, 5 * 60 * 1000);
				}

				if (timeForReannounceRound)
				{
					peers.clear();
					addingPeerId = 0;

					peers = trackers.announce(trackerReannounceId);
					trackerReannounceId = (trackerReannounceId + 1) % trackers.count;
				}
			}

			if (!localTest)
				addUniquePeers(pexAdd, peers, addingPeerId);

			uint32_t currentActives = 0;
			for (auto& p : peerComms)
			{
				if (p->active && p->state.finishedHandshake)
					currentActives++;
			}

			if(check.hit(1,1000))
				GENERAL_INFO_LOG("Peers: " << currentActives << "/" << peerComms.size() << "/" << peers.size() << "    speed: " << scheduler.getSpeed() << "    progress: " << scheduler.getPercentage() << "% / " << scheduler.getDownloadedSize()/(1024.f*1024.f) << "mb\n");
		}

		for (auto& peer : peerComms)
		{
			peer->stop();
		}

		service1.join();
	}
}
