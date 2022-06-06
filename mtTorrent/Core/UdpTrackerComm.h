#pragma once

#include "Interface.h"
#include "utils/UdpAsyncComm.h"
#include "ITracker.h"

namespace mtt
{
	class UdpTrackerComm : public Tracker
	{
	public:

		UdpTrackerComm();
		~UdpTrackerComm();

		void init(std::string host, std::string port, std::string path, TorrentPtr core) override;
		void deinit() override;

		void announce() override;

	private:

		void fail();

		UdpCommPtr udp;
		UdpRequest comm;

		bool onConnectUdpResponse(UdpRequest comm, DataBuffer* data);
		bool onAnnounceUdpResponse(UdpRequest comm, DataBuffer* data);

		struct TrackerMessage
		{
			uint32_t action = 0;
			uint32_t transaction = 0;
		};

		struct ConnectResponse : public TrackerMessage
		{
			uint64_t connectionId = 0;
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

		uint64_t connectionId = 0;
	};
}