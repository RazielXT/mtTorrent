#pragma once

#include "Interface.h"
#include "utils\UdpAsyncComm.h"
#include "ITracker.h"

namespace mtt
{
	class UdpTrackerComm : public Tracker
	{
	public:

		UdpTrackerComm();
		~UdpTrackerComm();

		virtual void init(std::string host, std::string port, TorrentPtr core) override;

		virtual void announce() override;

	private:

		void fail();

		UdpCommPtr udp;
		UdpRequest comm;

		bool onConnectUdpResponse(UdpRequest comm, DataBuffer* data);
		bool onAnnounceUdpResponse(UdpRequest comm, DataBuffer* data);

		struct TrackerMessage
		{
			uint32_t action;
			uint32_t transaction;
		};

		struct ConnectResponse : public TrackerMessage
		{
			uint64_t connectionId;
		};

		struct UdpAnnounceResponse : public AnnounceResponse
		{
			TrackerMessage udp;
		};

		enum Action
		{
			Connnect = 0,
			Announce,
			Scrape,
			Error
		};

		enum Event
		{
			None = 0,
			Completed,
			Started,
			Stopped
		};

		DataBuffer createConnectRequest();
		ConnectResponse getConnectResponse(DataBuffer& buffer);
		void connect();

		DataBuffer createAnnounceRequest();
		UdpAnnounceResponse getAnnounceResponse(DataBuffer& buffer);

		bool validResponse(TrackerMessage& resp);

		TrackerMessage lastMessage;

		uint64_t connectionId;
	};
}