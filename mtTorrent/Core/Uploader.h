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
		bool pieceRequest(PeerCommunication* p, PieceBlockInfo& info);

		uint64_t uploaded = 0;

	private:

		TorrentPtr torrent;
	};
}