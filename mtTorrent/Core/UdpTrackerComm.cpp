#include "UdpTrackerComm.h"
#include "utils/PacketHelper.h"
#include "utils/Network.h"
#include "Configuration.h"
#include "Logging.h"
#include "Core.h"

#define UDP_TRACKER_LOG(x) WRITE_LOG("UDP Tracker " << info.hostname << " " << x)

using namespace mtt;

mtt::UdpTrackerComm::UdpTrackerComm()
{
}

void UdpTrackerComm::init(std::string host, std::string port, CorePtr t)
{
	info.hostname = host;
	core = t;

	udp = UdpAsyncComm::Get();
	comm = udp->create(host, port);
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

void mtt::UdpTrackerComm::fail()
{
	if (state < Connected)
		state = Initialized;
	else if (state < Announced)
		state = Connected;
	else
		state = Announced;

	if (onFail)
		onFail();
}

bool mtt::UdpTrackerComm::onConnectUdpResponse(UdpRequest comm, DataBuffer* data)
{
	if (!data)
	{
		fail();

		return false;
	}

	auto response = getConnectResponse(*data);

	if (validResponse(response))
	{
		state = Connected;
		connectionId = response.connectionId;

		announce();

		return true;
	}
	else if (response.transaction == lastMessage.transaction)
	{
		fail();

		return true;
	}

	return false;
}

DataBuffer UdpTrackerComm::createAnnounceRequest()
{
	auto transaction = (uint32_t)rand();

	lastMessage = { Announce, transaction };

	PacketBuilder packet;
	packet.add64(connectionId);
	packet.add32(Announce);
	packet.add32(transaction);

	packet.add(core->torrent.info.hash, 20);
	packet.add(mtt::config::internal_.hashId, 20);

	packet.add64(0);
	packet.add64(0);
	packet.add64(0);

	packet.add32(Started);
	packet.add32(0);

	packet.add32(mtt::config::internal_.trackerKey);
	packet.add32(mtt::config::internal_.maxPeersPerTrackerRequest);
	packet.add32(mtt::config::external.tcpPort);
	packet.add16(0);

	return packet.getBuffer();
}

bool mtt::UdpTrackerComm::onAnnounceUdpResponse(UdpRequest comm, DataBuffer* data)
{
	if (!data)
	{
		fail();

		return false;
	}

	auto announceMsg = getAnnounceResponse(*data);

	if (validResponse(announceMsg.udp))
	{
		UDP_TRACKER_LOG("received peers:" << announceMsg.peers.size() << ", p: " << announceMsg.seedCount << ", l: " << announceMsg.leechCount);
		state = Announced;
		info.leechers = announceMsg.leechCount;
		info.seeds = announceMsg.seedCount;
		info.peers = (uint32_t)announceMsg.peers.size();
		info.announceInterval = announceMsg.interval;

		if (onAnnounceResult)
			onAnnounceResult(announceMsg);

		return true;
	}
	else if (announceMsg.udp.transaction == lastMessage.transaction)
	{
		fail();

		return true;
	}

	return false;
}

void mtt::UdpTrackerComm::connect()
{
	UDP_TRACKER_LOG("connecting");
	state = Connecting;

	udp->sendMessage(createConnectRequest(), comm, std::bind(&UdpTrackerComm::onConnectUdpResponse, this, std::placeholders::_1, std::placeholders::_2));
}

bool mtt::UdpTrackerComm::validResponse(TrackerMessage& resp)
{
	return resp.action == lastMessage.action && resp.transaction == lastMessage.transaction;
}

void mtt::UdpTrackerComm::announce()
{
	if (state < Connected)
		connect();
	else
	{
		UDP_TRACKER_LOG("announcing");

		if (state == Announced)
			state = Reannouncing;
		else
			state = Announcing;

		udp->sendMessage(createAnnounceRequest(), comm, std::bind(&UdpTrackerComm::onAnnounceUdpResponse, this, std::placeholders::_1, std::placeholders::_2));
	}
}

UdpTrackerComm::ConnectResponse UdpTrackerComm::getConnectResponse(DataBuffer& buffer)
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

UdpTrackerComm::UdpAnnounceResponse UdpTrackerComm::getAnnounceResponse(DataBuffer& buffer)
{
	PacketReader packet(buffer);

	UdpAnnounceResponse resp;

	resp.udp.action = packet.pop32();
	resp.udp.transaction = packet.pop32();

	if (resp.udp.action == Error && buffer.size() > 8)
	{
		UDP_TRACKER_LOG("announce msg: " << (const char*)buffer.data() + 8);
	}

	if (buffer.size() < 26)
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
