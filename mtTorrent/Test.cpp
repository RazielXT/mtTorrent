#include "Test.h"
#include "BencodeParser.h"
#include "ServiceThreadpool.h"
#include "Configuration.h"
#include "MetadataReconstruction.h"

using namespace mtt;

#define WAITFOR(x) { while (!(x)) Sleep(50); }

void testInit()
{
	for (size_t i = 0; i < 20; i++)
	{
		mtt::config::internal.hashId[i] = (uint8_t)rand();
	}
}

void LocalWithTorrentFile::metadataPieceReceived(mtt::ext::UtMetadata::Message& msg)
{
	utmMsg = msg;
}

void LocalWithTorrentFile::run()
{
	testInit();

	BencodeParser file;	
	if (!file.parseFile("D:\\test.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	ServiceThreadpool service;
	PeerCommunication2 peer(torrent.info, *this, service.io);

	Addr address;
	address.addrBytes.insert(address.addrBytes.end(), { 127,0,0,1 });
	address.port = 8999;

	peer.start(address);

	WAITFOR(failed || peer.state.finishedHandshake)

	WAITFOR(failed || peer.ext.utm.size)

	if (failed)
		return;

	if(!peer.ext.utm.size)
		return;

	mtt::MetadataReconstruction metadata;
	metadata.init(peer.ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer.requestMetadataPiece(mdPiece);

		WAITFOR(failed || !utmMsg.metadata.empty())

		if (failed || utmMsg.id != mtt::ext::UtMetadata::Data)
			return;

		metadata.addPiece(utmMsg.metadata, utmMsg.piece);
	}

	if (metadata.finished())
	{
		BencodeParser parse;
		auto info = parse.parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		std::cout << info.files[0].path[0];
	}
}
