#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"
#include "Configuration.h"
#include "Torrent.h"
#include "HttpsTrackerComm.h"

mtt::TrackerManager::TrackerManager(TorrentPtr t) : torrent(t)
{
}

void mtt::TrackerManager::start(AnnounceCallback callbk)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	announceCallback = callbk;

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
	std::lock_guard<std::mutex> guard(trackersMutex);

	stopAll();

	announceCallback = nullptr;
}

void mtt::TrackerManager::addTracker(std::string addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	TrackerInfo info;

	info.uri = Uri::Parse(addr);
	info.fullAddress = addr;

	if (!info.uri.port.empty())
	{
		auto i = findTrackerInfo(info.uri.host);

		if (i && i->uri.port == info.uri.port)
		{
			if ((i->uri.protocol == "http" || info.uri.protocol == "http") && (i->uri.protocol == "udp" || info.uri.protocol == "udp") && !i->comm)
			{
				i->uri.protocol = "udp";
				i->httpFallback = true;
			}
		}
		else
			trackers.push_back(info);
	}
	else if(info.uri.protocol == "https")
		trackers.push_back(info);
}

void mtt::TrackerManager::addTrackers(const std::vector<std::string>& trackers)
{
	for (auto& t : trackers)
	{
		addTracker(t);
	}
}

void mtt::TrackerManager::removeTrackers()
{
	std::lock_guard<std::mutex> guard(trackersMutex);
	trackers.clear();
}

std::shared_ptr<mtt::Tracker> mtt::TrackerManager::getTrackerByAddr(const std::string& addr)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& tracker : trackers)
	{
		if (tracker.fullAddress == addr)
			return tracker.comm;
	}

	return nullptr;
}

std::vector<std::pair<std::string, std::shared_ptr<mtt::Tracker>>> mtt::TrackerManager::getTrackers()
{
	std::vector<std::pair<std::string, std::shared_ptr<mtt::Tracker>>> out;

	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& t : trackers)
	{
		out.push_back({ t.fullAddress, t.comm });
	}

	return out;
}

uint32_t mtt::TrackerManager::getTrackersCount()
{
	return (uint32_t)trackers.size();
}

void mtt::TrackerManager::onAnnounce(AnnounceResponse& resp, Tracker* t)
{
	torrent->service.io.post([this, resp, t]()
		{
			std::lock_guard<std::mutex> guard(trackersMutex);

			if (auto trackerInfo = findTrackerInfo(t))
			{
				trackerInfo->retryCount = 0;
				trackerInfo->timer->schedule(resp.interval);
				trackerInfo->comm->info.nextAnnounce = (uint32_t)time(0) + resp.interval;
			}

			if (announceCallback)
				announceCallback(Status::Success, &resp, t);

			startNext();
		});
}

void mtt::TrackerManager::onTrackerFail(Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	if (auto trackerInfo = findTrackerInfo(t))
	{
		if (trackerInfo->httpFallback && !trackerInfo->httpFallbackUsed && trackerInfo->uri.protocol == "udp")
		{
			trackerInfo->httpFallbackUsed = true;
			trackerInfo->uri.protocol = "http";

			torrent->service.io.post([this, trackerInfo]()
				{
					trackerInfo->comm.reset();

					std::lock_guard<std::mutex> guard(trackersMutex);
					start(trackerInfo);
				});
		}
		else
		{
			if (trackerInfo->httpFallback && trackerInfo->httpFallbackUsed && trackerInfo->uri.protocol == "http")
			{
				trackerInfo->uri.protocol = "udp";
				start(trackerInfo);
			}
			else
			{
				trackerInfo->retryCount++;
				uint32_t nextRetry = 30 * trackerInfo->retryCount;
				trackerInfo->timer->schedule(nextRetry);
				trackerInfo->comm->info.nextAnnounce = (uint32_t)time(0) + nextRetry;
			}

			startNext();
		}

		if(announceCallback)
			announceCallback(Status::E_Unknown, nullptr, t);
	}
}

void mtt::TrackerManager::start(TrackerInfo* tracker)
{
	if (tracker->uri.protocol == "udp")
		tracker->comm = std::make_shared<UdpTrackerComm>();
	else if (tracker->uri.protocol == "http")
		tracker->comm = std::make_shared<HttpTrackerComm>();
#ifdef MTT_WITH_SSL
	else if (tracker->uri.protocol == "https")
		tracker->comm = std::make_shared<HttpsTrackerComm>();
#endif
	else
		return;

	tracker->comm->onFail = std::bind(&TrackerManager::onTrackerFail, this, tracker->comm.get());
	tracker->comm->onAnnounceResult = std::bind(&TrackerManager::onAnnounce, this, std::placeholders::_1, tracker->comm.get());

	tracker->comm->init(tracker->uri.host, tracker->uri.port, tracker->uri.path, torrent);
	tracker->timer = ScheduledTimer::create(torrent->service.io, std::bind(&Tracker::announce, tracker->comm.get()));
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
	for (auto& tracker : trackers)
	{
		if(tracker.comm)
			tracker.comm->deinit();
		tracker.comm = nullptr;
		if(tracker.timer)
			tracker.timer->disable();
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

mtt::TrackerManager::TrackerInfo* mtt::TrackerManager::findTrackerInfo(std::string host)
{
	for (auto& tracker : trackers)
	{
		if (tracker.uri.host == host)
			return &tracker;
	}

	return nullptr;
}
