#include "Test.h"
#include "utils/BencodeParser.h"
#include "utils/ServiceThreadpool.h"
#include "Configuration.h"
#include "MetadataReconstruction.h"
#include "utils/UdpAsyncClient.h"
#include "utils/Base32.h"
#include "utils/PacketHelper.h"
#include "TrackerManager.h"
#include "Storage.h"
#include "utils/TcpAsyncServer.h"

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
		//auto req = SendAsyncUdp(Addr({8,65,4,4},55555), packet.getBuffer(), service.io, std::bind(&TorrentTest::onUdpReceived, this, std::placeholders::_1, std::placeholders::_2));
#endif
	}

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
	peer.sendHandshake(Addr({ 127,0,0,1 }, 55391));

	WAITFOR(failed || (peer.state.finishedHandshake && peer.ext.state.enabled))

	if (failed || !peer.ext.utm.size)
		return;

	mtt::MetadataReconstruction metadata;
	metadata.init(peer.ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer.ext.requestMetadataPiece(mdPiece);

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
		peer->sendHandshake(nextAddr);

		WAITFOR(failed || (peer->state.finishedHandshake && peer->ext.state.enabled));

		if (failed || !peer->ext.utm.size)
			continue;
		else
			break;
	}

	mtt::MetadataReconstruction metadata;
	metadata.init(peer->ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.requestMetadataPiece(mdPiece);

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
	//std::string link = "magnet:?xt=urn:btih:4YOP2LK2CO2KYSBIVG6IOYNCY3OFMWPD&tr=udp://tracker.coppersurfer.tk:6969/announce";
	std::string link = "magnet:?xt=urn:btih:7bbed572352ab881e9788a933decd884f6ccc58f&dn=%28Ebook+Martial+Arts%29+Aikido+-+Pressure+Points.pdf&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%2Fzer0day.ch%3A1337&tr=udp%3A%2F%2Fopen.demonii.com%3A1337&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969&tr=udp%3A%2F%2Fexodus.desync.com%3A6969";
	mtt::TorrentFileInfo parsedTorrent;
	parsedTorrent.parseMagnetLink(link);

	TorrentPtr torrent = std::make_shared<mtt::LoadedTorrent>();
	torrent->info = parsedTorrent.info;

	class TListener : public TrackerListener
	{
	public:

		std::vector<Addr> peers;
		TrackerStateInfo info;

		virtual void onAnnounceResult(AnnounceResponse& resp, TorrentPtr) override
		{
			peers = resp.peers;
		}

		virtual void trackerStateChanged(TrackerStateInfo& i, TorrentPtr) override
		{
			info = i;

			WRITE_LOG("UPDATE " << i.host << " " << i.state << " next update: " << i.updateInterval);
		}

	}
	tListener;

	mtt::TrackerManager trackers;
	trackers.add(torrent, parsedTorrent.announceList, tListener);
	trackers.start(torrent);

	WAITFOR(!tListener.peers.empty());

	WRITE_LOG("PEERS: " << tListener.peers.size());

	WAITFOR(false);
}

void TorrentTest::testStorageCheck()
{
	BencodeParser file;
	if (!file.parseFile("D:\\wifi.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, f });

	mtt::Storage storage(torrent.info.pieceSize);
	storage.setPath("D:\\test");
	storage.setSelection(selection);

	auto pieces = storage.checkStoredPieces(torrent.info.pieces);
}

void TorrentTest::testStorageLoad()
{
	BencodeParser file;
	if (!file.parseFile("D:\\wifi.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, f });

	mtt::Storage storage(torrent.info.pieceSize);
	storage.setPath("D:\\test");
	storage.setSelection(selection);

	mtt::Storage outStorage(torrent.info.pieceSize);
	outStorage.setPath("D:\\test\\out");
	outStorage.setSelection(selection);

	for (uint32_t i = 0; i < torrent.info.pieces.size(); i++)
	{
		auto blocksInfo = storage.makePieceBlocksInfo(i);

		mtt::DownloadedPiece piece;
		piece.index = i;
		piece.dataSize = blocksInfo.back().begin + blocksInfo.back().length;
		piece.data.resize(piece.dataSize);

		for (auto& blockInfo : blocksInfo)
		{
			auto block = storage.getPieceBlock(blockInfo);
			
			memcpy(piece.data.data() + block.info.begin, block.data.data(), block.info.length);
		}

		outStorage.storePiece(piece);
	}

	outStorage.flush();
}

void TorrentTest::testPeerListen()
{
	BencodeParser file;
	if (!file.parseFile("D:\\wifi.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, f });

	mtt::Storage storage(torrent.info.pieceSize);
	storage.setPath("D:\\test");
	storage.setSelection(selection);

	mtt::PiecesProgress progress;
	progress.fromList(storage.checkStoredPieces(torrent.info.pieces));

	ServiceThreadpool service(2);

	std::shared_ptr<TcpAsyncStream> peerStream;
	TcpAsyncServer server(service.io, mtt::config::external.listenPort, false);
	server.acceptCallback = [&](std::shared_ptr<TcpAsyncStream> c) { peerStream = c; };
	server.listen();

	WAITFOR(peerStream);

	class MyListener : public BasicPeerListener
	{
	public:

		virtual void handshakeFinished(PeerCommunication*) override
		{
			success = true;
		}

		virtual void connectionClosed(PeerCommunication*) override
		{
			fail = true;
		}

		virtual void messageReceived(PeerCommunication*, PeerMessage& msg) override
		{
			if (msg.id == Handshake)
			{
				success = memcmp(msg.handshake.info, torrent->info.hash, 20) == 0;
			}
			else if (msg.id == Interested)
			{
				comm->setChoke(false);
			}
			else if (msg.id == Request)
			{
				PieceBlockInfo blockInfo;
				blockInfo.begin = msg.request.begin;
				blockInfo.index = msg.request.index;
				blockInfo.length = msg.request.length;

				auto block = storage->getPieceBlock(blockInfo);
				comm->sendPieceBlock(block);
			}
		}

		Storage* storage;
		PeerCommunication* comm;
		TorrentFileInfo* torrent;
		bool success = false;
		bool fail = false;
	}
	listener;

	listener.torrent = &torrent;
	listener.storage = &storage;

	PeerCommunication comm(torrent.info, listener, service.io, peerStream);

	listener.comm = &comm;

	WAITFOR(listener.success || listener.fail);

	if (listener.success)
	{
		comm.sendBitfield(progress.toBitfield());

		WAITFOR(listener.fail);
	}
}

void TorrentTest::testDhtTable()
{
	BencodeParser file;
	if (!file.parseFile("D:\\wifi.torrent"))
		return;

	auto torrent = file.getTorrentFileInfo();

	mtt::dht::Communication dhtComm(*this);
	dhtComm.findPeers(torrent.info.hash);

	WAITFOR(dhtResult.finalCount != -1);

	WAITFOR(false);
}

void TorrentTest::start()
{
	testDhtTable();
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

void TorrentTest::testGetCountry()
{
	// Create a context that uses the default paths for
	// finding CA certificates.
	ssl::context ctx(ssl::context::tlsv12);
	ctx.set_default_verify_paths();

	// Open a socket and connect it to the remote host.
	boost::asio::io_service io_service;
	ssl_socket sock(io_service, ctx);
	tcp::resolver resolver(io_service);

	const char* server = "tools.keycdn.com";
	const char* req = "https://tools.keycdn.com/geo.json?host=";
	const char* targetHost = "www.google.com";

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << req << targetHost << " HTTP/1.1\r\n";
	request_stream << "Host: " << server << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	
	auto message = sendHttpsRequest(sock, resolver, request, server);
}
