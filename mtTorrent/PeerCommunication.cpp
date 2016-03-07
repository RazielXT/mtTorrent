#include "PeerCommunication.h"
#include "PacketHelper.h"
#include <iostream>

using namespace Torrent;

void PeerCommunication::start(TorrentFileInfo* torrent, char* pId, PeerInfo info)
{
	torretFile = torrent;
	peerId = pId;
	peerInfo = info;
	state.index = info.index;

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
	stream.init(peerInfo.ipStr.data(), port.data());

	auto requestData = getHandshakeMessage();
	stream.write(requestData);

	return communicate();
}

bool PeerCommunication::communicate()
{
	std::thread t(&PeerCommunication::startListening, this);

	while (!state.finished)
	{
		Sleep(100);

		auto messages = state.popMessages();

		for (auto& msg : messages)
			handleMessage(msg);
	}

	return true;
}

void PeerCommunication::startListening()
{
	static int c = 0;
	std::cout << "LISTEN: " << std::to_string(c++) << "\n";

	try
	{
		int retry = 3;

		while (retry > 0)
		{
			auto r = stream.blockingRead();

			if (!state.finishedHandshake && !r)
				retry--;

			auto message = readNextStreamMessage();

			while (message.id != Invalid)
			{
				state.pushMessage(message);
				message = readNextStreamMessage();

				retry = 3;
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << "MException: " << e.what() << "\n";
	}
}

Torrent::PeerMessage Torrent::PeerCommunication::readNextStreamMessage()
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
	state.amInterested = true;

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
		state.peerInterested = true;
		//state.finished = true;
	}

	if (message.id == Handshake)
	{
		state.finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		sendInterested();
	}
}

void Torrent::PeerState::pushMessage(PeerMessage msg)
{
	std::lock_guard<std::mutex> guard(messages_mutex);
	messages.push_back(msg);
}

std::vector<PeerMessage> Torrent::PeerState::popMessages()
{
	std::lock_guard<std::mutex> guard(messages_mutex);
	auto msgs = messages;// std::move(messages);
	messages.clear();

	return msgs;
}
