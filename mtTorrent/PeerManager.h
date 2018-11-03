#pragma once

#include "PeerManagerListener.h"
#include "TrackerManager.h"
#include "ProgressStatistics.h"

namespace mtt
{
	enum class PeerSource
	{
		Tracker,
		Pex,
		Dht
	};

	class PeerManager : public IPeerListener
	{
	public:

		PeerManager() {};

		void start(CorePtr core);

		void stop();

		void addTrackers(std::vector<std::string>& trackers);

		void connect(Addr& addr);

		bool active = false;

		PeerManagerListener* listener;

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

		ProgressStatistics statistics;

	private:

		struct KnownPeer
		{
			Addr address;
			PeerSource source;
		};
		std::vector<KnownPeer> knownPeers;

		std::vector<std::shared_ptr<PeerCommunication>> activePeers;

		CorePtr core;

		TrackerManager trackers;
	};
}
