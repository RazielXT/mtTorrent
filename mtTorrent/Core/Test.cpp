#include "Test.h"
#include "utils/TorrentFileParser.h"
#include "utils/ServiceThreadpool.h"
#include "Configuration.h"
#include "utils/UdpAsyncWriter.h"
#include "utils/Base32.h"
#include "utils/PacketHelper.h"
#include "TrackerManager.h"
#include "Storage.h"
#include "utils/TcpAsyncServer.h"
#include "utils/BencodeParser.h"
#include "Torrent.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Uploader.h"
#include "utils/HexEncoding.h"

using namespace mtt;

#define WAITFOR(x) { while (!(x)) Sleep(50); }
#define WAITFOR2(x, y) { while (!(x)) { y; Sleep(50);} }

#define TEST_LOG(x) WRITE_LOG(LogTypeTest, x)

void testInit()
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

TorrentTest::TorrentTest()
{
	testInit();
}

void TorrentTest::metadataPieceReceived(PeerCommunication*, mtt::ext::UtMetadata::Message& msg)
{
	utmMsg = msg;
}

void TorrentTest::testAsyncDhtUdpRequest()
{
	ServiceThreadpool service;

	std::string dhtRoot = "dht.transmissionbt.com";
	std::string dhtRootPort = "6881";

	std::string targetIdBase32 = "T323KFN5XLZAZZO2NDNCYX7OBMQTUV6U"; // "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);
	const char* clientId = "mt02";
	uint16_t transactionId = 54535;

	PacketBuilder packet(104);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::internal_.hashId, 20);
	packet.add("9:info_hash20:", 14);
	packet.add(targetId.data(), 20);
	packet.add("e1:q9:get_peers1:v4:", 20);
	packet.add(clientId, 4);
	packet.add("1:t2:", 5);
	packet.add(reinterpret_cast<char*>(&transactionId), 2);
	packet.add("1:y1:qe", 7);

	bool responded = false;
	auto udp = UdpAsyncComm::Get();
	udp->setBindPort(55466);

	auto respFunc = [&responded](UdpRequest r, DataBuffer* d)
	{
		responded = true;

		if (d)
		{
			BencodeParser parser;
			parser.parse(d->data(), d->size());
		}

		return true;
	};

	auto req = udp->sendMessage(packet.getBuffer(), dhtRoot, dhtRootPort, respFunc, false);

	WAITFOR(responded);
}

void TorrentTest::connectionClosed(PeerCommunication*, int)
{
	failed = true;
}

void TorrentTest::testAsyncDhtGetPeers()
{
	ServiceThreadpool service;
	mtt::dht::Communication dht;

	//ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J	boku 26
	//6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I	owarimonogagtari batch
	//56VYAWGYUTF7ETZZRB45C6FKJSKVBLRD	mushishi s2 22 rare
	std::string targetIdBase32 = "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J"; 
	auto targetId = base32decode(targetIdBase32);

	dht.findPeers((uint8_t*)targetId.data(), this);

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
			TEST_LOG("NO ACTIVE PEERS, RECEIVED: " << dhtResult.finalCount);
			return;
		}

		TEST_LOG("PEER " << nextPeerIdx);
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
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG(info.files[0].path[0]);
	}
}

std::string GetClipboardText()
{
	std::string text;
	if (OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData)
		{
			if (char* pszText = static_cast<char*>(GlobalLock(hData)))
				text = pszText;

			GlobalUnlock(hData);
		}

		CloseClipboard();
	}

	return text;
}

std::vector<Addr> getPeersFromTrackers(mtt::TorrentFileInfo& parsedTorrent)
{
	TorrentPtr torrent = std::make_shared<mtt::Torrent>();
	torrent->infoFile.info = parsedTorrent.info;

	std::vector<Addr> peers;
	auto trUpdate = [&peers](Status s, AnnounceResponse* r, Tracker* t)
	{
		if (s == Status::Success && r && peers.empty() && !r->peers.empty())
		{
			peers = r->peers;
			TEST_LOG(peers.size() << " peers from " << t->info.hostname << "\n");
		}
	};

	mtt::TrackerManager trackers(torrent);
	trackers.addTrackers(parsedTorrent.announceList);
	trackers.start(trUpdate);

	WAITFOR(!peers.empty());

	trackers.stop();

	return peers;
}

void TorrentTest::testTrackers()
{
	std::string link = GetClipboardText();
	mtt::TorrentFileInfo parsedTorrent;
	parsedTorrent.parseMagnetLink(link);

	auto addr = getPeersFromTrackers(parsedTorrent);

	WAITFOR(false);
}

std::string findFileByPiece(mtt::TorrentFileInfo& file, uint32_t piece)
{
	for (auto& f : file.info.files)
	{
		if (f.startPieceIndex <= piece && f.endPieceIndex >= piece)
			return f.path.back();
	}

	return "";
}

void TorrentTest::testStorageCheck()
{
	auto torrent = mtt::TorrentFileParser::parseFile("D:\\hunter.torrent");

	mtt::Storage storage(torrent.info);
	storage.setPath("D:\\Torrent");

	ServiceThreadpool pool;

	bool finished = false;
	auto onFinish = [&](std::shared_ptr<PiecesCheck>) { finished = true; TEST_LOG("Finished"); };

	auto checking = storage.checkStoredPiecesAsync(torrent.info.pieces, pool.io, onFinish);

	WAITFOR2(finished, TEST_LOG(checking->piecesChecked << "/" << checking->piecesCount));

	WAITFOR(false);
}

void TorrentTest::testStorageLoad()
{
	auto torrent = mtt::TorrentFileParser::parseFile("D:\\wifi.torrent");

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, f });

	mtt::Storage storage(torrent.info);
	storage.setPath("D:\\test");
	storage.preallocateSelection(selection);

	mtt::Storage outStorage(torrent.info);
	outStorage.setPath("D:\\test\\out");
	outStorage.preallocateSelection(selection);

	mtt::DownloadedPiece piece;

	for (uint32_t i = 0; i < torrent.info.pieces.size(); i++)
	{
		auto blocksInfo = torrent.info.makePieceBlocksInfo(i);
		piece.init(i, torrent.info.pieceSize, (uint32_t)blocksInfo.size());

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
	auto torrent = mtt::TorrentFileParser::parseFile("D:\\wifi.torrent");

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, f });

	mtt::Storage storage(torrent.info);
	storage.setPath("D:\\test");
	storage.preallocateSelection(selection);

	mtt::PiecesProgress progress;
	progress.fromList(storage.checkStoredPieces(torrent.info.pieces));

	ServiceThreadpool service(2);

	std::shared_ptr<TcpAsyncStream> peerStream;
	TcpAsyncServer server(service.io, mtt::config::external.tcpPort, false);
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

		virtual void connectionClosed(PeerCommunication*, int) override
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

	PeerCommunication comm(torrent.info, listener, peerStream);

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
	std::string saveFilePath = ".\\dht.sv";
	std::string saveFile;

	{
		std::ifstream inFile(saveFilePath, std::ios_base::binary | std::ios_base::in);

		if (inFile)
			saveFile = std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
	}

	mtt::BencodeParser::Object obj;
	auto s = sizeof(obj);

	auto torrent = mtt::TorrentFileParser::parseFile("D:\\hunter.torrent");

	mtt::dht::Communication dhtComm;
	dhtComm.load(saveFile);

	//ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J	boku 26
	//6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I	owarimonogagtari batch
	//56VYAWGYUTF7ETZZRB45C6FKJSKVBLRD	mushishi s2 22 rare
	//80E5879D864C7FF12C3232C890BF971E254C6503 less rare
	//47e2bac3c75e738f17eaeffae949c80c61e7e675 very rare fear
	TorrentFileInfo info;
	info.parseMagnetLink("80E5879D864C7FF12C3232C890BF971E254C6503");
	auto targetId = info.info.hash;
	//dhtComm.findNode(targetId);

	dhtComm.findPeers(targetId, this);
	WAITFOR(dhtResult.finalCount != -1);

	//dhtComm.findNode(mtt::config::internal.hashId);
	//Sleep(25000);

	saveFile = dhtComm.save();

	{
		std::ofstream out(saveFilePath, std::ios_base::binary);
		out << saveFile;
	}

	WAITFOR(false);
}

void TorrentTest::testTorrentFileSerialization()
{
	auto torrent = mtt::TorrentFileParser::parseFile("D:\\hunter.torrent");
	auto file = torrent.createTorrentFileData();
	auto torrentOut = mtt::TorrentFileParser::parse(file.data(), file.size());
	bool ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = mtt::TorrentFileParser::parseFile("D:\\folk.torrent");
	file = torrent.createTorrentFileData();
	torrentOut = mtt::TorrentFileParser::parse(file.data(), file.size());
	ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = mtt::TorrentFileParser::parseFile("D:\\wifi.torrent");
	file = torrent.createTorrentFileData();
	std::ofstream fileOut("D:\\wifi2.torrent", std::ios_base::binary);
	fileOut.write((const char*)file.data(), file.size());
	torrentOut = mtt::TorrentFileParser::parse(file.data(), file.size());
	ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = mtt::TorrentFileParser::parseFile("D:\\Shoujo.torrent");
	file = torrent.createTorrentFileData();
	torrentOut = mtt::TorrentFileParser::parse(file.data(), file.size());
	ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;
}

void TorrentTest::bigTestGetTorrentFileByLink()
{
	std::string link = "magnet:?xt=urn:btih:5AYWR2LK3ORHWRI2Y6BVBUX6QAUF2SDP&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";//GetClipboardText();
	mtt::TorrentFileInfo parsedTorrent;
	parsedTorrent.parseMagnetLink(link);

	std::vector<Addr> addr;// = getPeersFromTrackers(parsedTorrent);
	addr.push_back(Addr({ 185, 21, 217, 75 }, 55239));
	addr.push_back(Addr({ 78,57,164,108 }, 8999));
	addr.push_back(Addr({ 88,206,177,148 }, 63500));
	ServiceThreadpool service;

	std::vector<std::shared_ptr<PeerCommunication>> peers;

	for(auto& a : addr)
	{
		auto peer = std::make_shared<PeerCommunication>(parsedTorrent.info, *this, service.io);
		failed = false;
		peer->sendHandshake(a);

		WAITFOR(failed || (peer->state.finishedHandshake && peer->ext.state.enabled));

		if (!failed)
			peers.push_back(peer);
	}

	failed = peers.empty();

	mtt::MetadataReconstruction metadata;
	metadata.init(peers.front()->ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peers.front()->ext.requestMetadataPiece(mdPiece);

		WAITFOR(failed || !utmMsg.metadata.empty())

			if (failed || utmMsg.id != mtt::ext::UtMetadata::Data)
				return;

		metadata.addPiece(utmMsg.metadata, utmMsg.piece);
		utmMsg.metadata.clear();
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG(info.files[0].path[0]);

		mtt::Storage storage(info);
		storage.setPath("E:\\Torrent");

		DownloadSelection selection;
		for (auto&f : info.files)
			selection.files.push_back({ false, f });

		PiecesProgress piecesTodo;
		piecesTodo.select(selection);

		peers.front()->setInterested(true);

		DownloadedPiece pieceTodo;
		std::mutex pieceMtx;
		bool finished = false;
		uint32_t finishedPieces = 0;
		onPeerMsg = [&](mtt::PeerMessage& msg)
		{
			std::lock_guard<std::mutex> guard(pieceMtx);
			if (msg.id == Piece)
			{
				pieceTodo.addBlock(msg.piece);

				if (pieceTodo.remainingBlocks == 0)
				{
					storage.storePiece(pieceTodo);
					piecesTodo.addPiece(pieceTodo.index);
					finished = true;
					finishedPieces++;
				}
			}
		};

		while(finishedPieces < info.pieces.size())
		{
			auto p = piecesTodo.firstEmptyPiece();

			auto blocks = info.makePieceBlocksInfo(p);
			TEST_LOG("Requesting idx " << p);

			finished = false;
			pieceTodo.init(p, info.getPieceSize(p), info.getPieceBlocksCount(p));

			for (auto& b : blocks)
			{
				peers.front()->requestPieceBlock(b);
			}

			WAITFOR(finished);
		}
	}
	WAITFOR(false);
}

void TorrentTest::testMetadataReceive()
{
	auto torrent = mtt::TorrentFileParser::parseFile("D:\\wifi.torrent");

	ServiceThreadpool service;

	PeerCommunication peer(torrent.info, *this, service.io);
	peer.sendHandshake(Addr({ 127,0,0,1 }, 31132));

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
		utmMsg.metadata.clear();
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG(info.files[0].path[0]);
	}
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

	TEST_LOG("DHT received values count :" << count)

		return count;
}

void TorrentTest::findingPeersFinished(uint8_t* hash, uint32_t count)
{
	TEST_LOG("DHT final values count :" << count)

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

void idealGeneralTest()
{
	TorrentPtr torrent = Torrent::fromFile("G:\\giant.torrent");

	if (!torrent)
		return;

	auto blocks = torrent->infoFile.info.makePieceBlocksInfo(0);

	torrent->start();

	while (!torrent->finished())
	{
		TEST_LOG(torrent->currentProgress() << "%\n");
		TEST_LOG("Connected: " << torrent->peers->connectedCount() << ", all: " << torrent->peers->receivedCount() << "\n");
		Sleep(1000);
	}
}

const char* magnetLinkWithTrackers = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";
const char* magnetLite = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=udp://tracker.coppersurfer.tk:6969/announce";

void TorrentTest::idealMagnetLinkTest()
{
	bool finished = false;
	auto onMetadataUpdate = [&finished](Status s, mtt::MetadataDownloadState& state)
	{
		if (state.finished)
			finished = true;

		TEST_LOG("UTM status " << (int)s << " from " << hexToString(state.source, 20) << ", parts: " << state.receivedParts << "/" << state.partsCount);
	};

	TorrentPtr torrent = Torrent::fromMagnetLink(magnetLite, onMetadataUpdate);

	if (!torrent)
		return;

	WAITFOR(finished);

	TEST_LOG(torrent->name() << "\n");

	torrent->stop();
}

void idealPeersRetrievalTest()
{
	TorrentPtr torrent = Torrent::fromFile("filepath");

	if (!torrent)
		return;

	auto onPeersUpdate = [&torrent](Status s, const std::string& source)
	{
		TEST_LOG("Peers update from " << source << "with status " << (int)s << "\n");

		if (s == Status::Success)
		{
			auto info = torrent->peers->getSourceInfo(source);

			TEST_LOG(source << " seed/leech: " << info.seeds << "/" << info.leechers << "\n");
		}
	};

	torrent->peers->start(onPeersUpdate, nullptr);

	WAITFOR(false);
}

void idealTorrentStateTest()
{
	TorrentPtr torrent = Torrent::fromFile("G:\\test.torrent");

	if (!torrent|| torrent->name().empty())
		return;

	bool finished = false;
	auto onCheckFinish = [&finished](std::shared_ptr<PiecesCheck>)
	{
		finished = true;
	};

	torrent->checkFiles(onCheckFinish);

	WAITFOR(finished);

	auto progress = torrent->currentProgress();
}

void idealLocalTest()
{
	TorrentPtr torrent = Torrent::fromFile("G:\\test.torrent");

	if (!torrent)
		return;

	bool checked = false;
	auto onCheckFinish = [&checked](std::shared_ptr<PiecesCheck>)
	{
		checked = true;
	};

	torrent->checkFiles(onCheckFinish);

	WAITFOR(checked);

	torrent->peers->trackers.removeTrackers();

	if (!torrent->start())
		return;

	torrent->peers->connect(Addr({ 127,0,0,1 }, 31132));

	while (!torrent->finished())
	{
		TEST_LOG("Progress: " << torrent->currentProgress() << "(" << torrent->downloaded()/(1024.f*1024) << "MB) (" << torrent->downloadSpeed()/(1024.f * 1024) << " MBps)");
		Sleep(1000);
	}

	TEST_LOG("Finished");
}

void TorrentTest::start()
{
	testPeerListen();
}
