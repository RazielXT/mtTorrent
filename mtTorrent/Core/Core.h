#pragma once
#include "Torrent.h"

namespace mtt
{
	namespace dht
	{
		class Communication;
	}

	class Core
	{
	public:

		std::shared_ptr<dht::Communication> dht;

		TorrentPtr torrent;

		void init();

		void start();
	};
}
