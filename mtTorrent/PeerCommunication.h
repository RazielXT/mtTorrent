#pragma once
#include <vector>
#include "TcpAsyncStream.h"
#include "BencodeParser.h"
#include "PeerMessage.h"
#include "TorrentDefines.h"
#include "ExtensionProtocol.h"

namespace mtt
{
	struct PeerState
	{
		bool finishedHandshake = false;

		bool amChoking = true;
		bool amInterested = false;

		bool peerChoking = true;
		bool peerInterested = false;
	};

	class PeerCommunication
	{
	public:

		PeerCommunication(boost::asio::io_service* io_service, ProgressScheduler* scheduler);

		ExtensionProtocol ext;
		PeerState state;

		PeerInfo peerInfo;
		TorrentFileInfo* torrent;
		ProgressScheduler* scheduler;

		PiecesProgress peerPieces;

		std::mutex schedule_mutex;
		PieceDownloadInfo scheduledPieceInfo;
		DownloadedPiece downloadingPiece;

		bool validPiece();

		void start(TorrentFileInfo* torrent, PeerInfo peerInfo);
		void stop();

		void connectionOpened();
		void handshake(PeerInfo& peerInfo);

		void dataReceived();
		void connectionClosed();
		bool active = false;

		DataBuffer getHandshakeMessage();

		std::mutex read_mutex;
		PeerMessage readNextStreamMessage();
		TcpAsyncStream stream;

		void sendInterested();
		void sendHandshakeExt();

		void sendBlockRequest(PieceBlockInfo& block);
		void schedulePieceDownload(bool forceNext = false);

		void handleMessage(PeerMessage& msg);
	};

}