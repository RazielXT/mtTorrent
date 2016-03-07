#include "Communicator.h"
#include "PacketHelper.h"
#include "Network.h"

#include <sstream>
#include <future>
#include <iostream>

using namespace Torrent;

std::string cutStringPart(std::string& source, std::vector<char> endChars, int cutAdd)
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

std::vector<char> Communicator::getConnectRequest()
{
	auto transaction = generateTransaction();
	uint64_t connectId = 0x41727101980;

	PacketBuilder packet;
	packet.add64(connectId);
	packet.add32(Connnect);
	packet.add32(transaction);

	return packet.getBuffer();
}

void Communicator::generatePeerId()
{
	peerId[0] = 'M';
	peerId[1] = 'T';
	peerId[2] = '0';
	peerId[3] = '-';
	peerId[4] = '1';
	peerId[5] = '-';
	for (size_t i = 6; i < 20; i++)
	{
		peerId[i] = static_cast<uint8_t>(rand()%255);
	}
}

std::vector<char> Communicator::getAnnouncingRequest(ConnectMessage& response)
{
	auto transaction = generateTransaction();

	PacketBuilder packet;
	packet.add64(response.connectionId);
	packet.add32(Announce);
	packet.add32(transaction);

	auto& iHash = torrent.info.infoHash;
	packet.add(iHash.data(), iHash.size());

	generatePeerId();
	packet.add(peerId, 20);

	packet.add64(0);
	packet.add64(0);
	packet.add64(0);

	packet.add32(Started);
	packet.add32(0);

	packet.add32(key);
	packet.add32(maxPeers);
	packet.add32(listenPort);
	packet.add16(0);

	return packet.getBuffer();
}

ConnectMessage getConnectResponse(std::vector<char> buffer)
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

void Communicator::initIds()
{
	srand(time(NULL));

	generatePeerId();
	key = static_cast<uint32_t>(rand());
}

AnnounceResponse getAnnounceResponse(std::vector<char> buffer)
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
		p.ip = packet.pop32();
		p.port = packet.pop16();

		p.ipAddr[3] = *reinterpret_cast<uint8_t*>(&p.ip);
		p.ipAddr[2] = *(reinterpret_cast<uint8_t*>(&p.ip) + 1);
		p.ipAddr[1] = *(reinterpret_cast<uint8_t*>(&p.ip) + 2);
		p.ipAddr[0] = *(reinterpret_cast<uint8_t*>(&p.ip) + 3);

		p.ipStr = std::to_string(p.ipAddr[0]) + "." + std::to_string(p.ipAddr[1]) + "." + std::to_string(p.ipAddr[2]) + "." + std::to_string(p.ipAddr[3]);
		p.index = static_cast<uint16_t>(i);

		resp.peers.push_back(p);
	}

	return resp;
}

uint32_t Communicator::generateTransaction()
{
	return static_cast<uint32_t>(rand());
}

AnnounceResponse Communicator::announceUdpTracker(std::string host, std::string port)
{
	std::cout << "Announcing to tracker " << host << "\n";

	AnnounceResponse announceMsg;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock(io_service);
	sock.open(udp::v4());

	boost::asio::ip::udp::endpoint local(
		boost::asio::ip::address_v4::any(),
		listenPort);
	sock.bind(local);

	auto messageData = getConnectRequest();

	auto message = sendUdpRequest(sock, resolver, messageData, host.data(), port.data());

	auto response = getConnectResponse(message);

	if (response.action == 0)
	{
		auto announce = getAnnouncingRequest(response);

		auto announceMsgBuffer = sendUdpRequest(sock, resolver, announce, host.data(), port.data());

		announceMsg = getAnnounceResponse(announceMsgBuffer);
	}

	return announceMsg;
}

void Communicator::test()
{
	if (!torrent.parse("D:\\dead.torrent"))
		return;

	auto url = torrent.info.announceList[8];

	std::string protocol = cutStringPart(url, { ':' }, 2);
	std::string hostname = cutStringPart(url, { ':', '/' }, 0);
	std::string port = cutStringPart(url, { '/' }, 0);

	if (port.empty())
		port = "6969";

	if (protocol == "udp")
	{
		auto resp = announceUdpTracker(hostname, port);

		std::cout << "Tracker " << hostname << " returned " << std::to_string(resp.peers.size()) << " peers\n";

		if (resp.peers.size())
		{
			PeerCommunication peer[100];

			std::future<void> futures[100];

			for (size_t i = 0; i < resp.peers.size(); i++)
			{
				futures[i] = std::async(&PeerCommunication::start, &peer[i], &torrent.info, peerId, resp.peers[i]);
			}			

			for (size_t i = 0; i < resp.peers.size(); i++)
			{
				futures[i].get();
			}
		}
	}
	
}
