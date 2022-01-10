#pragma once

#include "PeerMessage.h"
#include "ExtensionProtocol.h"
#include "IPeerListener.h"
#include "PiecesProgress.h"
#include "PeerStream.h"

namespace mtt
{
	struct PeerCommunicationState
	{
		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;

		enum : uint16_t
		{
			Disconnected,
			Connecting,
			Connected,
			Handshake,
			Established
		}
		action = Disconnected;
	};

	struct PeerInfo
	{
		PeerInfo();

		PiecesProgress pieces;
		uint8_t id[20];
		uint8_t protocol[8];

		bool supportsExtensions();
		bool supportsDht();
	};

	class PeerCommunication : public std::enable_shared_from_this<PeerCommunication>
	{
	public:

		PeerCommunication(TorrentInfo& torrent, IPeerListener& listener, asio::io_service& io_service);
		~PeerCommunication();

		size_t fromStream(std::shared_ptr<TcpAsyncStream> stream, const BufferView& streamData);
		size_t fromStream(utp::StreamPtr stream, const BufferView& streamData);

		PeerInfo info;
		PeerCommunicationState state;

		void sendHandshake(const Addr& address);
		void sendHandshake();

		void setInterested(bool enabled);
		void setChoke(bool enabled);

		void requestPieceBlock(PieceBlockInfo& pieceInfo);
		bool isEstablished() const;

		void sendKeepAlive();
		void sendBitfield(const DataBuffer& bitfield);
		void sendHave(uint32_t pieceIdx);
		void sendPieceBlock(const PieceBlock& block);

		void sendPort(uint16_t port);

		void close();

		ext::ExtensionProtocol ext;

		const std::shared_ptr<PeerStream> getStream() const;

	protected:

		FileLog log;

		IPeerListener& listener;

		TorrentInfo& torrent;

		std::shared_ptr<PeerStream> stream;

		void handleMessage(PeerMessage& msg);

		void connectionOpened();
		size_t dataReceived(const BufferView& buffer);
		void connectionClosed(int);

		void initializeStream();
		void resetState();
	};

}