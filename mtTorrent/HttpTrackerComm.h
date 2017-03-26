#pragma once

#include "TorrentDefines.h"

namespace mtt
{
	class HttpTrackerComm
	{
	public:

		HttpTrackerComm(TorrentFileInfo* tInfo);

		AnnounceResponse announceTracker(std::string host, std::string port);

	private:

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		ClientInfo* client;
		TorrentFileInfo* torrent;
	};
}