#include "TrackerCommunication.h"
#include "PacketHelper.h"
#include <iostream>
#include "HttpTrackerComm.h"
#include <thread>

using namespace mtt;

mtt::TrackerCollector::TrackerCollector(TorrentFileInfo* t)
{
	torrent = t;
	trackersCount = torrent->announceList.size();
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

void mtt::TrackerCollector::announceAsync()
{
	size_t max = std::min<size_t>(20, trackersCount);
	TRACKER_LOG("Announcing " << max << " trackers...\n");
	collectingTodo = max;

	for (int i = 0; i < max; i++)
	{
		std::thread([this,i]()
		{
			auto r = announce(i);

			std::lock_guard<std::mutex> guard(resultsMutex);

			collectingTodo--;

			for (auto& p : r)
			{
				if (std::find(asyncResults.begin(), asyncResults.end(), p) == asyncResults.end())
					asyncResults.push_back(p);
			}
		}
		).detach();
	}
}

void mtt::TrackerCollector::waitForAnyResults()
{
	while (collectingTodo>0 && asyncResults.empty())
	{
		Sleep(100);
	}
}

std::vector<PeerInfo> mtt::TrackerCollector::getResults()
{
	waitForAnyResults();

	std::lock_guard<std::mutex> guard(resultsMutex);

	auto r = asyncResults;
	asyncResults.clear();

	return r;
}

std::vector<PeerInfo> mtt::TrackerCollector::announce(size_t id)
{
	std::vector<PeerInfo> out;

	if (torrent->announceList.size() > id)
	{
		auto url = torrent->announceList[id];

		std::string protocol = cutStringPart(url, { ':' }, 2);
		std::string hostname = cutStringPart(url, { ':', '/' }, 0);
		std::string port = cutStringPart(url, { '/' }, 0);

		if (protocol == "udp")
		{
			UdpTrackerComm comm(torrent);
			auto resp = comm.announceTracker(hostname, port);
			out = resp.peers;
		}
		if (protocol == "http")
		{
			HttpTrackerComm comm(torrent);
			auto resp = comm.announceTracker(hostname, port);
			out = resp.peers;
		}
	}

	return out;
}
