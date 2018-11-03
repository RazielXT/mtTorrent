#include "Core.h"

mtt::TorrentsCollection instance;

mtt::TorrentsCollection& mtt::TorrentsCollection::Get()
{
	return instance;
}

mtt::CorePtr mtt::TorrentsCollection::GetTorrent(uint8_t* id)
{
	for (auto& t : torrents)
	{
		if (memcmp(id, t->torrent.info.hash, 20) == 0)
			return t;
	}

	auto ptr = std::make_shared<mtt::TorrentCore>();
	memcpy(id, ptr->torrent.info.hash, 20);
	torrents.push_back(ptr);

	return ptr;
}

mtt::CorePtr mtt::TorrentsCollection::GetTorrent(TorrentFileInfo& info)
{
	for (auto& t : torrents)
	{
		if (memcmp(info.info.hash, t->torrent.info.hash, 20) == 0)
			return t;
	}

	auto ptr = std::make_shared<mtt::TorrentCore>();
	ptr->torrent = info;
	torrents.push_back(ptr);

	return ptr;
}
