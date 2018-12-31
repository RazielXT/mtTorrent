#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"

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
}

void mtt::Core::start()
{
	init();

	torrent = Torrent::fromFile("G:\\hunter.torrent");

	if (!torrent)
		return;

	auto onCheckFinish = [this](std::shared_ptr<PiecesCheck>)
	{
		torrent->peers->trackers.removeTrackers();

		//if (!torrent->start())
		//	return;

		//torrent->peers->connect(Addr({ 127,0,0,1 }, 31132));
	};

	torrent->checkFiles(onCheckFinish);
}

