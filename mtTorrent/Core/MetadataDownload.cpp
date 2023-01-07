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

void mtt::MetadataDownload::start(std::function<void(const Event&, MetadataReconstruction&)> f)
{
	onUpdate = f;
	state.active = true;
	addEventLog(nullptr, Event::Searching);

	//peers.trackers.removeTrackers();

	peers.start(this);
	peers.startConnecting();
	//peers.connect(Addr({ 127,0,0,1 }, 31313));

	std::lock_guard<std::mutex> guard(stateMutex);
	retryTimer = ScheduledTimer::create(service.io, [this]()
		{
			const auto currentTime = mtt::CurrentTimestamp();
			if (state.active && !state.finished && lastActivityTime + 5 < currentTime)
			{
				std::lock_guard<std::mutex> guard(stateMutex);
				for (const auto& a : activeComms)
				{
					requestPiece(a);
				}
			}
			return ScheduledTimer::DurationSeconds(5);
		}
	);

	retryTimer->schedule(ScheduledTimer::DurationSeconds(5));
}

void mtt::MetadataDownload::stop()
{
	if (retryTimer)
		retryTimer->disable();
	retryTimer = nullptr;

	state.active = false;
	{
		std::lock_guard<std::mutex> guard(stateMutex);

		addEventLog(nullptr, Event::End);
		activeComms.clear();
	}

	peers.stop();
}

std::vector<mtt::MetadataDownload::Event> mtt::MetadataDownload::getEvents(size_t startIndex) const
{
	std::lock_guard<std::mutex> guard(stateMutex);

	if (startIndex >= eventLog.size())
		return {};

	return { eventLog.begin() + startIndex, eventLog.end() };
}

size_t mtt::MetadataDownload::getEventsCount() const
{
	return eventLog.size();
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
	std::lock_guard<std::mutex> guard(stateMutex);

	for (auto it = activeComms.begin(); it != activeComms.end(); it++)
	{
		if (it->get() == p)
		{
			activeComms.erase(it);
			addEventLog(p->info.id, Event::Disconnected);
			break;
		}
	}
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
			std::lock_guard<std::mutex> guard(stateMutex);

			addEventLog(p->info.id, Event::Receive, msg.piece);

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
				if (!state.finished)
				{
					state.finished = true;
					service.io.post([this]() { stop(); });
				}
			}
			else
			{
				requestPiece(p->shared_from_this());
			}
		}
		else if (msg.type == ext::UtMetadata::Type::Reject)
		{
			peers.disconnect(p);

			std::lock_guard<std::mutex> guard(stateMutex);

			addEventLog(p->info.id, Event::Reject, msg.piece);
		}
	}
}

void mtt::MetadataDownload::extendedHandshakeFinished(PeerCommunication* peer, const ext::Handshake& handshake)
{
	if (handshake.metadataSize && peer->ext.utm.enabled())
	{
		std::lock_guard<std::mutex> guard(stateMutex);

		addEventLog(peer->info.id, Event::Connected);

		if (metadata.pieces == 0)
			metadata.init(handshake.metadataSize);

		activeComms.push_back(peer->shared_from_this());
		requestPiece(peer->shared_from_this());
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
		lastActivityTime = mtt::CurrentTimestamp();

		addEventLog(peer->info.id, Event::Request, mdPiece);
	}
}

void mtt::MetadataDownload::addEventLog(const uint8_t* id, Event::Type e, uint32_t index)
{
	Event info;
	info.type = e;
	info.index = index;
	if (id)
		memcpy(info.sourceId, id, 20);

	eventLog.emplace_back(info);
	BT_UTM_LOG(info.toString());

	onUpdate(info, metadata);
}

std::string mtt::MetadataDownload::Event::toString() const
{
	if (type == mtt::MetadataDownload::Event::Connected)
		return hexToString(sourceId, 20) + " connected";
	if (type == mtt::MetadataDownload::Event::Disconnected)
		return hexToString(sourceId, 20) + " disconnected";
	if (type == mtt::MetadataDownload::Event::End)
		return "Stopped";
	if (type == mtt::MetadataDownload::Event::Searching)
		return "Searching for peers";
	if (type == mtt::MetadataDownload::Event::Request)
		return hexToString(sourceId, 20) + " requesting " + std::to_string(index);
	if (type == mtt::MetadataDownload::Event::Receive)
		return hexToString(sourceId, 20) + " sent " + std::to_string(index);
	if (type == mtt::MetadataDownload::Event::Reject)
		return hexToString(sourceId, 20) + " rejected index " + std::to_string(index);

	return "";
}
