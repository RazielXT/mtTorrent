#pragma once

#include "Storage.h"
#include "PiecesProgress.h"
#include "PeerCommunication.h"
#include "utils/ServiceThreadpool.h"
#include "PeerManager.h"
#include "MetadataReconstruction.h"
#include "Interface.h"

namespace mtt
{
	class MetadataDownload : public PeerManagerListener
	{
	public:

		MetadataDownload();

		void start(CorePtr core, std::function<void(bool)> onFinish);
		void stop();

		MetadataReconstruction metadata;

	private:

		std::shared_ptr<PeerCommunication> activeComm;
		std::vector<Addr> possibleAddrs;

		std::function<void(bool)> onFinish;

		CorePtr core;

		virtual void onConnected(std::shared_ptr<PeerCommunication>, Addr&) override;
		virtual void onConnectFail(Addr&) override;
		virtual void onAddrReceived(std::vector<Addr>&) override;

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

		void requestPiece();
		void connectNext();
		bool active = false;
	};
}