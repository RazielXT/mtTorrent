#include "HttpTrackerComm.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/UrlEncoding.h"
#include "Torrent.h"
#include "utils/HttpHeader.h"


#define HTTP_TRACKER_LOG(x) WRITE_LOG(LogTypeHttpTracker, x)

using namespace mtt;

mtt::HttpTrackerComm::HttpTrackerComm()
{
}

mtt::HttpTrackerComm::~HttpTrackerComm()
{
	deinit();
}

void mtt::HttpTrackerComm::deinit()
{
	if (tcpComm)
		tcpComm->close();

	tcpComm.reset();
}

void mtt::HttpTrackerComm::init(std::string host, std::string port, std::string path, TorrentPtr t)
{
	info.hostname = host;
	info.path = path;
	info.port = port;
	torrent = t;

	info.state = TrackerState::Initialized;
}

void mtt::HttpTrackerComm::initializeStream()
{
	tcpComm = std::make_shared<TcpAsyncStream>(torrent->service.io);
	tcpComm->onConnectCallback = std::bind(&HttpTrackerComm::onTcpConnected, this);
	tcpComm->onCloseCallback = [this](int code) {onTcpClosed(code); };
	tcpComm->onReceiveCallback = std::bind(&HttpTrackerComm::onTcpReceived, this);

	tcpComm->init(info.hostname, info.port);
}

void mtt::HttpTrackerComm::fail()
{
	tcpComm.reset();

	if (info.state == TrackerState::Announcing || info.state == TrackerState::Reannouncing)
	{
		if (info.state == TrackerState::Reannouncing)
			info.state = TrackerState::Alive;
		else
			info.state = TrackerState::Offline;

		if (onFail)
			onFail();
	}
}

void mtt::HttpTrackerComm::onTcpClosed(int)
{
	if(info.state != TrackerState::Announced)
		fail();
}

void mtt::HttpTrackerComm::onTcpConnected()
{
	info.state = std::max(info.state, TrackerState::Alive);
}

void mtt::HttpTrackerComm::onTcpReceived()
{
	auto respData = tcpComm->getReceivedData();

	if (info.state == TrackerState::Announcing || info.state == TrackerState::Reannouncing)
	{
		mtt::AnnounceResponse announceResp;
		auto msgSize = readAnnounceResponse((const char*)respData.data(), respData.size(), announceResp);

		if (msgSize == 0)
			return;
		else if (msgSize == -1)
		{
			tcpComm->consumeData(respData.size());
			fail();
			return;
		}
		else
		{
			tcpComm->consumeData(msgSize);
			info.state = TrackerState::Announced;

			info.leechers = announceResp.leechCount;
			info.seeds = announceResp.seedCount;
			info.peers = (uint32_t)announceResp.peers.size();
			info.announceInterval = announceResp.interval;
			info.lastAnnounce = (uint32_t)::time(0);

			HTTP_TRACKER_LOG("received peers:" << announceResp.peers.size() << ", p: " << announceResp.seedCount << ", l: " << announceResp.leechCount);

			if (onAnnounceResult)
				onAnnounceResult(announceResp);
		}
	}
}

DataBuffer mtt::HttpTracker::createAnnounceRequest(std::string path, std::string host, std::string port)
{
	PacketBuilder builder(500);
	builder << "GET " << path << "?info_hash=" << UrlEncode(torrent->hash(), 20);
	builder << "&peer_id=" << UrlEncode(mtt::config::getInternal().hashId, 20);
	builder << "&port=" << std::to_string(mtt::config::getExternal().connection.tcpPort);
	builder << "&uploaded=" << std::to_string(torrent->uploaded());
	builder << "&downloaded=" << std::to_string(torrent->downloaded());
	builder << "&left=" << std::to_string(torrent->dataLeft());
	builder << "&numwant=" << std::to_string(mtt::config::getInternal().maxPeersPerTrackerRequest);
	builder << "&compact=1&no_peer_id=0&key=" << std::to_string(mtt::config::getInternal().trackerKey);
	builder << "&event=" << (torrent->finished() ? "completed" : "started");
	builder << " HTTP/1.1\r\n";
	builder << "User-Agent: " << MT_NAME << "\r\n";
	builder << "Connection: close\r\n";
	builder << "Host: " << host;
	if (!port.empty())
		builder << ":" << port;
	builder << "\r\n";
	builder << "Cache-Control: no-cache\r\n\r\n";

	return builder.getBuffer();
}

uint32_t mtt::HttpTracker::readAnnounceResponse(const char* buffer, size_t bufferSize, AnnounceResponse& response)
{
	auto info = HttpHeaderInfo::read(buffer, bufferSize);

	if (!info.valid || !info.success)
		return -1;

	if (info.dataSize && info.dataStart && (info.dataStart + info.dataSize) <= bufferSize)
	{
		try
		{
			BencodeParser parser;
			parser.parse((const uint8_t*)buffer + info.dataStart, info.dataSize);
			auto root = parser.getRoot();

			if (root && root->isMap())
			{
				auto interval = root->getIntItem("min interval");
				if (!interval)
					interval = root->getIntItem("interval");

				response.interval = interval ? interval->getInt() : 5 * 60;

				auto seeds = root->getIntItem("complete");
				response.seedCount = seeds ? seeds->getInt() : 0;

				auto leechs = root->getIntItem("incomplete");
				response.leechCount = leechs ? leechs->getInt() : 0;

				auto peers = root->getTxtItem("peers");
				if (peers && peers->size % 6 == 0)
				{
					PacketReader reader(peers->data, peers->size);

					int count = peers->size / 6;
					for (int i = 0; i < count; i++)
					{
						uint32_t addr = swap32(reader.pop32());
						response.peers.push_back(Addr(addr, reader.pop16()));
					}
				}
			}
		}
		catch (...)
		{
		}

		return info.dataStart + info.dataSize;
	}

	return 0;
}

void mtt::HttpTrackerComm::announce()
{
	HTTP_TRACKER_LOG("announcing");

	if (!tcpComm)
		initializeStream();

	if (info.state == TrackerState::Announced)
		info.state = TrackerState::Reannouncing;
	else
		info.state = TrackerState::Announcing;

	auto request = createAnnounceRequest(info.path, info.hostname, info.port);

	tcpComm->write(request);
}
