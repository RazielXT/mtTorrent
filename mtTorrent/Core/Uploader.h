#pragma once
#include "Interface.h"

namespace mtt
{
	class PeerCommunication;

	class Uploader
	{
	public:

		Uploader(TorrentPtr);

		void isInterested(PeerCommunication* p);
		void wantsUnchoke(PeerCommunication* p);
		bool pieceRequest(PeerCommunication* p, PieceBlockInfo& info);

	private:

		TorrentPtr torrent;
	};
}