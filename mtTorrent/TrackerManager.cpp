#include "TrackerManager.h"
#include "HttpTrackerComm.h"
#include "UdpTrackerComm.h"

mtt::TrackerManager::TrackerManager(boost::asio::io_service& service, TrackerListener& l) : io(service), listener(l)
{
}

void mtt::TrackerManager::onAnnounce(AnnounceResponse& resp, Tracker*)
{
	std::lock_guard<std::mutex> guard(trackersMutex);

	listener.onAnnounceResult(resp, torrent);
}

void mtt::TrackerManager::onTrackerFail(Tracker*)
{
	std::lock_guard<std::mutex> guard(trackersMutex);
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
