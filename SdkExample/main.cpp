#include "Api\Core.h"
#include "Api\Configuration.h"

#include <windows.h>

int main()
{
	auto core = mttApi::Core::create();

	auto settings = mtt::config::getExternal();
	settings.files.defaultDirectory = "./";
	mtt::config::setValues(settings.files);
	settings.dht.enable = true;
	mtt::config::setValues(settings.dht);

	core->registerAlerts((int) mtt::AlertId::MetadataFinished);

	//file test
	/*
	auto addResult = core->addFile("D:\\test.torrent");
	if (addResult.second)
	{
		addResult.second->start();
	}
	*/

	//magnet test
	size_t magnetProgressLogPos = 0;
	auto addResult = core->addMagnet("60B101018A32FBDDC264C1A2EB7B7E9A99DBFB6A");
	if (addResult.second)
	{
		addResult.second->start();
	}

	while(!addResult.second->finished())
	{
		Sleep(500);

		auto alerts = core->popAlerts();

		for (auto& a : alerts)
		{
			if (a->id == mtt::AlertId::MetadataFinished)
			{
				auto mdAlert = a->getAs<mtt::MetadataAlert>();
				printf("Name: %s, Metadata download finished\n", core->getTorrent(mdAlert->hash)->name().c_str());
			}
		}

		auto torrents = core->getTorrents();

		for (auto t : torrents)
		{
			auto state = t->getState();

			switch (state)
			{
			case mttApi::Torrent::State::CheckingFiles:
				printf("Name: %s, checking files... %f%%\n", t->name().c_str(), t->checkingProgress() * 100);
				break;
			case mttApi::Torrent::State::DownloadingMetadata:
			{
				if (auto magnet = t->getMagnetDownload())
				{
					mtt::MetadataDownloadState utmState = magnet->getState();
					if(utmState.partsCount == 0)
						printf("Metadata download getting peers... Connected peers: %u, Found peers: %u\n", t->getPeers()->connectedCount(), t->getPeers()->receivedCount());
					else
						printf("Metadata download progress: %u / %u, Connected peers: %u, Found peers: %u\n", utmState.receivedParts, utmState.partsCount, t->getPeers()->connectedCount(), t->getPeers()->receivedCount());

					std::vector<std::string> logs;
					if (auto count = magnet->getDownloadLog(logs, magnetProgressLogPos))
					{
						magnetProgressLogPos += count;

						for (auto& l : logs)
							printf("Metadata log: %s\n", l.data());
					}
				}
				break;
			}
			case mttApi::Torrent::State::Downloading:
				printf("Name: %s, Progress: %f%%, Speed: %u bps, Connected peers: %u, Found peers: %u\n", t->name().c_str(), t->currentProgress()*100, t->getFileTransfer()->getDownloadSpeed(), t->getPeers()->connectedCount(), t->getPeers()->receivedCount());
				break;
			case mttApi::Torrent::State::Seeding:
				printf("Name: %s finished, upload speed: %u\n", t->name().c_str(), t->getFileTransfer()->getUploadSpeed());
				break;
			case mttApi::Torrent::State::Interrupted:
				printf("Name: %s interrupted, problem code: %d\n", t->name().c_str(), (int)t->getLastError());
				break;
			case mttApi::Torrent::State::Inactive:
			default:
				break;
			}
		}
	}

	//optional, stopped automatically in core destructor
	for (auto t : core->getTorrents())
	{
		t->stop();
	}

	return 0;
}