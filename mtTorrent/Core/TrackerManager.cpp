#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"
#include "Configuration.h"

mtt::TrackerManager::TrackerManager()
{
}

std::string cutStringPart(std::string& source, DataBuffer endChars, int cutAdd)
{
	auto id = source.find(endChars[0]);

	for (auto c : endChars)
	{
		auto nid = source.find(c);

		if (nid < id)
			id = nid;
	}

	if (id == std::string::npos)
		return "";

	std::string ret = source.substr(0, id);
	source = source.substr(id + 1 + cutAdd, std::string::npos);

	return ret;
}

void mtt::TrackerManager::add(TorrentPtr torrent, std::vector<std::string> trackers, TrackerListener& listener)
{
	std::shared_ptr<TorrentTrackers> target;

	for (auto& torrentTrackers : torrents)
	{
		if (torrentTrackers->torrent == torrent)
			target = torrentTrackers;
	}

	if (!target)
	{
		target = std::make_shared<TorrentTrackers>(service.io, listener);
		target->torrent = torrent;
		torrents.push_back(target);
	}

	for (auto& t : trackers)
	{
		target->addTracker(t);
	}

	service.adjust((uint32_t)torrents.size());
}

void mtt::TrackerManager::start(TorrentPtr torrent)
{
	for (auto& torrentTrackers : torrents)
	{
		if(torrentTrackers->torrent == torrent)
		{
			for (size_t i = 0; i < 3 && i < torrentTrackers->trackers.size(); i++)
			{
				auto& t = torrentTrackers->trackers[i];

				if (!t.comm)
				{
					torrentTrackers->start(&t);
				}
			}
		}
	}
}

void mtt::TrackerManager::stop(TorrentPtr torrent)
{
	for (auto& torrentTrackers : torrents)
	{
		if (torrentTrackers->torrent == torrent)
			torrentTrackers->stopAll();
	}
}

void mtt::TrackerManager::TorrentTrackers::addTracker(std::string addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	TorrentTrackers::TrackerInfo info;

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

mtt::TrackerManager::TorrentTrackers::TorrentTrackers(boost::asio::io_service& service, TrackerListener& l) : io(service), listener(l), udpMgr(service, mtt::config::external.listenPortUdp)
{
}

void mtt::TrackerManager::TorrentTrackers::onAnnounce(AnnounceResponse& resp, Tracker* t)
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

void mtt::TrackerManager::TorrentTrackers::onTrackerFail(Tracker* t)
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

void mtt::TrackerManager::TorrentTrackers::start(TrackerInfo* tracker)
{
	if (tracker->protocol == "udp")
		tracker->comm = std::make_shared<UdpTrackerComm>(udpMgr);
	else if (tracker->protocol == "http")
		tracker->comm = std::make_shared<HttpTrackerComm>();
	else
		return;

	tracker->comm = std::make_shared<HttpTrackerComm>();

	tracker->comm->onFail = std::bind(&TrackerManager::TorrentTrackers::onTrackerFail, this, tracker->comm.get());
	tracker->comm->onAnnounceResult = std::bind(&TrackerManager::TorrentTrackers::onAnnounce, this, std::placeholders::_1, tracker->comm.get());

	tracker->comm->init(tracker->host, tracker->port, io, torrent);
	tracker->timer = TrackerTimer::create(io, std::bind(&Tracker::announce, tracker->comm.get()));
	tracker->retryCount = 0;

	tracker->comm->announce();
}

void mtt::TrackerManager::TorrentTrackers::startNext()
{
	for (auto& tracker : trackers)
	{
		if (!tracker.comm)
			start(&tracker);
	}
}

void mtt::TrackerManager::TorrentTrackers::stopAll()
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& tracker : trackers)
	{
		tracker.comm = nullptr;
		tracker.timer = nullptr;
		tracker.retryCount = 0;
	}
}

mtt::TrackerManager::TorrentTrackers::TrackerInfo* mtt::TrackerManager::TorrentTrackers::findTrackerInfo(Tracker* t)
{
	for (auto& tracker : trackers)
	{
		if (tracker.comm.get() == t)
			return &tracker;
	}

	return nullptr;
}

mtt::TrackerStateInfo mtt::TrackerManager::TorrentTrackers::TrackerInfo::getStateInfo()
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

mtt::TrackerManager::TorrentTrackers::TrackerInfo* mtt::TrackerManager::TorrentTrackers::findTrackerInfo(std::string host)
{
	for (auto& tracker : trackers)
	{
		if (tracker.host == host)
			return &tracker;
	}

	return nullptr;
}

mtt::TrackerTimer::TrackerTimer(boost::asio::io_service& io, std::function<void()> callback) : timer(io), func(callback)
{
}

void mtt::TrackerTimer::schedule(uint32_t secondsOffset)
{
	timer.async_wait(std::bind(&TrackerTimer::checkTimer, shared_from_this()));
	timer.expires_from_now(boost::posix_time::seconds(secondsOffset));
}

void mtt::TrackerTimer::disable()
{
	func = nullptr;
	timer.expires_at(boost::posix_time::pos_infin);
}

uint32_t mtt::TrackerTimer::getSecondsTillNextUpdate()
{
	if (timer.expires_at().is_infinity())
		return 0;
	else if (timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
		return 0;
	else
	{
		auto time = timer.expires_at() - boost::asio::deadline_timer::traits_type::now();
		return (uint32_t)time.total_seconds();
	}
}

void mtt::TrackerTimer::checkTimer()
{
	if (timer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		timer.expires_at(boost::posix_time::pos_infin);

		if (func)
			func();
	}

	timer.async_wait(std::bind(&TrackerTimer::checkTimer, shared_from_this()));
}

std::shared_ptr<mtt::TrackerTimer> mtt::TrackerTimer::create(boost::asio::io_service& io, std::function<void()> callback)
{
	return std::make_shared<mtt::TrackerTimer>(io, callback);
}
