#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "utils/TcpAsyncServer.h"
#include "IncomingPeersListener.h"
#include "State.h"
#include <boost/filesystem.hpp>

void mtt::Core::init()
{
	for (size_t i = 0; i < 20; i++)
	{
		mtt::config::internal_.hashId[i] = (uint8_t)rand();
	}

	mtt::config::internal_.trackerKey = 1111;

	mtt::config::external.defaultDirectory = "E:\\";

	mtt::config::internal_.defaultRootHosts = { { "dht.transmissionbt.com", "6881" },{ "router.bittorrent.com" , "6881" } };

	mtt::config::external.tcpPort = mtt::config::external.udpPort = 55125;

	mtt::config::internal_.programFolderPath = ".\\data\\";
	
	mtt::config::internal_.stateFolder = "state";

	dht = std::make_shared<dht::Communication>();

	if(mtt::config::external.enableDht)
		dht->start();

	auto folderPath = mtt::config::internal_.programFolderPath + mtt::config::internal_.stateFolder;

	boost::filesystem::path dir(folderPath);
	if (!boost::filesystem::exists(dir))
	{
		boost::system::error_code ec;

		boost::filesystem::create_directory(mtt::config::internal_.programFolderPath, ec);
		boost::filesystem::create_directory(dir, ec);
	}

	listener = std::make_shared<IncomingPeersListener>([this](std::shared_ptr<TcpAsyncStream> s, const uint8_t* hash)
	{
		auto t = getTorrent(hash);
		if (t)
		{
			t->peers->add(s);
		}
	}
	);

	TorrentsList list;
	list.loadState();

	for (auto& t : list.torrents)
	{
		auto tPtr = Torrent::fromSavedState(t.name);

		if(!tPtr)
			continue;

		if (auto t = getTorrent(tPtr->hash()))
			continue;

		torrents.push_back(tPtr);
	}
}

void mtt::Core::deinit()
{
	listener->stop();
	mtt::dht::Communication::get().save();

	TorrentsList list;
	for (auto& t : torrents)
	{
		list.torrents.push_back({ t->hashString() });
		t->save();
	}

	list.saveState();
}

mtt::TorrentPtr mtt::Core::addFile(const char* filename)
{
	auto torrent = Torrent::fromFile(filename);

	if (!torrent)
		return nullptr;

	if (auto t = getTorrent(torrent->hash()))
		return t;

	saveTorrentFile(torrent);
	torrents.push_back(torrent);
	torrent->checkFiles();

	return torrent;
}

mtt::TorrentPtr mtt::Core::addMagnet(const char* magnet)
{
	auto torrent = Torrent::fromMagnetLink(magnet);

	if (!torrent)
		return nullptr;

	if (auto t = getTorrent(torrent->hash()))
		return t;

	auto onMetadataUpdate = [this, torrent](Status s, mtt::MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished)
		{
			saveTorrentFile(torrent);
			torrent->checkFiles();
		}
	};

	torrent->downloadMetadata(onMetadataUpdate);
	torrents.push_back(torrent);

	return torrent;
}

mtt::TorrentPtr mtt::Core::getTorrent(const uint8_t* hash)
{
	for (auto t : torrents)
	{
		if (memcmp(t->hash(), hash, 20) == 0)
			return t;
	}

	return nullptr;
}

void mtt::Core::saveTorrentFile(TorrentPtr t)
{
	auto folderPath = mtt::config::internal_.programFolderPath + mtt::config::internal_.stateFolder + "\\" + t->hashString() + ".torrent";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	file << t->infoFile.createTorrentFileData();
}

