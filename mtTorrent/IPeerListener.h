#pragma once
#include "PeerMessage.h"
#include "ExtensionProtocol.h"

namespace mtt
{
	struct DownloadedPiece;
	class PeerCommunication;

	class IPeerListener
	{
	public:

		virtual void handshakeFinished(PeerCommunication*) = 0;
		virtual void connectionClosed(PeerCommunication*) = 0;

		virtual void messageReceived(PeerCommunication*, PeerMessage&) = 0;

		virtual void progressUpdated(PeerCommunication*) = 0;
		virtual void pieceReceived(PeerCommunication*, DownloadedPiece* piece) = 0;

		virtual void extHandshakeFinished(PeerCommunication*) = 0;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) = 0;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) = 0;
	};
}