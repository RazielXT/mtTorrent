#pragma once

#include "Interface.h"
#include "UdpAsyncClient.h"
#include "ITracker.h"

namespace mtt
{
	class UdpTrackerComm : public Tracker
	{
	public:

		UdpTrackerComm();

		virtual void start(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* torrent) override;

		virtual void announce() override;

	private:

		UdpRequest udpComm;
		void onConnectUdpResponse(DataBuffer* data, PackedUdpRequest* source);
		void onAnnounceUdpResponse(DataBuffer* data, PackedUdpRequest* source);

		struct TrackerMessage
		{
			uint32_t action;
			uint32_t transaction;
		};

		struct ConnectResponse : public TrackerMessage
		{
			uint64_t connectionId;
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
		ConnectResponse getConnectResponse(DataBuffer buffer);

		DataBuffer createAnnounceRequest();
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		bool validResponse(TrackerMessage& resp);

		TrackerMessage lastMessage;

		uint64_t connectionId;
	};
}