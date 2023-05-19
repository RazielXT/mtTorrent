#include "PeerCommunication.h"
#include "utils/PacketHelper.h"
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include <fstream>

#define BT_LOG(x) WRITE_LOG(x)

namespace mtt
{
	namespace bt
	{
		DataBuffer createHandshake(const uint8_t* torrentHash, const uint8_t* clientHash)
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

		DataBuffer createBlockRequest(const PieceBlockInfo& block)
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

		DataBuffer createExtendedHandshake(const std::string& ip, size_t metadataSize)
		{
			BencodeWriter writer;
			writer.data.reserve(100);

			writer.data.resize(sizeof(uint32_t)); //length
			writer.data.push_back(mtt::PeerMessage::Extended);
			writer.data.push_back((uint8_t)ext::Type::Handshake);

			writer.startMap();

			{
				writer.startRawMapItem("1:m");

				writer.addRawItem("11:ut_metadata", (uint8_t)ext::Type::UtMetadata);
				writer.addRawItem("6:ut_pex", (uint8_t)ext::Type::Pex);
				writer.addRawItem("12:ut_holepunch", (uint8_t)ext::Type::UtHolepunch);

				writer.endMap();
			}

			if (metadataSize)
				writer.addRawItem("13:metadata_size", metadataSize);

			writer.addRawItem("1:p", mtt::config::getExternal().connection.tcpPort);
			writer.addRawItem("1:v", MT_NAME);
			writer.addRawItem("6:yourip", ip);

			writer.endMap();

			PacketBuilder::Assign32(writer.data.data(), (uint32_t)(writer.data.size() - sizeof(uint32_t)));

			return writer.data;
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

mtt::PeerCommunication::PeerCommunication(TorrentInfo& t, IPeerListener& l, asio::io_context& io_context) : torrent(t), listener(l)
{
	stream = std::make_shared<PeerStream>(io_context);
	CREATE_LOG(Peer);
	INDEX_LOG();
}

mtt::PeerCommunication::~PeerCommunication()
{
}

size_t mtt::PeerCommunication::fromStream(std::shared_ptr<PeerStream> s, const BufferView& streamData)
{
	stream = s;
	initializeStream();
	state.action = PeerState::Connected;

	NAME_LOG(stream->getAddressName());
	BT_LOG("RemoteConnected");

	return dataReceived(streamData);
}

void mtt::PeerCommunication::initializeStream()
{
	stream->onOpenCallback = std::bind(&PeerCommunication::connectionOpened, shared_from_this());
	stream->onCloseCallback = [this](int code) { connectionClosed(code); };
	stream->onReceiveCallback = [this](const BufferView& buffer) { return dataReceived(buffer); };
}

void mtt::PeerCommunication::connect(const Addr& address)
{
	NAME_LOG(address.toString());

	if (state.action != PeerState::Disconnected)
		resetState();

	state.action = PeerState::Connecting;

	initializeStream();
	stream->open(address, torrent.hash);
}

void mtt::PeerCommunication::sendHandshake()
{
	BT_LOG("sendHandshake");

	state.action = PeerState::Handshake;
	send(mtt::bt::createHandshake(torrent.hash, mtt::config::getInternal().hashId));
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
	stats.connectionTime = stats.lastActivityTime = mtt::CurrentTimestamp();
	state.action = PeerState::Connected;
	BT_LOG("connected");

	sendHandshake();
}

void mtt::PeerCommunication::close()
{
	if (state.action != PeerState::Disconnected)
	{
		state.action = PeerState::Disconnected;
		stream->close();
	}
}

const std::shared_ptr<mtt::PeerStream> mtt::PeerCommunication::getStream() const
{
	return stream;
}

void mtt::PeerCommunication::connectionClosed(int code)
{
	BT_LOG("connectionClosed " << code);
	listener.connectionClosed(this, code);

	stream->onOpenCallback = nullptr;
}

void mtt::PeerCommunication::setInterested(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amInterested == enabled)
		return;

	BT_LOG("setInterested " << enabled);

	state.amInterested = enabled;
	send(mtt::bt::createStateMessage(enabled ? PeerMessage::Interested : PeerMessage::NotInterested));
}

void mtt::PeerCommunication::setChoke(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amChoking == enabled)
		return;

	BT_LOG("setChoke " << enabled);

	state.amChoking = enabled;
	send(mtt::bt::createStateMessage(enabled ? PeerMessage::Choke : PeerMessage::Unchoke));
}

void mtt::PeerCommunication::requestPieceBlock(const PieceBlockInfo& pieceInfo)
{
	if (!isEstablished())
		return;

	BT_LOG("requestPieceBlock " << pieceInfo.index << pieceInfo.begin << pieceInfo.length);

	send(mtt::bt::createBlockRequest(pieceInfo));
}

bool mtt::PeerCommunication::isEstablished() const
{
	return state.action == PeerState::Established;
}

void mtt::PeerCommunication::sendKeepAlive()
{
	if (!isEstablished())
		return;

	BT_LOG("sendKeepAlive");
	send(DataBuffer(4, 0));
}

void mtt::PeerCommunication::sendHave(uint32_t pieceIdx)
{
	if (!isEstablished())
		return;

	BT_LOG("sendHave " << pieceIdx);
	send(mtt::bt::createHave(pieceIdx));
}

void mtt::PeerCommunication::sendPieceBlock(const PieceBlock& block)
{
	if (!isEstablished())
		return;

	BT_LOG("sendPieceBlock " << block.info.index);
	stats.uploaded += block.info.length;

	send(mtt::bt::createPiece(block));
}

void mtt::PeerCommunication::sendBitfield(const DataBuffer& bitfield)
{
	if (!isEstablished())
		return;

	BT_LOG("sendBitfield");
	send(mtt::bt::createBitfield(bitfield));
}

void mtt::PeerCommunication::resetState()
{
	state = PeerState();
	info = PeerInfo();
}

void mtt::PeerCommunication::sendPort(uint16_t port)
{
	if (!isEstablished())
		return;

	BT_LOG("sendPort " << port);
	send(mtt::bt::createPort(port));
}

void mtt::PeerCommunication::send(DataBuffer data)
{
	stream->write(data);
}

void mtt::PeerCommunication::handleMessage(PeerMessage& message)
{
	stats.lastActivityTime = mtt::CurrentTimestamp();

	if (message.id == PeerMessage::Bitfield)
	{
		info.pieces.fromBitfieldData(message.bitfield);

		BT_LOG("Bitfield progress: " << info.pieces.getPercentage());
	}
	else if (message.id == PeerMessage::Have)
	{
		info.pieces.addPiece(message.havePieceIndex);

		BT_LOG("Have " << message.havePieceIndex);
	}
	else if (message.id == PeerMessage::Unchoke)
	{
		BT_LOG("Unchoke");
		state.peerChoking = false;
		stream->setMinBandwidthRequest(BlockMaxSize + 20);
	}
	else if (message.id == PeerMessage::Choke)
	{
		BT_LOG("Choke");
		state.peerChoking = true;
		stream->setMinBandwidthRequest(100);
	}
	else if (message.id == PeerMessage::NotInterested)
	{
		BT_LOG("NotInterested");
		state.peerInterested = true;
	}
	else if (message.id == PeerMessage::Interested)
	{
		BT_LOG("Interested");
		state.peerInterested = false;
	}
	else if (message.id == PeerMessage::Extended)
	{
		BT_LOG("Extended message " << message.extended.id);
		handleExtendedMessage(message.extended.id, message.extended.data);
	}
	else if (message.id == PeerMessage::Handshake)
	{
		BT_LOG("Handshake");

		if (state.action == PeerState::Handshake || state.action == PeerState::Connected)
		{
			if (state.action == PeerState::Connected)
				sendHandshake();

			state.action = PeerState::Established;
			BT_LOG("finished handshake");
			memcpy(info.id, message.handshake.peerId, 20);
			memcpy(info.protocol, message.handshake.reservedBytes, 8);
			info.pieces.init(torrent.pieces.size());

			if (info.supportsExtensions() && ext.state == PeerExtensions::Disabled)
				sendExtendedHandshake();

			if (info.supportsDht() && mtt::config::getExternal().dht.enabled)
				sendPort(mtt::config::getExternal().connection.udpPort);

			listener.handshakeFinished(this);
		}
	}
	else if (message.id == PeerMessage::Piece)
	{
		stats.downloaded += message.piece.info.length;
		BT_LOG("Piece " << message.piece.info.index << message.piece.info.begin << message.piece.info.length);
	}
	else
		BT_LOG("OtherMessage " << message.id);

	listener.messageReceived(this, message);
}

void mtt::PeerCommunication::handleExtendedMessage(char id, const BufferView& data)
{
	auto type = ext::Type(id);

	if (type == ext::Type::Handshake)
	{
		BencodeParser parser;
		if (!parser.parse(data.data, data.size))
			return;

		ext::Handshake handshake;
		auto root = parser.getRoot();

		if (root->isMap())
		{
			if (auto extensionsInfo = root->getDictObject("m"))
			{
				for (const auto& e : *extensionsInfo)
				{
					handshake.messageIds[e.getTxt()] = e.getDictItemValue().getInt();
				}
			}

			if (auto version = root->getTxtItem("v"))
				handshake.client = info.client = version->toString();

			if (auto ip = root->getTxtItem("yourip"); ip && ip->size == 4)
				memcpy(handshake.yourIp, ip->data, 4);

			if (auto metadataSize = root->getIntObject("metadata_size"))
				handshake.metadataSize = (uint32_t)metadataSize->getInt();
		}

		if (ext.state == PeerExtensions::Disabled)
			sendExtendedHandshake();

		ext.state = PeerExtensions::Enabled;
		ext.pex.init("ut_pex", handshake, this);
		ext.utm.init("ut_metadata", handshake, this);
		ext.holepunch.init("ut_holepunch", handshake, this);

		listener.extendedHandshakeFinished(this, handshake);
	}
	else
		listener.extendedMessageReceived(this, type, data);
}

void mtt::PeerCommunication::sendExtendedHandshake()
{
	send(bt::createExtendedHandshake(stream->getIpString(), torrent.data.size()));
	ext.state = PeerExtensions::Handshake;
}
