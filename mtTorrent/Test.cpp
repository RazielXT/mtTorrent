#include "Test.h"
#include "BencodeParser.h"
#include "ServiceThreadpool.h"
#include "Configuration.h"
#include "MetadataReconstruction.h"
#include "UdpAsyncClient.h"
#include "utils\Base32.h"
#include "PacketHelper.h"
#include "TrackerManager.h"

using namespace mtt;

#define WAITFOR(x) { while (!(x)) Sleep(50); }

void testInit()
{
	for (size_t i = 0; i < 20; i++)
	{
		mtt::config::internal.hashId[i] = (uint8_t)rand();
	}
}

TorrentTest::TorrentTest()
{
	testInit();
}

void TorrentTest::metadataPieceReceived(PeerCommunication*, mtt::ext::UtMetadata::Message& msg)
{
	utmMsg = msg;
}

void TorrentTest::onUdpReceived(DataBuffer* data, PackedUdpRequest* source)
{
	if (data)
		udpResult = *data;
	else
		udpResult.push_back(6);
}

#define OFFICE
void TorrentTest::testAsyncUdpRequest()
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
#ifndef OFFICE
		auto req = SendAsyncUdp(dhtRoot, dhtRootPort, true, packet.getBuffer(), service.io, std::bind(&TorrentTest::onUdpReceived, this, std::placeholders::_1, std::placeholders::_2));
#else
		auto req = SendAsyncUdp(Addr({8,65,4,4},55555), packet.getBuffer(), service.io, std::bind(&TorrentTest::onUdpReceived, this, std::placeholders::_1, std::placeholders::_2));
#endif


		WAITFOR(!udpResult.empty());
	}
	
	if (udpResult.size() == 1)
		return;

	service.stop();
}

void TorrentTest::testMetadataReceive()
{
	BencodeParser file;	
	if (!file.parseFile("D:\\test.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	ServiceThreadpool service;

	PeerCommunication peer(torrent.info, *this, service.io);
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
		WRITE_LOG(info.files[0].path[0]);
	}
}

void TorrentTest::connectionClosed(PeerCommunication*)
{
	failed = true;
}

void TorrentTest::testAsyncDhtGetPeers()
{
	ServiceThreadpool service;
	mtt::dht::Communication dht(*this);

	//ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J	boku 26
	//6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I	owarimonogagtari batch
	//56VYAWGYUTF7ETZZRB45C6FKJSKVBLRD	mushishi s2 22 rare
	std::string targetIdBase32 = "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J"; 
	auto targetId = base32decode(targetIdBase32);

	dht.findPeers((uint8_t*)targetId.data());

	WAITFOR(dhtResult.finalCount != -1 || !dhtResult.values.empty());

	if (dhtResult.finalCount == 0)
		return;

	//dht.stopFindingPeers((uint8_t*)targetId.data());

	TorrentInfo info;
	memcpy(info.hash, targetId.data(), 20);
	std::shared_ptr<PeerCommunication> peer;
	uint32_t nextPeerIdx = 0;

	while (true)
	{
		WAITFOR(dhtResult.finalCount != -1 || nextPeerIdx < dhtResult.values.size());

		if (nextPeerIdx >= dhtResult.values.size())
		{
			WRITE_LOG("NO ACTIVE PEERS, RECEIVED: " << dhtResult.finalCount);
			return;
		}

		WRITE_LOG("PEER " << nextPeerIdx);
		peer = std::make_shared<PeerCommunication>(info, *this, service.io);
		Addr nextAddr;

		{
			std::lock_guard<std::mutex> guard(dhtResult.resultMutex);
			nextAddr = dhtResult.values[nextPeerIdx++];
		}

		failed = false;
		peer->start(nextAddr);

		WAITFOR(failed || peer->state.finishedHandshake);

		if (!peer->info.supportsExtensions())
			continue;

		WAITFOR(failed || peer->ext.state.enabled);

		if (failed)
			continue;
		else if (!peer->ext.utm.size)
			continue;
		else
			break;
	}


	mtt::MetadataReconstruction metadata;
	metadata.init(peer->ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->requestMetadataPiece(mdPiece);

		WAITFOR(failed || !utmMsg.metadata.empty());

		if (failed || utmMsg.id != mtt::ext::UtMetadata::Data)
			return;

		metadata.addPiece(utmMsg.metadata, utmMsg.piece);

		utmMsg.metadata.clear();
	}

	if (metadata.finished())
	{
		BencodeParser parse;
		auto info = parse.parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		WRITE_LOG(info.files[0].path[0]);
	}
}

void TorrentTest::testTrackers()
{
	//std::string link = "magnet:?xt=urn:btih:4YOP2LK2CO2KYSBIVG6IOYNCY3OFMWPD&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechers-paradise.org:6969/announce&tr=http://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=http://tracker.opentrackr.org:1337/announce&tr=udp://tracker.zer0day.to:1337/announce&tr=http://explodie.org:6969/announce&tr=http://p4p.arenabg.com:1337/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=http://mgtracker.org:6969/announce&tr=udp://mgtracker.org:6969/announce";
	//std::string link = "magnet:?xt=urn:btih:4YOP2LK2CO2KYSBIVG6IOYNCY3OFMWPD&tr=http://nyaa.tracker.wf:7777/announce";
	std::string link = "magnet:?xt=urn:btih:4YOP2LK2CO2KYSBIVG6IOYNCY3OFMWPD&tr=udp://tracker.coppersurfer.tk:6969/announce";

	mtt::TorrentFileInfo info;
	info.parseMagnetLink(link);

	ServiceThreadpool service;
	service.start(2);

	class TListener : public TrackerListener
	{
	public:

		std::vector<Addr> peers;

		virtual void onAnnounceResult(AnnounceResponse& resp, TorrentFileInfo*) override
		{
			peers = resp.peers;
		}
	}
	tListener;

	mtt::TrackerManager trackers(service.io, tListener);
	trackers.init(&info);
	trackers.start();

	WAITFOR(!tListener.peers.empty());

	WRITE_LOG("PEERS: " << tListener.peers.size());
}

void TorrentTest::start()
{
	testTrackers();
}

uint32_t TorrentTest::onFoundPeers(uint8_t* hash, std::vector<Addr>& values)
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

	WRITE_LOG("DHT received values count :" << count)

	return count;
}

void TorrentTest::findingPeersFinished(uint8_t* hash, uint32_t count)
{
	WRITE_LOG("DHT final values count :" << count)

	dhtResult.finalCount = count;
}
