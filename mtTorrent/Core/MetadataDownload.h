#pragma once

#include "Storage.h"
#include "PeerCommunication.h"
#include "utils/ServiceThreadpool.h"
#include "Peers.h"
#include "MetadataReconstruction.h"
#include "Interface.h"
#include "Api/MagnetDownload.h"

namespace mtt
{
	class MetadataDownload : public mttApi::MagnetDownload, public IPeerListener
	{
	public:

		MetadataDownload(Peers&);

		void start(std::function<void(Status, MetadataDownloadState&)> onUpdate, asio::io_service& io);
		void stop();

		MetadataDownloadState state;
		MetadataReconstruction metadata;

		struct EventInfo
		{
			uint8_t sourceId[20];
			enum Action { Connected, Disconnected, Request, Receive, Searching, End } action;
			uint32_t index;

			std::string toString();
		};

		std::vector<EventInfo> getEvents(size_t startIndex = 0);
		size_t getEventsCount();

		std::vector<ActivePeerInfo> getPeersInfo();

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
		virtual void progressUpdated(PeerCommunication*, uint32_t) override;

		void requestPiece(std::shared_ptr<PeerCommunication> peer);

		std::vector<EventInfo> eventLog;
		void addEventLog(uint8_t* id, EventInfo::Action action, uint32_t index);

		std::shared_ptr<ScheduledTimer> retryTimer;
		uint32_t lastActivityTime = 0;
	};
}