#include "Torrent.h"
#include "utils/TorrentFileParser.h"
#include "MetadataDownload.h"
#include "Peers.h"
#include "Configuration.h"
#include "FileTransfer.h"
#include "State.h"
#include "utils/HexEncoding.h"
#include "utils/Filesystem.h"
#include "AlertsManager.h"
#include <filesystem>

mtt::Torrent::Torrent() : service(0), files(*this)
{
}

mtt::Torrent::~Torrent()
{
}

mtt::TorrentPtr mtt::Torrent::fromFile(mtt::TorrentFileInfo fileInfo)
{
	if (fileInfo.info.name.empty())
		return nullptr;

	mtt::TorrentPtr torrent = std::make_shared<Torrent>();

	torrent->infoFile = std::move(fileInfo);
	torrent->loadedState = LoadedState::Full;

	torrent->peers = std::make_unique<Peers>(*torrent);
	torrent->fileTransfer = std::make_unique<FileTransfer>(*torrent);
	torrent->addedTime = mtt::CurrentTimestamp();

	torrent->initialize();

	return torrent;
}

mtt::TorrentPtr mtt::Torrent::fromMagnetLink(std::string link)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();

	if (torrent->infoFile.parseMagnetLink(link) != Status::Success)
		return nullptr;

	torrent->peers = std::make_unique<Peers>(*torrent);
	torrent->fileTransfer = std::make_unique<FileTransfer>(*torrent);
	torrent->addedTime = mtt::CurrentTimestamp();

	return torrent;
}

mtt::TorrentPtr mtt::Torrent::fromSavedState(std::string name)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();

	TorrentState state(torrent->files.progress.pieces);
	if (!state.load(name))
		return nullptr;

	torrent->infoFile.info.name = state.info.name;
	torrent->infoFile.info.pieceSize = state.info.pieceSize;
	torrent->infoFile.info.fullSize = state.info.fullSize;
	decodeHexa(name, torrent->infoFile.info.hash);

	if (state.info.name.empty())
	{
		if (!torrent->loadFileInfo())
			return nullptr;

		torrent->stateChanged = true;
	}

	torrent->files.initialize(state);
	torrent->addedTime = state.addedTime;

	if (torrent->addedTime == 0)
		torrent->addedTime = mtt::CurrentTimestamp();

	torrent->peers = std::make_unique<Peers>(*torrent);
	torrent->fileTransfer = std::make_unique<FileTransfer>(*torrent);

	torrent->fileTransfer->downloader.downloaded = state.downloaded;
	torrent->fileTransfer->uploader->uploaded = state.uploaded;
	torrent->fileTransfer->downloader.unfinishedPieces.add(state.unfinishedPieces);

	if (state.started)
		torrent->start();

	return torrent;
}

bool mtt::Torrent::loadFileInfo()
{
	if (loadedState == LoadedState::Full)
		return true;

	if (utmDl)
		return false;

	loadedState = LoadedState::Full;

	DataBuffer buffer;
	if (!Torrent::loadSavedTorrentFile(hashString(), buffer))
	{
		lastError = Status::E_NoData;
		return false;
	}

	infoFile = mtt::TorrentFileParser::parse(buffer.data(), buffer.size());

	if (peers)
		peers->reloadTorrentInfo();

	return true;
}

void mtt::Torrent::save()
{
	if (!stateChanged)
		return;

	TorrentState saveState(files.progress.pieces);
	saveState.info.name = name();
	saveState.info.pieceSize = infoFile.info.pieceSize;
	saveState.info.fullSize = infoFile.info.fullSize;
	saveState.downloadPath = files.storage.getPath();
	saveState.lastStateTime = files.getLastModifiedTime();
	saveState.addedTime = addedTime;
	saveState.started = started;
	saveState.uploaded = fileTransfer->uploader->uploaded;
	saveState.downloaded = fileTransfer->downloader.downloaded;
	saveState.unfinishedPieces = fileTransfer->downloader.unfinishedPieces.getState();

	for (auto& f : files.selection)
		saveState.selection.push_back({ f.selected, f.priority });

	saveState.save(hashString());

	stateChanged = saveState.started;
}

void mtt::Torrent::saveTorrentFile(const uint8_t* data, std::size_t size)
{
	auto folderPath = mtt::config::getInternal().stateFolder + pathSeparator + hashString() + ".torrent";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	file.write((const char*)data, size);
}

void mtt::Torrent::removeMetaFiles()
{
	auto path = mtt::config::getInternal().stateFolder + pathSeparator + hashString();
	std::remove((path + ".torrent").data());
	std::remove((path + ".state").data());
}

void mtt::Torrent::prepareForChecking()
{
	fileTransfer->stop();
	lastError = Status::Success;
	activityTime = TimeClock::now();
}

void mtt::Torrent::refreshAfterChecking(const PiecesCheck& check)
{
	if (!check.rejected)
	{
		stateChanged = true;

		if (started)
			start();
		else
			stop();
	}

	AlertsManager::Get().torrentAlert(Alerts::Id::TorrentCheckFinished, *this);
}

void mtt::Torrent::refreshSelection()
{
	stateChanged = true;

	if (started)
	{
		fileTransfer->refreshSelection();
	}
}

bool mtt::Torrent::loadSavedTorrentFile(const std::string& hash, DataBuffer& out)
{
	auto filename = mtt::config::getInternal().stateFolder + pathSeparator + hash + ".torrent";
	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
		return false;

	out = DataBuffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return !out.empty();
}

void mtt::Torrent::downloadMetadata()
{
	service.start(4);

	if (!utmDl)
		utmDl = std::make_unique<MetadataDownload>(*peers, service);

	utmDl->start([this](const MetadataDownload::Event& e, MetadataReconstruction& metadata)
	{
		if (e.type == MetadataDownload::Event::End && metadata.finished())
		{
			infoFile.info = metadata.getRecontructedInfo();
			infoFile.info.data = std::move(metadata.buffer);

			loadedState = LoadedState::Full;

			auto fileData = infoFile.createTorrentFileData();
			saveTorrentFile(fileData.data(), fileData.size());
			peers->reloadTorrentInfo();

			activityTime = TimeClock::now();

			initialize();
			AlertsManager::Get().metadataAlert(Alerts::Id::MetadataInitialized, *this);

			if (isActive())
				service.post([this]() { start(); });
		}
	});

	activityTime = TimeClock::now();
}

mttApi::Torrent::State mtt::Torrent::getState() const
{
	if (files.checking)
		return mttApi::Torrent::State::CheckingFiles;
	if (stopping)
		return mttApi::Torrent::State::Stopping;
	if (utmDl && utmDl->state.active)
		return mttApi::Torrent::State::DownloadingMetadata;
	if (!started)
		return lastError == Status::Success ? mttApi::Torrent::State::Stopped : mttApi::Torrent::State::Interrupted;

	return mttApi::Torrent::State::Active;
}

bool mtt::Torrent::isActive() const
{
	return started && !stopping;
}

mttApi::Torrent::TimePoint mtt::Torrent::getStateTimestamp() const
{
	return activityTime;
}

void mtt::Torrent::initialize()
{
	stateChanged = true;
	files.initialize();
}

mtt::Status mtt::Torrent::start()
{
	std::unique_lock<std::mutex> guard(stateMutex);

#ifdef PEER_DIAGNOSTICS
	std::filesystem::path logDir = Storage::utf8Path(".\\logs\\" + name());
	if (!std::filesystem::exists(logDir))
	{
		std::error_code ec;
		std::filesystem::create_directory(logDir, ec);
	}
#endif

	if (getState() == mttApi::Torrent::State::DownloadingMetadata)
	{
		started = true;
		return Status::Success;
	}

	if (utmDl && !utmDl->state.finished)
	{
		downloadMetadata();
		return Status::Success;
	}

	if (loadFileInfo())
		lastError = files.start();

	if (lastError != mtt::Status::Success)
	{
		guard.unlock();
		stop(StopReason::Internal);
		return lastError;
	}

	service.start(5);

	started = true;
	stateChanged = true;
	activityTime = TimeClock::now();

	if (!files.checking)
		fileTransfer->start();

	return Status::Success;
}

void mtt::Torrent::stop(StopReason reason)
{
	if (stopping)
		return;

	std::lock_guard<std::mutex> guard(stateMutex);

	files.stop();

	if (utmDl)
	{
		utmDl->stop();
	}

	activityTime = TimeClock::now();

	if (!started)
	{
		save();

		return;
	}

	stopping = true;
	fileTransfer->stop();

	if (reason != StopReason::Internal)
		service.stop();

	if (reason == StopReason::Manual)
		started = false;

	save();
	started = false;

	if (reason != StopReason::Internal)
		lastError = Status::Success;

	stopping = false;
	activityTime = TimeClock::now();
}

bool mtt::Torrent::finished() const
{
	return files.progress.finished();
}

bool mtt::Torrent::selectionFinished() const
{
	return files.progress.selectedFinished();
}

mtt::Timestamp mtt::Torrent::getTimeAdded() const
{
	return addedTime;
}

const uint8_t* mtt::Torrent::hash() const
{
	return infoFile.info.hash;
}

std::string mtt::Torrent::hashString() const
{
	return hexToString(infoFile.info.hash, 20);
}

const std::string& mtt::Torrent::name() const
{
	return infoFile.info.name;
}

float mtt::Torrent::progress() const
{
	float progress = files.progress.getPercentage();

	if (infoFile.info.pieceSize)
	{
		float unfinishedPieces = fileTransfer->downloader.getUnfinishedPiecesDownloadSize() / (float)infoFile.info.pieceSize;
		progress += unfinishedPieces / files.progress.pieces.size();
	}

	if (progress > 1.0f)
		progress = 1.0f;

	return progress;
}

float mtt::Torrent::selectionProgress() const
{
	if (files.progress.selectedPieces == files.progress.pieces.size())
		return progress();

	float progress = files.progress.getSelectedPercentage();

	if (infoFile.info.pieceSize)
	{
		auto unfinishedPieces = fileTransfer->downloader.getUnfinishedPiecesDownloadSizeMap();

		uint32_t unfinishedSize = 0;
		for (auto& p : unfinishedPieces)
		{
			if (files.progress.selectedPiece(p.first))
				unfinishedSize += p.second;
		}

		if (files.progress.selectedPieces)
			progress += unfinishedSize / (float)(infoFile.info.pieceSize * files.progress.selectedPieces);
	}

	if (progress > 1.0f)
		progress = 1.0f;

	return progress;
}

uint64_t mtt::Torrent::downloaded() const
{
	return files.progress.getReceivedBytes(infoFile.info.pieceSize, infoFile.info.fullSize) + fileTransfer->downloader.getUnfinishedPiecesDownloadSize();
}

size_t mtt::Torrent::downloadSpeed() const
{
	return fileTransfer->getDownloadSpeed();
}

uint64_t mtt::Torrent::uploaded() const
{
	return fileTransfer->uploader->uploaded;
}

size_t mtt::Torrent::uploadSpeed() const
{
	return fileTransfer->getUploadSpeed();
}

uint64_t mtt::Torrent::receivedBytes() const
{
	return fileTransfer->downloader.downloaded;
}

uint64_t mtt::Torrent::dataLeft() const
{
	return infoFile.info.fullSize - downloaded();
}
