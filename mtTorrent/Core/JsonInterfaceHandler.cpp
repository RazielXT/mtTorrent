#ifdef MTT_JSON_INTERFACE

#include "Api/Core.h"
#include "Api/Configuration.h"
#include "Public/JsonInterface.h"
#include "utils/HexEncoding.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <time.h>

extern std::shared_ptr<mttApi::Core> core;

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

			core = mttApi::Core::create();
		}
		else if (id == mttJson::MessageId::Deinit)
		{
			core.reset();
		}
		else if (id == mttJson::MessageId::Add)
		{
			mttApi::TorrentPtr t;

			if(requestJs.HasMember("filepath"))
				t = core->addFile(requestJs["filepath"].GetString()).second;
			else if (requestJs.HasMember("magnet"))
				t = core->addMagnet(requestJs["magnet"].GetString()).second;

			if (!t)
				return mtt::Status::E_InvalidInput;

			if (output)
			{
				auto response = "{ \"hash\" : \"" + hexToString(t->getFileInfo().info.hash, 20) + "\" }";
				*output = response;
			}
		}
		else if (id == mttJson::MessageId::Start)
		{
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->start();
		}
		else if (id == mttJson::MessageId::Stop)
		{
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			t->stop();
		}
		else if (id == mttJson::MessageId::Remove)
		{
			mtt::Status status = mtt::Status::E_InvalidInput;
			bool removeFiles = requestJs.HasMember("removeFiles") ? requestJs["removeFiles"].IsTrue() : false;

			if (requestJs.HasMember("hash"))
				status = core->removeTorrent(requestJs["hash"].GetString(), removeFiles);

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
			auto torrents = core->getTorrents();
			for (size_t i = 0; i < torrents.size(); i++)
			{
				writer.StartObject();

				auto t = torrents[i];
				writer.Key("hash");
				writer.String(hexToString(t->getFileInfo().info.hash, 20).data());
				writer.Key("active");
				writer.Bool((t->getStatus() != mttApi::Torrent::State::Stopped));

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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			auto& torrentFile = t->getFileInfo();
			writer.StartObject();
			writer.Key("name");
			writer.String(torrentFile.info.name.data(), (uint32_t)torrentFile.info.name.length());
			writer.Key("size");
			writer.Uint64(torrentFile.info.fullSize);

			writer.Key("files");
			writer.StartArray();
			for (auto& f : torrentFile.info.files)
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
				mttApi::TorrentPtr t = core->getTorrent(hashItem->name.GetString());

				if(!t)
					continue;

				auto& torrentFile = t->getFileInfo();

				writer.StartObject();
				writer.Key("hash");
				writer.String(hashItem->name.GetString());
				writer.Key("name");
				writer.String(torrentFile.info.name.data(), (uint32_t)torrentFile.info.name.length());
				writer.Key("connectedPeers");
				writer.Uint(t->getPeers()->connectedCount());
				writer.Key("foundPeers");
				writer.Uint(t->getPeers()->receivedCount());

				writer.Key("downloaded");
				writer.Uint64(t->downloaded());
				writer.Key("uploaded");
				writer.Uint64(t->uploaded());
				writer.Key("downloadSpeed");
				writer.Uint64(t->getFileTransfer()->getDownloadSpeed());
				writer.Key("uploadSpeed");
				writer.Uint64(t->getFileTransfer()->getUploadSpeed());

				writer.Key("progress");
				writer.Double(t->currentProgress());
				writer.Key("selectionProgress");
				writer.Double(t->currentSelectionProgress());

				writer.Key("status");
				writer.Uint((uint32_t)t->getLastError());

			/*	if (t->utmDl && !t->utmDl->state.finished)
				{
					writer.Key("utmActive");
					writer.Bool(true);
				}
				*/
				//if (t->checking)
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("progress");
			writer.StartArray();

			auto peers = t->getFileTransfer()->getPeersInfo();
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
				writer.String(peer.address.data());

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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			writer.Key("sources");
			writer.StartArray();

			uint32_t currentTime = (uint32_t)::time(0);
			auto sources = t->getPeers()->getSourcesInfo();
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();
			auto piecesCount = t->getFileInfo().info.pieces.size();
			writer.Key("count");
			writer.Uint64(piecesCount);

			writer.Key("pieces");
			std::string piecesStr(piecesCount, '0');
			if (piecesCount > 0)
			{
				size_t size = 0;
				t->getReceivedPieces(nullptr, size);

				if (size > 0)
				{
					std::vector<uint32_t> piecesList;
					piecesList.resize(size);
					t->getReceivedPieces(piecesList.data(), size);

					for (auto& idx : piecesList)
					{
						piecesStr[idx] = '1';
					}
				}
			}
			writer.String(piecesStr.data());


			writer.Key("requests");
			auto requests = t->getFileTransfer()->getCurrentRequests();
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			auto utm = t->getMagnetDownload();
			if (!utm)
				return mtt::Status::E_NoData;

			mtt::MetadataDownloadState state = utm->getState();

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();

			writer.Key("finished");
			writer.Bool(state.finished);
			writer.Key("totalParts");
			writer.Bool(state.partsCount);
			writer.Key("receivedParts");
			writer.Bool(state.receivedParts);

			writer.EndObject();

			if (output)
			{
				output->assign(s.GetString(), s.GetLength());
			}
		}
		else if (id == mttJson::MessageId::GetMagnetLinkProgressLogs)
		{
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			uint32_t logStart = requestJs.HasMember("start") ? requestJs["start"].GetUint() : 0;

			auto utm = t->getMagnetDownload();
			if (!utm)
				return mtt::Status::E_NoData;

			std::vector<std::string> logs;
			if (utm->getDownloadLog(logs, logStart) == 0)
				return mtt::Status::E_NoData;

			writer.StartObject();

			writer.Key("logs");
			writer.StartArray();
			for (auto& l : logs)
			{
				writer.String(l.data(),(uint32_t) l.length());
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t)
				return mtt::Status::E_InvalidInput;

			js::StringBuffer s;
			js::Writer<js::StringBuffer> writer(s);

			writer.StartObject();

			writer.Key("files");
			writer.StartArray();
			auto selection = t->getFilesSelection();
			for (auto& f : selection.files)
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("selection"))
				return mtt::Status::E_InvalidInput;

			auto selection = requestJs["selection"].GetArray();
	
			if (t->getFileInfo().info.files.size() != selection.Size())
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
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("name"))
				return mtt::Status::E_InvalidInput;

			t->getPeers()->refreshSource(requestJs["name"].GetString());
		}
		else if (id == mttJson::MessageId::AddPeer)
		{
			mttApi::TorrentPtr t;

			if (requestJs.HasMember("hash"))
				t = core->getTorrent(requestJs["hash"].GetString());

			if (!t || !requestJs.HasMember("address"))
				return mtt::Status::E_InvalidInput;

			t->getPeers()->connect(requestJs["address"].GetString());
		}
		else
			return mtt::Status::E_InvalidInput;

		return mtt::Status::Success;
	}
}

#endif // MTT_JSON_INTERFACE
