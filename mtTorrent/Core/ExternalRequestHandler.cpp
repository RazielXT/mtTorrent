#include "ExternalRequestHandler.h"
#include "MetadataDownload.h"
#include "utils/TorrentFileParser.h"
#include "Core.h"

void ExternalRequestHandler::openMagnetLink(uint32_t requestId, std::string& link, std::function<void(bool,mtt::CorePtr)> onResult)
{
	//std::string link = "magnet:?xt=urn:btih:5AYWR2LK3ORHWRI2Y6BVBUX6QAUF2SDP&tr=http://nyaa.tracker.wf:7777/announce&tr=udp://tracker.coppersurfer.tk:6969/announce&tr=udp://tracker.internetwarriors.net:1337/announce&tr=udp://tracker.leechersparadise.org:6969/announce&tr=udp://tracker.opentrackr.org:1337/announce&tr=udp://open.stealth.si:80/announce&tr=udp://p4p.arenabg.com:1337/announce&tr=udp://mgtracker.org:6969/announce&tr=udp://tracker.tiny-vps.com:6969/announce&tr=udp://peerfect.org:6969/announce&tr=http://share.camoe.cn:8080/announce&tr=http://t.nyaatracker.com:80/announce&tr=https://open.kickasstracker.com:443/announce";//GetClipboardText();
	mtt::TorrentFileInfo parsedTorrent;

	if (parsedTorrent.parseMagnetLink(link) == mtt::Status::Success)
	{
		auto ptr = mtt::TorrentsCollection::Get().GetTorrent(parsedTorrent);

		if (ptr->torrent.info.files.empty())
		{
			struct MetadataDownloadTask : public RequestTask
			{
				mtt::MetadataDownload dl;

				virtual void stop() override
				{
					dl.stop();
				}
			};

			auto dlTask = std::make_shared<MetadataDownloadTask>();
			dlTask->requestId = requestId;
			tasks.push_back(dlTask);

			dlTask->dl.start(ptr, [this, dlTask, ptr, onResult](bool success)
			{
				if (success)
				{
					ptr->torrent.info = mtt::TorrentFileParser::parseTorrentInfo(dlTask->dl.metadata.buffer.data(), dlTask->dl.metadata.buffer.size());
				}

				onResult(success, ptr);
				stopRequest(dlTask->requestId);
			});
		}
	}
}

void ExternalRequestHandler::stopRequest(uint32_t requestId)
{
	for(auto it = tasks.begin(); it != tasks.end(); it++)
		if ((*it)->requestId == requestId)
		{
			(*it)->stop();
			tasks.erase(it);
			return;
		}
}
