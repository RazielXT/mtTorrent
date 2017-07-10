#pragma once

#include "Interface2.h"

namespace mtt
{
	struct AnnounceResponse
	{
		uint32_t interval = 5 * 60;

		uint32_t leechCount = 0;
		uint32_t seedCount = 0;

		std::vector<Addr> peers;
	};

	class HttpTrackerComm
	{
	public:

		HttpTrackerComm(TorrentFileInfo* tInfo);

		AnnounceResponse announceTracker(std::string host, std::string port);

	private:

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		TorrentFileInfo* torrent;
	};
}