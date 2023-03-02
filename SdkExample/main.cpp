#include "mtTorrent/Api/Core.h"
#include "mtTorrent/Api/Configuration.h"
#include <thread>
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

	core->registerAlerts(mtt::Alerts::Id::MetadataInitialized);

	auto [addStatus, addedTorrent] = core->addMagnet("bc8e4a520c4dfee5058129bd990bb8c7334f008e");

	//auto [addStatus, addedTorrent] = core->addFile("path.torrent");

	if (addedTorrent)
	{
		addedTorrent->start();
	}
	else
	{
		std::cout << "Adding torrent failed, status " << (int)addStatus << std::endl;
		return 0;
	}

	while (!addedTorrent->finished())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		auto alerts = core->popAlerts();

		for (const auto& a : alerts)
		{
			if (a->id == mtt::Alerts::Id::MetadataInitialized)
			{
				auto mdAlert = a->getAs<mtt::MetadataAlert>();
				std::cout << "Metadata loaded: " << mdAlert->torrent->name() << std::endl;
			}
		}

		auto torrents = core->getTorrents();

		for (const auto& t : torrents)
		{
			auto state = t->getState();

			switch (state)
			{
			case mttApi::Torrent::State::CheckingFiles:
				std::cout << "Checking files... " << t->getFiles().checkingProgress() * 100 << "%%" << std::endl;
				break;
			case mttApi::Torrent::State::DownloadingMetadata:
			{
				if (auto magnet = t->getMetadataDownload())
				{
					mtt::MetadataDownloadState utmState = magnet->getState();
					std::cout << "Metadata download getting peers... Connected: " << t->getPeers().connectedCount() << ", Found: " << t->getPeers().receivedCount() << std::endl;
					if (utmState.partsCount != 0)
						std::cout << "Metadata download progress: " << utmState.receivedParts << "/" << utmState.partsCount << std::endl;

					std::vector<std::string> logs;
					static size_t magnetProgressLogPos = 0;
					if (auto count = magnet->getDownloadLog(logs, magnetProgressLogPos))
					{
						magnetProgressLogPos += count;

						for (auto& l : logs)
							std::cout << l << std::endl;
					}
				}
				break;
			}
			case mttApi::Torrent::State::Active:
				if (!t->finished())
					std::cout << "Progress : " << t->progress() * 100 << "%%, Speed : " << t->getFileTransfer().getDownloadSpeed() << " bps, Connected peers : " << t->getPeers().connectedCount() << ", Received peers : " << t->getPeers().receivedCount() << std::endl;
				else
					std::cout << "Finished, upload speed: " << t->getFileTransfer().getUploadSpeed() << std::endl;
				break;
			case mttApi::Torrent::State::Stopped:
				if (auto err = (int)t->getLastError())
					std::cout << "Interrupted, error code: " << err << std::endl;
				break;
			}
		}
	}

	//optional, stopped automatically in core destructor
	for (const auto& t : core->getTorrents())
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
			std::cout << torrent->name() << " progress: " << torrent->progress() * 100 << "%%\n";
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	else
	{
		std::cout << "Adding torrent failed with error " << (int)status;
		return;
	}

	auto& torrentFile = torrent->getMetadata();

	if (!torrentFile.about.createdBy.empty())
		std::cout << "Torrent created by " << torrentFile.about.createdBy;

	for (const auto& file : torrent->getFilesInfo())
	{
		std::cout << "File: " << file.name << std::endl;
		std::cout << "File size: " << file.size << std::endl;

		//subdirectory
		if (!file.path.empty())
		{
			std::string fileDirectory;
			for (auto& p : file.path)
			{
				fileDirectory.append(p);
				fileDirectory += "\\";
			}

			std::cout << "Directory: " << fileDirectory << std::endl;
		}
	}

	std::cout << "Torrent divided into " << torrentFile.info.pieces.size() << " pieces, with piece size " << torrentFile.info.pieceSize << std::endl;

	torrent->getFiles().select(0, false);
	torrent->getFiles().setPriority(1, mtt::PriorityHigh);

	torrent->getFiles().setLocationPath(R"(path\to\new\download\path)", false);

	if (!torrent->selectionFinished())
		std::cout << "Selection progress: " << torrent->selectionProgress() * 100 << "%%" << std::endl;

	auto& transfer = torrent->getFileTransfer();

	std::cout << "Current download speed: " << transfer.getDownloadSpeed() << " bytes per second" << std::endl;
	std::cout << "Currently downloading " << transfer.getCurrentRequests().size() << " pieces" << std::endl;

	auto peers = torrent->getPeers().getConnectedPeersInfo();

	std::cout << "Currently connected to " << peers.size() << " peers" << std::endl;

	for (const auto& peer : peers)
		std::cout << "Address: " << peer.address << ", speed " << peer.downloadSpeed << ", torrent client" << peer.client << std::endl;

	if (auto magnet = torrent->getMetadataDownload())
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
		auto& peers = torrent->getPeers();

		std::cout << "Found peers " << peers.receivedCount() << std::endl;
		std::cout << "Connected peers " << peers.connectedCount() << std::endl;

		auto sources = peers.getSourcesInfo();
		for (const auto& source : sources)
		{
			if (source.state >= mtt::TrackerState::Connected)
				std::cout << source.hostname << " returned " << source.peers << " peers" << std::endl;
		}

		peers.refreshSource(sources.front().hostname);

		//connect locally running client
		peers.connect("127.0.0.1:55555");
	}

	core->registerAlerts(mtt::Alerts::Id::MetadataInitialized | mtt::Alerts::Id::TorrentAdded);

	//...

	auto alerts = core->popAlerts();
	for (const auto& alert : alerts)
	{
		if (alert->id == mtt::Alerts::Id::MetadataInitialized)
		{
			std::cout << alert->getAs<mtt::MetadataAlert>()->torrent->name() << " metadata loaded" << std::endl;
		}
		else if (alert->id == mtt::Alerts::Id::TorrentAdded)
		{
			std::cout << alert->getAs<mtt::TorrentAlert>()->torrent->name() << " added" << std::endl;
		}
	}
}
