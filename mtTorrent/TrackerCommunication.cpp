#include "TrackerCommunication.h"
#include "PacketHelper.h"
#include <iostream>

using namespace Torrent;

Torrent::TrackerCollector::TrackerCollector(ClientInfo* c, TorrentInfo* t)
{
	client = c;
	torrent = t;

	count = torrent->announceList.size();
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

std::vector<PeerInfo> Torrent::TrackerCollector::announceAll()
{
	count = torrent->announceList.size();

	size_t max = std::min<size_t>(20, count);
	std::future<std::vector<PeerInfo>> f[20];

	int count = 0;
	for (int i = 0; i < max; i++)
	{
		f[count] = std::async(&TrackerCollector::announce, this, i);
		count++;
	}

	std::vector<PeerInfo> resp;

	for (int i = 0; i < count; i++)
	{
		auto peers = f[i].get();

		for (auto& p : peers)
		{
			if (std::find(resp.begin(), resp.end(), p) == resp.end())
				resp.push_back(p);
		}
	}
	
	std::cout << "Unique peers: " << std::to_string(resp.size()) << "\n";

	return resp;
}

std::vector<PeerInfo> Torrent::TrackerCollector::announce(size_t id)
{
	std::vector<PeerInfo> out;

	if (torrent->announceList.size() > id)
	{
		UdpTrackerComm comm;
		comm.setInfo(client, torrent);

		auto url = torrent->announceList[id];

		std::string protocol = cutStringPart(url, { ':' }, 2);
		std::string hostname = cutStringPart(url, { ':', '/' }, 0);
		std::string port = cutStringPart(url, { '/' }, 0);

		if (protocol == "udp")
		{
			auto resp = comm.announceUdpTracker(hostname, port);
			out = resp.peers;
		}
	}

	return out;
}
