#include "PeerCommunication2.h"
#include "PacketHelper.h"
#include "Configuration.h"

using namespace mtt;

namespace mtt
{
	namespace bt
	{
		DataBuffer createHandshake(uint8_t* torrentHash, uint8_t* clientHash)
		{
			PacketBuilder packet(70);
			packet.add(19);
			packet.add("BitTorrent protocol", 19);

			uint8_t reserved_byte[8] = { 0 };
			reserved_byte[5] |= 0x10;	//Extension Protocol

			packet.add(reserved_byte, 8);

			packet.add(torrentHash, 20);
			packet.add(clientHash, 20);

			return packet.getBuffer();
		}

		DataBuffer createInterested()
		{
			PacketBuilder packet(5);
			packet.add32(1);
			packet.add(Interested);

			return packet.getBuffer();
		}

		DataBuffer createBlockRequest(PieceBlockInfo& block)
		{
			PacketBuilder packet;
			packet.add32(13);
			packet.add(Request);
			packet.add32(block.index);
			packet.add32(block.begin);
			packet.add32(block.length);

			return packet.getBuffer();
		}
	}
}

mtt::PeerStateInfo::PeerStateInfo()
{
	memset(id, 0, 20);
	memset(protocol, 0, 8);
}

PeerCommunication2::PeerCommunication2(TorrentInfo& t, IPeerListener& l, boost::asio::io_service& io_service) : torrent(t), listener(l), stream(io_service)
{
	stream.onConnectCallback = std::bind(&PeerCommunication2::connectionOpened, this);
	stream.onCloseCallback = std::bind(&PeerCommunication2::connectionClosed, this);
	stream.onReceiveCallback = std::bind(&PeerCommunication2::dataReceived, this);

	ext.pex.onPexMessage = std::bind(&mtt::IPeerListener::pexReceived, &listener, std::placeholders::_1);
	ext.utm.onUtMetadataMessage = std::bind(&mtt::IPeerListener::metadataPieceReceived, &listener, std::placeholders::_1);
}

void PeerCommunication2::start(Addr& address)
{
	if (state.action == PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Connecting;
		stream.connect(address.addrBytes, address.port, address.ipv6);
	}
}

void PeerCommunication2::dataReceived()
{
	std::lock_guard<std::mutex> guard(read_mutex);

	auto message = readNextStreamMessage();

	while (message.id != Invalid)
	{
		handleMessage(message);
		message = readNextStreamMessage();
	}
}

void PeerCommunication2::connectionOpened()
{
	if (!state.finishedHandshake)
	{
		state.action = PeerCommunicationState::Handshake;
		stream.write(mtt::bt::createHandshake(torrent.hash, mtt::config::internal.hashId));
	}
}

void mtt::PeerCommunication2::stop()
{
	if (state.action != PeerCommunicationState::Disconnected)
		stream.close();
}

void mtt::PeerCommunication2::connectionClosed()
{
	state.action = PeerCommunicationState::Disconnected;
	listener.connectionClosed();
}

mtt::PeerMessage mtt::PeerCommunication2::readNextStreamMessage()
{
	auto data = stream.getReceivedData();
	PeerMessage msg(data);

	if (msg.id != Invalid)
		stream.consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream.consumeData(data.size());

	return msg;
}

bool mtt::PeerCommunication2::sendInterested()
{
	if (state.action != PeerCommunicationState::Idle)
		return false;

	state.amInterested = true;

	stream.write(mtt::bt::createInterested());

	return true;
}

bool mtt::PeerCommunication2::requestMetadataPiece(uint32_t index)
{
	if (state.action != PeerCommunicationState::Idle)
		return false;

	if (ext.utm.size == 0)
		return false;

	stream.write(ext.utm.createMetadataRequest(index));

	return true;
}

bool mtt::PeerCommunication2::requestPiece(PieceDownloadInfo& pieceInfo)
{
	if (state.action != PeerCommunicationState::Idle)
		return false;

	if (!info.pieces.hasPiece(pieceInfo.index))
		return false;

	std::lock_guard<std::mutex> guard(schedule_mutex);

	scheduledPieceInfo = pieceInfo;
	downloadingPiece.reset(torrent.pieceSize);
	downloadingPiece.index = scheduledPieceInfo.index;

	requestPieceBlock();

	return true;
}

void mtt::PeerCommunication2::enableExtensions()
{
	ext.enabled;
	state.action = PeerCommunicationState::Handshake;
	stream.write(ext.getExtendedHandshakeMessage());
}

void mtt::PeerCommunication2::requestPieceBlock()
{
	auto& b = scheduledPieceInfo.blocksLeft.back();
	scheduledPieceInfo.blocksLeft.pop_back();

	state.action = PeerCommunicationState::TransferringData;
	stream.write(mtt::bt::createBlockRequest(b));
}

void mtt::PeerCommunication2::handleMessage(PeerMessage& message)
{
	if (message.id != Piece)
		PEER_LOG(peerInfo.ipStr << "_ID:" << std::to_string(message.id) << ", size: " << std::to_string(message.messageSize) << "\n");

	if (message.id == KeepAlive)
	{
	}

	if (message.id == Bitfield)
	{
		PEER_LOG(peerInfo.ipStr << " BITFIELD size: " << std::to_string(message.bitfield.size()) << ", expected: " << std::to_string(torrent->expectedBitfieldSize) << "\n");

		info.pieces.fromBitfield(message.bitfield, torrent.pieces.size());

		PEER_LOG(peerInfo.ipStr << " new percentage: " << std::to_string(peerPieces.getPercentage()) << "\n");

		listener.progressUpdated();
	}

	if (message.id == Have)
	{
		info.pieces.addPiece(message.havePieceIndex);

		PEER_LOG(peerInfo.ipStr << " new percentage: " << std::to_string(peerPieces.getPercentage()) << "\n");

		listener.progressUpdated();
	}

	if (message.id == Piece)
	{
		PEER_LOG("Piece id: " << std::to_string(message.piece.info.index) << ", size: " << std::to_string(message.piece.info.length) << "\n");

		bool finished = false;
		bool success = false;

		{
			std::lock_guard<std::mutex> guard(schedule_mutex);

			if (message.piece.info.index != downloadingPiece.index)
			{
				PEER_LOG(peerInfo.ipStr << " Invalid block!! \n")
				finished = true;
			}
			else
			{
				downloadingPiece.addBlock(message.piece);

				if (downloadingPiece.receivedBlocks == scheduledPieceInfo.blocksCount)
				{
					finished = success = true;
				}
				else
				{
					requestPieceBlock();
				}
			}
		}

		if (finished)
		{
			state.action = PeerCommunicationState::Idle;
			listener.pieceReceived(success ? &downloadingPiece : nullptr);
		}
	}

	if (message.id == Unchoke)
	{
		state.peerChoking = false;
	}

	if (message.id == Extended)
	{
		auto type = ext.load(message.extended.id, message.extended.data);

		PEER_LOG(peerInfo.ipStr << " Ext message type " std::to_string(type) << "\n");

		if (type == mtt::ext::HandshakeEx)
		{
			state.action = PeerCommunicationState::Idle;
			listener.handshakeFinished();
		}
	}

	if (message.id == Handshake)
	{
		state.action = PeerCommunicationState::Idle;

		if (!state.finishedHandshake)
		{
			state.finishedHandshake = true;
			PEER_LOG(peerInfo.ipStr << " finished handshake\n");

			memcpy(info.id, message.handshake.peerId, 20);
			memcpy(info.protocol, message.handshake.reservedBytes, 8);

			if(info.protocol[5] & 0x10)
				enableExtensions();
			else
				listener.handshakeFinished();
		}
	}

	listener.messageReceived(message);
}
