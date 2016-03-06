#include "Communicator.h"
#include "Network.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <future>
#include <iostream>

#define swap16 _byteswap_ushort
#define swap32 _byteswap_ulong
#define swap64 _byteswap_uint64

using namespace Torrent;

struct PacketReader
{
	uint8_t pop()
	{
		uint8_t i = *reinterpret_cast<uint8_t*>(buf.data());
		buf.erase(buf.begin());
		return i;
	}

	uint16_t pop16()
	{
		uint16_t i = swap16(*reinterpret_cast<uint16_t*>(buf.data()));
		buf.erase(buf.begin(), buf.begin() + sizeof i);
		return i;
	}

	uint32_t pop32()
	{
		uint32_t i = swap32(*reinterpret_cast<uint32_t*>(buf.data()));
		buf.erase(buf.begin(), buf.begin() + sizeof i);
		return i;
	}

	uint64_t pop64()
	{
		uint64_t i = swap64(*reinterpret_cast<uint64_t*>(buf.data()));
		buf.erase(buf.begin(), buf.begin() + sizeof i);
		return i;
	}

	PacketReader(std::vector<char> buffer)
	{
		buf = buffer;
	}

	std::vector<char> buf;
};

struct PacketBuilder
{
	void add(char c)
	{
		buf.sputn(&c, 1);
		size ++;
	}

	void add16(uint16_t i)
	{
		i = swap16(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add32(uint32_t i)
	{
		i = swap32(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add64(uint64_t i)
	{
		i = swap64(i);
		buf.sputn(reinterpret_cast<char*>(&i), sizeof i);
		size += sizeof i;
	}

	void add(const char* b, size_t length)
	{
		/*std::vector<char> temp(b, b + length);
		for (size_t i = 0; i < (size_t)(length*0.5f); i++)
		{
			char t = temp[i];
			temp[i] = temp[length - i - 1];
			temp[length - i - 1] = t;
		}

		buf.sputn(temp.data(), length);*/
		buf.sputn(b, length);
		size += length;
	}

	std::vector<char> getBuffer()
	{
		std::vector<char> out;

		out.resize(size);
		buf.sgetn(&out[0], size);

		return out;
	}

	std::stringbuf buf;
	uint64_t size = 0;
};

std::string url_encode(const std::string &value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		std::string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		// make sure c is positive for msvc assertion
		if (c >= 0 && (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')) {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char)c);
		escaped << std::nouppercase;
	}

	return escaped.str();
}

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

	size_t count = static_cast<size_t>(packet.buf.size() / 6.0f);

	for (size_t i = 0; i < count; i++)
	{
		AnnounceResponsePeer p;
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
	torrent.parse("D:\\dead.torrent");

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
				futures[i] = std::async(&PeerCommunication::start, &peer[i], &torrent, peerId, resp.peers[i]);
			}			

			for (size_t i = 0; i < resp.peers.size(); i++)
			{
				futures[i].get();
			}
		}
	}
	
}

void PeerCommunication::start(TorrentFileParser* torrent, char* pId, AnnounceResponsePeer info)
{
	torretFile = torrent;
	peerId = pId;
	peerInfo = info;

	try
	{
		handshake(peerInfo);
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
}

PeerMessage PeerMessage::loadMessage(std::vector<char>& data)
{
	PeerMessage msg;

	if (data.empty())
		return msg;

	if (data.size() >= 68 && data[0] == 19)
	{
		//auto str = std::string(data.begin() + 1, data.begin() + 20);
		//if (str == "BitTorrent protocol")
		msg.id = Handshake;

		msg.messageSize = 68;
		memcpy(msg.peer_id, &data[0] + 20 + 8 + 20, 20);

		return msg;
	}

	PacketReader reader(data);

	auto size = reader.pop32();
	msg.messageSize = size + 4;

	//incomplete
	if (reader.buf.size() < size)
		return msg;

	if (size == 0)
	{
		msg.id = KeepAlive;
	}	
	else
	{
		msg.id = PeerMessageId(reader.pop());	

		if (msg.id == Have && size == 5)
		{
			msg.pieceIndex = reader.pop32();		
		}
		else if (msg.id == Bitfield)
		{
			msg.bitfield = reader.buf;
		}
		else if (msg.id == Request && size == 13)
		{
			msg.request.index = reader.pop32();
			msg.request.begin = reader.pop32();
			msg.request.length = reader.pop32();
		}
		else if (msg.id == Request && size > 9)
		{
			msg.piece.index = reader.pop32();
			msg.piece.begin = reader.pop32();
			msg.piece.block = reader.buf;
		}
		else if (msg.id == Cancel && size == 13)
		{
			msg.request.index = reader.pop32();
			msg.request.begin = reader.pop32();
			msg.request.length = reader.pop32();
		}
		else if (msg.id == Port && size == 3)
		{
			msg.port = reader.pop16();
		}
	}

	if (msg.id >= Invalid)
	{
		msg.id = Invalid;
		msg.messageSize = 0;
	}
		
	return msg;
}

std::vector<char> PeerCommunication::getHandshakeMessage()
{
	std::string protocol = "BitTorrent protocol";

	PacketBuilder packet;
	packet.add(static_cast<char>(protocol.length()));
	packet.add(protocol.data(), protocol.size());

	for (size_t i = 0; i < 8; i++)
		packet.add(0);

	packet.add(torretFile->info.infoHash.data(), torretFile->info.infoHash.size());
	packet.add(peerId, 20);

	return packet.getBuffer();
}

bool PeerCommunication::handshake(AnnounceResponsePeer& peerInfo)
{
	auto port = std::to_string(peerInfo.port);
	stream.connect(peerInfo.ipStr.data(), port.data());

	auto requestData = getHandshakeMessage(); 
	stream.write(requestData);

	return communicate();
}

bool PeerCommunication::communicate()
{
	while (true)
	{
		stream.blockingRead();

		auto message = getNextStreamMessage();

		if (message.messageSize == 0 && !finishedHandshake)
			break;

		while (message.id != Invalid)
		{
			handleMessage(message);

			message = getNextStreamMessage();
		}
	}

	return true;
}

Torrent::PeerMessage Torrent::PeerCommunication::getNextStreamMessage()
{
	auto data = stream.getReceivedData();

	auto msg = PeerMessage::loadMessage(data);

	if (msg.id != Invalid)
		stream.consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream.resetData();

	return msg;
}

void Torrent::PeerCommunication::setInterested()
{
	amInterested = true;

	PacketBuilder packet;
	packet.add32(1);
	packet.add(Interested);

	auto interestedMsg = packet.getBuffer();

	stream.write(interestedMsg);
}

void Torrent::PeerCommunication::handleMessage(PeerMessage& message)
{
	std::cout << peerInfo.ipStr << "_ID2:" << std::to_string(message.id) << ", size: " << std::to_string(message.messageSize) << "\n";

	if (message.id == Handshake)
	{
		finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		setInterested();
	}	
}
