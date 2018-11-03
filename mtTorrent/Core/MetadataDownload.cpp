#include "MetadataDownload.h"
#include "MetadataReconstruction.h"
#include "Core.h"

mtt::MetadataDownload::MetadataDownload()
{
}

void mtt::MetadataDownload::start(CorePtr c, std::function<void(bool)> f)
{
	core = c;
	onFinish = f;
	active = true;

	core->peerMgr.listener = this;
	core->peerMgr.start(core);

	/*for (auto& a : addr)
	{
		auto peer = std::make_shared<PeerCommunication>(torrent->info, *this, service.io);
		{
			std::lock_guard<std::mutex> guard(mtx);
			peers.push_back(peer);
		}

		peer->sendHandshake(a);
	}

	addTask(std::make_shared<UtMetadataReconstructionTask>());
	tasks.push_back();

	mtt::MetadataReconstruction metadata;
	metadata.init(peers.front()->ext.utm.size);

	while (!metadata.finished() && !failed)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peers.front()->ext.requestMetadataPiece(mdPiece);

		WAITFOR(failed || !utmMsg.metadata.empty())

			if (failed || utmMsg.id != mtt::ext::UtMetadata::Data)
				return;

		metadata.addPiece(utmMsg.metadata, utmMsg.piece);
		utmMsg.metadata.clear();
	}

	if (metadata.finished())
	{
		auto info = mtt::TorrentFileParser::parseTorrentInfo(metadata.buffer.data(), metadata.buffer.size());
		WRITE_LOG(info.files[0].path[0]);

		DownloadSelection selection;
		for (auto&f : info.files)
			selection.files.push_back({ false, f });

		mtt::Storage storage(info.pieceSize);
		storage.setPath("E:\\Torrent");
		storage.setSelection(selection);

		PiecesProgress piecesTodo;
		piecesTodo.fromSelection(selection);

		for (auto& p : peers)
			p->setInterested(true);

		std::vector<DownloadedPiece> piecesTodo;
		std::mutex pieceMtx;
		uint32_t neededBlocks = 0;
		bool finished = false;
		uint32_t finishedPieces = 0;
		onPeerMsg = [&](mtt::PeerMessage& msg)
		{
			std::lock_guard<std::mutex> guard(pieceMtx);
			if (msg.id == Piece)
			{
				pieceTodo.addBlock(msg.piece);

				if (pieceTodo.receivedBlocks == neededBlocks)
				{
					storage.storePiece(pieceTodo);
					piecesTodo.addPiece(pieceTodo.index);
					finished = true;
					finishedPieces++;
				}
			}
		};

		auto requestPiece = [&]()
		{
			auto p = piecesTodo.firstEmptyPiece();

			auto blocks = storage.makePieceBlocksInfo(p);
			WRITE_LOG("Requesting idx " << p);

			neededBlocks = blocks.size();
			finished = false;
			pieceTodo.reset(info.pieceSize);
			pieceTodo.index = p;

			for (auto& b : blocks)
			{
				peer.requestPieceBlock(b);
			}

			WAITFOR(finished);
		}
	}
	WAITFOR(false);*/
}

void mtt::MetadataDownload::stop()
{
	core->peerMgr.stop();
	activeComm = nullptr;
	active = false;
	onFinish = nullptr;
}

void mtt::MetadataDownload::onConnected(std::shared_ptr<PeerCommunication> peer, Addr&)
{
	if (peer->ext.utm.size)
	{
		activeComm = peer;

		if(metadata.buffer.empty())
			metadata.init(peer->ext.utm.size);

		requestPiece();
	}
	else
	{
		connectNext();
	}
}

void mtt::MetadataDownload::onConnectFail(Addr&)
{
	connectNext();
}

void mtt::MetadataDownload::onAddrReceived(std::vector<Addr>& addrs)
{
	possibleAddrs = addrs;

	if(!activeComm)
		connectNext();
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication*)
{
}

void mtt::MetadataDownload::connectionClosed(PeerCommunication* p)
{
	if (activeComm.get() == p)
	{
		connectNext();
	}
}

void mtt::MetadataDownload::messageReceived(PeerCommunication*, PeerMessage&)
{
}

void mtt::MetadataDownload::metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message& msg)
{
	metadata.addPiece(msg.metadata, msg.piece);

	if (metadata.finished() && active)
	{
		onFinish(true);
	}
	else
	{
		requestPiece();
	}
}

void mtt::MetadataDownload::extHandshakeFinished(PeerCommunication*)
{
}

void mtt::MetadataDownload::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::MetadataDownload::progressUpdated(PeerCommunication*)
{
}

void mtt::MetadataDownload::requestPiece()
{
	if (active && activeComm)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		activeComm->ext.requestMetadataPiece(mdPiece);
	}
}

void mtt::MetadataDownload::connectNext()
{
	activeComm = nullptr;

	if (active)
	{
		core->peerMgr.connect(possibleAddrs.back());
		possibleAddrs.pop_back();
	}
}
