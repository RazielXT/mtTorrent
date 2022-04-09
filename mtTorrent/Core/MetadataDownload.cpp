#include "MetadataDownload.h"
#include "MetadataReconstruction.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"

#define BT_UTM_LOG(x) WRITE_GLOBAL_LOG(MetadataDownload, x)

mtt::MetadataDownload::MetadataDownload(Peers& p, ServiceThreadpool& s) : peers(p), service(s)
{
}

void mtt::MetadataDownload::start(std::function<void(Status, MetadataDownloadState&)> f)
{
	onUpdate = f;
	state.active = true;

	//peers.trackers.removeTrackers();

	peers.start([this](Status s, mtt::PeerSource source)
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		if (s == Status::Success && state.active)
		{
			evalComms();
		}
	}
	, this);

	//peers.connect(Addr({ 127,0,0,1 }, 31132));

	std::lock_guard<std::mutex> guard(commsMutex);
	retryTimer = ScheduledTimer::create(service.io, [this]()
		{
			auto currentTime = (uint32_t) time(0);
			if (state.active && !state.finished && lastActivityTime + 5 < currentTime)
			{
				std::lock_guard<std::mutex> guard(commsMutex);
				for (auto a : activeComms)
				{
					requestPiece(a);
				}

				if(retryTimer)
					retryTimer->schedule(5);
			}
		}
	);

	retryTimer->schedule(5);
}

void mtt::MetadataDownload::stop()
{
	if (retryTimer)
		retryTimer->disable();
	retryTimer = nullptr;

	addEventLog(nullptr, EventInfo::End, 0);

	if(!state.finished && state.active)
		onUpdate(Status::I_Stopped, state);

	state.active = false;
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		activeComms.clear();
	}

	peers.stop();
}

std::vector<mtt::MetadataDownload::EventInfo> mtt::MetadataDownload::getEvents(size_t startIndex) const
{
	std::lock_guard<std::mutex> guard(commsMutex);

	if(startIndex >= eventLog.size())
		return {};

	return { eventLog.begin() + startIndex, eventLog.end() };
}

size_t mtt::MetadataDownload::getEventsCount() const
{
	return eventLog.size();
}

void mtt::MetadataDownload::evalComms()
{
	if (activeComms.size() < mtt::config::getExternal().connection.maxTorrentConnections && !state.finished)
	{
		addEventLog(nullptr, EventInfo::Searching, (uint32_t)activeComms.size());
		peers.connectNext(mtt::config::getExternal().connection.maxTorrentConnections);
		BT_UTM_LOG("searching for more peers");
	}
}

void mtt::MetadataDownload::removeBackup(PeerCommunication* p)
{
	for (auto it = activeComms.begin(); it != activeComms.end(); it++)
	{
		if (it->get() == p)
		{
			addEventLog(p->info.id, EventInfo::Disconnected, 0);
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
	
		addEventLog(peer->info.id, EventInfo::Connected, 0);
		activeComms.push_back(peer);
		requestPiece(peer);
	}
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication* p)
{
	if (!p->info.supportsExtensions())
	{
		BT_UTM_LOG("not supportsExtensions");

		peers.disconnect(p);
	}
}

void mtt::MetadataDownload::connectionClosed(PeerCommunication* p, int)
{
	std::lock_guard<std::mutex> guard(commsMutex);

	onUpdate(Status::E_ConnectionClosed, state);

	removeBackup(p);
}

void mtt::MetadataDownload::messageReceived(PeerCommunication* p, PeerMessage& msg)
{
}

void mtt::MetadataDownload::metadataPieceReceived(PeerCommunication* p, ext::UtMetadata::Message& msg)
{
	{
		std::lock_guard<std::mutex> guard(commsMutex);
		addEventLog(p->info.id, EventInfo::Receive, msg.piece);
		BT_UTM_LOG("received piece idx " << msg.piece);
		metadata.addPiece(msg.metadata, msg.piece);
		state.partsCount = metadata.pieces;
		state.receivedParts++;

		if (metadata.finished() && state.active)
		{
			state.finished = true;
		}
		else
		{
			requestPiece(p->shared_from_this());
		}
	}

	if (state.finished)
	{
		service.io.post([this]() { stop(); });
	}

	onUpdate(Status::Success, state);
}

void mtt::MetadataDownload::extHandshakeFinished(PeerCommunication* peer)
{
	if (peer->ext.isSupported(ext::MessageType::UtMetadataEx))
		addToBackup(peer->shared_from_this());
	else
	{
		BT_UTM_LOG("no UtMetadataEx support " << peer->getStream()->getAddress());
		peers.disconnect(peer);
	}
}

void mtt::MetadataDownload::pexReceived(PeerCommunication*, ext::PeerExchange::Message&)
{
}

void mtt::MetadataDownload::progressUpdated(PeerCommunication*, uint32_t)
{
}

void mtt::MetadataDownload::requestPiece(std::shared_ptr<PeerCommunication> peer)
{
	if (state.active && !state.finished)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.requestMetadataPiece(mdPiece);
		BT_UTM_LOG("requesting piece idx " << mdPiece);
		addEventLog(peer->info.id, EventInfo::Request, mdPiece);
		onUpdate(Status::I_Requesting, state);
		lastActivityTime = (uint32_t)time(0);
	}
}

void mtt::MetadataDownload::addEventLog(uint8_t* id, EventInfo::Action action, uint32_t index)
{
	if (!eventLog.empty() && action == EventInfo::Searching && eventLog.back().action == action && eventLog.back().index == index)
		return;

	EventInfo info;
	info.action = action;
	info.index = index;
	if (id)
		memcpy(info.sourceId, id, 20);

	eventLog.emplace_back(info);
}

std::string mtt::MetadataDownload::EventInfo::toString()
{
	if (action == mtt::MetadataDownload::EventInfo::Connected)
		return hexToString(sourceId, 20) + " connected";
	else if (action == mtt::MetadataDownload::EventInfo::Disconnected)
		return hexToString(sourceId, 20) + " disconnected";
	else if (action == mtt::MetadataDownload::EventInfo::End)
		return "Finished";
	else if (action == mtt::MetadataDownload::EventInfo::Searching)
		return "Searching for peers, current count " + std::to_string(index);
	else if (action == mtt::MetadataDownload::EventInfo::Request)
		return hexToString(sourceId, 20) + " requesting " + std::to_string(index);
	else if (action == mtt::MetadataDownload::EventInfo::Receive)
		return hexToString(sourceId, 20) + " sent " + std::to_string(index);
	else
		return "";
}
