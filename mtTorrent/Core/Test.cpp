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
#include "FileTransfer.h"
#include "utils/HexEncoding.h"
#include "AlertsManager.h"
#include "Utp/UtpManager.h"
#include <iostream>
#include <chrono>

using namespace mtt;

#define WAITFOR(x) { while (!(x)) std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
#define WAITFOR2(x, y) { while (!(x)) { y; std::this_thread::sleep_for(std::chrono::milliseconds(50));} }

#define TEST_LOG(x) WRITE_LOG(LogTypeTest, x)

void testInit()
{
	auto filesSettings = mtt::config::getExternal().files;
	filesSettings.defaultDirectory = "./";
	mtt::config::setValues(filesSettings);

	auto connectionSettings = mtt::config::getExternal().connection;
	connectionSettings.tcpPort = connectionSettings.udpPort = 55125;
	mtt::config::setValues(connectionSettings);
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
	ServiceThreadpool service(1);

	std::string dhtRoot = "dht.transmissionbt.com";
	std::string dhtRootPort = "6881";

	std::string targetIdBase32 = "T323KFN5XLZAZZO2NDNCYX7OBMQTUV6U"; // "ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J";
	auto targetId = base32decode(targetIdBase32);
	const char* clientId = "mt02";
	uint16_t transactionId = 54535;

	PacketBuilder packet(104);
	packet.add("d1:ad2:id20:", 12);
	packet.add(mtt::config::getInternal().hashId, 20);
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
	ServiceThreadpool service(1);
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
	/*if (OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData)
		{
			if (char* pszText = static_cast<char*>(GlobalLock(hData)))
				text = pszText;

			GlobalUnlock(hData);
		}

		CloseClipboard();
	}*/

	return text;
}

std::vector<Addr> getPeersFromTrackers(mtt::TorrentFileInfo& parsedTorrent)
{
	TorrentPtr torrent = std::make_shared<mtt::Torrent>();
	torrent->infoFile.info = parsedTorrent.info;

	std::vector<Addr> peers;
	auto trUpdate = [&peers](Status s, const AnnounceResponse* r, Tracker* t)
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

static TorrentFileInfo parseTorrentFile(const char* filepath)
{
	std::ifstream file(filepath, std::ios_base::binary);

	if (!file.good())
		return TorrentFileInfo{};

	DataBuffer buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return mtt::TorrentFileParser::parse(buffer.data(), buffer.size());
}

void TorrentTest::testStorageCheck()
{
	auto torrent = parseTorrentFile("D:\\hunter.torrent");

	mtt::Storage storage(torrent.info);
	storage.setPath("D:\\Torrent");

	ServiceThreadpool pool(1);

	bool finished = false;
	auto onFinish = [&](std::shared_ptr<PiecesCheck>) { finished = true; TEST_LOG("Finished"); };

// 	auto checking = storage.checkStoredPiecesAsync(torrent.info.pieces, pool.io, onFinish);
// 
// 	WAITFOR2(finished, TEST_LOG(checking->piecesChecked << "/" << checking->piecesCount));
// 
// 	WAITFOR(false);
}

void TorrentTest::testStorageLoad()
{
	auto torrent = parseTorrentFile("C:\\test\\wifi.torrent");

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, Priority::Normal, f });

	mtt::Storage storage(torrent.info);
	storage.setPath("C:\\test");
	storage.preallocateSelection(selection);

	mtt::Storage outStorage(torrent.info);
	outStorage.setPath("C:\\test\\out");
	outStorage.preallocateSelection(selection);

	mtt::DownloadedPiece piece;
	DataBuffer buffer;

	for (uint32_t i = 0; i < torrent.info.pieces.size(); i++)
	{
		auto blocksInfo = torrent.info.makePieceBlocksInfo(i);
		piece.init(i, torrent.info.pieceSize, (uint32_t)blocksInfo.size());

		for (auto& blockInfo : blocksInfo)
		{		
			storage.loadPieceBlock(blockInfo, buffer);		
			memcpy(piece.data.data() + blockInfo.begin, buffer.data(), buffer.size());
		}

		outStorage.storePiece(piece);
	}

	mtt::PiecesCheck checkResults;
	outStorage.checkStoredPieces(checkResults, torrent.info.pieces);
	for (auto r : checkResults.pieces)
	{
		if (r != 1)
		{
			std::cout << "Invalid piece copy!" << std::endl;
		}
	}
}

void TorrentTest::testPeerListen()
{
	auto torrent = parseTorrentFile("D:\\wifi.torrent");

	DownloadSelection selection;
	for (auto&f : torrent.info.files)
		selection.files.push_back({ true, Priority::Normal, f });

	mtt::Storage storage(torrent.info);
	storage.setPath("D:\\test");
	storage.preallocateSelection(selection);

	mtt::PiecesCheck check;
	storage.checkStoredPieces(check, torrent.info.pieces);

	mtt::PiecesProgress progress;
	progress.fromList(check.pieces);

	ServiceThreadpool service(2);

	std::shared_ptr<TcpAsyncStream> peerStream;
	TcpAsyncServer server(service.io, mtt::config::getExternal().connection.tcpPort, false);
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
				PieceBlock block;
				block.info.begin = msg.request.begin;
				block.info.index = msg.request.index;
				block.info.length = msg.request.length;

				DataBuffer buffer;
				storage->loadPieceBlock(block.info, buffer);

				block.buffer = buffer;
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

	PeerCommunication comm(torrent.info, listener);
	comm.fromStream(peerStream, {nullptr, 0});
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
	mtt::BencodeParser::Object obj;
	auto s = sizeof(obj);

	auto torrent = parseTorrentFile("D:\\hunter.torrent");

	mtt::dht::Communication dhtComm;
	dhtComm.load();

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

	dhtComm.save();

	WAITFOR(false);
}

void TorrentTest::testTorrentFileSerialization()
{
	auto torrent = parseTorrentFile("D:\\hunter.torrent");
	auto file = torrent.createTorrentFileData();
	auto torrentOut = mtt::TorrentFileParser::parse((const uint8_t*)file.data(), file.size());
	bool ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = parseTorrentFile("D:\\folk.torrent");
	file = torrent.createTorrentFileData();
	torrentOut = mtt::TorrentFileParser::parse((const uint8_t*)file.data(), file.size());
	ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = parseTorrentFile("D:\\wifi.torrent");
	file = torrent.createTorrentFileData();
	std::ofstream fileOut("D:\\wifi2.torrent", std::ios_base::binary);
	fileOut.write((const char*)file.data(), file.size());
	torrentOut = mtt::TorrentFileParser::parse((const uint8_t*)file.data(), file.size());
	ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;

	torrent = parseTorrentFile("D:\\Shoujo.torrent");
	file = torrent.createTorrentFileData();
	torrentOut = mtt::TorrentFileParser::parse((const uint8_t*)file.data(), file.size());
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
	ServiceThreadpool service(1);

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
			selection.files.push_back({ false, Priority::Normal, f });

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
	auto torrent = parseTorrentFile("D:\\wifi.torrent");

	ServiceThreadpool service(1);

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

uint32_t TorrentTest::dhtFoundPeers(const uint8_t* hash, std::vector<Addr>& values)
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

void TorrentTest::dhtFindingPeersFinished(const uint8_t* hash, uint32_t count)
{
	TEST_LOG("DHT final values count :" << count)

		dhtResult.finalCount = count;
}

static TorrentPtr torrentFromFile(const char* path)
{
	std::ifstream file(path, std::ios_base::binary);

	if (!file.good())
		return nullptr;

	DataBuffer buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return Torrent::fromFile(std::move(mtt::TorrentFileParser::parse(buffer.data(), buffer.size())));
}

void idealGeneralTest()
{
	TorrentPtr torrent = torrentFromFile("G:\\giant.torrent");

	if (!torrent)
		return;

	auto blocks = torrent->infoFile.info.makePieceBlocksInfo(0);

	torrent->start();

	while (!torrent->finished())
	{
		TEST_LOG(torrent->currentProgress() << "%\n");
		TEST_LOG("Connected: " << torrent->peers->connectedCount() << ", all: " << torrent->peers->receivedCount() << "\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

const char* magnetLinkWithTrackers = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";
const char* magnetLite = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=udp://tracker.coppersurfer.tk:6969/announce";
const char* tempLink = "magnet:?xt=urn:btih:HMSXZ6DMI56OOTADFNV5R77O34WK4S65&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";
void TorrentTest::idealMagnetLinkTest()
{
	auto& alerts = mtt::AlertsManager::Get();
	alerts.registerAlerts((uint32_t)AlertCategory::Metadata);

	TorrentPtr torrent = Torrent::fromMagnetLink(tempLink);

	if (!torrent)
		return;

	torrent->downloadMetadata();

	while (true)
	{
		auto newAlerts = alerts.popAlerts();
		for (auto& a : newAlerts)
		{
			if (a->id == AlertId::MetadataUpdated)
			{
				TEST_LOG("Metadata updated\n");
			}
			else if (a->id == AlertId::MetadataFinished)
			{
				TEST_LOG("Metadata finished\n");
				break;
			}
		}
	}

	TEST_LOG(torrent->name() << "\n");

	torrent->stop();
}

void idealPeersRetrievalTest()
{
	TorrentPtr torrent = torrentFromFile("filepath");

	if (!torrent)
		return;

	auto onPeersUpdate = [&torrent](Status s, mtt::PeerSource source)
	{
		TEST_LOG("Peers update from " << (int)source << "with status " << (int)s << "\n");
	};

	torrent->peers->start(onPeersUpdate, nullptr);

	WAITFOR(false);
}

void idealTorrentStateTest()
{
	TorrentPtr torrent = torrentFromFile("G:\\test.torrent");

	if (!torrent|| torrent->name().empty())
		return;

	torrent->checkFiles();

	WAITFOR(!torrent->checking);

	auto progress = torrent->currentProgress();
}

void idealLocalTest()
{
	TorrentPtr torrent = torrentFromFile("G:\\test.torrent");

	if (!torrent)
		return;

	torrent->checkFiles();

	WAITFOR(!torrent->checking);

	torrent->peers->trackers.removeTrackers();

	if (torrent->start() != mtt::Status::Success)
		return;

	torrent->peers->connect(Addr({ 127,0,0,1 }, 31132));

	while (!torrent->finished())
	{
		TEST_LOG("Progress: " << torrent->currentProgress() << "(" << torrent->downloaded()/(1024.f*1024) << "MB) (" << torrent->downloadSpeed()/(1024.f * 1024) << " MBps)");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	TEST_LOG("Finished");
}

void testBandwidthLimit()
{
	class TestPeer : public BandwidthUser
	{
	public:
		uint32_t m_quota = 0;

		virtual void assignBandwidth(int amount) override
		{
			m_quota += amount;
		}

		virtual bool isActive() override
		{
			return true;
		}
	};

	std::vector<std::shared_ptr<TestPeer>> peers;
	for (int i = 0; i < 2; i++)
	{
		peers.push_back(std::make_shared<TestPeer>());
	}

	uint32_t limit = 1024 * 1024;

	BandwidthChannel globalBw;
	globalBw.setLimit(limit);

	BandwidthChannel torrentBw;
	//torrentBw.throttle(limit);

	BandwidthManager& manager = BandwidthManager::Get();

	BandwidthChannel* channels[BandwidthRequest::MaxBandwidthChannels] = { &globalBw, &torrentBw };

	for (auto p : peers)
	{
		manager.requestBandwidth(p, 40000000, 1, channels, 2);
	}
	//manager.request_bandwidth(peers[0], 1000, 10, channels, 2);
	//manager.request_bandwidth(peers[1], 40000000, 1, channels, 2);

	int tick_interval = 500;
	int fullTime = 10;
	for (int i = 0; i < int(fullTime * 1000 / tick_interval); ++i)
	{
		manager.updateQuotas(tick_interval);
	}


	float sum = 0.f;
	float const err = std::max(limit / peers.size() * 0.3f, 1000.f);
	for (auto p : peers)
	{
		sum += p->m_quota;
		std::cout << p->m_quota / fullTime << " target: " << (limit / peers.size()) << std::endl;
	}
	sum /= fullTime;
	std::cout << "sum: " << sum << " target: " << limit << std::endl;
}

namespace mtt
{
	namespace bt
	{
		DataBuffer createHandshake(uint8_t* torrentHash, const uint8_t* clientHash);
	};
};


std::vector<DataBuffer> packets;
std::mutex packetMutex;
utp::TimePoint packetTime;

extern void serializeFileLog();

void serializePackets()
{
	packetMutex.lock();
	std::ofstream file("utpPackets", std::ios_base::binary | std::ios_base::app);

	for (auto& p : packets)
	{
		size_t size = p.size();
		file.write((const char*)&size, sizeof(size));
		file.write((const char*)p.data(), p.size());
	}

	packets.clear();
	packetMutex.unlock();

	serializeFileLog();
}

void recordUtpPacket(const BufferView& data)
{
	packetMutex.lock();
	packets.push_back(DataBuffer(data.data, data.data + data.size));
	packetMutex.unlock();
}

static bool idHigherThan(std::uint32_t lhs, std::uint32_t rhs)
{
	std::uint32_t dist_down = (lhs - rhs) & 0xFFFF;
	std::uint32_t dist_up = (rhs - lhs) & 0xFFFF;

	return dist_up > dist_down;
}

void testSerializedCommunication(TorrentInfo& torrentInfo)
{
	std::ifstream file("utpPackets", std::ios_base::binary);

	DataBuffer receiveBuffer;
	receiveBuffer.reserve(10000000);
	uint16_t seq = 0;
	bool first = true;
	std::map<uint16_t, DataBuffer> outOfOrder;
	std::vector<uint16_t> duplicates;
	size_t dataFull = 0;

	while (!file.eof())
	{
		size_t size = 0;
		file.read((char*)&size, sizeof(size));

		DataBuffer packet;
		packet.resize(size);
		file.read((char*)packet.data(), size);

		dataFull += sizeof(size) + size;

		if (size < 20)
			break;

		const utp::MessageHeader* utpHeader = (const utp::MessageHeader*)packet.data();
		auto headerSize = utp::parseHeaderSize(utpHeader, size);
		uint16_t headerSeq = utpHeader->seq_nr;

		if (first)
		{
			seq = headerSeq - 1;
			first = false;
		}

		if (utpHeader->getType() == utp::MessageType::ST_DATA)
		{
			auto dataSize = size - headerSize;

			uint16_t next = seq;
			next += 1;

			if (headerSeq == next)
			{
				receiveBuffer.insert(receiveBuffer.end(), packet.data() + headerSize, packet.data() + size);
				seq++;

				while (true)
				{
					auto it = outOfOrder.find(next);
					if (it != outOfOrder.end())
					{
						receiveBuffer.insert(receiveBuffer.end(), it->second.begin(), it->second.end());
						seq++;
						outOfOrder.erase(it);
					}
					else
						break;
				}
			}
			else if (idHigherThan(headerSeq,seq))
			{
				outOfOrder[headerSeq] = DataBuffer(packet.data() + headerSize, packet.data() + size);
			}
			else
				duplicates.push_back(headerSeq);
		}
	}

	std::cout << "Total datasize: " << receiveBuffer.size();

	size_t bufferPos = 0;
	auto readNextMessage = [&]()
	{
		PeerMessage msg({ receiveBuffer.data() + bufferPos, receiveBuffer.size() - bufferPos });

		if (msg.id != Invalid)
			bufferPos += msg.messageSize;
		else if (!msg.messageSize)
			bufferPos = receiveBuffer.size();

		return std::move(msg);
	};

	auto message = readNextMessage();

	std::map<uint32_t, DownloadedPiece> pieces;

	std::vector<mtt::PeerMessageId> messages;

	while (message.id != Invalid)
	{
		if (message.id == Piece)
		{
// 			if (message.piece.info.index == 1099)
// 			{
// 				std::cout << "piece " << message.piece.info.begin << ", " << message.piece.info.length << "\n";
// 			}

			auto idx = message.piece.info.index;
			if (pieces.find(idx) == pieces.end())
			{
				uint32_t blocksCount = torrentInfo.getPieceBlocksCount(idx);
				pieces[idx].init(idx, torrentInfo.getPieceSize(idx), blocksCount);
			}

			auto pieceIt = pieces.find(idx);
			pieceIt->second.addBlock(message.piece);

			if (pieceIt->second.remainingBlocks == 0)
			{
				if (pieceIt->second.isValid(torrentInfo.pieces[idx].hash))
				{
					std::cout << "Valid piece " << idx << "\n";
				}
				else
				{
					std::cout << "Invalid piece " << idx << "\n";
				}

				pieces.erase(pieceIt);
			}
		}
		else
			messages.push_back(message.id);

		message = readNextMessage();
	}

	messages.clear();
}

void testUtpLocalConnection()
{
	auto torrent = parseTorrentFile("D:\\hero83.torrent");
	TorrentInfo& torrentInfo = torrent.info;

	//testSerializedCommunication(torrentInfo);
	//return;

	std::remove("utpPackets");
	std::remove("fileLogger");

	packets.reserve(1000000);
	bool requestTest = false;

	ServiceThreadpool pool(2);

	int handledCount = 0;
	mtt::utp::Manager utpMgr;
	utpMgr.start(mtt::config::getExternal().connection.udpPort);

	auto udpReceiver = std::make_shared<UdpAsyncReceiver>(pool.io, mtt::config::getExternal().connection.udpPort, false);
	udpReceiver->receiveCallback = [&](udp::endpoint& e, std::vector<DataBuffer*>& d)
	{
		if (utpMgr.onUdpPacket(e, d))
			handledCount++;
	};
	udpReceiver->listen();

	//decodeHexa("6DEB1179339B447DC3A44A6ED9EEFE2565871116", torrentInfo.hash);

	mtt::Storage storage(torrentInfo);
	storage.setPath("C:\\Torrent\\mtt", false);

	class TestPeerListener : public BasicPeerListener
	{
	public:
		TestPeerListener(mtt::Storage& s) : storage(s) {}
		virtual void messageReceived(PeerCommunication* p, PeerMessage& msg) override
		{
			if (msg.id == Piece)
			{
				piece.addBlock(msg.piece);
				askNext = true;
			}
			if (msg.id == Request)
			{
				DataBuffer buffer;
				storage.loadPieceBlock(msg.request, buffer);

				PieceBlock block;
				block.info = msg.request;
				block.buffer = buffer;

				if (block.info.begin == 0)
					std::cout << "Send piece " << block.info.index << " block start " << block.info.begin << std::endl;
				p->sendPieceBlock(block);
				requests++;
			}
			if (msg.id == Interested)
			{
				p->setChoke(false);
			}
		}

		int requests = 0;
		mtt::DownloadedPiece piece;
		bool askNext = {};
		mtt::Storage& storage;
	}
	peerListener(storage);

	auto peer = std::make_shared<PeerCommunication>(torrentInfo, peerListener, pool.io);
	peer->sendHandshake(Addr("127.0.0.1:34000"));

	WAITFOR(peer->isEstablished());

	if (requestTest)
	{
		peer->setInterested(true);

		WAITFOR(!peer->state.peerChoking);

		uint32_t pieceIdx = 0;
		uint32_t blocksCount = torrentInfo.getPieceBlocksCount(pieceIdx);
		peerListener.piece.init(pieceIdx, torrentInfo.getPieceSize(pieceIdx), blocksCount);

		while (peerListener.piece.remainingBlocks > 0)
		{
			peerListener.askNext = false;

			auto blockIdx = blocksCount - peerListener.piece.remainingBlocks;
			auto block = torrentInfo.getPieceBlockInfo(pieceIdx, blockIdx);
			peer->requestPieceBlock(block);

			WAITFOR(peerListener.askNext);
		}

		auto success = peerListener.piece.isValid(torrentInfo.pieces[pieceIdx].hash);
	}
	else
	{
		DataBuffer bitfield;
		bitfield.assign(torrentInfo.expectedBitfieldSize, 0xFF);
		peer->sendBitfield(bitfield);

		WAITFOR(!peer->state.amChoking);

		getchar();
		getchar();
	}

	serializePackets();
}

std::mutex logMtx;
#define SyncLog(x) { logMtx.lock();  std::cout << x << std::endl; logMtx.unlock(); }

void testUtpLocalProtocol()
{
	bool requestTest = false;

	ServiceThreadpool pool(2);
	auto torrent = parseTorrentFile("D:\\hero83.torrent");
	TorrentInfo& torrentInfo = torrent.info;
	//decodeHexa("6DEB1179339B447DC3A44A6ED9EEFE2565871116", torrentInfo.hash);

	class TestPeerListener : public BasicPeerListener
	{
	public:
		TestPeerListener(ServiceThreadpool& pool, const char* filepath, TorrentInfo& info, uint16_t port) : torrentInfo(info)
		{
			storage.init(torrentInfo, filepath);

			utpMgr.start(port);
			udpReceiver = std::make_shared<UdpAsyncReceiver>(pool.io, port, false);
			udpReceiver->receiveCallback = [&](udp::endpoint& e, std::vector<DataBuffer*>& d)
			{
				utpMgr.onUdpPacket(e, d);
			};
			udpReceiver->listen();
		}
		virtual void messageReceived(PeerCommunication* p, PeerMessage& msg) override
		{
			if (msg.id == Piece)
			{
				SyncLog("Receive piece " << msg.piece.info.index << " block start " << msg.piece.info.begin << " idx " << (msg.piece.info.begin + 1) / 16384);
				pieces[msg.piece.info.index].addBlock(msg.piece);

				if (pieces[msg.piece.info.index].remainingBlocks == 0)
				{
					auto piece = std::move(pieces[msg.piece.info.index]);
					pieces.erase(msg.piece.info.index);

					if (piece.isValid(torrentInfo.pieces[piece.index].hash))
					{
						SyncLog("Valid piece " << piece.index);
					}
					else
					{
						SyncLog("Invalid piece " << piece.index);
					}
				}
			}
			if (msg.id == Request)
			{
				DataBuffer buffer;
				storage.loadPieceBlock(msg.request, buffer);

				PieceBlock block;
				block.info = msg.request;
				block.buffer = buffer;

				if (block.info.begin >= 0)
					SyncLog("Send piece " << block.info.index << " block start " << block.info.begin << " idx " << (block.info.begin + 1) / 16384);
				p->sendPieceBlock(block);
			}
			if (msg.id == Interested)
			{
				p->setChoke(false);
			}
		}

		std::map<uint32_t,mtt::DownloadedPiece> pieces;
		mtt::Storage storage;
		TorrentInfo& torrentInfo;

		mtt::utp::Manager utpMgr;
		std::shared_ptr<UdpAsyncReceiver> udpReceiver;
	};

	TestPeerListener uploaderListener(pool, "C:\\Torrent\\mtt", torrentInfo, 57000);
	TestPeerListener downloaderListener(pool, "C:\\Torrent\\mtt2", torrentInfo, 56000);

	auto uploader = std::make_shared<PeerCommunication>(torrentInfo, uploaderListener, pool.io);
	auto downloader = std::make_shared<PeerCommunication>(torrentInfo, downloaderListener, pool.io);

	uploaderListener.utpMgr.onConnection = [&](utp::StreamPtr s)
	{
		downloader->fromStream(s, {});
	};

	uploader->sendHandshake(Addr("127.0.0.1:57000"));

	WAITFOR(uploader->isEstablished());

	{
		uploader->setInterested(true);

		WAITFOR(!uploader->state.peerChoking);

		for (uint32_t idx = 0; idx < torrentInfo.pieceSize; idx++)
		{
			uint32_t blocksCount = torrentInfo.getPieceBlocksCount(idx);
			downloaderListener.pieces[idx].init(idx, torrentInfo.getPieceSize(idx), blocksCount);

			for (uint32_t i = 0; i < blocksCount; i++)
			{
				auto block = torrentInfo.getPieceBlockInfo(idx, i);
				downloader->requestPieceBlock(block);
			}

			WAITFOR(downloaderListener.pieces.size() < 10);
		}
	}
}

void TorrentTest::start()
{
	testStorageLoad();
	//testUtpLocalConnection();
}
