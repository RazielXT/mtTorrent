#pragma once

#include "Interface.h"
#include "utils\UdpAsyncMgr.h"
#include "ITracker.h"

namespace mtt
{
	class UdpTrackerComm : public Tracker
	{
	public:

		UdpTrackerComm(UdpAsyncMgr& udp);

		virtual void init(std::string host, std::string port, boost::asio::io_service& io, TorrentPtr torrent) override;

		virtual void announce() override;

	private:

		void fail();

		UdpAsyncMgr& udp;
		UdpConnection comm;

		bool onConnectUdpResponse(UdpConnection comm, DataBuffer* data);
		bool onAnnounceUdpResponse(UdpConnection comm, DataBuffer* data);

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