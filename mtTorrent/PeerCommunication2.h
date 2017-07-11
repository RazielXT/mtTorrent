#pragma once
#include <vector>
#include "TcpAsyncStream.h"
#include "BencodeParser.h"
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
			Handshake,
			TransferringData,
			Idle
		}
		action = Disconnected;
	};

	struct PeerStateInfo
	{
		PeerStateInfo();

		PiecesProgress pieces;
		uint8_t id[20];
		uint8_t protocol[8];
	};

	class PeerCommunication2
	{
	public:

		PeerCommunication2(TorrentInfo& torrent, IPeerListener& listener, boost::asio::io_service& io_service);

		PeerStateInfo info;
		PeerCommunicationState state;

		void start(Addr& address);
		bool sendInterested();
		bool requestPiece(PieceDownloadInfo& pieceInfo);

		void stop();

		bool requestMetadataPiece(uint32_t index);

		ext::ExtensionProtocol ext;

	private:

		TorrentInfo& torrent;
		IPeerListener& listener;

		std::mutex schedule_mutex;
		PieceDownloadInfo scheduledPieceInfo;
		DownloadedPiece downloadingPiece;
		
		TcpAsyncStream stream;

		std::mutex read_mutex;
		mtt::PeerMessage readNextStreamMessage();
		void handleMessage(PeerMessage& msg);

		void connectionOpened();
		void dataReceived();
		void connectionClosed();

		void sendHandshakeExt();
		void requestPieceBlock();
	};

}