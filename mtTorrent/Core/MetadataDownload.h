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

		MetadataReconstruction metadata;

	private:

		std::mutex commsMutex;
		PeerCommunication* primaryComm;
		std::vector<PeerCommunication*> backupComm;
		void switchPrimaryComm();
		void removeBackup(PeerCommunication*);
		void addToBackup(PeerCommunication*);

		std::function<void(Status, MetadataDownloadState&)> onUpdate;
		MetadataDownloadState state;

		Peers& peers;

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

		void requestPiece();
		bool active = false;
	};
}