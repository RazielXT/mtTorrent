#pragma once

#include "Interface.h"

namespace Torrent
{
	class HttpTrackerComm
	{
	public:

		HttpTrackerComm();
		void setInfo(ClientInfo* client, TorrentInfo* tInfo);

		AnnounceResponse announceTracker(std::string host, std::string port);

	private:

		DataBuffer createAnnounceRequest(std::string host, std::string port);
		AnnounceResponse getAnnounceResponse(DataBuffer buffer);

		ClientInfo* client;
		TorrentInfo* torrent;
	};
}