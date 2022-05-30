#include "UdpTrackerComm.h"
#include "utils/PacketHelper.h"
#include "utils/Network.h"
#include "Configuration.h"
#include "Logging.h"
#include "Torrent.h"

#define UDP_TRACKER_LOG(x) WRITE_GLOBAL_LOG(UdpTracker, info.hostname << " " << x)

using namespace mtt;

mtt::UdpTrackerComm::UdpTrackerComm()
{
}

mtt::UdpTrackerComm::~UdpTrackerComm()
{
	deinit();
}

void UdpTrackerComm::init(std::string host, std::string port, std::string, TorrentPtr t)
{
	info.hostname = host;
	torrent = t;

	udp = UdpAsyncComm::Get();
	comm = udp->create(host, port);
}

void mtt::UdpTrackerComm::deinit()
{
	udp->removeCallback(comm);
}

DataBuffer UdpTrackerComm::createConnectRequest()
{
	auto transaction = Random::Number();
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
	UDP_TRACKER_LOG("fail");

	if (info.state < TrackerState::Connected)
		info.state = TrackerState::Offline;
	else if (info.state < TrackerState::Announced)
		info.state = TrackerState::Connected;
	else
		info.state = TrackerState::Announced;

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
		info.state = TrackerState::Connected;
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

/*
0       64-bit integer  connection_id
8       32-bit integer  action          1 // announce
12      32-bit integer  transaction_id
16      20-byte string  info_hash
36      20-byte string  peer_id
56      64-bit integer  downloaded
64      64-bit integer  left
72      64-bit integer  uploaded
80      32-bit integer  event           0 // 0: none; 1: completed; 2: started; 3: stopped
84      32-bit integer  IP address      0 // default
88      32-bit integer  key
92      32-bit integer  num_want        -1 // default
96      16-bit integer  port
*/
DataBuffer UdpTrackerComm::createAnnounceRequest()
{
	auto transaction = Random::Number();

	lastMessage = { Announce, transaction };

	PacketBuilder packet(102);
	packet.add64(connectionId);
	packet.add32(Announce);
	packet.add32(transaction);

	packet.add(torrent->hash(), 20);
	packet.add(mtt::config::getInternal().hashId, 20);

	packet.add64(torrent->downloaded());
	packet.add64(torrent->dataLeft());
	packet.add64(torrent->uploaded());

	packet.add32(torrent->finished() ? Completed : Started);
	packet.add32(0);

	packet.add32(mtt::config::getInternal().trackerKey);
	packet.add32(mtt::config::getInternal().maxPeersPerTrackerRequest);
	packet.add16(mtt::config::getExternal().connection.tcpPort);

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
		info.state = TrackerState::Announced;
		info.leechers = announceMsg.leechCount;
		info.seeds = announceMsg.seedCount;
		info.peers = (uint32_t)announceMsg.peers.size();
		info.announceInterval = announceMsg.interval;
		info.lastAnnounce = (uint32_t)::time(0);

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
	info.state = TrackerState::Connecting;

	udp->sendMessage(createConnectRequest(), comm, std::bind(&UdpTrackerComm::onConnectUdpResponse, this, std::placeholders::_1, std::placeholders::_2), 5);
}

bool mtt::UdpTrackerComm::validResponse(TrackerMessage& resp)
{
	return resp.action == lastMessage.action && resp.transaction == lastMessage.transaction;
}

void mtt::UdpTrackerComm::announce()
{
	if (info.state < TrackerState::Connected)
		connect();
	else
	{
		UDP_TRACKER_LOG("announcing");

		if (info.state == TrackerState::Announced)
			info.state = TrackerState::Reannouncing;
		else
			info.state = TrackerState::Announcing;

		udp->sendMessage(createAnnounceRequest(), comm, std::bind(&UdpTrackerComm::onAnnounceUdpResponse, this, std::placeholders::_1, std::placeholders::_2), 5);
	}
}

UdpTrackerComm::ConnectResponse UdpTrackerComm::getConnectResponse(DataBuffer& buffer)
{
	ConnectResponse out;

	if (buffer.size() >= sizeof(ConnectResponse))
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

	if (buffer.size() < 26)
		return resp;

	resp.interval = packet.pop32();
	resp.leechCount = packet.pop32();
	resp.seedCount = packet.pop32();

	size_t count = static_cast<size_t>(packet.getRemainingSize() / 6.0f);

	for (size_t i = 0; i < count; i++)
	{
		uint32_t ip = *reinterpret_cast<const uint32_t*>(packet.popRaw(sizeof(uint32_t)));
		resp.peers.push_back(Addr(ip, packet.pop16()));
	}

	return resp;
}
