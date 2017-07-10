#include "UdpTrackerComm.h"
#include "PacketHelper.h"
#include "Network.h"
#include <iostream>
#include "Configuration.h"

using namespace mtt;

ConnectResponse UdpTrackerComm::getConnectResponse(DataBuffer buffer)
{
	ConnectResponse out;

	if (buffer.size() >= sizeof ConnectResponse)
	{
		PacketReader packet(buffer);

		out.action = packet.pop32();
		out.transaction = packet.pop32();
		out.connectionId = packet.pop64();
	}

	return out;
}

AnnounceResponse UdpTrackerComm::getAnnounceResponse(DataBuffer buffer)
{
	PacketReader packet(buffer);

	AnnounceResponse resp;

	auto action = packet.pop32();
	auto transaction = packet.pop32();

	if (!validResponse(TrackerMessage{ action, transaction }) || buffer.size() < 26)
		return resp;

	resp.interval = packet.pop32();
	resp.leechCount = packet.pop32();
	resp.seedCount = packet.pop32();

	size_t count = static_cast<size_t>(packet.getRemainingSize() / 6.0f);

	for (size_t i = 0; i < count; i++)
	{
		uint32_t ip = packet.pop32();
		resp.peers.push_back(Addr((char*)&ip, packet.pop16(), false));
	}

	return resp;
}

AnnounceResponse UdpTrackerComm::announceTracker(std::string host, std::string port)
{
	TRACKER_LOG("Announcing to UDP tracker " << host << "\n");

	AnnounceResponse announceMsg;

	boost::asio::io_service io_service;
	udp::resolver resolver(io_service);

	udp::socket sock(io_service);
	sock.open(udp::v4());

	/*boost::asio::ip::udp::endpoint local(
	boost::asio::ip::address_v4::any(),
	client->listenPort);
	sock.bind(local);*/

	auto messageData = createConnectRequest();

	try
	{
		auto message = sendUdpRequest(sock, resolver, messageData, host.data(), port.data(), 5000);
		auto response = getConnectResponse(message);

		if (validResponse(response))
		{
			auto announce = createAnnounceRequest(response);
			auto announceMsgBuffer = sendUdpRequest(sock, resolver, announce, host.data(), port.data());
			announceMsg = getAnnounceResponse(announceMsgBuffer);
		}

		TRACKER_LOG("Udp Tracker " << host << " returned peers:" << std::to_string(announceMsg.peers.size()) << ", p: " << std::to_string(announceMsg.seedCount) << ", l: " << std::to_string(announceMsg.leechCount) << "\n");
	}
	catch (const std::exception&e)
	{
		TRACKER_LOG("Udp " << host << " exception: " << e.what() << "\n");
	}

	return announceMsg;
}

DataBuffer UdpTrackerComm::createAnnounceRequest(ConnectResponse& response)
{
	auto transaction = (uint32_t)rand();

	lastMessage = { Announce, transaction };

	PacketBuilder packet;
	packet.add64(response.connectionId);
	packet.add32(Announce);
	packet.add32(transaction);

	packet.add(torrent->info.hash, 20);
	packet.add(mtt::config::internal.hashId, 20);

	packet.add64(0);
	packet.add64(0);
	packet.add64(0);

	packet.add32(Started);
	packet.add32(0);

	packet.add32(mtt::config::internal.key);
	packet.add32(mtt::config::external.maxPeersPerRequest);
	packet.add32(mtt::config::external.listenPort);
	packet.add16(0);

	return packet.getBuffer();
}

DataBuffer UdpTrackerComm::createConnectRequest()
{
	auto transaction = (uint32_t)rand();
	uint64_t connectId = 0x41727101980;

	lastMessage = { Connnect, transaction };

	PacketBuilder packet;
	packet.add64(connectId);
	packet.add32(Connnect);
	packet.add32(transaction);

	return packet.getBuffer();
}

bool mtt::UdpTrackerComm::validResponse(TrackerMessage& resp)
{
	return resp.action == lastMessage.action && resp.transaction == lastMessage.transaction;
}

mtt::UdpTrackerComm::UdpTrackerComm(TorrentFileInfo* t) : torrent(t)
{
}
