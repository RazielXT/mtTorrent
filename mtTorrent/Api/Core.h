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

		API_EXPORT std::pair<mtt::Status, TorrentPtr> addFile(const char* filename);
		API_EXPORT std::pair<mtt::Status, TorrentPtr> addFile(const uint8_t* data, size_t size);
		API_EXPORT std::pair<mtt::Status, TorrentPtr> addMagnet(const char* magnet);

		API_EXPORT TorrentPtr getTorrent(const uint8_t* hash);
		API_EXPORT TorrentPtr getTorrent(const char* hash);

		API_EXPORT std::vector<TorrentPtr> getTorrents();
		API_EXPORT std::shared_ptr<Listener> getListener();

		API_EXPORT mtt::Status removeTorrent(const uint8_t* hash, bool deleteFiles);
		API_EXPORT mtt::Status removeTorrent(const char* hash, bool deleteFiles);

		API_EXPORT void registerAlerts(uint32_t alertMask);
		API_EXPORT std::vector<std::unique_ptr<mtt::AlertMessage>> popAlerts();
	};
}
