#pragma once

#include "PeerMessage.h"
#include "IPeerListener.h"
#include "PiecesProgress.h"
#include "PeerStream.h"
#include "PexExtension.h"
#include "HolepunchExtension.h"
#include "MetadataExtension.h"

namespace mtt
{
	struct PeerInfo
	{
		PeerInfo();

		PiecesProgress pieces;
		uint8_t id[20];
		uint8_t protocol[8];
		std::string client;

		bool supportsExtensions();
		bool supportsDht();
	};

	struct PeerExtensions
	{
		enum
		{
			Disabled,
			Handshake,
			Enabled
		}
		state = Disabled;

		bool enabled() const { return state == Enabled; }

		ext::PeerExchange::Remote pex;
		ext::UtMetadata utm;
		ext::UtHolepunch holepunch;
	};

	class PeerCommunication : public std::enable_shared_from_this<PeerCommunication>
	{
	public:

		PeerCommunication(TorrentInfo& torrent, IPeerListener& listener, asio::io_context& io_context);
		~PeerCommunication();

		PeerInfo info;
		PeerState state;
		PeerExtensions ext;
		PeerStats stats;

		void connect(const Addr& address);
		size_t fromStream(std::shared_ptr<PeerStream> stream, const BufferView& streamData);
		bool isEstablished() const;

		void setInterested(bool enabled);
		void setChoke(bool enabled);

		void requestPieceBlock(const PieceBlockInfo& pieceInfo);

		void sendKeepAlive();
		void sendBitfield(const DataBuffer& bitfield);
		void sendHave(uint32_t pieceIdx);
		void sendPieceBlock(const PieceBlock& block);

		void sendPort(uint16_t port);

		void send(DataBuffer);
		void close();

		const std::shared_ptr<PeerStream> getStream() const;

	protected:

		FileLog log;

		IPeerListener& listener;

		TorrentInfo& torrent;

		std::shared_ptr<PeerStream> stream;

		void sendHandshake();
		void sendExtendedHandshake();

		void handleMessage(PeerMessage& msg);
		void handleExtendedMessage(char id, const BufferView& data);

		void connectionOpened();
		size_t dataReceived(const BufferView& buffer);
		void connectionClosed(int);

		void initializeStream();
		void resetState();
	};

}