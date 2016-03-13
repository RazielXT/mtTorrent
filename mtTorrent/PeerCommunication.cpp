#include "PeerCommunication.h"
#include "PacketHelper.h"
#include <iostream>
#include "ProgressScheduler.h"

using namespace Torrent;

PeerCommunication::PeerCommunication(ClientInfo* cInfo) : stream(*cInfo->network.io_service)
{
	client = cInfo;
	ext.setInfo(cInfo);

	stream.setOnConnectCallback(std::bind(&PeerCommunication::connectionOpened, this));
	stream.setOnCloseCallback(std::bind(&PeerCommunication::connectionClosed, this));
	stream.setOnReceiveCallback(std::bind(&PeerCommunication::dataReceived, this));
}

void PeerCommunication::start(TorrentInfo* tInfo, PeerInfo info)
{
	active = true;
	torrent = tInfo;
	peerInfo = info;

	pieces.init(torrent->pieces.size());
	
	auto port = std::to_string(peerInfo.port);
	stream.connect(peerInfo.ipStr, port);
}

DataBuffer PeerCommunication::getHandshakeMessage()
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

void PeerCommunication::handshake(PeerInfo& peerInfo)
{
	auto requestData = getHandshakeMessage();
	stream.write(requestData);
}

void PeerCommunication::dataReceived()
{
	auto message = readNextStreamMessage();

	while (message.id != Invalid)
	{
		handleMessage(message);
		message = readNextStreamMessage();
	}
}

void PeerCommunication::connectionOpened()
{
	if(!state.finishedHandshake)
		handshake(peerInfo);
}

void Torrent::PeerCommunication::connectionClosed()
{
	active = false;
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

void Torrent::PeerCommunication::sendHandshakeExt()
{
	auto data = ext.getExtendedHandshakeMessage();
	stream.write(data);
}

void Torrent::PeerCommunication::sendBlockRequest(PieceBlockInfo& block)
{
	PacketBuilder packet;
	packet.add32(13);
	packet.add(Request);
	packet.add32(block.index);
	packet.add32(block.begin);
	packet.add32(block.length);

	stream.write(packet.getBuffer());
}

void Torrent::PeerCommunication::schedulePieceDownload()
{
	if (downloadingPieceInfo.blocks.empty())
		downloadingPieceInfo = client->scheduler->getNextPieceDownload(pieces);

	if (!downloadingPieceInfo.blocks.empty())
	{
		auto b = downloadingPieceInfo.blocks.back();
		downloadingPieceInfo.blocks.pop_back();
		downloadingPiece.index = b.index;
		
		sendBlockRequest(b);	
	}
	else
		stream.close();
}

void Torrent::PeerCommunication::handleMessage(PeerMessage& message)
{
	std::cout << peerInfo.ipStr << "_ID:" << std::to_string(message.id) << ", size: " << std::to_string(message.messageSize) << "\n";
	std::cout << "Read " << getTimestamp() << "\n";

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

	if (message.id == Piece)
	{
		downloadingPiece.blocks.push_back({ message.piece });
		std::cout << peerInfo.ipStr << " block added from " << std::to_string(downloadingPiece.index) << "\n";

		if (downloadingPieceInfo.blocks.empty())
		{
			client->scheduler->addDownloadedPiece(downloadingPiece);
			downloadingPiece.blocks.clear();

			std::cout << peerInfo.ipStr << " Piece Added, Percentage: " << std::to_string(client->scheduler->getPercentage()) << "\n";
		}

		schedulePieceDownload();
	}

	if (message.id == Unchoke)
	{
		state.amChoking = false;

		schedulePieceDownload();
	}

	if (message.id == Extended)
	{
		std::cout << peerInfo.ipStr << " Ext msg: " << std::string(message.extended.data.begin(), message.extended.data.end()) << "\n";

		auto type = ext.load(message.extended.id, message.extended.data);

		std::cout << peerInfo.ipStr << " Ext Type " << std::to_string(message.extended.id) << " resolve :" << std::to_string(type) << "\n";
	}

	if (message.id == Handshake)
	{
		state.finishedHandshake = true;
		std::cout << peerInfo.ipStr << "_has peer id:" << std::string(message.peer_id, message.peer_id + 20) << "\n";
		
		sendHandshakeExt();
		sendInterested();
	}
}

int gcount = 0;

void Torrent::PeerInfo::setIp(uint32_t addr)
{
	ip = addr;

	uint8_t ipAddr[4];
	ipAddr[3] = *reinterpret_cast<uint8_t*>(&ip);
	ipAddr[2] = *(reinterpret_cast<uint8_t*>(&ip) + 1);
	ipAddr[1] = *(reinterpret_cast<uint8_t*>(&ip) + 2);
	ipAddr[0] = *(reinterpret_cast<uint8_t*>(&ip) + 3);

	ipStr = std::to_string(ipAddr[0]) + "." + std::to_string(ipAddr[1]) + "." + std::to_string(ipAddr[2]) + "." + std::to_string(ipAddr[3]);
}

float Torrent::PiecesProgress::getPercentage()
{
	return downloadedPieces / static_cast<float>(piecesCount);
}


void Torrent::PiecesProgress::init(size_t size)
{
	piecesCount = size;
	piecesProgress.resize(size);
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

void Torrent::PiecesProgress::fromBitfield(DataBuffer& bitfield)
{
	downloadedPieces = 0;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;
		piecesProgress[i] = value;

		if (value)
			downloadedPieces++;
	}
}

DataBuffer Torrent::PiecesProgress::toBitfield()
{
	return{};
}
