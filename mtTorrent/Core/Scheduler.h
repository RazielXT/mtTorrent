#pragma once

#include "Storage.h"
#include "PiecesProgress.h"
#include "PeerManagerListener.h"

namespace mtt
{
	class Scheduler : public PeerManagerListener
	{
	public:

		CorePtr core;
		Storage storage;


		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;
		virtual void onConnected(std::shared_ptr<PeerCommunication>, Addr&) override;
		virtual void onConnectFail(Addr&) override;
		virtual void onAddrReceived(std::vector<Addr>&) override;

	};
}
