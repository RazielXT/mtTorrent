#pragma once
#include <vector>
#include "utils/TcpAsyncStream.h"
#include "PeerMessage.h"
#include "ExtensionProtocol.h"
#include "IPeerListener.h"
#include "PiecesProgress.h"

namespace mtt
{
	struct PeerCommunicationState
	{
		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;

		enum
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
		PeerCommunication(TorrentInfo& torrent, IPeerListener& listener);
		~PeerCommunication();

		void setStream(std::shared_ptr<TcpAsyncStream> stream);

		PeerInfo info;
		PeerCommunicationState state;

		void sendHandshake(Addr& address);
		void sendHandshake();

		void setInterested(bool enabled);
		void setChoke(bool enabled);

		void requestPieceBlock(PieceBlockInfo& pieceInfo);
		bool isEstablished();

		void sendKeepAlive();
		void sendBitfield(DataBuffer& bitfield);
		void sendHave(uint32_t pieceIdx);
		void sendPieceBlock(PieceBlock& block);

		void sendPort(uint16_t port);

		void stop();

		ext::ExtensionProtocol ext;

		Addr getAddress();
		std::string getAddressName();

	protected:

		IPeerListener& listener;

		TorrentInfo& torrent;

		std::shared_ptr<TcpAsyncStream> stream;

		std::mutex read_mutex;
		mtt::PeerMessage readNextStreamMessage();
		void handleMessage(PeerMessage& msg);

		void connectionOpened();
		void dataReceived();
		void connectionClosed(int);

		void initializeCallbacks();
		void resetState();

		std::mutex logMtx;
		std::vector<std::string> logs;
		void LogMsg(std::stringstream& s);
		void SerializeLogs();
	};

}