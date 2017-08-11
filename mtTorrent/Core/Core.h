#pragma once
#include "TorrentDefines.h"
#include "Torrent.h"
#include "Interface.h"
#include <memory>

namespace mtt
{
	class Core
	{
	public:

		Core();

		uint32_t addTorrent(const char* filepath);
		std::shared_ptr<Torrent> getTorrent(uint32_t id);

		boost::asio::io_service io_service;
		ClientInfo client;
		std::vector<std::shared_ptr<Torrent>> loadedTorrents;
	};
}
