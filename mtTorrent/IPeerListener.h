#pragma once
#include "PeerMessage.h"
#include "ExtensionProtocol.h"

namespace mtt
{
	struct DownloadedPiece;

	class IPeerListener
	{
	public:

		virtual void handshakeFinished() = 0;
		virtual void connectionClosed() = 0;

		virtual void messageReceived(PeerMessage&) = 0;

		virtual void progressUpdated() = 0;
		virtual void pieceReceived(DownloadedPiece* piece) = 0;

		virtual void extHandshakeFinished() = 0;
		virtual void metadataPieceReceived(ext::UtMetadata::Message&) = 0;
		virtual void pexReceived(ext::PeerExchange::Message&) = 0;
	};
}
