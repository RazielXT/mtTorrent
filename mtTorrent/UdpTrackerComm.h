#pragma once

#include "HttpTrackerComm.h"

namespace mtt
{
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

	class UdpTrackerComm
	{
	public:

		UdpTrackerComm(TorrentFileInfo* tInfo);

		AnnounceResponse announceTracker(std::string host, std::string port);

	private:

		DataBuffer createConnectRequest();
		ConnectResponse getConnectResponse(DataBuffer buffer);

		DataBuffer createAnnounceRequest(ConnectResponse& response);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		bool validResponse(TrackerMessage& resp);
		TrackerMessage lastMessage;

		uint32_t connectionId;
		TorrentFileInfo* torrent;
	};
}