#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"
#include "Configuration.h"

mtt::TrackerManager::TrackerManager(TorrentPtr t, std::vector<std::string> tr, TrackerListener& l, boost::asio::io_service& ioService, UdpAsyncComm& udpMgr) : udp(udpMgr), io(ioService), listener(l), torrent(t)
{
	for (auto& t : tr)
	{
		addTracker(t);
	}
}

std::string cutStringPart(std::string& source, DataBuffer endChars, int cutAdd)
{
	size_t id = std::string::npos;

	for (auto c : endChars)
	{
		auto nid = source.find(c);

		if (nid < id)
			id = nid;
	}

	std::string ret = source.substr(0, id);

	if (id == std::string::npos)
		source = "";
	else
		source = source.substr(id + 1 + cutAdd, std::string::npos);

	return ret;
}

void mtt::TrackerManager::start()
{
	for (size_t i = 0; i < 3 && i < trackers.size(); i++)
	{
		auto& t = trackers[i];

		if (!t.comm)
		{
			start(&t);
		}
	}
}

void mtt::TrackerManager::stop()
{
	stopAll();
}

void mtt::TrackerManager::addTracker(std::string addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	TrackerInfo info;

	info.protocol = cutStringPart(addr, { ':' }, 2);
	info.host = cutStringPart(addr, { ':', '/' }, 0);
	info.port = cutStringPart(addr, { '/' }, 0);

	if (!info.port.empty())
	{
		auto i = findTrackerInfo(info.host);

		if (i && i->port == info.port)
		{
			if ((i->protocol == "http" || info.protocol == "http") && (i->protocol == "udp" || info.protocol == "udp") && !i->comm)
			{
				i->protocol = "udp";
				i->httpFallback = true;
			}
		}
		else
			trackers.push_back(info);
	}
}

void mtt::TrackerManager::onAnnounce(AnnounceResponse& resp, Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	if(auto trackerInfo = findTrackerInfo(t))
	{
		trackerInfo->retryCount = 0;
		trackerInfo->timer->schedule(resp.interval);

		listener.trackerStateChanged(trackerInfo->getStateInfo(), torrent);
	}

	listener.onAnnounceResult(resp, torrent);

	startNext();
}

void mtt::TrackerManager::onTrackerFail(Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	if (auto trackerInfo = findTrackerInfo(t))
	{
		if (trackerInfo->httpFallback && !trackerInfo->httpFallbackUsed && trackerInfo->protocol == "udp")
		{
			trackerInfo->httpFallbackUsed = true;
			trackerInfo->protocol = "http";
			start(trackerInfo);
		}
		else
		{
			if (trackerInfo->httpFallback && trackerInfo->httpFallbackUsed && trackerInfo->protocol == "http")
			{
				trackerInfo->protocol = "udp";
				start(trackerInfo);
			}
			else
			{
				trackerInfo->retryCount++;
				trackerInfo->timer->schedule(5 * 60 * trackerInfo->retryCount);
			}

			startNext();
		}

		listener.trackerStateChanged(trackerInfo->getStateInfo(), torrent);
	}
}

void mtt::TrackerManager::start(TrackerInfo* tracker)
{
	if (tracker->protocol == "udp")
		tracker->comm = std::make_shared<UdpTrackerComm>(udp);
	else if (tracker->protocol == "http")
		tracker->comm = std::make_shared<HttpTrackerComm>();
	else
		return;

	tracker->comm->onFail = std::bind(&TrackerManager::onTrackerFail, this, tracker->comm.get());
	tracker->comm->onAnnounceResult = std::bind(&TrackerManager::onAnnounce, this, std::placeholders::_1, tracker->comm.get());

	tracker->comm->init(tracker->host, tracker->port, io, torrent);
	tracker->timer = ScheduledTimer::create(io, std::bind(&Tracker::announce, tracker->comm.get()));
	tracker->retryCount = 0;

	tracker->comm->announce();
}

void mtt::TrackerManager::startNext()
{
	for (auto& tracker : trackers)
	{
		if (!tracker.comm)
			start(&tracker);
	}
}

void mtt::TrackerManager::stopAll()
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& tracker : trackers)
	{
		tracker.comm = nullptr;
		tracker.timer = nullptr;
		tracker.retryCount = 0;
	}
}

mtt::TrackerManager::TrackerInfo* mtt::TrackerManager::findTrackerInfo(Tracker* t)
{
	for (auto& tracker : trackers)
	{
		if (tracker.comm.get() == t)
			return &tracker;
	}

	return nullptr;
}

mtt::TrackerStateInfo mtt::TrackerManager::TrackerInfo::getStateInfo()
{
	TrackerStateInfo out;
	out.host = host;
	out.fullAddress = protocol + "://" + host + ":" + port;

	out.nextUpdate = timer ? timer->getSecondsTillNextUpdate() : 0;

	out.updateInterval = comm->info.announceInterval;
	out.state = comm->state;
	out.peers = comm->info.peers;
	out.seeds = comm->info.seeds;
	out.leechers = comm->info.leechers;

	return out;
}

mtt::TrackerManager::TrackerInfo* mtt::TrackerManager::findTrackerInfo(std::string host)
{
	for (auto& tracker : trackers)
	{
		if (tracker.host == host)
			return &tracker;
	}

	return nullptr;
}
