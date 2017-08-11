#include "Core.h"

mtt::ClientInfo* clInfo = nullptr;

mtt::ClientInfo* mtt::getClientInfo()
{
	return clInfo;
}

mtt::Core::Core()
{
	srand((unsigned int)time(NULL));

	{
		size_t hashLen = strlen(MT_HASH_NAME);
		memcpy(client.hashId, MT_HASH_NAME, hashLen);

		for (size_t i = hashLen; i < 20; i++)
		{
			client.hashId[i] = static_cast<uint8_t>(rand() % 255);
		}
	}

	client.key = static_cast<uint32_t>(rand());

	client.network.io_service = &io_service;

	clInfo = &client;

	client.settings.outDirectory = "D:\\";
}

uint32_t mtt::Core::addTorrent(const char* filepath)
{
	auto t = std::make_shared<Torrent>(filepath);

	if(t->id)
		loadedTorrents.push_back(t);

	return t->id;
}

std::shared_ptr<mtt::Torrent> mtt::Core::getTorrent(uint32_t id)
{
	for (auto t : loadedTorrents)
		if (t->id == id)
			return t;

	return nullptr;
}

