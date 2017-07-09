#pragma once

#include "BencodeParser.h"
#include "TrackerCommunication.h"
#include "Interface2.h"
#include "ProgressScheduler.h"
#include "IPeerListener.h"

namespace mtt
{
	enum PeerSource
	{
		Tracker,
		Pex,
		Dht
	};

	class PeerMgr : public IPeerListener
	{
	public:

		PeerMgr(TorrentFileInfo* torrent, ProgressScheduler& scheduler);

		void start();

	private:

		struct KnownPeer
		{
			Addr address;
			PeerSource source;
		};
		std::vector<KnownPeer> knownPeers;
		std::vector<PeerCommunication2> activePeers;

		TorrentInfo* torrentInfo;
		TrackerCollector trackers;
		ProgressScheduler& scheduler;
	};
}
