#pragma once

#include "Storage.h"
#include "PeerCommunication.h"
#include "utils/ServiceThreadpool.h"
#include "Peers.h"
#include "MetadataReconstruction.h"
#include "Interface.h"
#include "Api/MetadataDownload.h"

namespace mtt
{
	class MetadataDownload : public mttApi::MetadataDownload, public IPeerListener
	{
	public:

		MetadataDownload(Peers&, ServiceThreadpool& service);

		struct Event;
		void start(std::function<void(const Event&, MetadataReconstruction&)> onUpdate);
		void stop();

		MetadataDownloadState state;

		struct Event
		{
			enum Type { Searching, Connected, Disconnected, Request, Receive, Reject, End };
			Type type;
			uint8_t sourceId[20];
			uint32_t index;

			std::string toString() const;
		};

		std::vector<Event> getEvents(size_t startIndex = 0) const;
		size_t getEventsCount() const;

	private:

		MetadataReconstruction metadata;

		mutable std::mutex stateMutex;
		std::vector<std::shared_ptr<PeerCommunication>> activeComms;

		std::function<void(const Event&, MetadataReconstruction&)> onUpdate;

		Peers& peers;

		void handshakeFinished(PeerCommunication*) override;
		void connectionClosed(PeerCommunication*, int) override;
		void messageReceived(PeerCommunication*, PeerMessage&) override;
		void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) override;
		void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView& data) override;

		void requestPiece(std::shared_ptr<PeerCommunication> peer);

		std::vector<Event> eventLog;
		void addEventLog(const uint8_t* id, Event::Type e, uint32_t index = 0);

		std::shared_ptr<ScheduledTimer> retryTimer;
		Timestamp lastActivityTime = 0;

		ServiceThreadpool& service;
	};
}