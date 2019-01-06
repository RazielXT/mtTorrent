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

	//peers.trackers.removeTrackers();

	peers.start([this](Status s, mtt::PeerSource source)
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		if (s == Status::Success && active && activeComms.empty())
		{
			evalComms();
		}
	}
	, this);

	//peers.connect(Addr({ 127,0,0,1 }, 31132));
}

void mtt::MetadataDownload::stop()
{
	if(!state.finished && active)
		onUpdate(Status::I_Stopped, state);

	active = false;
	activeComms.clear();
	peers.stop();
}

void mtt::MetadataDownload::evalComms()
{
	if (activeComms.empty() && !state.finished)
	{
		peers.connectNext(10);
		BT_UTM_LOG("searching for more peers");
	}
}

void mtt::MetadataDownload::removeBackup(PeerCommunication* p)
{
	for (auto it = activeComms.begin(); it != activeComms.end(); it++)
	{
		if (it->get() == p)
		{
			activeComms.erase(it);
			break;
		}
	}

	evalComms();
}

void mtt::MetadataDownload::addToBackup(std::shared_ptr<PeerCommunication> peer)
{
	std::lock_guard<std::mutex> guard(commsMutex);

	if (peer->ext.utm.size)
	{
		if (metadata.pieces == 0)
			metadata.init(peer->ext.utm.size);
		
		activeComms.push_back(peer);
		requestPiece(peer);
	}
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication* p)
{
	if (!p->info.supportsExtensions())
	{
		peers.disconnect(p);
		BT_UTM_LOG("unwanted");
	}
}

void mtt::MetadataDownload::connectionClosed(PeerCommunication* p, int)
{
	std::lock_guard<std::mutex> guard(commsMutex);

	BT_UTM_LOG("source disconnected");
	//memcpy(state.source, primaryComm->info.id, 20);
	onUpdate(Status::E_ConnectionClosed, state);
	removeBackup(p);
}

void mtt::MetadataDownload::messageReceived(PeerCommunication*, PeerMessage&)
{
}

void mtt::MetadataDownload::metadataPieceReceived(PeerCommunication* p, ext::UtMetadata::Message& msg)
{
	std::lock_guard<std::mutex> guard(commsMutex);
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
		auto peer = peers.getPeer(p);

		if (peer)
		{
			requestPiece(peer);
		}
	}

	onUpdate(Status::Success, state);

	if(state.finished)
		stop();
}

void mtt::MetadataDownload::extHandshakeFinished(PeerCommunication* peer)
{
	if (peer->ext.isSupported(ext::MessageType::UtMetadataEx))
		addToBackup(peers.getPeer(peer));
	else
	{
		BT_UTM_LOG("unwanted");
		peers.disconnect(peer);
	}
}

void mtt::MetadataDownload::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::MetadataDownload::progressUpdated(PeerCommunication*)
{
}

void mtt::MetadataDownload::requestPiece(std::shared_ptr<PeerCommunication> peer)
{
	if (active && !state.finished)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.requestMetadataPiece(mdPiece);
		memcpy(state.source, peer->info.id, 20);
		BT_UTM_LOG("requesting piece idx " << mdPiece);
		onUpdate(Status::I_Requesting, state);
	}
}
