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
			auto t = core->addFile((const char*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			memcpy(output, t->getFileInfo().info.hash, 20);
		}
		else if (id == mtBI::MessageId::AddFromMetadata)
		{
			auto t = core->addMagnet((const char*)request);

			if (!t)
				return mtt::Status::E_InvalidInput;

			memcpy(output, t->getFileInfo().info.hash, 20);
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
		else if (id == mtBI::MessageId::GetTorrents)
		{
			auto resp = (mtBI::TorrentsList*) output;
			auto torrents = core->getTorrents();
			resp->count = (uint32_t)torrents.size();
			if (resp->count == resp->list.size())
			{
				for (size_t i = 0; i < resp->count; i++)
				{
					auto t = torrents[i];
					memcpy(resp->list[i].hash, t->getFileInfo().info.hash, 20);
					resp->list[i].active = (t->getStatus() != mttApi::Torrent::State::Stopped);
				}
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
				resp->count = (uint32_t)std::min(resp->peers.size(), peers.size());
				for (size_t i = 0; i < resp->count; i++)
				{
					auto& peer = peers[i];
					auto& out = resp->peers[i];
					out.addr = peer.address;
					out.progress = peer.percentage;
					out.dlSpeed = peer.downloadSpeed;
					out.upSpeed = peer.uploadSpeed;
					out.client = peers[i].client;
					out.country = peers[i].country;
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
			resp->filesCount = (uint32_t)torrent->getFileInfo().info.files.size();

			if (resp->filenames.size() == resp->filesCount)
			{
				for (size_t i = 0; i < resp->filenames.size(); i++)
				{
					resp->filenames[i] = torrent->getFileInfo().info.files[i].path.back();
					resp->filesizes[i] = torrent->getFileInfo().info.files[i].size;
				}
			}

			resp->downloadLocation = torrent->getLocationPath();
		}
		else if (id == mtBI::MessageId::GetSourcesInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;
			auto resp = (mtBI::SourcesInfo*) output;
			resp->count = torrent->getPeers()->getSourcesCount();

			if (resp->count == resp->sources.size())
			{
				auto sources = torrent->getPeers()->getSourcesInfo();
				if (sources.size() < resp->count)
					resp->count = (uint32_t)sources.size();

				uint32_t currentTime = (uint32_t)time(0);

				for (uint32_t i = 0; i < resp->count; i++)
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
		}
		else if (id == mtBI::MessageId::GetPiecesInfo)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;	

			auto resp = (mtBI::PiecesInfo*) output;
			resp->piecesCount = (uint32_t)torrent->getFileInfo().info.pieces.size();
			resp->bitfieldSize = (uint32_t)torrent->getFileInfo().info.expectedBitfieldSize;
			resp->requestSize = (uint32_t)(torrent->getFileTransfer() ? torrent->getFileTransfer()->getCurrentRequestsCount() : 0);

			if(resp->bitfield.size() == resp->bitfieldSize)
				torrent->getPiecesBitfield(resp->bitfield);

			if (!resp->requests.empty())
			{
				auto requests = torrent->getFileTransfer()->getCurrentRequests();
				if (resp->requests.size() > requests.size())
					resp->requests.resize(requests.size());

				for (size_t i = 0; i < resp->requests.size(); i++)
				{
					resp->requests[i] = requests[i];
				}

				resp->requestSize = (uint32_t)resp->requests.size();
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
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			auto resp = (mtBI::MagnetLinkProgressLogs*) output;

			std::vector<std::string> logs;
			if (!torrent->getMetadataDownloadLog(logs, resp->start))
				return mtt::Status::E_NoData;

			if (resp->count == 0)
				resp->count = (uint32_t)logs.size();
			else
			{
				if (logs.size() >= (resp->start + resp->count))
					for (size_t i = resp->start; i < resp->start + resp->count; i++)
					{
						resp->logs[i - resp->start] = logs[i];
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
		else if (id == mtBI::MessageId::GetTorrentFilesSelection)
		{
			auto torrent = core->getTorrent((const uint8_t*)request);
			if (!torrent)
				return mtt::Status::E_InvalidInput;

			auto files = torrent->getFilesSelection().files;

			auto resp = (mtBI::TorrentFilesSelection*) output;
			if (resp->count == 0)
				resp->count = (uint32_t)files.size();
			else if (resp->selection.size() == files.size())
			{
				for (size_t i = 0; i < resp->selection.size(); i++)
				{
					mtBI::FileSelection& f = resp->selection[i];
					f.name = files[i].info.path.back();
					f.selected = files[i].selected;
					f.size = files[i].info.size;
					f.pieceStart = files[i].info.startPieceIndex;
					f.pieceEnd = files[i].info.endPieceIndex;
				}
			}
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
		else
			return mtt::Status::E_InvalidInput;

		return mtt::Status::Success;
	}
}
