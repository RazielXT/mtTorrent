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
	pieces.prepare(torrent->expectedBitfieldSize, torrent->pieces.size());

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
	stream.setTarget(peerInfo.ipStr.data(), port.data());

	auto requestData = getHandshakeMessage();
	stream.write(requestData);

	return communicate();
}

bool PeerCommunication::communicate()
{
	while (stream.active())
	{
		Sleep(50);

		auto message = readNextStreamMessage();

		while (message.id != Invalid)
		{
			handleMessage(message);
			message = readNextStreamMessage();
		}
	}

	return true;
}

Torrent::PeerMessage Torrent::PeerCommunication::readNextStreamMessage()
{
	auto data = stream.getReceivedData();
	PeerMessage msg(data);

	if (msg.id != Invalid)
		stream.consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream.consumeData(data.size());

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

	if (message.id == Bitfield)
	{
		std::cout << peerInfo.ipStr << "BITFIELD size: " << std::to_string(message.bitfield.size()) << ", expected: " << std::to_string(torretFile->expectedBitfieldSize) << "\n";

		pieces.bitfield = message.bitfield;
	}

	if (message.id == Have)
	{
		pieces.addPiece(message.havePieceIndex);
	}

	if (message.id == Interested)
	{
		state.peerInterested = true;
		stream.close();
	}

	if (message.id == Handshake)
	{
		state.finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		sendInterested();
	}
}

void Torrent::PiecesBitfield::prepare(size_t bitsize, size_t pieces)
{
	bitfield.resize(bitsize);
	piecesCount = pieces;
}

float Torrent::PiecesBitfield::getPercentage()
{
	if (piecesCount)
	{
		float r = 0;

		for (size_t i = 0; i < piecesCount; i++)
		{
			size_t idx = i / 8.0f;
			unsigned char bitmask = 255 >> i % 8;

			auto value = bitfield[idx] & bitmask;
			r += value ? 1 : 0;
		}

		return r / piecesCount;
	}

	return 0;
}

void Torrent::PiecesBitfield::addPiece(size_t index)
{
	if (index < piecesCount)
	{
		size_t idx = index / 8.0f;
		unsigned char bitmask = 255 >> index % 8;

		bitfield[idx] |= bitmask;
	}
}
