#include "PeerCommunication.h"
#include "utils/PacketHelper.h"
#include "Configuration.h"
#include <fstream>

#define BT_LOG(x) WRITE_LOG(LogTypeBt, "(" << getAddressName() << ") " << x)
#define LOG_MGS(x) {}//{std::stringstream ss; ss << x; LogMsg(ss);}
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
			uint32_t dataSize = 1 + 8 + (uint32_t)block.data.size();
			PacketBuilder packet(4 + dataSize);
			packet.add32(dataSize);
			packet.add(Piece);
			packet.add32(block.info.index);
			packet.add32(block.info.begin);
			packet.add(block.data.data(), block.data.size());

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

PeerCommunication::PeerCommunication(TorrentInfo& t, IPeerListener& l, boost::asio::io_service& io_service) : torrent(t), listener(l)
{
	stream = std::make_shared<TcpAsyncStream>(io_service);

	initializeCallbacks();
}

void mtt::PeerCommunication::setStream(std::shared_ptr<TcpAsyncStream> s)
{
	stream = s;
	state.action = PeerCommunicationState::Connected;

	initializeCallbacks();
	dataReceived();
}

void mtt::PeerCommunication::initializeCallbacks()
{
	{
		std::lock_guard<std::mutex> guard(stream->callbackMutex);
		stream->onConnectCallback = std::bind(&PeerCommunication::connectionOpened, this);
		stream->onCloseCallback = [this](int code) {connectionClosed(code); };
		stream->onReceiveCallback = std::bind(&PeerCommunication::dataReceived, this);
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
	if (state.action != PeerCommunicationState::Disconnected)
		resetState();

	state.action = PeerCommunicationState::Connecting;
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

void PeerCommunication::dataReceived()
{
	std::lock_guard<std::mutex> guard(read_mutex);

	auto message = readNextStreamMessage();

	while (message.id != Invalid)
	{
		handleMessage(message);
		message = readNextStreamMessage();
	}
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
		stream->close();
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

void mtt::PeerCommunication::connectionClosed(int code)
{
	if (!logs.empty())
	{
		LOG_MGS("Closed code " << code);
		SerializeLogs();
	}
	state.action = PeerCommunicationState::Disconnected;
	listener.connectionClosed(this, code);
}

mtt::PeerMessage mtt::PeerCommunication::readNextStreamMessage()
{
	auto data = stream->getReceivedData();
	PeerMessage msg(data);

	if (msg.id != Invalid)
		stream->consumeData(msg.messageSize);
	else if (!msg.messageSize)
		stream->consumeData(data.size());

	return msg;
}

mtt::PeerCommunication::~PeerCommunication()
{
	stream->close();
}

void mtt::PeerCommunication::setInterested(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amInterested == enabled)
		return;

	LOG_MGS("Interested");
	state.amInterested = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? Interested : NotInterested));
}

void mtt::PeerCommunication::setChoke(bool enabled)
{
	if (!isEstablished())
		return;

	if (state.amChoking == enabled)
		return;

	LOG_MGS("Choke");
	state.amChoking = enabled;
	stream->write(mtt::bt::createStateMessage(enabled ? Choke : Unchoke));
}

void mtt::PeerCommunication::requestPieceBlock(PieceBlockInfo& pieceInfo)
{
	if (!isEstablished())
		return;

	LOG_MGS("Request");
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

	LOG_MGS("Have");
	stream->write(mtt::bt::createHave(pieceIdx));
}

void mtt::PeerCommunication::sendPieceBlock(PieceBlock& block)
{
	if (!isEstablished())
		return;

	LOG_MGS("Piece");
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

void mtt::PeerCommunication::LogMsg(std::stringstream& s)
{
	std::lock_guard<std::mutex> guard(logMtx);

	time_t rawtime;
	struct tm timeinfo;
	char buffer[80];

	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);
	strftime(buffer, 80, "%T: ", &timeinfo);

	logs.push_back(buffer + s.str());
}

void mtt::PeerCommunication::SerializeLogs()
{
	std::lock_guard<std::mutex> guard(logMtx);
	std::ofstream file(stream->getHostname());
	for (auto& l : logs)
	{
		file << l << "\n";
	}
}

void mtt::PeerCommunication::sendPort(uint16_t port)
{
	if (!isEstablished())
		return;

	LOG_MGS("Port");
	stream->write(mtt::bt::createPort(port));
}

void mtt::PeerCommunication::handleMessage(PeerMessage& message)
{
	LOG_MGS("Received: " << message.id);

	if (message.id != Piece)
		BT_LOG("MSG ID:" << (int)message.id << ", size: " << message.messageSize);

	if (message.id == Bitfield)
	{
		info.pieces.fromBitfield(message.bitfield, torrent.pieces.size());

		BT_LOG("new percentage: " << std::to_string(info.pieces.getPercentage()));
		LOG_MGS("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, -1);
	}
	else if (message.id == Have)
	{
		info.pieces.addPiece(message.havePieceIndex);

		BT_LOG("new percentage: " << std::to_string(info.pieces.getPercentage()));
		LOG_MGS("Received progress: " << info.pieces.getPercentage());
		listener.progressUpdated(this, message.havePieceIndex);
	}
	else if (message.id == Unchoke)
	{
		state.peerChoking = false;
	}
	else if (message.id == Choke)
	{
		state.peerChoking = true;
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
