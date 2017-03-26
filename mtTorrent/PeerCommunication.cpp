#include "PeerCommunication.h"
#include "PacketHelper.h"
#include <iostream>
#include "ProgressScheduler.h"

using namespace mtt;

PeerCommunication::PeerCommunication(boost::asio::io_service* io_service, ProgressScheduler* sched) : scheduler(sched), stream(*io_service)
{
	stream.setOnConnectCallback(std::bind(&PeerCommunication::connectionOpened, this));
	stream.setOnCloseCallback(std::bind(&PeerCommunication::connectionClosed, this));
	stream.setOnReceiveCallback(std::bind(&PeerCommunication::dataReceived, this));
}

void PeerCommunication::start(TorrentFileInfo* tInfo, PeerInfo info)
{
	active = true;
	torrent = tInfo;
	peerInfo = info;

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
	packet.add(mtt::getClientInfo()->hashId, 20);

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

bool mtt::PeerCommunication::validPiece()
{
	unsigned char hash[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char*)downloadingPiece.data.data(), downloadingPiece.dataSize, hash);
	
	return memcmp(hash, scheduledPieceInfo.hash, SHA_DIGEST_LENGTH) == 0;
}

void mtt::PeerCommunication::stop()
{
	if (active)
		stream.close();

	active = false;
}

void mtt::PeerCommunication::connectionClosed()
{
	active = false;
}

mtt::PeerMessage mtt::PeerCommunication::readNextStreamMessage()
{
	std::lock_guard<std::mutex> guard(read_mutex);

	auto data = stream.getReceivedData();
	PeerMessage msg(data);

	if (msg.id != Invalid)
		stream.consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream.consumeData(data.size());

	return msg;
}

void mtt::PeerCommunication::sendInterested()
{
	state.amInterested = true;

	PacketBuilder packet;
	packet.add32(1);
	packet.add(Interested);

	auto interestedMsg = packet.getBuffer();
	stream.write(interestedMsg);
}

void mtt::PeerCommunication::sendHandshakeExt()
{
	auto data = ext.getExtendedHandshakeMessage();
	stream.write(data);
}

void mtt::PeerCommunication::sendBlockRequest(PieceBlockInfo& block)
{
	PacketBuilder packet;
	packet.add32(13);
	packet.add(Request);
	packet.add32(block.index);
	packet.add32(block.begin);
	packet.add32(block.length);

	stream.write(packet.getBuffer());
}

void mtt::PeerCommunication::schedulePieceDownload(bool forceNext)
{
	const int batchSize = 10;
	const int nextBatchMin = 4;
	static int pieceTodo = 0;

	if (forceNext || downloadingPiece.receivedBlocks == scheduledPieceInfo.blocksCount)
	{
		scheduledPieceInfo = scheduler->getNextPieceDownload(peerPieces);
		downloadingPiece.reset(torrent->pieceSize);
		pieceTodo = 0;
	}	
	else
		pieceTodo = std::max(0, pieceTodo - 1);

	/*if (!downloadingPieceInfo.blocksLeft.empty())
	{
		auto blocks = downloadingPieceInfo.blocksLeft;
		downloadingPieceInfo.blocksLeft.clear();

		for (auto& b : blocks)
		{
			downloadingPiece.index = b.index;
			sendBlockRequest(b);
		}
	}*/

	if(pieceTodo<nextBatchMin)
	for (size_t i = 0; i < batchSize; i++)
	if (!scheduledPieceInfo.blocksLeft.empty())
	{
		auto& b = scheduledPieceInfo.blocksLeft.back();
		scheduledPieceInfo.blocksLeft.pop_back();
		downloadingPiece.index = b.index;
		sendBlockRequest(b);
		pieceTodo++;
	}
}

void mtt::PeerCommunication::handleMessage(PeerMessage& message)
{
	if (message.id != Piece)
		std::cout << peerInfo.ipStr << "_ID:" << std::to_string(message.id) << ", size: " << std::to_string(message.messageSize) << "\n";

	if (message.id == KeepAlive)
	{
		schedulePieceDownload(true);
	}

	if (message.id == Bitfield)
	{
		std::cout << peerInfo.ipStr << "BITFIELD size: " << std::to_string(message.bitfield.size()) << ", expected: " << std::to_string(torrent->expectedBitfieldSize) << "\n";

		peerPieces.fromBitfield(message.bitfield, torrent->pieces.size());
		gcount++;

		std::cout << peerInfo.ipStr << "Percentage: " << std::to_string(peerPieces.getPercentage()) << "\n";
	}

	if (message.id == Have)
	{
		peerPieces.addPiece(message.havePieceIndex);

		std::cout << peerInfo.ipStr << "Percentage: " << std::to_string(peerPieces.getPercentage()) << "\n";
	}

	if (message.id == Piece)
	{
		std::lock_guard<std::mutex> guard(schedule_mutex);
			
		std::cout << "Piece id: " << std::to_string(message.piece.info.index) << ", size: " << std::to_string(message.piece.info.length) << "\n";

		if(message.piece.info.index != downloadingPiece.index)
			std::cout << peerInfo.ipStr << " Invalid block!! \n";
		else
		{
			downloadingPiece.addBlock(message.piece);
			//std::cout << peerInfo.ipStr << " block, index " << downloadingPiece.index << "==" << message.piece.info.index << " ,offset " << std::hex << message.piece.info.begin << "\n";

			if (downloadingPiece.receivedBlocks == scheduledPieceInfo.blocksCount)
			{
				if (validPiece())
				{
					scheduler->addDownloadedPiece(downloadingPiece);
					std::cout << peerInfo.ipStr << " Piece Added, Percentage: " << std::to_string(scheduler->getPercentage()) << "\n";
				}
				else
					std::cout << peerInfo.ipStr << " Invalid piece!! \n";
			}

			schedulePieceDownload();
		}
	}

	if (message.id == Unchoke)
	{
		std::lock_guard<std::mutex> guard(schedule_mutex);

		if (state.amChoking)
		{
			state.amChoking = false;

			schedulePieceDownload();
		}
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
