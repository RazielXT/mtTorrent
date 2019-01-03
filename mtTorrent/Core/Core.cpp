#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "utils/TcpAsyncServer.h"

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

	dht = std::make_shared<dht::Communication>();
	//dht->load("");

	pool = std::make_shared<ServiceThreadpool>();
	listener = std::make_shared<TcpAsyncServer>(pool->io, mtt::config::external.tcpPort, false);
	listener->acceptCallback = [this](std::shared_ptr<TcpAsyncStream> c)
	{
		torrents.back()->peers->add(c);
	};

	listener->listen();
}

mtt::TorrentPtr mtt::Core::addFile(const char* filename)
{
	auto torrent = Torrent::fromFile(filename);// "G:\\[HorribleSubs] JoJo's Bizarre Adventure - Golden Wind - 13 [720p].mkv.torrent");

	if (!torrent)
		return nullptr;

	torrents.push_back(torrent);

	auto onCheckFinish = [torrent](std::shared_ptr<PiecesCheck>)
	{
		torrent->peers->trackers.removeTrackers();

		//if (!torrent->start())
		//	return;

		//torrent->peers->connect(Addr({ 127,0,0,1 }, 31132));
	};

	torrent->checkFiles(onCheckFinish);

	return torrent;
}

mtt::TorrentPtr mtt::Core::getTorrent(const uint8_t* hash)
{
	for (auto t : torrents)
	{
		if (memcmp(t->infoFile.info.hash, hash, 20) == 0)
			return t;
	}

	return nullptr;
}

