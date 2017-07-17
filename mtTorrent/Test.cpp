#include "Test.h"
#include "BencodeParser.h"
#include "ServiceThreadpool.h"
#include "Configuration.h"
#include "MetadataReconstruction.h"
#include "UdpAsyncClient.h"
#include "utils\Base32.h"
#include "PacketHelper.h"

using namespace mtt;

#define WAITFOR(x) { while (!(x)) Sleep(50); }

void testInit()
{
	for (size_t i = 0; i < 20; i++)
	{
		mtt::config::internal.hashId[i] = (uint8_t)rand();
	}
}

LocalWithTorrentFile::LocalWithTorrentFile()
{
	testInit();
}

void LocalWithTorrentFile::metadataPieceReceived(mtt::ext::UtMetadata::Message& msg)
{
	utmMsg = msg;
}

void LocalWithTorrentFile::onUdpReceived(DataBuffer* data, PackedUdpRequest* source)
{
	if (data)
		udpResult = *data;
	else
		udpResult.push_back(6);
}

void LocalWithTorrentFile::testAsyncUdpRequest()
{
	ServiceThreadpool service;

	const char* dhtRoot = "dht.transmissionbt.com";
	const char* dhtRootPort = "6881";

	std::string targetIdBase32 = "T323KFN5XLZAZZO2NDNCYX7OBMQTUV6U"; // "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);
	const char* clientId = "mt02";
	uint16_t transactionId = 54535;

	PacketBuilder packet(104);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal.hashId, 20);
	packet.add("9:info_hash20:", 14);
	packet.add(targetId.data(), 20);
	packet.add("e1:q9:get_peers1:v4:", 20);
	packet.add(clientId, 4);
	packet.add("1:t2:", 5);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	{
		auto req = SendAsyncUdp(dhtRoot, dhtRootPort, true, packet.getBuffer(), service.io, std::bind(&LocalWithTorrentFile::onUdpReceived, this, std::placeholders::_1, std::placeholders::_2));

		WAITFOR(!udpResult.empty());

		req->client.close();
	}
	
	if (udpResult.size() == 1)
		return;

	service.stop();
}

void LocalWithTorrentFile::testMetadataReceive()
{
	BencodeParser file;	
	if (!file.parseFile("D:\\test.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	ServiceThreadpool service;

	PeerCommunication2 peer(torrent.info, *this, service.io);
	peer.start(Addr({ 127,0,0,1 }, 55391));

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

void LocalWithTorrentFile::testAsyncDhtGetPeers()
{
	ServiceThreadpool service;
	mtt::dht::Communication dht(*this);

	std::string targetIdBase32 = "T323KFN5XLZAZZO2NDNCYX7OBMQTUV6U"; // "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);

	dht.findPeers((uint8_t*)targetId.data());

	WAITFOR(dhtResult.finalCount != -1 || !dhtResult.values.empty());

	std::cout << dhtResult.finalCount;

	if (dhtResult.finalCount == 0)
		return;

	TorrentInfo info;
	memcpy(info.hash, targetId.data(), 20);
	PeerCommunication2 peer(info, *this, service.io);
	peer.start(dhtResult.values[0]);

	WAITFOR(failed || peer.state.finishedHandshake);

	WAITFOR(failed || peer.ext.utm.size);

	if (failed)
		return;

	if (!peer.ext.utm.size)
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

uint32_t LocalWithTorrentFile::onFoundPeers(uint8_t* hash, std::vector<Addr>& values)
{
	std::lock_guard<std::mutex> guard(dhtResult.resultMutex);

	uint32_t count = 0;
	for (auto& v : values)
	{
		bool found = false;

		for (auto& old : dhtResult.values)
		{
			if (old == v)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			dhtResult.values.push_back(v);
			count++;
		}
	}

	return count;
}

void LocalWithTorrentFile::findingPeersFinished(uint8_t* hash, uint32_t count)
{
	dhtResult.finalCount = count;
}
