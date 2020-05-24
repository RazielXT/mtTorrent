#include "PeerCommunication.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include <fstream>
#include "utils/HexEncoding.h"

#define BT_LOG(x) WRITE_LOG(LogTypeBt, "(" << getAddressName() << ") " << x)
#define LOG_MGS(x) BT_LOG(x)//{std::stringstream ss; ss << x; LogMsg(ss);}
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

			if (mtt::config::getExternal().dht.enable)
				reserved_byte[7] |= 0x80;

			packet.add(reserved_byte, 8);

			packet.add(torrentHash, 20);
			packet.add(clientHash, 20);

			return packet.getBuffer();
		}

		DataBuffer createStateMessage(PeerMessageId id)
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
			packet.add(Request);
			packet.add32(block.index);
			packet.add32(block.begin);
			packet.add32(block.length);

			return packet.getBuffer();
		}

		DataBuffer createHave(uint32_t idx)
		{
			PacketBuilder packet(9);
			packet.add32(5);
			packet.add(Have);
			packet.add32(idx);

			return packet.getBuffer();
		}

		DataBuffer createPort(uint16_t port)
		{
			PacketBuilder packet(9);
			packet.add32(3);
			packet.add(Port);
			packet.add16(port);

			return packet.getBuffer();
		}

		DataBuffer createBitfield(DataBuffer& bitfield)
		{
			PacketBuilder packet(5 + (uint32_t)bitfield.size());
			packet.add32(1 + (uint32_t)bitfield.size());
			packet.add(Bitfield);
			packet.add(bitfield.data(), bitfield.size());

			return packet.getBuffer();
		}

		DataBuffer createPiece(PieceBlock& block)
		{
			uint32_t dataSize = 1 + 8 + (uint32_t)block.buffer.size;
			PacketBuilder packet(4 + dataSize);
			packet.add32(dataSize);
			packet.add(Piece);
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

PeerCommunication::PeerCommunication(TorrentInfo& t, IPeerListener& l) : torrent(t), listener(l)
{
}

PeerCommunication::PeerCommunication(TorrentInfo& t, IPeerListener& l, asio::io_service& io_service) : torrent(t), listener(l)
{
	stream = std::make_shared<TcpAsyncLimitedStream>(io_service);
	initializeBandwidth();
}

uint32_t mtt::PeerCommunication::fromStream(std::shared_ptr<TcpAsyncLimitedStream> s, const BufferView& streamData)
{
	stream = s;

	state.action = PeerCommunicationState::Connected;
	initializeBandwidth();
	initializeCallbacks();

	return dataReceived(streamData);
}

void mtt::PeerCommunication::initializeBandwidth()
{
	BandwidthChannel* channels[2];
	channels[0] = BandwidthManager::Get().GetChannel("");
	channels[1] = BandwidthManager::Get().GetChannel(hexToString(torrent.hash, 20));

	stream->setBandwidthChannels(channels, 2);
}

void mtt::PeerCommunication::initializeCallbacks()
{
	{
		//std::lock_guard<std::mutex> guard(stream->callbackMutex);
		stream->onConnectCallback = std::bind(&PeerCommunication::connectionOpened, shared_from_this());
		stream->onCloseCallback = [this](int code) {connectionClosed(code); };
		stream->onReceiveCallback = std::bind(&PeerCommunication::dataReceived, shared_from_this(), std::placeholders::_1);
	}

	ext.stream = stream;

	ext.pex.onPexMessage = [this](mtt::ext::PeerExchange::Message& msg)
	{
		listener.pexReceived(this, msg);
	};
	ext.utm.onUtMetadataMessage = [this](mtt::ext::UtMetadata::Message& msg)
	{
		listener.metadataPieceReceived(this, msg);
	};
}

void PeerCommunication::sendHandshake(Addr& address)
{
	addLogEvent(Start, (uint16_t)state.action);

	if (state.action != PeerCommunicationState::Disconnected)
		resetState();

	state.action = PeerCommunicationState::Connecting;
	initializeCallbacks();
	stream->connect(address.addrBytes, address.port, address.ipv6);
}

void mtt::PeerCommunication::sendHandshake()
{
	if (!state.finishedHandshake && state.action == PeerCommunicationState::Connected)
	{
		LOG_MGS("Handshake");
		state.action = PeerCommunicationState::Handshake;
		stream->write(mtt::bt::createHandshake(torrent.hash, mtt::config::getInternal().hashId));
	}
}

size_t PeerCommunication::dataReceived(const BufferView& buffer)
{
	size_t consumedSize = 0;

	auto readNextMessage = [&]()
	{
		PeerMessage msg({ buffer.data + consumedSize, buffer.size - consumedSize });

		if (msg.id != Invalid)
			consumedSize += msg.messageSize;
		else if (!msg.messageSize)
			consumedSize = buffer.size;

		return std::move(msg);
	};

	auto message = readNextMessage();
	auto ptr = shared_from_this();

	while (message.id != Invalid)
	{
		handleMessage(message);
		message = readNextMessage();
	}

	return consumedSize;
}

void PeerCommunication::connectionOpened()
{
	state.action = PeerCommunicationState::Connected;

	sendHandshake();
}

void mtt::PeerCommunication::stop()
{
	if (state.action != PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Disconnected;
		stream->close(false);
	}
}

Addr mtt::PeerCommunication::getAddress()
{
	Addr out;
	out.set(stream->getEndpoint().address(), stream->getEndpoint().port());
	return out;
}

std::string mtt::PeerCommunication::getAddressName()
{
	return stream->getHostname();
}

uint64_t mtt::PeerCommunication::getReceivedDataCount()
{
	return stream->getReceivedDataCount();
}

void mtt::PeerCommunication::connectionClosed(int code)
{
	addLogEvent(End, (uint16_t)code);
	saveLogEvents();

	if (state.action != PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Disconnected;
		listener.connectionClosed(this, code);
	}
}

mtt::PeerCommunication::~PeerCommunication()
{
	//stream->close();
}

void mtt::PeerCommunication::setInterested(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amInterested == enabled)
		return;

	LOG_MGS("Interested " << enabled);
	addLogEvent(Want, 0);

	state.amInterested = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? Interested : NotInterested));
}

void mtt::PeerCommunication::setChoke(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amChoking == enabled)
		return;

	addLogEvent(Want, 1);
	LOG_MGS("Choke " << enabled);
	state.amChoking = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? Choke : Unchoke));
}

void mtt::PeerCommunication::requestPieceBlock(PieceBlockInfo& pieceInfo)
{
	if (!isEstablished())
		return;

	addLogEvent(Request, (uint16_t)pieceInfo.index, (char)((float)pieceInfo.begin / (16 * 1024.f)));

	LOG_MGS("Request " << pieceInfo.index);
	stream->write(mtt::bt::createBlockRequest(pieceInfo));
}

bool mtt::PeerCommunication::isEstablished()
{
	return state.action == PeerCommunicationState::Established;
}

void mtt::PeerCommunication::sendKeepAlive()
{
	if (!isEstablished())
		return;

	LOG_MGS("KeepAlive");
	stream->write(DataBuffer(4, 0));
}

void mtt::PeerCommunication::sendHave(uint32_t pieceIdx)
{
	if (!isEstablished())
		return;

	LOG_MGS("Have " << pieceIdx);
	stream->write(mtt::bt::createHave(pieceIdx));
}

void mtt::PeerCommunication::sendPieceBlock(PieceBlock& block)
{
	if (!isEstablished())
		return;

	LOG_MGS("Piece " << block.info.index);
	stream->write(mtt::bt::createPiece(block));
}

void mtt::PeerCommunication::sendBitfield(DataBuffer& bitfield)
{
	if (!isEstablished())
		return;

	LOG_MGS("Bitfield");
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

	LOG_MGS("Port " << port);
	stream->write(mtt::bt::createPort(port));
}

void mtt::PeerCommunication::handleMessage(PeerMessage& message)
{
	if(message.id == Piece)
		addLogEvent(RespPiece, (uint16_t)message.piece.info.index, (char)(message.piece.info.begin / (16 * 1024.f)));
	else
		addLogEvent(Msg, (uint16_t)message.id);

	LOG_MGS("Received MSG: " << message.id << ", size: " << message.messageSize);

	if (message.id == Bitfield)
	{
		info.pieces.fromBitfield(message.bitfield);

		LOG_MGS("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, -1);
	}
	else if (message.id == Have)
	{
		info.pieces.addPiece(message.havePieceIndex);

		LOG_MGS("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, message.havePieceIndex);
	}
	else if (message.id == Unchoke)
	{
		state.peerChoking = false;
		stream->setMinBandwidthRequest(BlockRequestMaxSize + 20);
	}
	else if (message.id == Choke)
	{
		state.peerChoking = true;
		stream->setMinBandwidthRequest(100);
	}
	else if (message.id == NotInterested)
	{
		state.peerInterested = true;
	}
	else if (message.id == Interested)
	{
		state.peerInterested = false;
	}
	else if (message.id == Extended)
	{
		auto type = ext.load(message.extended.id, message.extended.data);
		BT_LOG("ext message handled " << (int)type);

		if (type == mtt::ext::HandshakeEx)
		{
			listener.extHandshakeFinished(this);
		}
	}
	else if (message.id == Handshake)
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

				if (info.supportsDht() && mtt::config::getExternal().dht.enable)
					sendPort(mtt::config::getExternal().connection.udpPort);

				listener.handshakeFinished(this);
			}
			else
				state.action = PeerCommunicationState::Established;
		}
	}

	listener.messageReceived(this, message);
}

#ifdef PEER_DIAGNOSTICS
void mtt::PeerCommunication::addLogEvent(LogEvent e, uint16_t idx, char info /*= 0*/)
{
	std::lock_guard<std::mutex> guard(logmtx);
	logevents.push_back({ e, info, idx, clock() });
}

extern std::string FormatLogTime(long);

void mtt::PeerCommunication::saveLogEvents()
{
	std::lock_guard<std::mutex> guard(logmtx);

	if (logevents.empty())
		return;

	std::ofstream file("logs\\" + torrent.name + "\\" + stream->getHostname() + ".log", std::ios::app);

	if (!file)
		return;

	for (auto& l : logevents)
	{
		if (l.e == Msg)
			file << FormatLogTime(l.time) << (int)l.e << " Msg:" << l.idx << "\n";
		else if (l.e == Request || l.e == RespPiece)
			file << FormatLogTime(l.time) << (int)l.e << " Idx:" << l.idx << " Block:" << (int)l.info << "\n";
		else
			file << FormatLogTime(l.time) << (int)l.e << " Info:" << l.idx << "\n";
	}

	file << "\n\n\n";
}
#endif
