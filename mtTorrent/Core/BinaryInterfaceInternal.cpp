#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Dht/Communication.h"
#include "Configuration.h"
#include "Public/BinaryInterface.h"
#include "FileTransfer.h"
#include "utils/HexEncoding.h"

mtt::Core core;

extern "C"
{
	__declspec(dllexport) mtt::Status __cdecl Ioctl(mtBI::MessageId id, const void* request, void* output)
	{
		if (id == mtBI::MessageId::Init)
			core.init();
		else if (id == mtBI::MessageId::AddFromFile)
		{
			auto t = core.addFile((const char*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			memcpy(output, t->hash(), 20);
		}
		else if (id == mtBI::MessageId::AddFromMetadata)
		{
			auto t = core.addMagnet((const char*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			memcpy(output, t->hash(), 20);
		}
		else if (id == mtBI::MessageId::Start)
		{
			auto t = core.getTorrent((const uint8_t*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->start();
			//t->peers->connect(Addr({ 127,0,0,1 }, 31132));
		}
		else if (id == mtBI::MessageId::Stop)
		{
			auto t = core.getTorrent((const uint8_t*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->stop();
		}
		else if (id == mtBI::MessageId::GetTorrents)
		{
			auto resp = (mtBI::TorrentsList*) output;
			resp->count = (uint32_t)core.torrents.size();
			if (resp->count == resp->list.size())
			{
				for (size_t i = 0; i < resp->count; i++)
				{
					auto t = core.torrents[i];
					memcpy(resp->list[i].hash, t->hash(), 20);
					resp->list[i].active = (t->state != mtt::Torrent::State::Stopped);
				}
			}
		}
		else if (id == mtBI::MessageId::GetTorrentStateInfo)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentStateInfo*) output;
			resp->name.set(torrent->name());
			resp->connectedPeers = torrent->peers->connectedCount();
			resp->checking = torrent->checking;
			if (resp->checking)
				resp->checkingProgress = torrent->checkingProgress();
			resp->foundPeers = torrent->peers->receivedCount();
			resp->downloaded = torrent->downloaded();
			resp->downloadSpeed = torrent->downloadSpeed();
			resp->uploaded = torrent->uploaded();
			resp->uploadSpeed = torrent->uploadSpeed();
			resp->progress = torrent->currentProgress();
			resp->activeStatus = torrent->lastError;
		}
		else if (id == mtBI::MessageId::GetPeersInfo)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentPeersInfo*) output;
			auto peers = torrent->fileTransfer->getPeersInfo();
			resp->count = (uint32_t)std::min(resp->peers.size(), peers.size());
			for (size_t i = 0; i < resp->count; i++)
			{
				auto& peer = peers[i];
				auto& out = resp->peers[i];
				out.addr.set(peer.address.toString());
				out.progress = peer.percentage;
				out.dlSpeed = peer.downloadSpeed;
				out.upSpeed = peer.uploadSpeed;
				out.client.set(peers[i].client);
			}
		}
		else if (id == mtBI::MessageId::GetTorrentInfo)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentInfo*) output;
			resp->name.set(torrent->infoFile.info.name);
			resp->fullsize = torrent->infoFile.info.fullSize;
			resp->filesCount = (uint32_t)torrent->infoFile.info.files.size();

			if (resp->filenames.size() == resp->filesCount)
			{
				for (size_t i = 0; i < resp->filenames.size(); i++)
				{
					resp->filenames[i].set(torrent->infoFile.info.files[i].path.back());
					resp->filesizes[i] = torrent->infoFile.info.files[i].size;
				}
			}
		}
		else if (id == mtBI::MessageId::GetSourcesInfo)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::SourcesInfo*) output;
			resp->count = torrent->peers->getSourcesCount();

			if (resp->count == resp->sources.size())
			{
				auto sources = torrent->peers->getSourcesInfo();
				if (sources.size() < resp->count)
					resp->count = (uint32_t)sources.size();

				uint32_t currentTime = (uint32_t)time(0);

				for (uint32_t i = 0; i < resp->count; i++)
				{
					auto& to = resp->sources[i];
					auto& from = sources[i];

					to.name.set(from.hostname);
					to.peers = from.peers;
					to.seeds = from.seeds;
					to.interval = from.announceInterval;
					to.nextCheck = from.nextAnnounce < currentTime ? 0 : from.nextAnnounce - currentTime;

					if (from.state == mtt::TrackerState::Connected)
						memcpy(to.status, "Ready", 6);
					else if (from.state == mtt::TrackerState::Connecting)
						memcpy(to.status, "Connecting", 11);
					else if (from.state == mtt::TrackerState::Announcing || from.state == mtt::TrackerState::Reannouncing)
						memcpy(to.status, "Announcing", 11);
					else if (from.state == mtt::TrackerState::Announced)
						memcpy(to.status, "Announced", 10);
					else
						to.status[0] = 0;
				}
			}
		}
		else if (id == mtBI::MessageId::GetMagnetLinkProgress)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent || !torrent->utmDl)
				return mtt::Status::E_InvalidInput;

			auto resp = (mtBI::MagnetLinkProgress*) output;
			resp->finished = torrent->utmDl->state.finished;
			resp->progress = torrent->utmDl->state.partsCount == 0 ? 0 : torrent->utmDl->state.receivedParts / (float)torrent->utmDl->state.partsCount;
		}
		else if (id == mtBI::MessageId::GetMagnetLinkProgressLogs)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent || !torrent->utmDl)
				return mtt::Status::E_InvalidInput;

			auto resp = (mtBI::MagnetLinkProgressLogs*) output;
			if (resp->count == 0)
				resp->count = torrent->utmDl->getEventsCount();
			else
			{
				auto events = torrent->utmDl->getEvents();
				if (events.size() >= (resp->start + resp->count))
					for (size_t i = resp->start; i < events.size(); i++)
					{
						std::string txt;
						if (events[i].action == mtt::MetadataDownload::EventInfo::Connected)
							txt = hexToString(events[i].sourceId, 20) + " connected";
						else if (events[i].action == mtt::MetadataDownload::EventInfo::Disconnected)
							txt = hexToString(events[i].sourceId, 20) + " disconnected";
						else if (events[i].action == mtt::MetadataDownload::EventInfo::End)
							txt = "Finished";
						else if (events[i].action == mtt::MetadataDownload::EventInfo::Searching)
							txt = "Searching for peers, current count " + std::to_string(events[i].index);
						else if (events[i].action == mtt::MetadataDownload::EventInfo::Request)
							txt = hexToString(events[i].sourceId, 20) + " reqesting " + std::to_string(events[i].index);
						else if (events[i].action == mtt::MetadataDownload::EventInfo::Receive)
							txt = hexToString(events[i].sourceId, 20) + " sent " + std::to_string(events[i].index);

						resp->logs[i - resp->start].set(txt);
					}
			}
		}
		else if (id == mtBI::MessageId::GetSettings)
		{
			auto resp = (mtBI::SettingsInfo*) output;
			auto& settings = mtt::config::external;
			resp->dhtEnabled = settings.enableDht;
			resp->directory.set(settings.defaultDirectory);
			resp->maxConnections = settings.maxTorrentConnections;
			resp->tcpPort = settings.tcpPort;
			resp->udpPort = settings.udpPort;
		}
		else if (id == mtBI::MessageId::SetSettings)
		{
			auto info = (mtBI::SettingsInfo*) request;
			auto& settings = mtt::config::external;

			if (settings.enableDht != info->dhtEnabled)
			{
				settings.enableDht = info->dhtEnabled;

				if (settings.enableDht)
					core.dht->start();
				else
					core.dht->stop();
			}

			settings.defaultDirectory = info->directory.data;
			settings.maxTorrentConnections = info->maxConnections;
			settings.tcpPort = info->tcpPort;
			settings.udpPort = info->udpPort;
		}
		else if (id == mtBI::MessageId::GetTorrentFilesSelection)
		{
			auto torrent = core.getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			auto & files = torrent->files.selection.files;

			auto resp = (mtBI::TorrentFilesSelection*) output;
			if (resp->count == 0)
				resp->count = (uint32_t)files.size();
			else if (resp->selection.size() == files.size())
			{
				for (size_t i = 0; i < resp->selection.size(); i++)
				{
					mtBI::FileSelection& f = resp->selection[i];
					f.name.set(files[i].info.path.back());
					f.selected = files[i].selected;
					f.size = files[i].info.size;
				}
			}
		}
		else if (id == mtBI::MessageId::SetTorrentFilesSelection)
		{
			auto selection = (mtBI::TorrentFilesSelectionRequest*)request;
			auto torrent = core.getTorrent(selection->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			if (!torrent->files.selection.files.size() != selection->selection.size())
				return mtt::Status::E_InvalidInput;

			std::vector<bool> dlSelect;
			for (auto& f : selection->selection)
			{
				dlSelect.push_back(f.selected);
			}

			if (!torrent->selectFiles(dlSelect))
				return mtt::Status::E_InvalidInput;
		}
		else if (id == mtBI::MessageId::RefreshSource)
		{
			auto info = (mtBI::SourceId*) request;

			auto torrent = core.getTorrent(info->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			torrent->peers->refreshSource(info->name.data);
		}
		else
			return mtt::Status::E_InvalidInput;

		return mtt::Status::Success;
	}
}
