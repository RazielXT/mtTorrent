#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "utils/TcpAsyncServer.h"
#include "IncomingPeersListener.h"

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
	//dht->start();

	listener = std::make_shared<IncomingPeersListener>([this](std::shared_ptr<TcpAsyncStream> s, const uint8_t* hash)
	{
		auto t = getTorrent(hash);
		if (t)
		{
			t->peers->add(s);
		}
	}
	);
}

mtt::TorrentPtr mtt::Core::addFile(const char* filename)
{
	auto torrent = Torrent::fromFile(filename);// "G:\\[HorribleSubs] JoJo's Bizarre Adventure - Golden Wind - 13 [720p].mkv.torrent");

	if (!torrent)
		return nullptr;

	if (auto t = getTorrent(torrent->hash()))
		return t;

	torrents.push_back(torrent);

	auto onCheckFinish = [torrent](std::shared_ptr<PiecesCheck>)
	{
		//torrent->peers->trackers.removeTrackers();

		//if (!torrent->start())
		//	return;

		//torrent->peers->connect(Addr({ 127,0,0,1 }, 31132));
	};

	torrent->checkFiles(onCheckFinish);

	return torrent;
}

mtt::TorrentPtr mtt::Core::addMagnet(const char* magnet)
{
	auto torrent = Torrent::fromMagnetLink(magnet);

	if (!torrent)
		return nullptr;

	if (auto t = getTorrent(torrent->hash()))
		return t;

	auto onMetadataUpdate = [torrent](Status s, mtt::MetadataDownloadState& state)
	{
		auto onCheckFinish = [](std::shared_ptr<PiecesCheck>)
		{
		};

		if(s == Status::Success && state.finished)
			torrent->checkFiles(onCheckFinish);
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

