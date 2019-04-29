#include "Torrent.h"
#include "utils/TorrentFileParser.h"
#include "MetadataDownload.h"
#include "Peers.h"
#include "Configuration.h"
#include "FileTransfer.h"

mtt::TorrentPtr mtt::Torrent::fromFile(std::string filepath)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();
	torrent->infoFile = mtt::TorrentFileParser::parseFile(filepath.data());

	if (!torrent->infoFile.info.name.empty())
	{
		torrent->peers = std::make_unique<Peers>(torrent);
		torrent->fileTransfer = std::make_unique<FileTransfer>(torrent);
		torrent->init();
		return torrent;
	}
	else
		return nullptr;
}

mtt::TorrentPtr mtt::Torrent::fromMagnetLink(std::string link)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();
	if (torrent->infoFile.parseMagnetLink(link) != Status::Success)
		return nullptr;

	torrent->peers = std::make_unique<Peers>(torrent);
	torrent->fileTransfer = std::make_unique<FileTransfer>(torrent);

	return torrent;
}

void mtt::Torrent::downloadMetadata(std::function<void(Status, MetadataDownloadState&)> callback)
{
	state = State::DownloadUtm;
	utmDl = std::make_unique<MetadataDownload>(*peers);
	utmDl->start([this, callback](Status s, MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished)
		{
			infoFile.info = utmDl->metadata.getRecontructedInfo();
			init();
		}

		if (state.finished)
		{
			if (this->state == State::Started)
				start();
			else
				this->state = State::Stopped;
		}

		callback(s, state);
	}
	, service.io);
}

void mtt::Torrent::init()
{
	files.init(infoFile.info);
}

bool mtt::Torrent::start()
{
	if (state == State::DownloadUtm)
	{
		state = State::Started;
		return true;
	}

	if (!checking && !filesChecked())
	{
		checkFiles([this](std::shared_ptr<PiecesCheck> ch)
			{
				if (!ch->rejected)
					start();
			});
	}

	lastError = Status::E_InvalidInput;

	if (files.selection.files.empty())
		return false;

	lastError = files.prepareSelection();

	if (lastError != mtt::Status::Success)
		return false;

	service.start(2);

	state = State::Started;

	if (checking)
		return true;

	fileTransfer->start();

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

	if (fileTransfer)
	{
		fileTransfer->stop();
	}

	if (checking)
	{
		std::lock_guard<std::mutex> guard(checkStateMutex);

		if(checkState)
			checkState->rejected = true;
	}

	service.stop();
	state = State::Stopped;
	lastError = Status::Success;
}

std::shared_ptr<mtt::PiecesCheck> mtt::Torrent::checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish)
{
	auto checkFunc = [this, onFinish](std::shared_ptr<PiecesCheck> check)
	{
		{
			std::lock_guard<std::mutex> guard(checkStateMutex);
			checkState.reset();
		}

		checking = false;

		if (!check->rejected)
		{
			files.progress.fromList(check->pieces);
			checked = true;
		}

		if (state == State::Started)
			start();

		onFinish(check);
	};

	checking = true;
	std::lock_guard<std::mutex> guard(checkStateMutex);
	checkState = files.storage.checkStoredPiecesAsync(infoFile.info.pieces, service.io, checkFunc);
	return checkState;
}

void mtt::Torrent::checkFiles()
{
	checkFiles([](std::shared_ptr<PiecesCheck>) {});
}

float mtt::Torrent::checkingProgress()
{
	std::lock_guard<std::mutex> guard(checkStateMutex);

	if (checkState)
		return checkState->piecesChecked / (float)checkState->piecesCount;
	else
		return 1;
}

bool mtt::Torrent::filesChecked()
{
	return checked;
}

bool mtt::Torrent::selectFiles(std::vector<bool>& s)
{
	if (files.selection.files.size() != s.size())
		return false;

	for (size_t i = 0; i < s.size(); i++)
	{
		files.selection.files[i].selected = s[i];
	}

	files.select(files.selection);

	if (state == State::Started)
	{
		lastError = files.prepareSelection();

		if (fileTransfer)
			fileTransfer->reevaluate();

		return lastError == Status::Success;
	}

	return true;
}

bool mtt::Torrent::finished()
{
	return files.progress.getPercentage() == 1;
}

bool mtt::Torrent::selectionFinished()
{
	return files.progress.getSelectedPercentage() == 1;
}

uint8_t* mtt::Torrent::hash()
{
	return infoFile.info.hash;
}

std::string mtt::Torrent::name()
{
	return infoFile.info.name;
}

float mtt::Torrent::currentProgress()
{
	return files.progress.getPercentage();
}

float mtt::Torrent::currentSelectionProgress()
{
	return files.progress.getSelectedPercentage();
}

size_t mtt::Torrent::downloaded()
{
	return (size_t) (infoFile.info.fullSize*files.progress.getPercentage());
}

size_t mtt::Torrent::downloadSpeed()
{
	return fileTransfer ? fileTransfer->getDownloadSpeed() : 0;
}

size_t mtt::Torrent::uploaded()
{
	return fileTransfer ? fileTransfer->getUploadSum() : 0;
}

size_t mtt::Torrent::uploadSpeed()
{
	return fileTransfer ? fileTransfer->getUploadSpeed() : 0;
}

size_t mtt::Torrent::dataLeft()
{
	return infoFile.info.fullSize - downloaded();
}
