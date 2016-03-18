#pragma once
#include <vector>
#include "TcpAsyncStream.h"
#include "BencodeParser.h"
#include "PeerMessage.h"
#include "Interface.h"
#include "ExtensionProtocol.h"

namespace Torrent
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

		PeerCommunication(ClientInfo* client);

		ExtensionProtocol ext;
		PeerState state;

		PeerInfo peerInfo;
		TorrentInfo* torrent;
		ClientInfo* client;

		PiecesProgress peerPieces;

		std::mutex schedule_mutex;
		PieceDownloadInfo scheduledPieceInfo;
		DownloadedPiece downloadingPiece;

		bool validPiece();

		void start(TorrentInfo* torrent, PeerInfo peerInfo);
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