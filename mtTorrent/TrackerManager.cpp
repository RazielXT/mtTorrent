#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"

mtt::TrackerManager::TrackerManager(boost::asio::io_service& service, TrackerListener& l) : io(service), listener(l)
{
}

void mtt::TrackerManager::onAnnounce(AnnounceResponse& resp, Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& tracker : trackers)
	{
		if (tracker.comm.get() == t)
		{
			if (!tracker.timer)
				tracker.timer = TrackerTimer::create(io, std::bind(&Tracker::announce, t));

			tracker.retryCount = 0;
			tracker.timer->schedule(resp.interval);
		}
	}

	listener.onAnnounceResult(resp, torrent);
}

void mtt::TrackerManager::onTrackerFail(Tracker* t)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	for (auto& tracker : trackers)
	{
		if (tracker.comm.get() == t)
		{
			if (!tracker.timer)
				tracker.timer = TrackerTimer::create(io, std::bind(&Tracker::announce, t));

			tracker.retryCount++;
			tracker.timer->schedule(5*60* tracker.retryCount);
		}
	}
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

void mtt::TrackerManager::init(TorrentFileInfo* info)
{
	torrent = info;
	for (auto& t : info->announceList)
	{
		TrackerInfo info;
		
		auto url = t;
		info.protocol = cutStringPart(url, { ':' }, 2);
		info.host = cutStringPart(url, { ':', '/' }, 0);
		info.port = cutStringPart(url, { '/' }, 0);

		if(info.port.empty())
			continue;

		trackers.push_back(info);
	}
}

void mtt::TrackerManager::addTrackers(std::vector<std::string> trackers)
{

}

void mtt::TrackerManager::start()
{
	auto& t = trackers[0];

	if (!t.comm)
	{
		if (t.protocol == "udp")
			t.comm = std::make_shared<UdpTrackerComm>();
		else if (t.protocol == "http")
			t.comm = std::make_shared<HttpTrackerComm>();
		else
			return;

		t.comm->onFail = std::bind(&TrackerManager::onTrackerFail, this, t.comm.get());
		t.comm->onAnnounceResult = std::bind(&TrackerManager::onAnnounce, this, std::placeholders::_1, t.comm.get());

		t.comm->init(t.host, t.port, io, torrent);
	}

	t.comm->announce();
}

void mtt::TrackerManager::stop()
{
	for (auto& t : trackers)
	{
		t.comm = nullptr;
	}
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
