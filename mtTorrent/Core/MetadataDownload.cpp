#include "MetadataDownload.h"
#include "MetadataReconstruction.h"
#include "Torrent.h"

#define BT_UTM_LOG(x) WRITE_LOG(LogTypeBtUtm, x)

mtt::MetadataDownload::MetadataDownload(Peers& p) : peers(p)
{

}

void mtt::MetadataDownload::start(std::function<void(Status, MetadataDownloadState&)> f)
{
	onUpdate = f;
	active = true;

	peers.start([this](Status s, const std::string& source)
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		if (s == Status::Success && active && !primaryComm)
		{
			switchPrimaryComm();
		}
	}
	, this);
}

void mtt::MetadataDownload::stop()
{
	if(!state.finished && active)
		onUpdate(Status::I_Stopped, state);

	active = false;
	primaryComm = nullptr;
	peers.stop();
}

void mtt::MetadataDownload::switchPrimaryComm()
{
	if (backupComm.empty())
	{
		peers.connectNext(10);
		BT_UTM_LOG("searching for peers");
	}
	else
	{
		primaryComm = backupComm.back();
		BT_UTM_LOG("set primary source as " << primaryComm->getAddressName());
		memcpy(state.source, primaryComm->info.id, 20);
		backupComm.pop_back();

		if (metadata.buffer.empty())
		{
			metadata.init(primaryComm->ext.utm.size);
			BT_UTM_LOG("needed pieces: " << metadata.pieces);
		}

		requestPiece();
	}
}

void mtt::MetadataDownload::removeBackup(PeerCommunication* p)
{
	std::lock_guard<std::mutex> guard(commsMutex);
	for (auto it = backupComm.begin(); it != backupComm.end(); it++)
	{
		if (*it == p)
		{
			backupComm.erase(it);
			break;
		}
	}

	if (backupComm.empty())
		peers.connectNext(10);
}

void mtt::MetadataDownload::addToBackup(PeerCommunication* peer)
{
	std::lock_guard<std::mutex> guard(commsMutex);

	if(peer->ext.utm.size)
		backupComm.push_back(peer);

	if (!primaryComm)
		switchPrimaryComm();
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication* p)
{
}

void mtt::MetadataDownload::connectionClosed(PeerCommunication* p, int)
{
	if (primaryComm == p)
	{
		BT_UTM_LOG("primary source disconnected");
		onUpdate(Status::E_ConnectionClosed, state);
		switchPrimaryComm();
	}
	else
		removeBackup(p);
}

void mtt::MetadataDownload::messageReceived(PeerCommunication*, PeerMessage&)
{
}

void mtt::MetadataDownload::metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message& msg)
{
	BT_UTM_LOG("received piece idx " << msg.piece);
	metadata.addPiece(msg.metadata, msg.piece);
	state.partsCount = metadata.pieces;
	state.receivedParts++;

	if (metadata.finished() && active)
	{
		state.finished = true;
	}
	else
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		requestPiece();
	}

	onUpdate(Status::Success, state);

	if(state.finished)
		stop();
}

void mtt::MetadataDownload::extHandshakeFinished(PeerCommunication* peer)
{
	addToBackup(peer);
}

void mtt::MetadataDownload::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::MetadataDownload::progressUpdated(PeerCommunication*)
{
}

void mtt::MetadataDownload::requestPiece()
{
	if (active && primaryComm)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		primaryComm->ext.requestMetadataPiece(mdPiece);
		BT_UTM_LOG("requesting piece idx " << mdPiece);
		onUpdate(Status::I_Requesting, state);
	}
}
