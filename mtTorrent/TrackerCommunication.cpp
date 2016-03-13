#include "TrackerCommunication.h"
#include "PacketHelper.h"
#include "Network.h"
#include <iostream>

using namespace Torrent;

uint32_t Torrent::generateTransaction()
{
	return static_cast<uint32_t>(rand());
}

ConnectMessage getConnectResponse(DataBuffer buffer)
{
	ConnectMessage out;

	if (buffer.size() >= sizeof ConnectMessage)
	{
		PacketReader packet(buffer);

		out.action = packet.pop32();
		out.transactionId = packet.pop32();
		out.connectionId = packet.pop64();
	}

	return out;
}

AnnounceResponse getAnnounceResponse(DataBuffer buffer)
{
	PacketReader packet(buffer);

	AnnounceResponse resp;
	resp.action = packet.pop32();
	resp.transaction = packet.pop32();
	resp.interval = packet.pop32();

	resp.leechersCount = packet.pop32();
	resp.seedersCount = packet.pop32();

	size_t count = static_cast<size_t>(packet.getRemainingSize() / 6.0f);

	for (size_t i = 0; i < count; i++)
	{
		PeerInfo p;
		p.setIp(packet.pop32());
		p.port = packet.pop16();

		resp.peers.push_back(p);
	}

	return resp;
}

AnnounceResponse TrackerCommunication::announceUdpTracker(std::string host, std::string port)
{
	std::cout << "Announcing to tracker " << host << "\n";

	AnnounceResponse announceMsg;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock(io_service);
	sock.open(udp::v4());

	/*boost::asio::ip::udp::endpoint local(
		boost::asio::ip::address_v4::any(),
		client->listenPort);
	sock.bind(local);*/

	auto messageData = getConnectRequest();

	try
	{
		auto message = sendUdpRequest(sock, resolver, messageData, host.data(), port.data(), 5000);
		auto response = getConnectResponse(message);

		if (response.action == 0)
		{
			auto announce = getAnnouncingRequest(response);
			auto announceMsgBuffer = sendUdpRequest(sock, resolver, announce, host.data(), port.data());
			announceMsg = getAnnounceResponse(announceMsgBuffer);
		}

		std::cout << "Tracker " << host << " returned peers:" << std::to_string(announceMsg.peers.size()) << ", p: " << std::to_string(announceMsg.seedersCount) << ", l: " << std::to_string(announceMsg.leechersCount) << "\n";
	}
	catch (const std::exception&e)
	{
		std::cout << "Udp " << host << " exception: " << e.what() << "\n";
	}

	return announceMsg;
}

DataBuffer TrackerCommunication::getAnnouncingRequest(ConnectMessage& response)
{
	auto transaction = generateTransaction();

	PacketBuilder packet;
	packet.add64(response.connectionId);
	packet.add32(Announce);
	packet.add32(transaction);

	auto& iHash = torrent->infoHash;
	packet.add(iHash.data(), iHash.size());

	packet.add(client->hashId, 20);

	packet.add64(0);
	packet.add64(0);
	packet.add64(0);

	packet.add32(Started);
	packet.add32(0);

	packet.add32(client->key);
	packet.add32(client->maxPeersPerRequest);
	packet.add32(client->listenPort);
	packet.add16(0);

	return packet.getBuffer();
}

DataBuffer TrackerCommunication::getConnectRequest()
{
	auto transaction = generateTransaction();
	uint64_t connectId = 0x41727101980;

	PacketBuilder packet;
	packet.add64(connectId);
	packet.add32(Connnect);
	packet.add32(transaction);

	return packet.getBuffer();
}

void Torrent::TrackerCommunication::setInfo(ClientInfo* c, TorrentInfo* t)
{
	client = c;
	torrent = t;
}

Torrent::TrackerCommunication::TrackerCommunication()
{
}

Torrent::TrackerCollector::TrackerCollector(ClientInfo* c, TorrentInfo* t)
{
	client = c;
	torrent = t;
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
	size_t max = std::min<size_t>(10, torrent->announceList.size());
	std::future<Torrent::AnnounceResponse> f[10];
	TrackerCommunication comm[10];
	int count = 0;

	for (int i = 0; i < max; i++)
	{
		auto url = torrent->announceList[i];

		std::string protocol = cutStringPart(url, { ':' }, 2);
		std::string hostname = cutStringPart(url, { ':', '/' }, 0);
		std::string port = cutStringPart(url, { '/' }, 0);

		if (protocol == "udp")
		{
			comm[count].setInfo(client, torrent);
			f[count] = std::async(&TrackerCommunication::announceUdpTracker, &comm[count], hostname, port);
			count++;
		}
		
	}

	std::vector<PeerInfo> resp;

	for (int i = 0; i < count; i++)
	{
		Torrent::AnnounceResponse r = f[i].get();

		for (auto& p : r.peers)
		{
			if (std::find(resp.begin(), resp.end(), p) == resp.end())
				resp.push_back(p);
		}
	}
	
	std::cout << "Unique peers: " << std::to_string(resp.size()) << "\n";

	return resp;
}
