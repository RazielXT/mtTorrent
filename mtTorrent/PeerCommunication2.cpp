#include "PeerCommunication2.h"
#include "PacketHelper.h"
#include "BTProtocol.h"
#include "Configuration.h"

using namespace mtt;


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
}

void PeerCommunication2::startHandshake(Addr& address)
{
	if (state.action == PeerCommunicationState::Disconnected)
	{
		state.action = PeerCommunicationState::Handshake;
		stream.connect(address.addrBytes.data(), address.port, address.isIpv6());
	}
}

void PeerCommunication2::dataReceived()
{
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

		auto requestData = mtt::bt::createHandshake(torrent.hash, mtt::config::internal.hashId);
		stream.write(requestData);
	}
}

void mtt::PeerCommunication2::stop()
{
	if (state.action != PeerCommunicationState::Disconnected)
		stream.close();
}

void mtt::PeerCommunication2::connectionClosed()
{
	listener.connectionClosed();

	state.action = PeerCommunicationState::Disconnected;
}

mtt::PeerMessage mtt::PeerCommunication2::readNextStreamMessage()
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

void mtt::PeerCommunication2::sendInterested()
{
	state.amInterested = true;

	stream.write(mtt::bt::createInterested());
}

void mtt::PeerCommunication2::sendHandshakeExt()
{
	stream.write(ext.getExtendedHandshakeMessage());
}

void mtt::PeerCommunication2::requestPieceBlock()
{
	auto& b = scheduledPieceInfo.blocksLeft.back();
	scheduledPieceInfo.blocksLeft.pop_back();

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
		std::lock_guard<std::mutex> guard(schedule_mutex);

		PEER_LOG("Piece id: " << std::to_string(message.piece.info.index) << ", size: " << std::to_string(message.piece.info.length) << "\n");

		if (message.piece.info.index != downloadingPiece.index)
		{
			listener.pieceReceived(nullptr);
			PEER_LOG(peerInfo.ipStr << " Invalid block!! \n")
		}
		else
		{
			downloadingPiece.addBlock(message.piece);

			if (downloadingPiece.receivedBlocks == scheduledPieceInfo.blocksCount)
			{
				listener.pieceReceived(&downloadingPiece);
			}
			else
			{
				requestPieceBlock();
			}
		}
	}

	if (message.id == Unchoke)
	{
	}

	if (message.id == Extended)
	{
		PEER_LOG(peerInfo.ipStr << " Ext msg: " << std::string(message.extended.data.begin(), message.extended.data.end()) << "\n");

		auto type = ext.load(message.extended.id, message.extended.data);

		PEER_LOG(peerInfo.ipStr << " Ext Type " << std::to_string(message.extended.id) << " resolve :" << std::to_string(type) << "\n");
	}

	if (message.id == Handshake)
	{
		if (!state.finishedHandshake)
		{
			state.finishedHandshake = true;
			PEER_LOG(peerInfo.ipStr << " finished handshake\n");

			sendHandshakeExt();

			listener.handshakeFinished();
		}
	}
}
