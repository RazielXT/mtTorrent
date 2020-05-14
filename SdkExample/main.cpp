#include "Api\Core.h"
#include "Api\Configuration.h"

#include <windows.h>

int main()
{
	auto core = mttApi::Core::create();

	auto settings = mtt::config::getExternal();
	settings.files.defaultDirectory = "./";
	mtt::config::setValues(settings.files);

	core->registerAlerts((int) mtt::AlertId::MetadataFinished);

	auto addResult = core->addFile("D:\\boruto.torrent");
	if (addResult.second)
	{
		addResult.second->start();
	}

	addResult = core->addMagnet("60B101018A32FBDDC264C1A2EB7B7E9A99DBFB6A");
	if (addResult.second)
	{
		addResult.second->start();
	}

	for (int i = 0; i < 10; i++)
	{
		Sleep(1000);

		auto alerts = core->popAlerts();

		for (auto& a : alerts)
		{
			if (auto t = alerts[i]->getAs<mtt::MetadataAlert>())
			if (a->id == mtt::AlertId::MetadataFinished)
			{
				printf("Name: %s, Metadata download finished\n", core->getTorrent(t->hash)->name().c_str());
			}
		}

		auto torrents = core->getTorrents();

		for (auto t : torrents)
		{
			if (t->getStatus() == mttApi::Torrent::State::DownloadUtm)
			{
				if (auto magnet = t->getMagnetDownload())
				{
					mtt::MetadataDownloadState utmState = magnet->getState();
					printf("Name: %s, Metadata Progress: %d / %d, Found peers: %d\n", t->name().c_str(), (int)utmState.receivedParts, (int)utmState.partsCount, (int)t->getPeers()->receivedCount());
				}
			}
			else
				printf("Name: %s, Progress: %d, Speed: %d, Found peers: %d\n", t->name().c_str(), (int)t->downloaded(), (int)t->getFileTransfer()->getDownloadSpeed(), (int)t->getPeers()->receivedCount());
		}
	}

	for (auto t : core->getTorrents())
	{
		t->stop();
	}

	return 0;
}