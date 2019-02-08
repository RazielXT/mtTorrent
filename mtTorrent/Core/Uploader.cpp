#include "Uploader.h"
#include "Torrent.h"
#include "PeerCommunication.h"

mtt::Uploader::Uploader(TorrentPtr t)
{
	torrent = t;
}

void mtt::Uploader::isInterested(PeerCommunication* p)
{
	p->setChoke(false);
}

bool mtt::Uploader::pieceRequest(PeerCommunication* p, PieceBlockInfo& info)
{
	auto block = torrent->files.storage.getPieceBlock(info);
	p->sendPieceBlock(block);

	return true;
}

