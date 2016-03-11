#include "PeerCommunication.h"
#include "PacketHelper.h"
#include <iostream>

using namespace Torrent;

void PeerCommunication::start(TorrentInfo* tInfo, ClientInfo* cInfo, PeerInfo info)
{
	torrent = tInfo;
	client = cInfo;
	peerInfo = info;

	pieces.piecesCount = torrent->pieces.size();

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

	char reserved_byte[8] = { 0 };
	reserved_byte[5] |= 0x10;	//Extension Protocol

	packet.add(reserved_byte, 8);

	packet.add(torrent->infoHash.data(), torrent->infoHash.size());
	packet.add(client->hashId, 20);

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

void Torrent::PeerCommunication::sendBlockRequest(PieceBlockInfo& block)
{
	PacketBuilder packet;
	packet.add(Request);
	packet.add32(block.index);
	packet.add32(block.begin);
	packet.add32(block.length);

	stream.write(packet.getBuffer());
}

void Torrent::PeerCommunication::handleMessage(PeerMessage& message)
{
	std::cout << peerInfo.ipStr << "_ID:" << std::to_string(message.id) << ", size: " << std::to_string(message.messageSize) << "\n";

	if (message.id == Bitfield)
	{
		std::cout << peerInfo.ipStr << "BITFIELD size: " << std::to_string(message.bitfield.size()) << ", expected: " << std::to_string(torrent->expectedBitfieldSize) << "\n";

		pieces.fromBitfield(message.bitfield);
		gcount++;

		std::cout << peerInfo.ipStr << "Percentage: " << std::to_string(pieces.getPercentage()) << "\n";
	}

	if (message.id == Have)
	{
		pieces.addPiece(message.havePieceIndex);

		std::cout << peerInfo.ipStr << "Percentage: " << std::to_string(pieces.getPercentage()) << "\n";
	}

	if (message.id == Unchoke)
	{
		state.amChoking = false;
	}

	if (message.id == Extended)
	{
		std::cout << peerInfo.ipStr << "_ID: Ext" << std::to_string(message.id) << "\n";

		if (message.extended.id == HandshakeEx)
		{
			BencodeParser parser;
			parser.parse(message.extended.handshakeExt);
			
			if (pex.load(parser.parsedData))
			{
				std::cout << peerInfo.ipStr << "Pex contacts: " << pex.contactsMessage << "\n";
			}
		}
	}

	if (message.id == Handshake)
	{
		state.finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		sendInterested();
	}
}

int gcount = 0;

float Torrent::PiecesProgress::getPercentage()
{
	return piecesCount / static_cast<float>(downloadedPieces);
}

void Torrent::PiecesProgress::addPiece(size_t index)
{
	bool old = piecesProgress[index];

	if (!old)
	{
		piecesProgress[index] = true;
		downloadedPieces++;
	}
}

bool Torrent::PiecesProgress::hasPiece(size_t index)
{
	return piecesProgress[index];
}

void Torrent::PiecesProgress::fromBitfield(std::vector<char>& bitfield)
{
	downloadedPieces = 0;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;
		piecesProgress.push_back(value);

		if (value)
			downloadedPieces++;
	}
}

std::vector<char> Torrent::PiecesProgress::toBitfield()
{
	return{};
}
