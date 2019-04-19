#include "MetadataDownload.h"
#include "MetadataReconstruction.h"
#include "Torrent.h"
#include "Configuration.h"

#define BT_UTM_LOG(x) WRITE_LOG(LogTypeBtUtm, x)

mtt::MetadataDownload::MetadataDownload(Peers& p) : peers(p)
{

}

void mtt::MetadataDownload::start(std::function<void(Status, MetadataDownloadState&)> f)
{
	log.logName = "utm_" + std::to_string(rand());
	log.clear();

	onUpdate = f;
	active = true;

	//peers.trackers.removeTrackers();

	peers.start([this](Status s, mtt::PeerSource source)
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		if (s == Status::Success && active)
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
	if (activeComms.size() < mtt::config::external.maxTorrentConnections && !state.finished)
	{
		LOG_APPEND("request more peers, current " << activeComms.size());
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
			LOG_APPEND("removing backup " + p->getAddressName());
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
	
		LOG_APPEND("adding backup " + peer->getAddressName());
		activeComms.push_back(peer);
		requestPiece(peer);
	}
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication* p)
{
	LOG_APPEND("handshake finished " + p->getAddressName());

	if (!p->info.supportsExtensions())
	{
		BT_UTM_LOG("unwanted");
		LOG_APPEND("unwanted " + p->getAddressName());

		peers.disconnect(p);
	}
}

void mtt::MetadataDownload::connectionClosed(PeerCommunication* p, int)
{
	std::lock_guard<std::mutex> guard(commsMutex);

	BT_UTM_LOG("source disconnected");
	//memcpy(state.source, primaryComm->info.id, 20);
	onUpdate(Status::E_ConnectionClosed, state);

	LOG_APPEND("closed " + p->getAddressName());
	removeBackup(p);
}

void mtt::MetadataDownload::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
	LOG_APPEND("msg " << msg.id << " " << p->getAddressName());
}

void mtt::MetadataDownload::metadataPieceReceived(PeerCommunication* p, ext::UtMetadata::Message& msg)
{
	LOG_APPEND("received piece idx " << msg.piece << " " << p->getAddressName());

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
	LOG_APPEND("ext handshake finished " + peer->getAddressName());

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
		LOG_APPEND("requesting piece idx " << mdPiece << " " + peer->getAddressName());
		onUpdate(Status::I_Requesting, state);
	}
}
