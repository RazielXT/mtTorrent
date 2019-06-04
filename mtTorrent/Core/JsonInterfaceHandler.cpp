#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Dht/Communication.h"
#include "Configuration.h"
#include "Public/JsonInterface.h"
#include "FileTransfer.h"
#include "utils/HexEncoding.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

extern mtt::Core core;

namespace js = rapidjson;

namespace mtt::config
{
	extern void fromInternalJson(rapidjson::Value& internalSettings);
}

extern "C"
{
	__declspec(dllexport) mtt::Status __cdecl JsonIoctl(mttJson::MessageId id, const char* request, mtt::string* output)
	{
		js::Document requestJs;

		if (request)
		{
			requestJs.Parse(request);

			if (!requestJs.IsObject())
				return mtt::Status::E_InvalidInput;
		}

		if (id == mttJson::MessageId::Init)
		{
			if (requestJs.IsObject())
				mtt::config::fromInternalJson(requestJs);

			core.init();
		}
		else if (id == mttJson::MessageId::Deinit)
			core.deinit();
		else if (id == mttJson::MessageId::Add)
		{
			mtt::TorrentPtr t;

			if(requestJs.HasMember("filepath"))
				t = core.addFile(requestJs["filepath"].GetString());
			else if (requestJs.HasMember("magnet"))
				t = core.addMagnet(requestJs["magnet"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			if (output)
			{
				auto response = "{ \"hash\" : \"" + t->hashString() + "\" }";
				*output = response;
			}
		}
		else if (id == mttJson::MessageId::Start)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->start();
		}
		else if (id == mttJson::MessageId::Stop)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->stop();
		}
		else if (id == mttJson::MessageId::Remove)
		{
			mtt::Status status = mtt::Status::E_InvalidInput;
			bool removeFiles = requestJs.HasMember("removeFiles") ? requestJs["removeFiles"].IsTrue() : false;

			if (requestJs.HasMember("hash"))
				status = core.removeTorrent(requestJs["hash"].GetString(), removeFiles);

			if (status != mtt::Status::Success)
				return status;
		}
		else if (id == mttJson::MessageId::GetTorrents)
		{
			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("list");
			writer.StartArray();
			for (size_t i = 0; i < core.torrents.size(); i++)
			{
				writer.StartObject();

				auto t = core.torrents[i];
				writer.Key("hash");
				writer.String(t->hashString().data());
				writer.Key("active");
				writer.Bool((t->state != mtt::Torrent::State::Stopped));

				writer.EndObject();
			}
			writer.EndArray();
			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetTorrentInfo)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("name");
			writer.String(t->infoFile.info.name.data(), (uint32_t)t->infoFile.info.name.length());
			writer.Key("size");
			writer.Uint64(t->infoFile.info.fullSize);

			writer.Key("files");
			writer.StartArray();
			for (auto& f : t->infoFile.info.files)
			{
				writer.StartObject();
				writer.Key("path");
				writer.StartArray();
				for (auto& p : f.path)
					writer.String(p.data(), (uint32_t)p.length());
				writer.EndArray();

				writer.Key("size");
				writer.Uint64(f.size);
				writer.EndObject();
			}
			writer.EndArray();

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetTorrentsState)
		{
			auto hashArray = requestJs.FindMember("hash");

			if (hashArray == requestJs.MemberEnd() || !hashArray->value.IsArray())
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("states");
			writer.StartArray();

			for (auto hashItem = hashArray->value.MemberBegin(); hashItem != hashArray->value.MemberEnd(); hashItem++)
			{
				mtt::TorrentPtr t = core.getTorrent(hashItem->name.GetString());

				if(!t)
					continue;

				writer.StartObject();
				writer.Key("hash");
				writer.String(hashItem->name.GetString());
				writer.Key("name");
				writer.String(t->infoFile.info.name.data(), (uint32_t)t->infoFile.info.name.length());
				writer.Key("connectedPeers");
				writer.Uint(t->peers->connectedCount());
				writer.Key("foundPeers");
				writer.Uint(t->peers->receivedCount());

				writer.Key("downloaded");
				writer.Uint64(t->downloaded());
				writer.Key("uploaded");
				writer.Uint64(t->uploaded());
				writer.Key("downloadSpeed");
				writer.Uint64(t->downloadSpeed());
				writer.Key("uploadSpeed");
				writer.Uint64(t->uploadSpeed());

				writer.Key("progress");
				writer.Double(t->currentProgress());
				writer.Key("selectionProgress");
				writer.Double(t->currentSelectionProgress());

				writer.Key("status");
				writer.Uint((uint32_t)t->lastError);

				if (t->utmDl && !t->utmDl->state.finished)
				{
					writer.Key("utmActive");
					writer.Bool(true);
				}

				if (t->checking)
				{
					writer.Key("checkProgress");
					writer.Double(t->checkingProgress());
				}

				writer.EndObject();
			}

			writer.EndArray();
			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetPeersInfo)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("progress");
			writer.StartArray();

			auto peers = t->fileTransfer->getPeersInfo();
			for (auto& peer : peers)
			{
				writer.StartObject();
				writer.Key("id");
				writer.Double(peer.percentage);
				writer.Key("downloadSpeed");
				writer.Uint64(peer.downloadSpeed);
				writer.Key("uploadSpeed");
				writer.Uint64(peer.uploadSpeed);
				writer.Key("address");
				writer.String(peer.address.toString().data());

				if (!peer.client.empty())
				{
					writer.Key("client");
					writer.String(peer.client.data());
				}

				if (!peer.country.empty())
				{
					writer.Key("country");
					writer.String(peer.country.data());
				}
				writer.EndObject();
			}

			writer.EndArray();
			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetSourcesInfo)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("sources");
			writer.StartArray();

			uint32_t currentTime = (uint32_t)time(0);
			auto sources = t->peers->getSourcesInfo();
			for (auto& source : sources)
			{
				writer.StartObject();
				writer.Key("name");
				writer.String(source.hostname.data());
				writer.Key("peers");
				writer.Uint64(source.peers);
				writer.Key("seeds");
				writer.Uint64(source.seeds);
				writer.Key("leechers");
				writer.Uint64(source.leechers);
				writer.Key("interval");
				writer.Uint64(source.announceInterval);
				writer.Key("nextCheck");
				writer.Uint64(source.nextAnnounce < currentTime ? 0 : source.nextAnnounce - currentTime);

				writer.Key("status");

				if (source.state == mtt::TrackerState::Connected || source.state == mtt::TrackerState::Alive)
					writer.String("Ready");
				else if (source.state == mtt::TrackerState::Connecting)
					writer.String("Connecting");
				else if (source.state == mtt::TrackerState::Announcing || source.state == mtt::TrackerState::Reannouncing)
					writer.String("Announcing");
				else if (source.state == mtt::TrackerState::Announced)
					writer.String("Announced");
				else if (source.state == mtt::TrackerState::Offline)
					writer.String("Offline");
				else
					writer.String("Stopped");

				writer.EndObject();
			}

			writer.EndArray();
			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetPiecesInfo)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();

			auto piecesCount = t->files.progress.pieces.size();
			writer.Key("count");
			writer.Uint64(piecesCount);

			writer.Key("pieces");
			std::string pieces;
			if (piecesCount > 0)
			{
				pieces.resize(piecesCount);
				for (size_t i = 0; i < piecesCount; i++)
				{
					pieces[i] = t->files.progress.hasPiece((uint32_t)i) ? '1' : '0';
				}
			}
			writer.String(pieces.data());


			writer.Key("requests");
			auto requests = t->fileTransfer->getCurrentRequests();
			writer.StartArray();
			for (auto r : requests)
			{
				writer.Uint(r);
			}
			writer.EndArray();

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetMagnetLinkProgress)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t || !t->utmDl)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();

			writer.Key("finished");
			writer.Bool(t->utmDl->state.finished);
			writer.Key("totalParts");
			writer.Bool(t->utmDl->state.partsCount);
			writer.Key("receivedParts");
			writer.Bool(t->utmDl->state.receivedParts);

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetMagnetLinkProgressLogs)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t || !t->utmDl)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			uint32_t logStart = requestJs.HasMember("start") ? requestJs["start"].GetUint() : 0;

			writer.StartObject();

			writer.Key("logs");
			writer.StartArray();
			auto events = t->utmDl->getEvents();
			for (size_t i = logStart; i < logStart + events.size(); i++)
			{
				writer.String(events[i].toString().data());
			}
			writer.EndArray();

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetSettings)
		{
			if (output)
			{
				auto settings = mtt::config::getExternal().toJson();
				*output = settings;
			}
		}
		else if (id == mttJson::MessageId::SetSettings)
		{
			if(!mtt::config::fromJson(request))
				return mtt::Status::E_InvalidInput;
		}
		else if (id == mttJson::MessageId::GetTorrentFilesSelection)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			uint32_t logStart = requestJs.HasMember("start") ? requestJs["start"].GetUint() : 0;

			writer.StartObject();

			writer.Key("files");
			writer.StartArray();
			auto& files = t->files.selection.files;
			for (auto& f : files)
			{
				writer.StartObject();

				writer.String(f.info.path.back().data());
				writer.Bool(f.selected);
				writer.Uint64(f.info.size);
				writer.Uint(f.info.startPieceIndex);
				writer.Uint(f.info.endPieceIndex);

				writer.EndObject();
			}
			writer.EndArray();

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::SetTorrentFilesSelection)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("selection"))
				return mtt::Status::E_InvalidInput;

			auto selection = requestJs["selection"].GetArray();
	
			if (t->files.selection.files.size() != selection.Size())
				return mtt::Status::E_InvalidInput;

			std::vector<bool> dlSelect;
			for (auto& f : selection)
			{
				dlSelect.push_back(f.IsTrue());
			}

			if (!t->selectFiles(dlSelect))
				return mtt::Status::E_InvalidInput;
		}
		else if (id == mttJson::MessageId::RefreshSource)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("name"))
				return mtt::Status::E_InvalidInput;

			t->peers->refreshSource(requestJs["name"].GetString());
		}
		else if (id == mttJson::MessageId::AddPeer)
		{
			mtt::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core.getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("address"))
				return mtt::Status::E_InvalidInput;

			t->peers->connect(Addr(requestJs["address"].GetString()));
		}
		else
			return mtt::Status::E_InvalidInput;

		return mtt::Status::Success;
	}
}
