#pragma once
#include "Torrent.h"

class TcpAsyncServer;
class ServiceThreadpool;

namespace mtt
{
	namespace dht
	{
		class Communication;
	}

	class Core
	{
	public:

		std::shared_ptr<ServiceThreadpool> pool;
		std::shared_ptr<TcpAsyncServer> listener;
		std::shared_ptr<dht::Communication> dht;

		std::vector<TorrentPtr> torrents;

		void init();

		TorrentPtr addFile(const char* filename);

		TorrentPtr getTorrent(const uint8_t* hash);
	};
}
