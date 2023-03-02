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
#include "Core.h"
#include "IncomingPeersListener.h"
#include <iostream>
#include <chrono>
#include "utils/BigNumber.h"
#include "utils/SHA.h"
#include "utils/DiffieHellman.h"
#include <numeric>
#include <random>

using namespace mtt;

#define WAITFOR(x) { while (!(x)) std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
#define WAITFOR2(x, y) { while (!(x)) { y; std::this_thread::sleep_for(std::chrono::milliseconds(50));} }

#define TEST_PRINT(x) {std::cout << x << "\n";}
#define TEST_LOG(x) {TEST_PRINT(x) WRITE_GLOBAL_LOG(Test, x)}

void testInit()
{
	auto filesSettings = mtt::config::getExternal().files;
	filesSettings.defaultDirectory = "C:\\Torrent\\test";
	mtt::config::setValues(filesSettings);

	auto connectionSettings = mtt::config::getExternal().connection;
	connectionSettings.tcpPort = connectionSettings.udpPort = 55125;
	connectionSettings.enableUtpIn = connectionSettings.enableUtpOut = false;
	connectionSettings.upnpPortMapping = false;
	mtt::config::setValues(connectionSettings);

	auto internalSettings = mtt::config::getInternal();

	memcpy(internalSettings.hashId, "0123456789ABCDEFGHIJ", std::size(internalSettings.hashId));
	memcpy(internalSettings.hashId, MT_HASH_NAME, std::size(MT_HASH_NAME));

	internalSettings.programFolderPath = "C:\\Torrent\\test\\data";
	internalSettings.stateFolder = "C:\\Torrent\\test\\data\\state";

	mtt::config::setInternalValues(internalSettings);
}

TorrentTest::TorrentTest()
{
	testInit();
}

void testAsyncDhtUdpRequest()
{
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
	UdpAsyncComm udp;
	udp.setBindPort(mtt::config::getExternal().connection.udpPort);
	udp.listen({});

	auto respFunc = [&responded](UdpRequest r, const BufferView& d)
	{
		responded = true;

		if (d.size)
		{
			BencodeParser parser;
			parser.parse(d.data, d.size);
		}

		return true;
	};

	auto req = udp.create(dhtRoot, dhtRootPort);
	udp.sendMessage(packet.getBuffer(), req, respFunc);

	WAITFOR(responded);
}

class DhtTestListener : public mtt::dht::ResultsListener
{
public:

	struct
	{
		uint32_t finalCount = -1;

		std::mutex mutex;
		std::vector<Addr> values;
	}
	result;

	uint32_t dhtFoundPeers(const uint8_t* hash, const std::vector<Addr>& values) override
	{
		std::lock_guard<std::mutex> guard(result.mutex);

		uint32_t count = 0;
		for (auto& v : values)
		{
			bool found = false;

			for (auto& old : result.values)
			{
				if (old == v)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				result.values.push_back(v);
				count++;
			}
		}

		TEST_LOG("DHT received values count :" << count);

		return count;
	}

	void dhtFindingPeersFinished(const uint8_t* hash, uint32_t count) override
	{
		TEST_LOG("DHT final values count :" << count);

		result.finalCount = count;
	}
};

class TestPeerListener : public mtt::IPeerListener
{
public:

	void handshakeFinished(mtt::PeerCommunication*) override
	{
	}

	void connectionClosed(mtt::PeerCommunication*, int) override
	{
		closed++;
	}

	void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage& msg) override
	{
		if (onPeerMsg)
			onPeerMsg(msg);
	};

	void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake& h) override
	{
		metadataSize = h.metadataSize;
	}

	void extendedMessageReceived(PeerCommunication*, ext::Type t, const BufferView& data) override
	{
		if (t == ext::Type::UtMetadata)
			ext::UtMetadata::Load(data, utmMsg);
	}

	uint32_t closed = 0;

	mtt::ext::UtMetadata::Message utmMsg{};
	uint32_t metadataSize = 0;

	std::function<void(mtt::PeerMessage&)> onPeerMsg;
};

void testAsyncDhtGetPeers()
{
	UdpAsyncComm udp;
	udp.setBindPort(mtt::config::getExternal().connection.udpPort);
	udp.listen({});

	mtt::dht::Communication dht(udp);
	dht.start();

	//ZEF3LK3MCLY5HQGTIUVAJBFMDNQW6U3J	boku 26
	//6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I	owarimonogatari batch
	//56VYAWGYUTF7ETZZRB45C6FKJSKVBLRD	mushishi s2 22 rare
	auto targetId = base32decode("56VYAWGYUTF7ETZZRB45C6FKJSKVBLRD");

	DhtTestListener dhtListener;
	dht.findPeers((uint8_t*)targetId.data(), &dhtListener);

	WAITFOR(dhtListener.result.finalCount != -1 || !dhtListener.result.values.empty());

	if (dhtListener.result.finalCount == 0)
		return;

	//dht.stopFindingPeers((uint8_t*)targetId.data());

	ServiceThreadpool service(1);
	TestPeerListener peerListener;

	TorrentInfo info;
	memcpy(info.hash, targetId.data(), 20);
	std::shared_ptr<PeerCommunication> peer;
	uint32_t nextPeerIdx = 0;

	while (true)
	{
		WAITFOR(dhtListener.result.finalCount != -1 || nextPeerIdx < dhtListener.result.values.size());

		if (nextPeerIdx >= dhtListener.result.values.size())
		{
			TEST_LOG("NO ACTIVE PEERS, RECEIVED: " << dhtListener.result.finalCount);
			return;
		}

		TEST_LOG("PEER " << nextPeerIdx);
		peerListener = {};
		peer = std::make_shared<PeerCommunication>(info, peerListener, service.io);
		Addr nextAddr;

		{
			std::lock_guard<std::mutex> guard(dhtListener.result.mutex);
			nextAddr = dhtListener.result.values[nextPeerIdx++];
		}

		peer->connect(nextAddr);

		WAITFOR(peerListener.closed || (peer->isEstablished() && peer->ext.enabled()));

		if (!peerListener.closed && peer->ext.utm.enabled())
			break;
	}

	mtt::MetadataReconstruction metadata;
	metadata.init(peerListener.metadataSize);

	while (!metadata.finished() && !peerListener.closed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.utm.sendPieceRequest(mdPiece);

		WAITFOR(peerListener.closed || peerListener.utmMsg.metadata.size);

		if (peerListener.closed || peerListener.utmMsg.type != mtt::ext::UtMetadata::Type::Data)
			return;

		metadata.addPiece(peerListener.utmMsg.metadata, peerListener.utmMsg.piece);

		peerListener.utmMsg = {};
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG(info.name);
	}

	peer->close();
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

std::vector<Addr> getPeersFromTrackers(mtt::TorrentFileMetadata& parsedTorrent)
{
	TorrentPtr torrent = mtt::Torrent::fromFile(parsedTorrent);

	UdpAsyncComm udp;
	udp.setBindPort(mtt::config::getExternal().connection.udpPort);
	udp.listen({});

	torrent->service.start(1);

	std::vector<Addr> peers;

	auto trUpdate = [&peers](Status s, const AnnounceResponse* r, const Tracker& t)
	{
		if (s == Status::Success && r && peers.empty() && !r->peers.empty())
		{
			peers = r->peers;
			TEST_LOG(peers.size() << " peers from " << t.info.hostname << "\n");
		}
	};

	mtt::TrackerManager trackers(*torrent);
	trackers.addTrackers(parsedTorrent.announceList);
	trackers.start(trUpdate);

	WAITFOR(!peers.empty());

	trackers.stop();
	torrent->service.stop();
	udp.deinit();

	return peers;
}

void testTrackers()
{
	std::string link = GetClipboardText();
	mtt::TorrentFileMetadata parsedTorrent;
	parsedTorrent.parseMagnetLink(link);

	auto addr = getPeersFromTrackers(parsedTorrent);

	WAITFOR(false);
}

std::string findFileByPiece(mtt::TorrentFileMetadata& file, uint32_t piece)
{
	for (auto& f : file.info.files)
	{
		if (f.startPieceIndex <= piece && f.endPieceIndex >= piece)
			return f.name;
	}

	return "";
}

static TorrentFileMetadata parseTorrentFile(const char* filepath)
{
	std::ifstream file(filepath, std::ios_base::binary);

	if (!file.good())
		return TorrentFileMetadata{};

	DataBuffer buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return mtt::TorrentFileParser::parse(buffer.data(), buffer.size());
}

void testStorageCheck()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");

	mtt::Storage storage(torrent.info);
	storage.setPath("C:\\Torrent\\test");

	ServiceThreadpool pool(1);

	bool finished = false;
	auto onFinish = [&](std::shared_ptr<PiecesCheck>) { finished = true; TEST_LOG("Finished"); };

	PiecesProgress progress;
	progress.init(torrent.info.pieces.size());

	mtt::PiecesCheck check(progress);
	std::vector<bool> wanted(torrent.info.pieces.size(), true);
	storage.checkStoredPieces(check, torrent.info.pieces, 1, 0, wanted);
}

void testStorageLoad()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");

	DownloadSelection selection(torrent.info.files.size(), { false, PriorityNormal });

	mtt::Storage storage(torrent.info);
	storage.setPath("C:\\Torrent\\test");
	storage.preallocateSelection(selection);

	mtt::Storage outStorage(torrent.info);
	outStorage.setPath("C:\\Torrent\\test\\out");
	outStorage.preallocateSelection(selection);

	DataBuffer buffer;

	for (uint32_t i = 0; i < torrent.info.pieces.size(); i++)
	{
		auto blocksInfo = torrent.info.makePieceBlocksInfo(i);

		for (auto& blockInfo : blocksInfo)
		{
			buffer.resize(blockInfo.length);
			storage.loadPieceBlock(blockInfo, buffer.data());
		}

		std::vector<Storage::PieceBlockData> data = { { buffer.data(), PieceBlockInfo{i, 0, (uint32_t)buffer.size()} } };
		outStorage.storePieceBlocks(data);
	}

	PiecesProgress progress;
	progress.init(torrent.info.pieces.size());

	mtt::PiecesCheck checkResults(progress);
	std::vector<bool> wanted(torrent.info.pieces.size(), true);
	outStorage.checkStoredPieces(checkResults, torrent.info.pieces, 1, 0, wanted);
	for (auto r : progress.pieces)
	{
		if (r != 1)
		{
			std::cout << "Invalid piece copy!" << std::endl;
		}
	}
}

bool isValidPiece(const DataBuffer& data, const uint8_t* expectedHash)
{
	uint8_t hash[SHA_DIGEST_LENGTH];
	_SHA1((const uint8_t*)data.data(), data.size(), hash);

	return memcmp(hash, expectedHash, SHA_DIGEST_LENGTH) == 0;
}

void testDumpStoredPiece()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");
	uint32_t pieceIdx = torrent.info.files[3].endPieceIndex;

	mtt::Storage storage(torrent.info);
	storage.setPath("C:\\Torrent\\test", false);

	auto blocksInfo = torrent.info.makePieceBlocksInfo(pieceIdx);

	//DataBuffer mockData(20, 1);
	//storage.storePieceBlock(pieceIdx, blocksInfo[1521].begin, mockData);

	DataBuffer pieceData;
	pieceData.resize(torrent.info.pieceSize);

	DataBuffer buffer;
	for (auto& blockInfo : blocksInfo)
	{
		buffer.resize(blockInfo.length);
		auto status = storage.loadPieceBlock(blockInfo, buffer.data());
		memcpy(pieceData.data() + blockInfo.begin, buffer.data(), buffer.size());

		if (status != Status::Success)
			std::cout << blockInfo.begin << ": Problem " << (int)status << std::endl;
	}

	std::cout << "Piece: " << (isValidPiece(pieceData, torrent.info.pieces[pieceIdx].hash) ? "Valid" : "Not valid") << std::endl;
}

void testDownloadCache()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");

	mtt::Storage storage(torrent.info);
	ServiceThreadpool pool;
	pool.start(4);

	auto internalCfg = mtt::config::getInternal();
	internalCfg.downloadCachePerGroup = 10 * BlockMaxSize;
	mtt::config::setInternalValues(internalCfg);

	mtt::WriteCache cache(pool, storage);

	DataBuffer buffer;
	buffer.resize(BlockMaxSize);

	std::vector<int> order;
	order.resize(64);
	std::iota(order.begin(), order.end(), 0);
	std::shuffle(order.begin(), order.end(), std::minstd_rand{ 0 });

	for (int i = 0; i < 64; i++)
	{
		int blockPos = order[i];

		buffer[0] = (uint8_t)(blockPos + 1);

		mtt::PieceBlock block;
		block.buffer = buffer;
		block.info = { 0, blockPos * BlockMaxSize, BlockMaxSize };
		cache.storeBlock(block);
	}

	cache.flush();
	pool.stop();
}

void testPeerListen()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");

	DownloadSelection selection(torrent.info.files.size(), { false, PriorityNormal });
	selection.front().selected = true;

	mtt::Storage storage(torrent.info);
	storage.setPath("C:\\Torrent\\test");
	storage.preallocateSelection(selection);

	mtt::PiecesProgress progress;
	progress.init(torrent.info.pieces.size());

	mtt::PiecesCheck check(progress);
	std::vector<bool> wanted(torrent.info.pieces.size(), true);
	storage.checkStoredPieces(check, torrent.info.pieces, 1, 0, wanted);

	ServiceThreadpool service(2);

	std::shared_ptr<PeerStream> peerStream;
	TcpAsyncServer server(service.io, mtt::config::getExternal().connection.tcpPort, false);
	server.acceptCallback = [&](std::shared_ptr<TcpAsyncStream> c) { peerStream = std::make_shared<PeerStream>(service.io); peerStream->fromStream(c); };
	server.listen();

	WAITFOR(peerStream);

	class MyListener : public TestPeerListener
	{
	public:

		void handshakeFinished(PeerCommunication*) override
		{
			success = true;
		}

		void messageReceived(PeerCommunication*, PeerMessage& msg) override
		{
			if (msg.id == PeerMessage::Handshake)
			{
				success = memcmp(msg.handshake.info, torrent->info.hash, 20) == 0;
			}
			else if (msg.id == PeerMessage::Interested)
			{
				comm->setChoke(false);
			}
			else if (msg.id == PeerMessage::Request)
			{
				PieceBlock block;
				block.info.begin = msg.request.begin;
				block.info.index = msg.request.index;
				block.info.length = msg.request.length;

				DataBuffer buffer;
				buffer.resize(block.info.length);
				storage->loadPieceBlock(block.info, buffer.data());

				block.buffer = buffer;
				comm->sendPieceBlock(block);
			}
		}

		Storage* storage{};
		PeerCommunication* comm{};
		TorrentFileMetadata* torrent{};
		bool success = false;
	}
	listener;

	listener.torrent = &torrent;
	listener.storage = &storage;

	auto peer = std::make_shared<PeerCommunication>(torrent.info, listener, service.io);
	peer->fromStream(peerStream, { });
	listener.comm = peer.get();

	WAITFOR(listener.success || listener.closed);

	if (listener.success)
	{
		peer->sendBitfield(progress.toBitfieldData());

		WAITFOR(listener.closed);
	}
}

void testDhtTable()
{
	UdpAsyncComm udp;
	udp.setBindPort(mtt::config::getExternal().connection.udpPort);
	udp.listen({});

	mtt::dht::Communication dhtComm(udp);
	dhtComm.start();

	//494981B03AFDDED5C07B8D81B999A53496854826	common
	//6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I less rare
	//47e2bac3c75e738f17eaeffae949c80c61e7e675 very rare fear
	TorrentFileMetadata info;
	info.parseMagnetLink("494981B03AFDDED5C07B8D81B999A53496854826");

	auto targetId = info.info.hash;

	DhtTestListener listener;
	dhtComm.findPeers(targetId, &listener);
//	dhtComm.findNode(mtt::config::getInternal().hashId);
	WAITFOR(listener.result.finalCount != -1);
// 	Sleep(10000);

	dhtComm.stop();
	udp.deinit();
}

void testDownloader()
{
	auto parsedTorrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");

	DownloadSelection selection(parsedTorrent.info.files.size(), { false, PriorityNormal });
	selection[1].selected = true;
	selection[2].selected = true;

	TorrentPtr torrent = mtt::Torrent::fromFile(parsedTorrent);
	torrent->files.selectFile(1, true);
	torrent->files.selectFile(2, true);

	torrent->service.start(4);
	torrent->files.start();
// 	torrent->files.checkFiles();
// 	WAITFOR(!torrent->files.checking);

	Downloader dl(*torrent);
	dl.refreshSelection(selection, std::vector<uint32_t>(parsedTorrent.info.pieces.size(), 0));

	class MyListener : public TestPeerListener
	{
	public:
		MyListener(Downloader& dl) : downloader(dl) {};
		Downloader& downloader;

		void handshakeFinished(mtt::PeerCommunication* p) override
		{
			downloader.handshakeFinished(p);
		}
		void connectionClosed(mtt::PeerCommunication* p, int i) override
		{
			downloader.connectionClosed(p);
			TestPeerListener::connectionClosed(p, i);
		}
		void messageReceived(mtt::PeerCommunication* p, mtt::PeerMessage& msg) override
		{
			downloader.messageReceived(p, msg);
		};
	}
	peerListener(dl);

	auto peer = std::make_shared<PeerCommunication>(parsedTorrent.info, peerListener, torrent->service.io);
	peer->connect(Addr({ 127,0,0,1 }, 13131));

	WAITFOR(peerListener.closed || peer->isEstablished());

	if (peerListener.closed)
		return;

	WAITFOR(peerListener.closed || torrent->selectionFinished());
	WAITFOR(!torrent->files.checking)

	peer->close();

	torrent->service.stop();
}

void testTorrentFileSerialization()
{
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");
	auto file = torrent.createTorrentFileData();
	auto torrentOut = mtt::TorrentFileParser::parse((const uint8_t*)file.data(), file.size());
	bool ok = memcmp(torrent.info.hash, torrentOut.info.hash, 20) == 0;
}

void bigTestGetTorrentFileByLink()
{
	std::string link = "magnet:?xt=urn:btih:3CB11691C0AD8D327ECB9D5E242FDDAF0B2E32B0&dn=Bruce+Springsteen+-+Only+the+Strong+Survive+%282022%29+Mp3+320kbps+%5BPMEDIA%5D+%E2%AD%90%EF%B8%8F&tr=udp%3A%2F%2Ftracker.coppersurfer.tk%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.openbittorrent.com%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.opentrackr.org%3A1337&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.dler.org%3A6969%2Fannounce&tr=udp%3A%2F%2Fopentracker.i2p.rocks%3A6969%2Fannounce&tr=udp%3A%2F%2F47.ip-51-68-199.eu%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.internetwarriors.net%3A1337%2Fannounce&tr=udp%3A%2F%2F9.rarbg.to%3A2920%2Fannounce&tr=udp%3A%2F%2Ftracker.pirateparty.gr%3A6969%2Fannounce&tr=udp%3A%2F%2Ftracker.cyberia.is%3A6969%2Fannounce";
	link = "0E77F0FC2D7860B6470F99402202044E2EA268E1";

	mtt::TorrentFileMetadata parsedTorrent;
	parsedTorrent.parseMagnetLink(link);

	std::vector<Addr> addr = { Addr::fromString("185.149.90.102:51020") };//getPeersFromTrackers(parsedTorrent);
	TEST_LOG("received peers size " << addr.size());

	ServiceThreadpool service(1);
	TestPeerListener peerListener;
	std::shared_ptr<PeerCommunication> connectedPeer;

	for (auto& a : addr)
	{
		peerListener.closed = 0;

		auto peer = std::make_shared<PeerCommunication>(parsedTorrent.info, peerListener, service.io);
		peer->connect(a);
		TEST_LOG("connecting " << a.toString());
		WAITFOR(peerListener.closed || (peer->isEstablished() && peer->ext.enabled()));

		if (!peerListener.closed)
		{
			TEST_LOG("connected");
			connectedPeer = peer;
			break;
		}
	}

	if (!connectedPeer)
		return;

	mtt::MetadataReconstruction metadata;
	metadata.init(peerListener.metadataSize);

	while (!metadata.finished() && !peerListener.closed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		TEST_LOG("request piece " << mdPiece);
		connectedPeer->ext.utm.sendPieceRequest(mdPiece);

		WAITFOR(peerListener.closed || peerListener.utmMsg.metadata.size);

		if (peerListener.closed || peerListener.utmMsg.type != mtt::ext::UtMetadata::Type::Data)
			return;

		metadata.addPiece(peerListener.utmMsg.metadata, peerListener.utmMsg.piece);
		peerListener.utmMsg = {};
	}

	if (metadata.finished())
	{
		TorrentFileMetadata finfo;
		finfo.info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		auto data = finfo.createTorrentFileData();

		std::ofstream file("./outMt", std::ios::binary);

		if (!file)
			return;

		file.write((const char*)data.data(), data.size());
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG("metadata finished" << info.name);

		DownloadSelection selection(info.files.size(), { false, PriorityNormal });
		selection.front().selected = true;

		PiecesProgress piecesTodo;
		piecesTodo.select(info.files.front(), true);

		mtt::Storage storage(info);
		storage.preallocateSelection(selection);

		connectedPeer->setInterested(true);

		PieceState pieceTodo;
		DataBuffer blockBuffer;
		blockBuffer.resize(info.pieceSize);
		std::mutex pieceMtx;
		bool finished = false;
		uint32_t finishedPieces = 0;
		peerListener.onPeerMsg = [&](mtt::PeerMessage& msg)
		{
			std::lock_guard<std::mutex> guard(pieceMtx);
			if (msg.id == PeerMessage::Piece)
			{
				auto& piece = msg.piece;
				memcpy(blockBuffer.data() + piece.info.begin, msg.piece.buffer.data, msg.piece.buffer.size);

				//std::vector<Storage::PieceBlockData> data = { { { blockBuffer.data(), msg.piece.info } } };
				
				//storage.storePieceBlocks(data);
				pieceTodo.addBlock(piece);

				if (pieceTodo.remainingBlocks == 0)
				{
					piecesTodo.addPiece(pieceTodo.index);
					finished = true;
					finishedPieces++;

					{
						std::ofstream file("./piece" + std::to_string(piece.info.index), std::ios::binary);

						if (!file)
							return;

						file.write((const char*)blockBuffer.data(), pieceTodo.downloadedSize);
					}
				}
			}
		};

		//while (finishedPieces < info.pieces.size())
		{
// 			auto p = piecesTodo.firstEmptyPiece();
// 			if (p == -1)
// 				break;
			int p = 116;

			auto blocks = info.makePieceBlocksInfo(p);
			TEST_LOG("Requesting idx " << p);

			finished = false;
			pieceTodo.init(p, info.getPieceBlocksCount(p));

			for (auto& b : blocks)
			{
				connectedPeer->requestPieceBlock(b);
			}

			WAITFOR(finished || peerListener.closed);
		}
		TEST_LOG("finished " << finished);

		storage.deleteAll();
	}
}

void testMetadataReceive()
{
	TorrentFileMetadata torrent;
	torrent.parseMagnetLink("6F98F622E8DB6676D6645B6632871B41B97547CC");

	ServiceThreadpool service(1);

	TestPeerListener peerListener;
	auto peer = std::make_shared<PeerCommunication>(torrent.info, peerListener, service.io);
	peer->connect(Addr({ 127,0,0,1 }, 13131));

	WAITFOR(peerListener.closed || (peer->isEstablished() && peer->ext.enabled()));

	if (peerListener.closed || peerListener.metadataSize == 0)
		return;

	mtt::MetadataReconstruction metadata;
	metadata.init(peerListener.metadataSize);

	while (!metadata.finished() && !peerListener.closed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.utm.sendPieceRequest(mdPiece);

		WAITFOR(peerListener.closed || peerListener.utmMsg.metadata.size);

		if (peerListener.closed || peerListener.utmMsg.type != mtt::ext::UtMetadata::Type::Data)
			return;

		metadata.addPiece(peerListener.utmMsg.metadata, peerListener.utmMsg.piece);
		peerListener.utmMsg = {};
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		TEST_LOG(info.name);
	}
}

static TorrentPtr torrentFromFile(const char* path)
{
	std::ifstream file(path, std::ios_base::binary);

	if (!file.good())
		return nullptr;

	DataBuffer buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return Torrent::fromFile(std::move(mtt::TorrentFileParser::parse(buffer.data(), buffer.size())));
}

const char* magnetLinkWithTrackers = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";
const char* magnetLite = "magnet:?xt=urn:btih:CWPX2WK3PNDMJQYRT4KQ4L62Q4ABDPWA&tr=udp://tracker.coppersurfer.tk:6969/announce";
const char* tempLink = "magnet:?xt=urn:btih:HMSXZ6DMI56OOTADFNV5R77O34WK4S65&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";
void testMagnetLink()
{
	auto& alerts = mtt::AlertsManager::Get();
	alerts.registerAlerts(Alerts::Id::MetadataInitialized);

	TorrentPtr torrent = Torrent::fromMagnetLink(tempLink);

	if (!torrent)
		return;

	torrent->downloadMetadata();

	while (true)
	{
		auto newAlerts = alerts.popAlerts();
		for (auto& a : newAlerts)
		{
			if (a->id == Alerts::Id::MetadataInitialized)
			{
				TEST_LOG("Metadata finished\n");
				break;
			}
		}
	}

	TEST_LOG(torrent->name() << "\n");

	torrent->stop();
}

void torrentFilesCheckTest()
{
	TorrentPtr torrent = torrentFromFile("C:\\Torrent\\test\\test.torrent");

	if (!torrent|| torrent->name().empty())
		return;

	torrent->files.checkFiles();

	WAITFOR(!torrent->files.checking);

	auto progress = torrent->progress();
}

void localDownloadTest()
{
	mtt::Core core;
	core.init();

	for (auto t : core.getTorrents())
	{
		core.removeTorrent(t->hash(), true);
	}

	auto[status, torrent] = core.addFile("C:\\Torrent\\test\\test.torrent");

	if (!torrent)
		return;

// 	torrent->checkFiles();
// 	WAITFOR(!torrent->checking);

	std::vector<bool> selectionBool(torrent->infoFile.info.files.size());
	selectionBool[1] = selectionBool[2] = true;
	torrent->files.selectFiles(selectionBool);

	torrent->peers->trackers.removeTrackers();

	if (torrent->start() != mtt::Status::Success)
		return;

	torrent->peers->connect(Addr({ 127,0,0,1 }, 13131));

	while (!torrent->selectionFinished() || torrent->files.checking)
	{
		if (torrent->files.checking)
			TEST_LOG("Checking: " << torrent->files.checkingProgress())
		else
			TEST_LOG("Progress: " << torrent->selectionFinished() << "(" << torrent->downloaded()/(1024.f*1024) << "MB) (" << torrent->downloadSpeed() / (1024.f * 1024) << " MBps)");

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	TEST_LOG("Finished");

	core.deinit();
}

void testBandwidthLimit()
{
	class TestPeer : public BandwidthUser
	{
	public:
		uint32_t m_quota = 0;

		void assignBandwidth(int amount) override
		{
			m_quota += amount;
		}

		bool isActive() override
		{
			return true;
		}

		std::string name() override
		{
			return "";
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

	for (const auto& p : peers)
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
	for (const auto& p : peers)
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
}

void recordUtpPacket(const BufferView& data)
{
	packetMutex.lock();
	packets.emplace_back(data.data, data.data + data.size);
	packetMutex.unlock();
}

static bool idHigherThan(std::uint32_t lhs, std::uint32_t rhs)
{
	std::uint32_t dist_down = (lhs - rhs) & 0xFFFF;
	std::uint32_t dist_up = (rhs - lhs) & 0xFFFF;

	return dist_up > dist_down;
}

struct BufferedDownloadedPiece : public PieceState
{
	DataBuffer buffer;

	void init(uint32_t idx, const TorrentInfo& info)
	{
		buffer.resize(info.pieceSize);
		PieceState::init(idx, info.getPieceBlocksCount(idx));
	}

	bool addBlock(const PieceBlock& block)
	{
		if (PieceState::addBlock(block))
		{
			memcpy(buffer.data() + block.info.begin, block.buffer.data, block.info.length);
		}

		return true;
	}

	bool isValid(const uint8_t* expectedHash)
	{
		return isValidPiece(buffer, expectedHash);
	}
};

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

		auto utpHeader = (const utp::MessageHeader*)packet.data();
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

		if (msg.id != PeerMessage::Invalid)
			bufferPos += msg.messageSize;
		else if (!msg.messageSize)
			bufferPos = receiveBuffer.size();

		return msg;
	};

	auto message = readNextMessage();

	std::map<uint32_t, BufferedDownloadedPiece> pieces;

	std::vector<mtt::PeerMessage::Id> messages;

	while (message.id != PeerMessage::Invalid)
	{
		if (message.id == PeerMessage::Piece)
		{
// 			if (message.piece.info.index == 1099)
// 			{
// 				std::cout << "piece " << message.piece.info.begin << ", " << message.piece.info.length << "\n";
// 			}

			auto idx = message.piece.info.index;
			if (pieces.find(idx) == pieces.end())
			{
				pieces[idx].init(idx, torrentInfo);
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
	auto connectionSettings = mtt::config::getExternal().connection;
	connectionSettings.enableUtpIn = connectionSettings.enableUtpOut = true;
	connectionSettings.enableTcpIn = connectionSettings.enableTcpOut = false;
	connectionSettings.encryption = config::Encryption::Refuse;
	mtt::config::setValues(connectionSettings);

	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");
	TorrentInfo& torrentInfo = torrent.info;

// 	testSerializedCommunication(torrentInfo);
// 	return;

// 	std::remove("utpPackets");
// 	std::remove("fileLogger");
// 	packets.reserve(1000000);

	ServiceThreadpool pool(2);

	int handledCount = 0;
	mtt::utp::Manager utpMgr;
	utpMgr.start();

	auto udpReceiver = std::make_shared<UdpAsyncReceiver>(pool.io, connectionSettings.udpPort, false);
	udpReceiver->receiveCallback = [&](udp::endpoint& e, std::vector<BufferView>& d)
	{
		if (utpMgr.onUdpPacket(e, d))
			handledCount++;
	};
	udpReceiver->listen();

	mtt::Storage storage(torrentInfo);

	class MyTestPeerListener : public TestPeerListener
	{
	public:
		MyTestPeerListener(mtt::Storage& s) : storage(s) {}
		void messageReceived(PeerCommunication* p, PeerMessage& msg) override
		{
			if (msg.id == PeerMessage::Piece)
			{
				piece.addBlock(msg.piece);
				askNext = true;
			}
			if (msg.id == PeerMessage::Request)
			{
				DataBuffer buffer;
				buffer.resize(msg.request.length);
				storage.loadPieceBlock(msg.request, buffer.data());

				PieceBlock block;
				block.info = msg.request;
				block.buffer = buffer;

				if (block.info.begin == 0)
					std::cout << "Send piece " << block.info.index << " block start " << block.info.begin << std::endl;
				p->sendPieceBlock(block);
				requests++;
			}
			if (msg.id == PeerMessage::Interested)
			{
				p->setChoke(false);
			}
		}

		int requests = 0;
		BufferedDownloadedPiece piece;
		bool askNext = {};
		mtt::Storage& storage;
	}
	peerListener(storage);

	auto peer = std::make_shared<PeerCommunication>(torrentInfo, peerListener, pool.io);
	peer->connect(Addr::fromString("127.0.0.1:13131"));

	WAITFOR(peer->isEstablished());

 	const bool requestTest = true;
	if (requestTest)
	{
		peer->setInterested(true);

		WAITFOR(!peer->state.peerChoking);

		uint32_t pieceIdx = 139;
		uint32_t blocksCount = torrentInfo.getPieceBlocksCount(pieceIdx);
		peerListener.piece.init(pieceIdx, torrentInfo);

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
// 	else
// 	{
// 		DataBuffer bitfield;
// 		bitfield.assign(torrentInfo.expectedBitfieldSize, 0xFF);
// 		peer->sendBitfield(bitfield);
// 
// 		WAITFOR(!peer->state.amChoking);
// 
// 		getchar();
// 		getchar();
// 	}
// 
// 	serializePackets();

	peer->close();
	udpReceiver->stop();
	utpMgr.stop();
}

void testUtpLocalProtocol()
{
	bool requestTest = false;

	ServiceThreadpool pool(2);
	auto torrent = parseTorrentFile("C:\\Torrent\\test\\test.torrent");
	TorrentInfo& torrentInfo = torrent.info;

	class MyTestPeerListener : public TestPeerListener
	{
	public:
		MyTestPeerListener(ServiceThreadpool& pool, const char* filepath, TorrentInfo& info, uint16_t port) : torrentInfo(info), storage(info)
		{
			storage.init(filepath);

			auto conn = mtt::config::getExternal().connection;
			conn.udpPort = port;
			mtt::config::setValues(conn);

			utpMgr.start();
			udpReceiver = std::make_shared<UdpAsyncReceiver>(pool.io, port, false);
			udpReceiver->receiveCallback = [&](udp::endpoint& e, std::vector<BufferView>& d)
			{
				utpMgr.onUdpPacket(e, d);
			};
			udpReceiver->listen();
		}
		void messageReceived(PeerCommunication* p, PeerMessage& msg) override
		{
			if (msg.id == PeerMessage::Piece)
			{
				TEST_PRINT("Receive piece " << msg.piece.info.index << " block start " << msg.piece.info.begin << " idx " << (msg.piece.info.begin + 1) / 16384);
				pieces[msg.piece.info.index].addBlock(msg.piece);

				if (pieces[msg.piece.info.index].remainingBlocks == 0)
				{
					auto piece = std::move(pieces[msg.piece.info.index]);
					pieces.erase(msg.piece.info.index);

					if (piece.isValid(torrentInfo.pieces[piece.index].hash))
					{
						TEST_PRINT("Valid piece " << piece.index);
					}
					else
					{
						TEST_PRINT("Invalid piece " << piece.index);
					}
				}
			}
			if (msg.id == PeerMessage::Request)
			{
				DataBuffer buffer;
				buffer.resize(msg.request.length);
				storage.loadPieceBlock(msg.request, buffer.data());

				PieceBlock block;
				block.info = msg.request;
				block.buffer = buffer;

				if (block.info.begin >= 0)
					TEST_PRINT("Send piece " << block.info.index << " block start " << block.info.begin << " idx " << (block.info.begin + 1) / 16384);
				p->sendPieceBlock(block);
			}
			if (msg.id == PeerMessage::Interested)
			{
				p->setChoke(false);
			}
		}

		std::map<uint32_t, BufferedDownloadedPiece> pieces;
		mtt::Storage storage;
		TorrentInfo& torrentInfo;

		mtt::utp::Manager utpMgr;
		std::shared_ptr<UdpAsyncReceiver> udpReceiver;
	};

	MyTestPeerListener uploaderListener(pool, "C:\\Torrent\\mtt", torrentInfo, 57000);
	MyTestPeerListener downloaderListener(pool, "C:\\Torrent\\mtt2", torrentInfo, 56000);

	auto uploader = std::make_shared<PeerCommunication>(torrentInfo, uploaderListener, pool.io);
	auto downloader = std::make_shared<PeerCommunication>(torrentInfo, downloaderListener, pool.io);

	uploaderListener.utpMgr.setConnectionCallback([&](utp::StreamPtr s)
	{
		std::shared_ptr<PeerStream> stream = std::make_shared<PeerStream>(pool.io);
		stream->fromStream(s);

		downloader->fromStream(stream, {});
	});

	uploader->connect(Addr::fromString("127.0.0.1:57000"));

	WAITFOR(uploader->isEstablished());

	{
		uploader->setInterested(true);

		WAITFOR(!uploader->state.peerChoking);

		for (uint32_t idx = 0; idx < torrentInfo.pieceSize; idx++)
		{
			uint32_t blocksCount = torrentInfo.getPieceBlocksCount(idx);
			downloaderListener.pieces[idx].init(idx, torrentInfo);

			for (uint32_t i = 0; i < blocksCount; i++)
			{
				auto block = torrentInfo.getPieceBlockInfo(idx, i);
				downloader->requestPieceBlock(block);
			}

			WAITFOR(downloaderListener.pieces.size() < 10);
		}
	}
}

DataBuffer createHandshake(const uint8_t* torrentHash, const uint8_t* clientHash)
{
	PacketBuilder packet(70);
	packet.add(19);
	packet.add("BitTorrent protocol", 19);

	uint8_t reserved_byte[8] = { 0 };
	reserved_byte[5] |= 0x10;	//Extension Protocol

	if (mtt::config::getExternal().dht.enabled)
		reserved_byte[7] |= 0x80;

	packet.add(reserved_byte, 8);

	packet.add(torrentHash, 20);
	packet.add(clientHash, 20);

	return packet.getBuffer();
}

void testLocalPeerEncryption()
{
	const size_t keySize = 2 * sizeof(uint64_t);

	DataBuffer random(keySize);

	BigNumber privateKey;
	Random::Data(random);
	privateKey.Set(random.data(), keySize);

	BigNumber privateKey2;
	Random::Data(random);
	privateKey2.Set(random.data(), keySize);

	BigNumber generator(2, keySize);

	BigNumber publicKey;
	BigNumber::Powm(publicKey, generator, privateKey, privateKey2);

	TorrentPtr torrent = Torrent::fromMagnetLink("bc8e4a520c4dfee5058129bd990bb8c7334f008e");
	bool closed = false;

	ServiceThreadpool service;
	service.start(2);

	ProtocolEncryptionHandshake peHandshake;

	std::shared_ptr<TcpAsyncStream> stream = std::make_shared<TcpAsyncStream>(service.io);

	stream->onCloseCallback = [&](int)
	{
		stream.reset();
		closed = true;
	};
	stream->onConnectCallback = [&]()
	{
		DataBuffer randomId(20);
		Random::Data(randomId);
		DataBuffer request = peHandshake.initiate(torrent->hash(), createHandshake(torrent->hash(), randomId.data()));
		stream->write(request);
	};
	stream->onReceiveCallback = [&](BufferView data) -> size_t
	{
		if (peHandshake.established())
		{
			peHandshake.pe->decrypt(const_cast<uint8_t*>(data.data), data.size);
		}

		DataBuffer response;
		auto sz = peHandshake.readRemoteDataAsInitiator(data, response);

		if (!response.empty())
		{
			stream->write(response);
		}

		if (peHandshake.established())
		{
			//done
		}

		return sz;
	};

	stream->connect("127.0.0.1", 51630);

	WAITFOR(closed);
}

void testLocalPeerEncryptionListen()
{
	TorrentPtr torrent = Torrent::fromMagnetLink("bc8e4a520c4dfee5058129bd990bb8c7334f008e");
	ProtocolEncryptionListener listener;
	listener.addTorrent(torrent->hash());

	ProtocolEncryptionHandshake peHandshake;
	std::shared_ptr<TcpAsyncStream> stream;
	bool closed = false;

	ServiceThreadpool pool(1);

	TcpAsyncServer receiver(pool.io, 51515, false);
	receiver.acceptCallback = [&](std::shared_ptr<TcpAsyncStream> c)
	{
		stream = c;
		auto sPtr = c.get();

		c->onCloseCallback = [&](int)
		{
			stream.reset();
			closed = true;
		};
		c->onReceiveCallback = [sPtr, &peHandshake, &listener](BufferView data) -> size_t
		{
			if (data.size < 20)
				return 0;

			// standard handshake
			if (!peHandshake.started() && PeerMessage::startsAsHandshake(data))
			{
				PeerMessage msg(data);

				if (msg.id == PeerMessage::Handshake)
				{
					size_t sz = 0;// addPeer(sPtr, data, msg.handshake.info);
					if (sz == 0)
					{
						sPtr->close(false);
					}
					return sz;
				}

				return 0;
			}

			// protocol encryption handshake
			{
				//get pe

				DataBuffer response;
				size_t sz = peHandshake.readRemoteDataAsListener(data, response, listener);

				if (!response.empty())
				{
					sPtr->write(response);
				}

				if (peHandshake.established())
				{
					//sz += addPeer(sPtr, peHandshake);
				}
				else if (peHandshake.failed())
				{
					//removePeer
				}

				return sz;
			}

			return 0;
		};
	};

	receiver.listen();
	WAITFOR(closed);
}

void TorrentTest::start()
{
	testDownloadCache();
}
