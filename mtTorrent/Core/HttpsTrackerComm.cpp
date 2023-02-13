#include "HttpsTrackerComm.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/UrlEncoding.h"
#include "Torrent.h"
#include "utils/HttpHeader.h"

#ifdef MTT_WITH_SSL

#define HTTP_TRACKER_LOG(x) WRITE_GLOBAL_LOG(HttpTracker, x)

using namespace mtt;

mtt::HttpsTrackerComm::HttpsTrackerComm()
{
}

mtt::HttpsTrackerComm::~HttpsTrackerComm()
{
	deinit();
}

void mtt::HttpsTrackerComm::deinit()
{
	std::lock_guard<std::mutex> guard(commMutex);

	stream->stop();
}

void mtt::HttpsTrackerComm::init(std::string host, std::string port, std::string path, TorrentPtr t)
{
	info.hostname = host;
	info.path = path;
	info.port = port;
	torrent = t;

	info.state = TrackerState::Initialized;

	stream = std::make_shared<SslAsyncStream>(torrent->service.io);
	stream->init(host, "https");
}

void mtt::HttpsTrackerComm::fail()
{
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

void mtt::HttpsTrackerComm::onTcpReceived(const BufferView& responseData)
{
	if (info.state == TrackerState::Announcing)
	{
		mtt::AnnounceResponse announceResp;
		auto msgSize = readAnnounceResponse((const char*)responseData.data, responseData.size, announceResp);

		if (msgSize == 0)
			return;
		else if (msgSize == -1)
		{
			fail();
			return;
		}
		else
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
	}
}

void mtt::HttpsTrackerComm::announce()
{
	HTTP_TRACKER_LOG("announcing");

	info.state = TrackerState::Announcing;
	info.nextAnnounce = 0;

	std::lock_guard<std::mutex> guard(commMutex);

	auto request = createAnnounceRequest(info.path, info.hostname, info.port);

	stream->write(request, [this](const BufferView& response)
	{
		if (response.size == 0)
			fail();
		else
			onTcpReceived(response);
	});
}

#endif // MTT_WITH_SSL
