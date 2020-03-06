#include "HttpsTrackerComm.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/UrlEncoding.h"
#include "Torrent.h"
#include "utils/HttpHeader.h"

#ifdef MTT_WITH_SSL

#define HTTP_TRACKER_LOG(x) WRITE_LOG(LogTypeHttpTracker, x)

using namespace mtt;

mtt::HttpsTrackerComm::HttpsTrackerComm() : ctx(asio::ssl::context::tls)
{
}

mtt::HttpsTrackerComm::~HttpsTrackerComm()
{
	deinit();
}

void mtt::HttpsTrackerComm::deinit()
{
	socket.reset();
}

void mtt::HttpsTrackerComm::init(std::string host, std::string path, TorrentPtr t)
{
	info.hostname = host;
	urlpath = path;
	torrent = t;

	info.state = TrackerState::Initialized;
}

void mtt::HttpsTrackerComm::initializeStream()
{
	ctx.set_default_verify_paths();
	socket = std::make_shared<ssl_socket>(torrent->service.io, ctx);

	tcp::resolver resolver(torrent->service.io);
	openSslSocket(*socket, resolver, info.hostname.data());
}

void mtt::HttpsTrackerComm::fail()
{
	socket.reset();

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

void mtt::HttpsTrackerComm::onTcpClosed(int)
{
	if (info.state != TrackerState::Announced)
		fail();
}

void mtt::HttpsTrackerComm::onTcpConnected()
{
	info.state = std::max(info.state, TrackerState::Alive);
}

void mtt::HttpsTrackerComm::onTcpReceived(std::string& responseData)
{
	if (info.state == TrackerState::Announcing || info.state == TrackerState::Reannouncing)
	{
		mtt::AnnounceResponse announceResp;
		auto msgSize = readAnnounceResponse(responseData.data(), responseData.size(), announceResp);

		if (msgSize == 0)
			return;
		else if (msgSize == -1)
		{
			//tcpComm->consumeData(respData.size());
			fail();
			return;
		}
		else
		{
			//tcpComm->consumeData(msgSize);
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

void mtt::HttpsTrackerComm::announce()
{
	HTTP_TRACKER_LOG("announcing");

	auto hashStr = torrent->hashString();
	auto request = createAnnounceRequest(urlpath, info.hostname, "");

	std::string reqStr((char*)request.data(), request.size());

	initializeStream();

	if (info.state == TrackerState::Announced)
		info.state = TrackerState::Reannouncing;
	else
		info.state = TrackerState::Announcing;


	asio::streambuf buffer;
	std::ostream request_stream(&buffer);
	request_stream.write((char*)request.data(), request.size());

	auto response = sendHttpsRequest(*socket, buffer);
	
	onTcpReceived(response);

	socket.reset();
}

#endif // MTT_WITH_SSL
