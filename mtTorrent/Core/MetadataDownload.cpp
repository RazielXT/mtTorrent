#include "MetadataDownload.h"
#include "MetadataReconstruction.h"
#include "Torrent.h"
#include "Configuration.h"
#include "utils/HexEncoding.h"
#include "MetadataExtension.h"

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
			const auto currentTime = mtt::CurrentTimestamp();
			if (state.active && !state.finished && lastActivityTime + 5 < currentTime)
			{
				std::lock_guard<std::mutex> guard(commsMutex);
				for (const auto& a : activeComms)
				{
					requestPiece(a);
				}

				if (retryTimer)
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

	if (!state.finished && state.active)
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

	if (startIndex >= eventLog.size())
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

	addEventLog(peer->info.id, EventInfo::Connected, 0);
	activeComms.push_back(peer);
	requestPiece(peer);
}

void mtt::MetadataDownload::handshakeFinished(PeerCommunication* p)
{
	if (!p->info.supportsExtensions())
	{
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

void mtt::MetadataDownload::extendedMessageReceived(PeerCommunication* p, ext::Type t, const BufferView& data)
{
	if (!state.active)
		return;

	if (t == ext::Type::UtMetadata)
	{
		ext::UtMetadata::Message msg;
		if (!ext::UtMetadata::Load(data, msg))
			return;

		if (msg.type == ext::UtMetadata::Type::Data)
		{
			std::lock_guard<std::mutex> guard(commsMutex);
			addEventLog(p->info.id, EventInfo::Receive, msg.piece);

			if (metadata.buffer.size() != msg.totalSize)
			{
				BT_UTM_LOG("different total size " << msg.totalSize);
				return;
			}

			if (metadata.addPiece(msg.metadata, msg.piece))
			{
				state.partsCount = metadata.pieces;
				state.receivedParts++;
			}

			if (metadata.finished())
			{
				state.finished = true;
			}
			else
			{
				requestPiece(p->shared_from_this());
			}
		}
		else if (msg.type == ext::UtMetadata::Type::Reject)
		{
			addEventLog(p->info.id, EventInfo::Reject, msg.piece);
			peers.disconnect(p);
			return;
		}

		if (state.finished)
		{
			service.io.post([this]() { stop(); });
		}

		onUpdate(Status::Success, state);
	}
}

void mtt::MetadataDownload::extendedHandshakeFinished(PeerCommunication* peer, ext::Handshake& handshake)
{
	if (handshake.metadataSize && peer->ext.utm.enabled())
	{
		{
			std::lock_guard<std::mutex> guard(commsMutex);
			if (metadata.pieces == 0)
				metadata.init(handshake.metadataSize);
		}

		addToBackup(peer->shared_from_this());
	}
	else
	{
		BT_UTM_LOG("no UtMetadataEx support " << peer->getStream()->getAddress());
		peers.disconnect(peer);
	}
}

void mtt::MetadataDownload::requestPiece(std::shared_ptr<PeerCommunication> peer)
{
	if (state.active && !state.finished)
	{
		uint32_t mdPiece = metadata.getMissingPieceIndex();
		peer->ext.utm.sendPieceRequest(mdPiece);
		addEventLog(peer->info.id, EventInfo::Request, mdPiece);
		onUpdate(Status::I_Requesting, state);
		lastActivityTime = mtt::CurrentTimestamp();
	}
}

void mtt::MetadataDownload::addEventLog(const uint8_t* id, EventInfo::Action action, uint32_t index)
{
	if (!eventLog.empty() && action == EventInfo::Searching && eventLog.back().action == action && eventLog.back().index == index)
		return;

	EventInfo info;
	info.action = action;
	info.index = index;
	if (id)
		memcpy(info.sourceId, id, 20);

	eventLog.emplace_back(info);
	BT_UTM_LOG(info.toString());
}

std::string mtt::MetadataDownload::EventInfo::toString()
{
	if (action == mtt::MetadataDownload::EventInfo::Connected)
		return hexToString(sourceId, 20) + " connected";
	if (action == mtt::MetadataDownload::EventInfo::Disconnected)
		return hexToString(sourceId, 20) + " disconnected";
	if (action == mtt::MetadataDownload::EventInfo::End)
		return "Finished";
	if (action == mtt::MetadataDownload::EventInfo::Searching)
		return "Searching for peers, current count " + std::to_string(index);
	if (action == mtt::MetadataDownload::EventInfo::Request)
		return hexToString(sourceId, 20) + " requesting " + std::to_string(index);
	if (action == mtt::MetadataDownload::EventInfo::Receive)
		return hexToString(sourceId, 20) + " sent " + std::to_string(index);
	if (action == mtt::MetadataDownload::EventInfo::Reject)
		return hexToString(sourceId, 20) + " rejected index " + std::to_string(index);

	return "";
}
