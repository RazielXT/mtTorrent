#include "PeerCommunication.h"
#include "PacketHelper.h"
#include <iostream>

using namespace Torrent;

void PeerCommunication::start(TorrentFileInfo* torrent, char* pId, PeerInfo info)
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

std::vector<char> PeerCommunication::getHandshakeMessage()
{
	std::string protocol = "BitTorrent protocol";

	PacketBuilder packet;
	packet.add(static_cast<char>(protocol.length()));
	packet.add(protocol.data(), protocol.size());

	for (size_t i = 0; i < 8; i++)
		packet.add(0);

	packet.add(torretFile->infoHash.data(), torretFile->infoHash.size());
	packet.add(peerId, 20);

	return packet.getBuffer();
}

bool PeerCommunication::handshake(PeerInfo& peerInfo)
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
	PeerMessage msg(data);

	if (msg.id != Invalid)
		stream.consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream.resetData();

	return msg;
}

void Torrent::PeerCommunication::sendInterested()
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

	if (message.id == Interested)
	{
		peerInterested = true;
	}

	if (message.id == Handshake)
	{
		finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		sendInterested();
	}
}
