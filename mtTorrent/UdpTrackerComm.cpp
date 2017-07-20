#include "UdpTrackerComm.h"
#include "PacketHelper.h"
#include "Network.h"
#include <iostream>
#include "Configuration.h"

using namespace mtt;

mtt::UdpTrackerComm::UdpTrackerComm()
{
}

void UdpTrackerComm::startTracker(std::string host, std::string port, boost::asio::io_service& io, TorrentFileInfo* t)
{
	hostname = host;
	torrent = t;

	TRACKER_LOG("Connecting to UDP tracker " << hostname << "\n");

	udpComm = SendAsyncUdp(host, port, false, createConnectRequest(), io, std::bind(&UdpTrackerComm::onConnectUdpResponse, this, std::placeholders::_1, std::placeholders::_2));
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

void mtt::UdpTrackerComm::onConnectUdpResponse(DataBuffer* data, PackedUdpRequest* source)
{
	if (!data)
		return;

	auto response = getConnectResponse(*data);

	if (validResponse(response))
	{
		connectionId = response.connectionId;

		TRACKER_LOG("Announcing to UDP tracker " << hostname << "\n");

		udpComm->write(createAnnounceRequest(), std::bind(&UdpTrackerComm::onAnnounceUdpResponse, this, std::placeholders::_1, std::placeholders::_2));
	}
}

DataBuffer UdpTrackerComm::createAnnounceRequest()
{
	auto transaction = (uint32_t)rand();

	lastMessage = { Announce, transaction };

	PacketBuilder packet;
	packet.add64(connectionId);
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

void mtt::UdpTrackerComm::onAnnounceUdpResponse(DataBuffer* data, PackedUdpRequest* source)
{
	if (!data)
		return;

	auto announceMsg = getAnnounceResponse(*data);

	TRACKER_LOG("Udp Tracker " << hostname << " returned peers:" << std::to_string(announceMsg.peers.size()) << ", p: " << std::to_string(announceMsg.seedCount) << ", l: " << std::to_string(announceMsg.leechCount) << "\n");

	if (onAnnounceResponse)
		onAnnounceResponse(announceMsg);
}

bool mtt::UdpTrackerComm::validResponse(TrackerMessage& resp)
{
	return resp.action == lastMessage.action && resp.transaction == lastMessage.transaction;
}

UdpTrackerComm::ConnectResponse UdpTrackerComm::getConnectResponse(DataBuffer buffer)
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
		resp.peers.push_back(Addr(ip, packet.pop16()));
	}

	return resp;
}
