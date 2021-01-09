#include "mtTorrent/Api/Core.h"
#include "mtTorrent/Api/Configuration.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <iostream>
#include <tuple>

int main()
{
	auto settings = mtt::config::getExternal();
	settings.files.defaultDirectory = "./";
	mtt::config::setValues(settings.files);
	settings.dht.enabled = true;
	mtt::config::setValues(settings.dht);

	auto core = mttApi::Core::create();

	core->registerAlerts((int) mtt::AlertId::MetadataFinished);

	mtt::Status addStatus;
	mttApi::TorrentPtr addedTorrent;

	//magnet test
	std::tie(addStatus, addedTorrent) = core->addMagnet("magnetLink");
	
	//file test
	//std::tie(addStatus, addedTorrent) = core->addFile("path.torrent");

	if (addedTorrent)
	{
		addedTorrent->start();
	}
	else
	{
		printf("Adding torrent failed, status %d", (int)addStatus);
		return 0;
	}

	while(!addedTorrent->finished())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		auto alerts = core->popAlerts();

		for (auto& a : alerts)
		{
			if (a->id == mtt::AlertId::MetadataFinished)
			{
				auto mdAlert = a->getAs<mtt::MetadataAlert>();
				printf("Name: %s, Metadata download finished\n", mdAlert->torrent->name().c_str());
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
					static size_t magnetProgressLogPos = 0;
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

void example2()
{
	auto core = mttApi::Core::create();

	auto [status, torrent] = core->addFile("path/to/torrent/file.torrent");

	if (torrent)
	{
		torrent->start();

		while (!torrent->finished())
		{
			std::cout << torrent->name() << " progress: " << torrent->currentProgress() * 100 << "%%\n";
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	else
	{
		std::cout << "Adding torrent failed with error " << (int)status;
		return;
	}

	auto& torrentFile = torrent->getFileInfo();

	if (!torrentFile.about.createdBy.empty())
		std::cout << "Torrent created by " << torrentFile.about.createdBy;

	for (auto& file : torrentFile.info.files)
	{
		std::cout << "File: " << file.path.back() << std::endl;
		std::cout << "File size: " << file.size << std::endl;

		//subdirectory
		if (file.path.size() > 1)
		{
			std::string fileDirectory;
			for (size_t i = 0; i < file.path.size() - 1; i++)
				fileDirectory += file.path[i] + "\\";

			std::cout << "Directory: " << fileDirectory << std::endl;
		}
	}

	std::cout << "Torrent divided into " << torrentFile.info.pieces.size() << " pieces, with piece size " << torrentFile.info.pieceSize << std::endl;

	auto selection = torrent->getFilesSelection();

	std::vector<bool> newSelection;
	std::vector<mtt::Priority> newPriority;

	for (auto s : selection.files)
	{
		newSelection.push_back(!s.selected);
		newPriority.push_back(mtt::Priority::High);
	}

	torrent->selectFiles(newSelection);
	torrent->setFilesPriority(newPriority);
	torrent->setLocationPath("path\\to\\new\\download\\path");

	if (!torrent->selectionFinished())
		std::cout << "Selection progress: " << torrent->currentSelectionProgress() * 100 << "%%\n";

	auto transfer = torrent->getFileTransfer();

	std::cout << "Current download speed: " << transfer->getDownloadSpeed() << " bytes per second" << std::endl;

	auto peers = transfer->getPeersInfo();

	std::cout << "Currently connected to " << peers.size() << " peers" << std::endl;

	for (auto& peer : peers)
		std::cout << "Address: " << peer.address << ", speed " << peer.downloadSpeed << ", torrent client" << peer.client << std::endl;

	std::cout << "Currently downloading " << transfer->getCurrentRequests().size() << " pieces" << std::endl;

	if (auto magnet = torrent->getMagnetDownload())
	{
		mtt::MetadataDownloadState state = magnet->getState();

		if (!state.finished && state.partsCount)
			std::cout << "Metadata  progress " << state.receivedParts << "/" << state.partsCount << std::endl;

		std::vector<std::string> logs;
		if (auto count = magnet->getDownloadLog(logs, 0))
		{
			for (auto& l : logs)
				std::cout << "Metadata log: " << l << std::endl;
		}
	}

	{
		auto peers = torrent->getPeers();

		std::cout << "Found peers " << peers->receivedCount() << std::endl;
		std::cout << "Connected peers " << peers->connectedCount() << std::endl;

		auto sources = peers->getSourcesInfo();
		for (auto source : sources)
		{
			if (source.state >= mtt::TrackerState::Connected)
				std::cout << source.hostname << " has " << source.peers << " peers" << std::endl;
		}

		peers->refreshSource(sources.front().hostname);

		//connect locally running client
		peers->connect("127.0.0.1:55555");
	}

	core->registerAlerts((int)mtt::AlertId::MetadataFinished | (int)mtt::AlertId::TorrentAdded);

	//...

	auto alerts = core->popAlerts();
	for (const auto& alert : alerts)
	{
		if (alert->id == mtt::AlertId::MetadataFinished)
		{
			std::cout << alert->getAs<mtt::MetadataAlert>()->torrent->name() << " finished" << std::endl;
		}
		else if (alert->id == mtt::AlertId::TorrentAdded)
		{
			std::cout << alert->getAs<mtt::TorrentAlert>()->torrent->name() << " added" << std::endl;
		}
	}
}
