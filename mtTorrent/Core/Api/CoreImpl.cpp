#pragma once

#include "Core.h"
#include "IncomingPeersListener.h"

std::shared_ptr<mttApi::Core> mttApi::Core::create()
{
	class CorePtr : public mtt::Core
	{
	public:
		~CorePtr()
		{
			deinit();
		}
	};

	auto core = std::make_shared<CorePtr>();
	core->init();

	return core;
}

std::vector<mttApi::TorrentPtr> mttApi::Core::getTorrents()
{
	auto core = static_cast<mtt::Core*>(this);

	std::vector<mttApi::TorrentPtr> out;
	for (auto t = core->torrents.rbegin(); t != core->torrents.rend(); t++)
	{
		out.push_back(*t);
	}

	return out;
}

std::pair<mtt::Status, mttApi::TorrentPtr> mttApi::Core::addFile(const char* filename)
{
	return static_cast<mtt::Core*>(this)->addFile(filename);
}

std::pair<mtt::Status, mttApi::TorrentPtr> mttApi::Core::addFile(const uint8_t* data, size_t size)
{
	return static_cast<mtt::Core*>(this)->addFile(data, size);
}

std::pair<mtt::Status, mttApi::TorrentPtr> mttApi::Core::addMagnet(const char* magnet)
{
	return static_cast<mtt::Core*>(this)->addMagnet(magnet);
}

mttApi::TorrentPtr mttApi::Core::getTorrent(const uint8_t* hash)
{
	return static_cast<mtt::Core*>(this)->getTorrent(hash);
}

mttApi::TorrentPtr mttApi::Core::getTorrent(const char* hash)
{
	return static_cast<mtt::Core*>(this)->getTorrent(hash);
}

std::shared_ptr<mttApi::Listener> mttApi::Core::getListener()
{
	return static_cast<mtt::Core*>(this)->listener;
}

mtt::Status mttApi::Core::removeTorrent(const uint8_t* hash, bool deleteFiles)
{
	return static_cast<mtt::Core*>(this)->removeTorrent(hash, deleteFiles);
}

mtt::Status mttApi::Core::removeTorrent(const char* hash, bool deleteFiles)
{
	return static_cast<mtt::Core*>(this)->removeTorrent(hash, deleteFiles);
}

void mttApi::Core::registerAlerts(uint32_t alertMask)
{
	static_cast<mtt::Core*>(this)->alerts.registerAlerts(alertMask);
}

std::vector<std::unique_ptr<mtt::AlertMessage>> mttApi::Core::popAlerts()
{
	return static_cast<mtt::Core*>(this)->alerts.popAlerts();
}
