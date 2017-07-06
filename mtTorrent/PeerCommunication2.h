#pragma once
#include <vector>
#include "TcpAsyncStream.h"
#include "BencodeParser.h"
#include "PeerMessage.h"
#include "TorrentDefines.h"
#include "ExtensionProtocol.h"

namespace mtt
{
	class PeerCommunication2
	{
	public:

		PeerCommunication2(PeerInfo& info);

		struct
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
		}
		state;

		PiecesProgress peerPieces;
		PeerInfo info;

		bool requestPiece();
		void setOnPieceCallback(std::function<void()> func);

		bool requestMetadataPiece();
		void setOnMetadataPieceCallback(std::function<void()> func);

		void startHandshake();
		void setOnHandshakeFinishedCallback(std::function<void(bool)> func);

		void setOnPexReceivedCallback(std::function<void()> func);

		std::mutex schedule_mutex;
		PieceDownloadInfo scheduledPieceInfo;
		DownloadedPiece downloadingPiece;

		ExtensionProtocol ext;
		TcpAsyncStream stream;
	};

}