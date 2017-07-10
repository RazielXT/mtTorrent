#include "Test.h"
#include "BencodeParser.h"
#include "ServiceThreadpool.h"
#include "Configuration.h"

using namespace mtt;

#define WAITFOR(x) { while (!(x)) Sleep(50); }

void testInit()
{
	for (size_t i = 0; i < 20; i++)
	{
		mtt::config::internal.hashId[i] = (uint8_t)rand();
	}
}

void LocalWithTorrentFile::run()
{
	testInit();

	BencodeParser file;	
	if (!file.parseFile("D:\\test.torrent"))
		return;

	auto torrent = file.parseTorrentInfo();

	ServiceThreadpool service;
	PeerCommunication2 peer(torrent.info, *this, service.io);

	Addr address;
	address.addrBytes.insert(address.addrBytes.end(), { 127,0,0,1 });
	address.port = 55391;

	peer.start(address);

	WAITFOR(failed || peer.state.finishedHandshake)

	if (failed)
		return;
}
