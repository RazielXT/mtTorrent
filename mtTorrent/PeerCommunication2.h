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

		enum
		{
			Idle,
			Handshake,
			Downloading,
			Uploading,
			Metadata
		}
		action;
	};

	class PeerCommunication2
	{
	public:

		PeerCommunication2(PeerInfo& info);

		PiecesProgress pieces;
		PeerInfo info;
		PeerState state;

		bool requestPiece();
		std::function<void()> onReceivedPiece;

		bool requestMetadataPiece();
		std::function<void()> onReceivedMetadataPiece;

		void startHandshake();
		std::function<void(bool)> onHandshakeFinished;
		std::function<void()> onPexReceived;

	private:

		std::mutex schedule_mutex;
		PieceDownloadInfo scheduledPieceInfo;
		DownloadedPiece downloadingPiece;

		ext::ExtensionProtocol ext;
		TcpAsyncStream stream;
	};

}