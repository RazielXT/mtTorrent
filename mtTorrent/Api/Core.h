#pragma once

#include "Api/Torrent.h"
#include "Api/Listener.h"
#include <memory>

namespace mttApi
{
	using TorrentPtr = std::shared_ptr<Torrent>;

	class Core
	{
	public:

		static API_EXPORT std::shared_ptr<Core> create();

		API_EXPORT TorrentPtr addFile(const char* filename);
		API_EXPORT TorrentPtr addMagnet(const char* magnet);

		API_EXPORT TorrentPtr getTorrent(const uint8_t* hash);
		API_EXPORT TorrentPtr getTorrent(const char* hash);

		API_EXPORT std::vector<TorrentPtr> getTorrents();
		API_EXPORT std::shared_ptr<Listener> getListener();

		API_EXPORT mtt::Status removeTorrent(const uint8_t* hash, bool deleteFiles);
		API_EXPORT mtt::Status removeTorrent(const char* hash, bool deleteFiles);
	};
}
