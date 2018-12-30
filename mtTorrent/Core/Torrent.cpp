#include "Torrent.h"
#include "utils/TorrentFileParser.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Peers.h"
#include "Configuration.h"

mtt::TorrentPtr mtt::Torrent::fromFile(std::string filepath)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();
	torrent->infoFile = mtt::TorrentFileParser::parseFile(filepath.data());

	if (!torrent->infoFile.info.name.empty())
	{
		torrent->peers = std::make_unique<Peers>(torrent);
		torrent->downloader = std::make_unique<Downloader>(torrent);
		torrent->init();
		return torrent;
	}
	else
		return nullptr;
}

mtt::TorrentPtr mtt::Torrent::fromMagnetLink(std::string link, std::function<void(Status,MetadataDownloadState&)> callback)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();
	if (torrent->infoFile.parseMagnetLink(link) != Status::Success)
		return nullptr;

	torrent->peers = std::make_unique<Peers>(torrent);
	torrent->downloader = std::make_unique<Downloader>(torrent);
	torrent->utmDl = std::make_unique<MetadataDownload>(*torrent->peers);
	torrent->utmDl->start([torrent, callback](Status s, MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished)
		{
			torrent->infoFile.info = torrent->utmDl->metadata.getRecontructedInfo();
			torrent->init();
		}

		callback(s, state);
	}
	);

	return torrent;
}

void mtt::Torrent::init()
{
	files.init(infoFile.info);
}

bool mtt::Torrent::start()
{
	if (files.selection.files.empty())
		return false;

	if (files.prepareSelection() != mtt::Status::Success)
		return false;

	downloader->start();

	return true;
}

void mtt::Torrent::pause()
{

}

void mtt::Torrent::stop()
{
	if (utmDl)
	{
		utmDl->stop();
	}

	if (peers)
	{
		peers->stop();
	}

	service.stop();
}

std::shared_ptr<mtt::PiecesCheck> mtt::Torrent::checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish)
{
	auto checkFunc = [this, onFinish](std::shared_ptr<PiecesCheck> check)
	{
		if (!check->rejected)
		{
			files.progress.fromList(check->pieces);
		}

		onFinish(check);
	};

	return files.storage.checkStoredPiecesAsync(infoFile.info.pieces, service.io, checkFunc);
}

bool mtt::Torrent::finished()
{
	return files.progress.getPercentage() == 1;
}

std::string mtt::Torrent::name()
{
	return infoFile.info.name;
}

float mtt::Torrent::currentProgress()
{
	return files.progress.getPercentage();
}

size_t mtt::Torrent::downloaded()
{
	return (size_t) (infoFile.info.fullSize*files.progress.getPercentage());
}

size_t mtt::Torrent::downloadSpeed()
{
	return peers->statistics.getDownloadSpeed();
}
