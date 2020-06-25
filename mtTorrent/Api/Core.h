#pragma once

#include "Api/Torrent.h"
#include "Api/Listener.h"
#include <memory>
#include "Public/ModuleArray.h"

namespace mttApi
{
	using TorrentPtr = std::shared_ptr<Torrent>;

	class Core
	{
	public:

		/*
			Create core object providing access to creating/accessing and removing torrents
			Only one object should be created
		*/
		static API_EXPORT std::shared_ptr<Core> create();

		/*
			Add torrent file from path or loaded data
			If torrent already exists returns ptr and status I_AlreadyExists
		*/
		API_EXPORT std::pair<mtt::Status, TorrentPtr> addFile(const char* filename);
		API_EXPORT std::pair<mtt::Status, TorrentPtr> addFile(const uint8_t* data, size_t size);

		/*
			Add torrent file from magnet link
			If torrent already exists returns ptr and status I_AlreadyExists
			If torrent doesnt exist this automatically starts metadata download process
		*/
		API_EXPORT std::pair<mtt::Status, TorrentPtr> addMagnet(const char* magnet);

		/*
			Get torrent based on hash or nullptr if not found
		*/
		API_EXPORT TorrentPtr getTorrent(const uint8_t* hash);

		/*
			Get vector of all added torrents
		*/
		API_EXPORT std::vector<TorrentPtr> getTorrents();

		/*
			Get object of Listener, used for receiving incoming connections from other peers
		*/
		API_EXPORT std::shared_ptr<Listener> getListener();

		/*
			Remove torrent and optionally all created torrent files
		*/
		API_EXPORT mtt::Status removeTorrent(const uint8_t* hash, bool deleteFiles);

		/*
			Register to alerts about specific events
			See mtt::AlertId for list of available alerts
		*/
		API_EXPORT void registerAlerts(uint32_t alertMask);
		/*
			Get all alerts happening since last popAlerts call
		*/
		API_EXPORT std::vector<std::unique_ptr<mtt::AlertMessage>> popAlerts();
	};
}
