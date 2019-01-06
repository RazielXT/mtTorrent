#pragma once

#include "Storage.h"
#include "PeerCommunication.h"
#include "utils/ServiceThreadpool.h"
#include "Peers.h"
#include "MetadataReconstruction.h"
#include "Interface.h"

namespace mtt
{
	class MetadataDownload : public IPeerListener
	{
	public:

		MetadataDownload(Peers&);

		void start(std::function<void(Status, MetadataDownloadState&)> onUpdate);
		void stop();

		MetadataDownloadState state;
		MetadataReconstruction metadata;

	private:

		std::mutex commsMutex;
		std::vector<std::shared_ptr<PeerCommunication>> activeComms;
		void evalComms();
		void removeBackup(PeerCommunication*);
		void addToBackup(std::shared_ptr<PeerCommunication>);

		std::function<void(Status, MetadataDownloadState&)> onUpdate;

		Peers& peers;

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

		void requestPiece(std::shared_ptr<PeerCommunication> peer);
		bool active = false;
	};
}