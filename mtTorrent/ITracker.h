#pragma once
#include <functional>

namespace mtt
{
	class Tracker
	{
	public:

		virtual void start(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* torrent) = 0;

		virtual void announce() = 0;

		struct  
		{
			std::string hostname;

			uint32_t peers;
			uint32_t seeds;
			uint32_t announceInterval;
		}
		info;
		
		enum { Clear, Connecting, Announcing, Announced, Disconnected } state = Clear;

		std::function<void(AnnounceResponse&)> onAnnounceResult;

		TorrentFileInfo* torrent;
	};
}
