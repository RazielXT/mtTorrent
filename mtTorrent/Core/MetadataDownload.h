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

		MetadataDownload(Peers&, ServiceThreadpool& service);

		void start(std::function<void(Status, MetadataDownloadState&)> onUpdate);
		void stop();

		MetadataDownloadState state;
		MetadataReconstruction metadata;

		struct EventInfo
		{
			uint8_t sourceId[20];
			enum Action { Connected, Disconnected, Request, Receive, Reject, Searching, End } action;
			uint32_t index;

			std::string toString();
		};

		std::vector<EventInfo> getEvents(size_t startIndex = 0) const;
		size_t getEventsCount() const;

	private:

		mutable std::mutex commsMutex;
		std::vector<std::shared_ptr<PeerCommunication>> activeComms;
		void evalComms();
		void removeBackup(PeerCommunication*);
		void addToBackup(std::shared_ptr<PeerCommunication>);

		std::function<void(Status, MetadataDownloadState&)> onUpdate;

		Peers& peers;

		void handshakeFinished(PeerCommunication*) override;
		void connectionClosed(PeerCommunication*, int) override;
		void messageReceived(PeerCommunication*, PeerMessage&) override;
		void extendedHandshakeFinished(PeerCommunication*, ext::Handshake&) override;
		void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView& data) override;

		void requestPiece(std::shared_ptr<PeerCommunication> peer);

		std::vector<EventInfo> eventLog;
		void addEventLog(const uint8_t* id, EventInfo::Action action, uint32_t index);

		std::shared_ptr<ScheduledTimer> retryTimer;
		Timestamp lastActivityTime = 0;

		ServiceThreadpool& service;
	};
}