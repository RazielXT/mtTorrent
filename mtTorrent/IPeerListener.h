#pragma once

namespace mtt
{
	struct DownloadedPiece;

	class IPeerListener
	{
	public:

		virtual void handshakeFinished() = 0;
		virtual void connectionClosed() = 0;

		virtual void progressUpdated() = 0;
		virtual void pieceReceived(DownloadedPiece* piece) = 0;

		virtual void metadataPieceReceived() = 0;
		virtual void pexReceived() = 0;
	};
}
