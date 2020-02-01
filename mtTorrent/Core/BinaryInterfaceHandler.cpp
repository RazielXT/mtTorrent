#include "Api/Core.h"
#include "Api/Configuration.h"
#include "Public/BinaryInterface.h"
#include "utils/HexEncoding.h"
#include <algorithm>
#include <time.h>

extern std::shared_ptr<mttApi::Core> core;

extern "C"
{
	__declspec(dllexport) mtt::Status __cdecl Ioctl(mtBI::MessageId id, const void* request, void* output)
	{
		if (id == mtBI::MessageId::Init)
			core = mttApi::Core::create();
		else if (id == mtBI::MessageId::Deinit)
			core.reset();
		else if (id == mtBI::MessageId::AddFromFile)
		{
			auto[status, t] = core->addFile((const char*)request);

			if (t)
				memcpy(output, t->getFileInfo().info.hash, 20);

			return status;
		}
		else if (id == mtBI::MessageId::AddFromMetadata)
		{
			auto [status, t] = core->addMagnet((const char*)request);

			if (t)
				memcpy(output, t->getFileInfo().info.hash, 20);

			return status;
		}
		else if (id == mtBI::MessageId::Start)
		{
			auto t = core->getTorrent((const uint8_t*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->start();
		}
		else if (id == mtBI::MessageId::Stop)
		{
			auto t = core->getTorrent((const uint8_t*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->stop();
		}
		else if (id == mtBI::MessageId::Remove)
		{
			auto info = (mtBI::RemoveTorrentRequest*) request;
			auto s = core->removeTorrent(info->hash, info->deleteFiles);

			if (s != mtt::Status::Success)
				return s;
		}
		else if (id == mtBI::MessageId::CheckFiles)
		{
			auto t = core->getTorrent((const uint8_t*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->checkFiles();
		}
		else if (id == mtBI::MessageId::GetTorrents)
		{
			auto resp = (mtBI::TorrentsList*) output;
			auto torrents = core->getTorrents();
			resp->list.resize(torrents.size());

			for (size_t i = 0; i < torrents.size(); i++)
			{
				auto t = torrents[i];
				memcpy(resp->list[i].hash, t->getFileInfo().info.hash, 20);
				resp->list[i].active = (t->getStatus() != mttApi::Torrent::State::Stopped);
			}
		}
		else if (id == mtBI::MessageId::GetTorrentStateInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentStateInfo*) output;
			resp->name = torrent->name();
			resp->connectedPeers = torrent->getPeers()->connectedCount();
			resp->checking = torrent->getStatus() ==  mttApi::Torrent::State::Started && torrent->checkingProgress() < 1;
			if (resp->checking)
				resp->checkingProgress = torrent->checkingProgress();
			resp->foundPeers = torrent->getPeers()->receivedCount();
			resp->downloaded = torrent->downloaded();
			resp->uploaded = torrent->uploaded();
			resp->downloadSpeed = torrent->getFileTransfer() ? torrent->getFileTransfer()->getDownloadSpeed() : 0;
			resp->uploadSpeed = torrent->getFileTransfer() ? torrent->getFileTransfer()->getUploadSpeed() : 0;
			resp->progress = torrent->currentProgress();
			resp->selectionProgress = torrent->currentSelectionProgress();
			resp->activeStatus = torrent->getLastError();
			resp->started = torrent->getStatus() == mttApi::Torrent::State::Started;

			mtt::MetadataDownloadState utm;
			resp->utmActive = torrent->getMetadataDownloadState(utm) && !utm.finished;
		}
		else if (id == mtBI::MessageId::GetPeersInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentPeersInfo*) output;

			if (auto transfer = torrent->getFileTransfer())
			{
				auto peers = transfer->getPeersInfo();
				resp->peers.resize(peers.size());

				for (size_t i = 0; i < peers.size(); i++)
				{
					auto& peer = peers[i];
					auto& out = resp->peers[i];
					out.addr = peer.address;
					out.progress = peer.percentage;
					out.dlSpeed = peer.downloadSpeed;
					out.upSpeed = peer.uploadSpeed;
					out.client = peer.client;
					out.country = peer.country;
				}
			}
		}
		else if (id == mtBI::MessageId::GetTorrentInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::TorrentInfo*) output;
			resp->name = torrent->getFileInfo().info.name;
			resp->fullsize = torrent->getFileInfo().info.fullSize;

			auto selection = torrent->getFilesSelection();
			resp->files.resize(selection.files.size());

			for (size_t i = 0; i < selection.files.size(); i++)
			{
				resp->files[i].name = selection.files[i].info.path.back();
				resp->files[i].size = selection.files[i].info.size;
				resp->files[i].selected = selection.files[i].selected;
			}

			resp->downloadLocation = torrent->getLocationPath();
		}
		else if (id == mtBI::MessageId::GetSourcesInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::SourcesInfo*) output;
			auto sources = torrent->getPeers()->getSourcesInfo();
			resp->sources.resize(sources.size());
			uint32_t currentTime = (uint32_t)time(0);

			for (size_t i = 0; i < sources.size(); i++)
			{
				auto& to = resp->sources[i];
				auto& from = sources[i];

				to.name = from.hostname;
				to.peers = from.peers;
				to.seeds = from.seeds;
				to.leechers = from.leechers;
				to.interval = from.announceInterval;
				to.nextCheck = from.nextAnnounce < currentTime ? 0 : from.nextAnnounce - currentTime;

				if (from.state == mtt::TrackerState::Connected || from.state == mtt::TrackerState::Alive)
					to.status = mtBI::SourceInfo::Ready;
				else if (from.state == mtt::TrackerState::Connecting)
					to.status = mtBI::SourceInfo::Connecting;
				else if (from.state == mtt::TrackerState::Announcing || from.state == mtt::TrackerState::Reannouncing)
					to.status = mtBI::SourceInfo::Announcing;
				else if (from.state == mtt::TrackerState::Announced)
					to.status = mtBI::SourceInfo::Announced;
				else if (from.state == mtt::TrackerState::Offline)
					to.status = mtBI::SourceInfo::Offline;
				else
					to.status = mtBI::SourceInfo::Stopped;
			}
		}
		else if (id == mtBI::MessageId::GetPiecesInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;	

			auto resp = (mtBI::PiecesInfo*) output;
			resp->piecesCount = (uint32_t)torrent->getFileInfo().info.pieces.size();
			size_t receivedCount = 0;
			torrent->getReceivedPieces(nullptr, receivedCount);
			resp->receivedCount = (uint32_t)receivedCount;

			resp->bitfield.resize(torrent->getFileInfo().info.expectedBitfieldSize);
			torrent->getPiecesBitfield(resp->bitfield.data(), resp->bitfield.size());

			if (torrent->getFileTransfer())
			{
				auto requests = torrent->getFileTransfer()->getCurrentRequests();
				resp->requests.assign(requests.data(), requests.size());
			}
		}
		else if (id == mtBI::MessageId::GetMagnetLinkProgress)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			mtt::MetadataDownloadState utm;
			if (!torrent->getMetadataDownloadState(utm))
				return mtt::Status::E_NoData;

			auto resp = (mtBI::MagnetLinkProgress*) output;
			resp->finished = utm.finished;
			resp->progress = utm.partsCount == 0 ? 0 : utm.receivedParts / (float)utm.partsCount;
		}
		else if (id == mtBI::MessageId::GetMagnetLinkProgressLogs)
		{
			auto requestInfo = (mtBI::MagnetLinkProgressLogsRequest*)request;
			auto torrent = core->getTorrent(requestInfo->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			auto resp = (mtBI::MagnetLinkProgressLogsResponse*) output;
			resp->fullcount = (uint32_t)torrent->getMetadataDownloadLogSize();

			if (resp->fullcount)
			{
				std::vector<std::string> logs;
				if (!torrent->getMetadataDownloadLog(logs, requestInfo->start))
					return mtt::Status::E_NoData;

				resp->logs.resize(logs.size());
				for (size_t i = 0; i < logs.size(); i++)
				{
					resp->logs[i] = logs[i];
				}
			}
		}
		else if (id == mtBI::MessageId::GetSettings)
		{
			auto resp = (mtBI::SettingsInfo*) output;
			auto& settings = mtt::config::getExternal();
			resp->dhtEnabled = settings.dht.enable;
			resp->directory = settings.files.defaultDirectory;
			resp->maxConnections = settings.connection.maxTorrentConnections;
			resp->tcpPort = settings.connection.tcpPort;
			resp->udpPort = settings.connection.udpPort;
			resp->upnpEnabled = settings.connection.upnpPortMapping;
		}
		else if (id == mtBI::MessageId::SetSettings)
		{
			auto info = (mtBI::SettingsInfo*) request;
			auto settings = mtt::config::getExternal();

			settings.dht.enable = info->dhtEnabled;
			settings.files.defaultDirectory = info->directory.data;
			settings.connection.maxTorrentConnections = info->maxConnections;
			settings.connection.tcpPort = info->tcpPort;
			settings.connection.udpPort = info->udpPort;
			settings.connection.upnpPortMapping = info->upnpEnabled;

			mtt::config::setValues(settings.dht);
			mtt::config::setValues(settings.connection);
			mtt::config::setValues(settings.files);
		}
		else if (id == mtBI::MessageId::SetTorrentFilesSelection)
		{
			auto selection = (mtBI::TorrentFilesSelectionRequest*)request;
			auto torrent = core->getTorrent(selection->hash);
			if (!torrent)
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

			auto torrent = core->getTorrent(info->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			torrent->getPeers()->refreshSource(info->name.data);
		}
		else if (id == mtBI::MessageId::AddPeer)
		{
			auto info = (mtBI::AddPeerRequest*) request;

			auto torrent = core->getTorrent(info->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			torrent->getPeers()->connect(info->addr.data);
		}
		else if (id == mtBI::MessageId::GetUpnpInfo)
		{
			auto info = (mtt::string*) output;
			*info = core->getListener()->getUpnpReadableInfo();
		}
		else if (id == mtBI::MessageId::SetTorrentPath)
		{
			auto info = (mtBI::TorrentSetPathRequest*) request;

			auto torrent = core->getTorrent(info->hash);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			return torrent->setLocationPath(info->path.data);
		}
		else if (id == mtBI::MessageId::RegisterAlerts)
		{
			auto info = (mtBI::RegisterAlertsRequest*) request;
			core->registerAlerts(info->categoryMask);
		}
		else if (id == mtBI::MessageId::PopAlerts)
		{
			auto out = (mtBI::AlertsList*) output;
			auto alerts = core->popAlerts();
			
			if (!alerts.empty())
			{
				out->alerts.resize(alerts.size());

				for (size_t i = 0; i < alerts.size(); i++)
				{
					out->alerts[i].id = alerts[i]->id;

					if (auto t = alerts[i]->getAs<mtt::TorrentAlert>())
						memcpy(out->alerts[i].hash, t->hash, 20);
					if (auto t = alerts[i]->getAs<mtt::MetadataAlert>())
						memcpy(out->alerts[i].hash, t->hash, 20);
				}
			}
		}
		else
			return mtt::Status::E_InvalidInput;

		return mtt::Status::Success;
	}
}
