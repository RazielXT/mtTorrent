#include "PeerCommunication.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include <fstream>

enum class PeerEventType : uint8_t
{
	Connect, Connected, RemoteConnected, Disconnect, ReceiveMessage, Interested, Choke, RequestPiece, Piece
};
#define DIAGNOSTICS(eventType, x) WRITE_DIAGNOSTIC_LOG(std::string(#eventType)/*(char)PeerEventType::##eventType*/ << x)
#define BT_LOG(x) WRITE_LOG(x)

namespace mtt
{
	namespace bt
	{
		DataBuffer createHandshake(uint8_t* torrentHash, const uint8_t* clientHash)
		{
			PacketBuilder packet(70);
			packet.add(19);
			packet.add("BitTorrent protocol", 19);

			uint8_t reserved_byte[8] = { 0 };
			reserved_byte[5] |= 0x10;	//Extension Protocol

			if (mtt::config::getExternal().dht.enabled)
				reserved_byte[7] |= 0x80;

			packet.add(reserved_byte, 8);

			packet.add(torrentHash, 20);
			packet.add(clientHash, 20);

			return packet.getBuffer();
		}

		DataBuffer createStateMessage(PeerMessage::Id id)
		{
			PacketBuilder packet(5);
			packet.add32(1);
			packet.add(id);

			return packet.getBuffer();
		}

		DataBuffer createBlockRequest(PieceBlockInfo& block)
		{
			PacketBuilder packet(17);
			packet.add32(13);
			packet.add(PeerMessage::Request);
			packet.add32(block.index);
			packet.add32(block.begin);
			packet.add32(block.length);

			return packet.getBuffer();
		}

		DataBuffer createHave(uint32_t idx)
		{
			PacketBuilder packet(9);
			packet.add32(5);
			packet.add(PeerMessage::Have);
			packet.add32(idx);

			return packet.getBuffer();
		}

		DataBuffer createPort(uint16_t port)
		{
			PacketBuilder packet(9);
			packet.add32(3);
			packet.add(PeerMessage::Port);
			packet.add16(port);

			return packet.getBuffer();
		}

		DataBuffer createBitfield(const DataBuffer& bitfield)
		{
			PacketBuilder packet(5 + (uint32_t)bitfield.size());
			packet.add32(1 + (uint32_t)bitfield.size());
			packet.add(PeerMessage::Bitfield);
			packet.add(bitfield.data(), bitfield.size());

			return packet.getBuffer();
		}

		DataBuffer createPiece(const PieceBlock& block)
		{
			uint32_t dataSize = 1 + 8 + (uint32_t)block.buffer.size;
			PacketBuilder packet(4 + dataSize);
			packet.add32(dataSize);
			packet.add(PeerMessage::Piece);
			packet.add32(block.info.index);
			packet.add32(block.info.begin);
			packet.add(block.buffer.data, block.buffer.size);

			return packet.getBuffer();
		}
	}
}

mtt::PeerInfo::PeerInfo()
{
	memset(id, 0, 20);
	memset(protocol, 0, 8);
}

bool mtt::PeerInfo::supportsExtensions()
{
	return (protocol[5] & 0x10) != 0;
}

bool mtt::PeerInfo::supportsDht()
{
	return (protocol[7] & 0x80) != 0;
}

mtt::PeerCommunication::PeerCommunication(TorrentInfo& t, IPeerListener& l, asio::io_service& io_service) : torrent(t), listener(l)
{
	stream = std::make_shared<PeerStream>(io_service);
	CREATE_LOG(Peer);
}

mtt::PeerCommunication::~PeerCommunication()
{
	NAME_LOG(stream->getAddressName());
}

size_t mtt::PeerCommunication::fromStream(std::shared_ptr<TcpAsyncStream> s, const BufferView& streamData)
{
	initializeStream();
	stream->fromStream(s);

	DIAGNOSTICS(RemoteConnected, state.action);

	return dataReceived(streamData);
}

size_t mtt::PeerCommunication::fromStream(utp::StreamPtr s, const BufferView& streamData)
{
	initializeStream();
	stream->fromStream(s);

	DIAGNOSTICS(RemoteConnected, state.action);

	return dataReceived(streamData);
}

void mtt::PeerCommunication::initializeStream()
{
	ext.write = [this](const DataBuffer& data)
	{
		stream->write(data);
	};

	ext.pex.onPexMessage = [this](mtt::ext::PeerExchange::Message& msg)
	{
		listener.pexReceived(this, msg);
	};
	ext.utm.onUtMetadataMessage = [this](mtt::ext::UtMetadata::Message& msg)
	{
		listener.metadataPieceReceived(this, msg);
	};

	stream->onOpenCallback = std::bind(&PeerCommunication::connectionOpened, shared_from_this());
	stream->onCloseCallback = [this](int code) { connectionClosed(code); };
	stream->onReceiveCallback = [this](const BufferView& buffer) { return dataReceived(buffer); };
}

void mtt::PeerCommunication::sendHandshake(const Addr& address)
{
	DIAGNOSTICS(Connect, state.action);

	if (state.action != PeerCommunicationState::Disconnected)
		resetState();

	state.action = PeerCommunicationState::Connecting;

	initializeStream();
	stream->open(address);
}

void mtt::PeerCommunication::sendHandshake()
{
	if (!state.finishedHandshake && state.action == PeerCommunicationState::Connected)
	{
		BT_LOG("Handshake");

		state.action = PeerCommunicationState::Handshake;
		stream->write(mtt::bt::createHandshake(torrent.hash, mtt::config::getInternal().hashId));
	}
}

size_t mtt::PeerCommunication::dataReceived(const BufferView& buffer)
{
	size_t consumedSize = 0;

	auto readNextMessage = [&]()
	{
		PeerMessage msg({ buffer.data + consumedSize, buffer.size - consumedSize });

		if (msg.id != PeerMessage::Invalid)
			consumedSize += msg.messageSize;
		else if (!msg.messageSize)
			consumedSize = buffer.size;

		return msg;
	};

	auto message = readNextMessage();
	auto ptr = shared_from_this();

	while (message.id != PeerMessage::Invalid)
	{
		handleMessage(message);
		message = readNextMessage();
	}

	return consumedSize;
}

void mtt::PeerCommunication::connectionOpened()
{
	state.action = PeerCommunicationState::Connected;
	DIAGNOSTICS(Connected, state.action);

	sendHandshake();
}

void mtt::PeerCommunication::close()
{
	if (state.action != PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Disconnected;
		stream->close();
	}
}

const std::shared_ptr<mtt::PeerStream> mtt::PeerCommunication::getStream() const
{
	return stream;
}

void mtt::PeerCommunication::connectionClosed(int code)
{
	DIAGNOSTICS(Disconnect, code);

	if (state.action != PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Disconnected;
		listener.connectionClosed(this, code);
	}

	stream->onOpenCallback = nullptr;
}

void mtt::PeerCommunication::setInterested(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amInterested == enabled)
		return;

	DIAGNOSTICS(Interested, enabled);

	state.amInterested = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? PeerMessage::Interested : PeerMessage::NotInterested));
}

void mtt::PeerCommunication::setChoke(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amChoking == enabled)
		return;

	DIAGNOSTICS(Choke, enabled);

	state.amChoking = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? PeerMessage::Choke : PeerMessage::Unchoke));
}

void mtt::PeerCommunication::requestPieceBlock(PieceBlockInfo& pieceInfo)
{
	if (!isEstablished())
		return;

	DIAGNOSTICS(RequestPiece, pieceInfo.index << pieceInfo.begin << pieceInfo.length);

	stream->write(mtt::bt::createBlockRequest(pieceInfo));
}

bool mtt::PeerCommunication::isEstablished() const
{
	return state.action == PeerCommunicationState::Established;
}

void mtt::PeerCommunication::sendKeepAlive()
{
	if (!isEstablished())
		return;

	BT_LOG("KeepAlive");
	stream->write(DataBuffer(4, 0));
}

void mtt::PeerCommunication::sendHave(uint32_t pieceIdx)
{
	if (!isEstablished())
		return;

	BT_LOG("Have " << pieceIdx);
	stream->write(mtt::bt::createHave(pieceIdx));
}

void mtt::PeerCommunication::sendPieceBlock(const PieceBlock& block)
{
	if (!isEstablished())
		return;

	BT_LOG("Piece " << block.info.index);
	stream->write(mtt::bt::createPiece(block));
}

void mtt::PeerCommunication::sendBitfield(const DataBuffer& bitfield)
{
	if (!isEstablished())
		return;

	BT_LOG("Bitfield");
	stream->write(mtt::bt::createBitfield(bitfield));
}

void mtt::PeerCommunication::resetState()
{
	state = PeerCommunicationState();
	info = PeerInfo();
}

void mtt::PeerCommunication::sendPort(uint16_t port)
{
	if (!isEstablished())
		return;

	BT_LOG("Port " << port);
	stream->write(mtt::bt::createPort(port));
}

void mtt::PeerCommunication::handleMessage(PeerMessage& message)
{
	if(message.id == PeerMessage::Piece)
		DIAGNOSTICS(Piece, message.piece.info.index << message.piece.info.begin << message.piece.info.length)
	else
		DIAGNOSTICS(ReceiveMessage, message.id);

	if (message.id == PeerMessage::Bitfield)
	{
		info.pieces.fromBitfield(message.bitfield);

		BT_LOG("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, -1);
	}
	else if (message.id == PeerMessage::Have)
	{
		info.pieces.addPiece(message.havePieceIndex);

		BT_LOG("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, message.havePieceIndex);
	}
	else if (message.id == PeerMessage::Unchoke)
	{
		state.peerChoking = false;
		stream->setMinBandwidthRequest(BlockRequestMaxSize + 20);
	}
	else if (message.id == PeerMessage::Choke)
	{
		state.peerChoking = true;
		stream->setMinBandwidthRequest(100);
	}
	else if (message.id == PeerMessage::NotInterested)
	{
		state.peerInterested = true;
	}
	else if (message.id == PeerMessage::Interested)
	{
		state.peerInterested = false;
	}
	else if (message.id == PeerMessage::Extended)
	{
		auto type = ext.load(message.extended.id, message.extended.data);
		BT_LOG("ext message handled " << (int)type);

		if (type == mtt::ext::HandshakeEx)
		{
			listener.extHandshakeFinished(this);
		}
	}
	else if (message.id == PeerMessage::Handshake)
	{
		if (state.action == PeerCommunicationState::Handshake || state.action == PeerCommunicationState::Connected)
		{
			if (!state.finishedHandshake)
			{
				if(state.action == PeerCommunicationState::Connected)
					stream->write(mtt::bt::createHandshake(torrent.hash, mtt::config::getInternal().hashId));

				state.action = PeerCommunicationState::Established;
				state.finishedHandshake = true;
				BT_LOG("finished handshake");

				memcpy(info.id, message.handshake.peerId, 20);
				memcpy(info.protocol, message.handshake.reservedBytes, 8);
				info.pieces.init(torrent.pieces.size());

				if (info.supportsExtensions())
					ext.sendHandshake();

				if (info.supportsDht() && mtt::config::getExternal().dht.enabled)
					sendPort(mtt::config::getExternal().connection.udpPort);

				listener.handshakeFinished(this);
			}
			else
				state.action = PeerCommunicationState::Established;
		}
	}

	listener.messageReceived(this, message);
}
