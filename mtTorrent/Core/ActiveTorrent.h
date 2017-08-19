#pragma once

#include "dht/Communication.h"


namespace mtt
{
	class ActiveTorrent : public dht::ResultsListener
	{
	public:

		ActiveTorrent(TorrentPtr torrent, dht::Communication& dht);
		~ActiveTorrent();

		//DownloadScheduler scheduler;
		//PeerMgr peers;
		//TrackerManager trackers;
		//Storage storage;

		virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
		virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;
	};
}
