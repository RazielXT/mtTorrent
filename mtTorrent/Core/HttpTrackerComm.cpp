#include "HttpTrackerComm.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/UrlEncoding.h"
#include "Torrent.h"
#include "utils/HttpHeader.h"


#define HTTP_TRACKER_LOG(x) WRITE_GLOBAL_LOG(HttpTracker, x)

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
	std::lock_guard<std::mutex> guard(commMutex);

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
	tcpComm->onConnectCallback = std::bind(&HttpTrackerComm::onTcpConnected, shared_from_this());
	tcpComm->onCloseCallback = [this](int code) {onTcpClosed(code); };
	tcpComm->onReceiveCallback = std::bind(&HttpTrackerComm::onTcpReceived, shared_from_this(), std::placeholders::_1);

	tcpComm->init(info.hostname, info.port);
}

void mtt::HttpTrackerComm::fail()
{
	{
		std::lock_guard<std::mutex> guard(commMutex);
		tcpComm.reset();
	}

	if (info.state == TrackerState::Announcing)
	{
		if (info.lastAnnounce)
			info.state = TrackerState::Alive;
		else
			info.state = TrackerState::Offline;

		if (onFail)
			onFail();
	}
}

void mtt::HttpTrackerComm::onTcpClosed(int)
{
	if (info.state != TrackerState::Announced)
		fail();
}

void mtt::HttpTrackerComm::onTcpConnected()
{
	info.state = std::max(info.state, TrackerState::Alive);
}

size_t mtt::HttpTrackerComm::onTcpReceived(const BufferView& respData)
{
	if (info.state == TrackerState::Announcing)
	{
		mtt::AnnounceResponse announceResp;
		auto msgSize = readAnnounceResponse((const char*)respData.data, respData.size, announceResp);

		if (msgSize == -1)
		{
			fail();
			return respData.size;
		}

		if (msgSize != 0)
		{
			info.state = TrackerState::Announced;

			info.leeches = announceResp.leechCount;
			info.seeds = announceResp.seedCount;
			info.peers = (uint32_t)announceResp.peers.size();
			info.announceInterval = announceResp.interval;
			info.lastAnnounce = mtt::CurrentTimestamp();

			HTTP_TRACKER_LOG("received peers:" << announceResp.peers.size() << ", p: " << announceResp.seedCount << ", l: " << announceResp.leechCount);

			if (onAnnounceResult)
				onAnnounceResult(announceResp);
		}
		return msgSize;
	}
	return 0;
}

DataBuffer mtt::HttpTracker::createAnnounceRequest(std::string path, std::string host, std::string port)
{
	PacketBuilder builder(520);
	builder << "GET " << path << "?info_hash=" << UrlEncode(torrent->hash(), 20);
	builder << "&peer_id=" << UrlEncode(mtt::config::getInternal().hashId, 20);
	builder << "&port=" << std::to_string(mtt::config::getExternal().connection.tcpPort);
	builder << "&uploaded=" << std::to_string(torrent->uploaded());
	builder << "&downloaded=" << std::to_string(torrent->downloaded());
	builder << "&left=" << std::to_string(torrent->dataLeft());
	builder << "&numwant=" << std::to_string(mtt::config::getInternal().maxPeersPerTrackerRequest);
	builder << "&compact=1&no_peer_id=0&key=" << std::to_string(mtt::config::getInternal().trackerKey);
	builder << "&event=" << (torrent->finished() ? "completed" : "started");
	if (mtt::config::getExternal().connection.encryption == config::Encryption::Require)
		builder << "&requirecrypto=1";
	else if (mtt::config::getExternal().connection.encryption == config::Encryption::Allow)
		builder << "&supportcrypto=1";
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

uint32_t mtt::HttpTracker::readAnnounceResponse(const char* buffer, std::size_t bufferSize, AnnounceResponse& response)
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
				response.interval = root->getInt("min interval");
				if (!response.interval)
					response.interval = root->getInt("interval");
				if (!response.interval)
					response.interval = 5 * 60;

				response.seedCount = root->getInt("complete");
				response.leechCount = root->getInt("incomplete");

				auto peers = root->getTxtItem("peers");
				if (peers && peers->size % 6 == 0)
				{
					PacketReader reader(peers->data, peers->size);

					int count = peers->size / 6;
					for (int i = 0; i < count; i++)
					{
						uint32_t addr = swap32(reader.pop32());
						response.peers.emplace_back(addr, reader.pop16());
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

	std::lock_guard<std::mutex> guard(commMutex);

	if (!tcpComm)
		initializeStream();

	info.state = TrackerState::Announcing;
	info.nextAnnounce = 0;

	auto request = createAnnounceRequest(info.path, info.hostname, info.port);

	tcpComm->write(request);
}
