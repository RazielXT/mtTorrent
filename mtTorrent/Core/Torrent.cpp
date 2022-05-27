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

mtt::Torrent::Torrent() : service(0), files(infoFile.info)
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
	torrent->initialize();

	torrent->peers = std::make_shared<Peers>(torrent);
	torrent->fileTransfer = std::make_shared<FileTransfer>(torrent);
	torrent->addedTime = (uint64_t)time(0);

	return torrent;
}

mtt::TorrentPtr mtt::Torrent::fromMagnetLink(std::string link)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();

	if (torrent->infoFile.parseMagnetLink(link) != Status::Success)
		return nullptr;

	torrent->peers = std::make_shared<Peers>(torrent);
	torrent->fileTransfer = std::make_shared<FileTransfer>(torrent);
	torrent->addedTime = (uint64_t)time(0);

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
		if(!torrent->loadFileInfo())
			return nullptr;

		torrent->stateChanged = true;
	}

	torrent->files.initialize(state.selection, state.downloadPath);
	torrent->lastFileTime = state.lastStateTime;
	torrent->addedTime = state.addedTime;

	if (torrent->addedTime == 0)
		torrent->addedTime = (int64_t)time(0);

	torrent->peers = std::make_shared<Peers>(torrent);
	torrent->fileTransfer = std::make_shared<FileTransfer>(torrent);

	torrent->fileTransfer->getDownloadSum() = state.downloaded;
	torrent->fileTransfer->getUploadSum() = state.uploaded;
	torrent->fileTransfer->addUnfinishedPieces(state.unfinishedPieces);

	if (state.version == 0)
	{
		torrent->loadFileInfo();
		if (torrent->fileTransfer->getDownloadSum() == 0)
		{
			torrent->fileTransfer->getDownloadSum() = torrent->downloaded();

			if (torrent->fileTransfer->getDownloadSum())
				torrent->stateChanged = true;
		}
	}

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
	saveState.lastStateTime = lastFileTime = files.storage.getLastModifiedTime();
	saveState.addedTime = addedTime;
	saveState.started = state == ActiveState::Started;
	saveState.uploaded = fileTransfer->getUploadSum();
	saveState.downloaded = fileTransfer->getDownloadSum();

	if (fileTransfer)
		saveState.unfinishedPieces = std::move(fileTransfer->getUnfinishedPiecesState());

	for (auto& f : files.selection)
		saveState.selection.push_back({ f.selected, f.priority });

	saveState.save(hashString());

	stateChanged = saveState.started;
}

void mtt::Torrent::saveTorrentFile(const char* data, size_t size)
{
	auto folderPath = mtt::config::getInternal().stateFolder + pathSeparator + hashString() + ".torrent";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	file.write(data, size);
}

void mtt::Torrent::removeMetaFiles()
{
	auto path = mtt::config::getInternal().stateFolder + pathSeparator + hashString();
	std::remove((path + ".torrent").data());
	std::remove((path + ".state").data());
}

bool mtt::Torrent::importTrackers(const mtt::TorrentFileInfo& otherFileInfo)
{
	std::vector<std::string> added;

	for (auto& t : otherFileInfo.announceList)
	{
		if (std::find(infoFile.announceList.begin(), infoFile.announceList.end(), t) == infoFile.announceList.end())
		{
			added.push_back(t);
			infoFile.announceList.push_back(t);
		}
	}

	if (!added.empty())
	{
		if(infoFile.announce.empty())
			infoFile.announce = infoFile.announceList.front();

		DataBuffer buffer;
		if (loadSavedTorrentFile(hashString(), buffer))
		{
			TorrentFileParser::ParsedInfo parseInfo;
			auto loadedFile = mtt::TorrentFileParser::parse(buffer.data(), buffer.size(), &parseInfo);
			
			if (!loadedFile.info.name.empty())
			{
				auto newFile = infoFile.createTorrentFileData((const uint8_t*)parseInfo.infoStart, parseInfo.infoSize);
				saveTorrentFile(newFile.data(), newFile.size());
			}
		}

		peers->trackers.addTrackers(added);
	}

	return !added.empty();
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

void mtt::Torrent::refreshLastState()
{
	if (!checking)
	{
		bool needRecheck = false;
		uint64_t filesTime = 0;

		const auto& tfiles = infoFile.info.files;
		for (size_t i = 0; i < tfiles.size(); i++)
		{
			auto fileTime = files.storage.getLastModifiedTime(i);

			if (filesTime < fileTime)
				filesTime = fileTime;

			bool checkFile = !lastFileTime;
			if (!fileTime && files.progress.hasPiece(tfiles[i].startPieceIndex) && tfiles[i].size != 0)
				needRecheck = true;
		}

		if (filesTime != lastFileTime || needRecheck)
		{
			fileTransfer->clearUnfinishedPieces();

			if (filesTime == 0)
				lastFileTime = 0;

			checkFiles();
		}
	}
}

void mtt::Torrent::downloadMetadata()
{
	activityTime = TimeClock::now();
	service.start(4);

	if(!utmDl)
		utmDl = std::make_shared<MetadataDownload>(*peers, service);

	utmDl->start([this](Status s, MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished && infoFile.info.files.empty())
		{
			infoFile.info = utmDl->metadata.getRecontructedInfo();
			loadedState = LoadedState::Full;

			auto fileData = infoFile.createTorrentFileData(utmDl->metadata.buffer.data(), utmDl->metadata.buffer.size());
			saveTorrentFile(fileData.data(), fileData.size());

			initialize();
			peers->reloadTorrentInfo();

			AlertsManager::Get().metadataAlert(AlertId::MetadataFinished, this);

			if (this->state == ActiveState::Started)
				service.io.post([this]() { start(); });
		}
	});
}

mttApi::Torrent::State mtt::Torrent::getState() const
{
	if (checking)
		return mttApi::Torrent::State::CheckingFiles;
	else if (stopping)
		return mttApi::Torrent::State::Stopping;
	else if (utmDl && utmDl->state.active)
		return mttApi::Torrent::State::DownloadingMetadata;
	else if (state == mttApi::Torrent::ActiveState::Stopped)
		return lastError == Status::Success ? mttApi::Torrent::State::Inactive : mttApi::Torrent::State::Interrupted;
	else if (finished())
		return mttApi::Torrent::State::Seeding;
	else
		return mttApi::Torrent::State::Downloading;
}

mttApi::Torrent::TimePoint mtt::Torrent::getActiveTimestamp() const
{
	return activityTime;
}

void mtt::Torrent::initialize()
{
	stateChanged = true;
	files.setDefaults(infoFile.info);
	AlertsManager::Get().torrentAlert(AlertId::TorrentAdded, this);
}

mtt::Status mtt::Torrent::start()
{
	std::unique_lock<std::mutex> guard(stateMutex);

#ifdef PEER_DIAGNOSTICS
	std::filesystem::path logDir = std::filesystem::u8path(".\\logs\\" + name());
	if (!std::filesystem::exists(logDir))
	{
		std::error_code ec;
		std::filesystem::create_directory(logDir, ec);
	}
#endif

	if (getState() == mttApi::Torrent::State::DownloadingMetadata)
	{
		state = ActiveState::Started;
		return Status::Success;
	}

	if (utmDl && !utmDl->state.finished)
	{
		downloadMetadata();
		return Status::Success;
	}

	if (loadFileInfo())
	{
		refreshLastState();

		if (!checking)
		{
			lastError = files.prepareSelection();

			if (lastError == mtt::Status::Success)
				lastFileTime = files.storage.getLastModifiedTime();
		}
	}

	if (lastError != mtt::Status::Success)
	{
		guard.unlock();
		stop(StopReason::Internal);
		return lastError;
	}

	service.start(5);

	state = ActiveState::Started;
	stateChanged = true;
	activityTime = TimeClock::now();

	if (!checking)
		fileTransfer->start();

	return Status::Success;
}

void mtt::Torrent::stop(StopReason reason)
{
	if (stopping)
		return;

	std::lock_guard<std::mutex> guard(stateMutex);

	if (checking)
	{
		std::lock_guard<std::mutex> guard(checkStateMutex);

		if (checkState)
			checkState->rejected = true;

		checking = false;
	}

	if (utmDl)
	{
		utmDl->stop();
	}

	if (state == mttApi::Torrent::ActiveState::Stopped)
	{
		save();
		return;
	}

	stopping = true;

	if (fileTransfer)
	{
		fileTransfer->stop();
	}

	if (reason != StopReason::Internal)
		service.stop();

	if(reason == StopReason::Manual)
		state = ActiveState::Stopped;

	save();

	state = ActiveState::Stopped;
	if(reason != StopReason::Internal)
		lastError = Status::Success;
	stateChanged = true;
	stopping = false;
}

void mtt::Torrent::checkFiles(bool all)
{
	if (checking)
		return;

	if (all)
	{
		lastFileTime = 0;
	}

	if (state == mttApi::Torrent::ActiveState::Stopped)
	{
		activityTime = TimeClock::now();
	}

	auto request = std::make_shared<mtt::PiecesCheck>(files.progress);
	request->piecesCount = (uint32_t)files.progress.pieces.size();

	std::vector<bool> wantedChecks(infoFile.info.pieces.size());
	const auto& tfiles = infoFile.info.files;
	for (size_t i = 0; i < tfiles.size(); i++)
	{
		bool checkFile = !lastFileTime;
		if (!checkFile)
		{
			auto fileTime = files.storage.getLastModifiedTime(i);

			if (lastFileTime < fileTime || (!fileTime && files.progress.selectedPiece(tfiles[i].startPieceIndex)))
				checkFile = true;
		}

		if (checkFile)
		{
			for (uint32_t p = tfiles[i].startPieceIndex; p <= tfiles[i].endPieceIndex; p++)
				wantedChecks[p] = true;
		}
	}

	files.progress.removeReceived(wantedChecks);

	auto localOnFinish = [this, request]()
	{
		checking = false;

		{
			std::lock_guard<std::mutex> guard(checkStateMutex);
			checkState.reset();
		}

		if (!request->rejected)
		{
			files.progress.select(infoFile.info, files.selection);
			lastFileTime = files.storage.getLastModifiedTime();
			stateChanged = true;

			if (state == ActiveState::Started)
				start();
			else
				stop();
		}
		else
			lastFileTime = 0;
	};

	checking = true;
	lastError = Status::Success;

	loadFileInfo();

	const uint32_t WorkersCount = 4;
	service.start(WorkersCount);

	auto finished = std::make_shared<uint32_t>(0);

	for (uint32_t i = 0; i < WorkersCount; i++)
		service.io.post([WorkersCount, i, finished, localOnFinish, request, wantedChecks, this]()
			{
				files.storage.checkStoredPieces(*request.get(), infoFile.info.pieces, WorkersCount, i, wantedChecks);

				if (++(*finished) == WorkersCount)
					localOnFinish();
			});

	{
		std::lock_guard<std::mutex> guard(checkStateMutex);
		checkState = request;
	}
}

float mtt::Torrent::checkingProgress() const
{
	std::lock_guard<std::mutex> guard(checkStateMutex);

	if (checkState)
		return checkState->piecesChecked / (float)checkState->piecesCount;
	else
		return 1;
}

bool mtt::Torrent::selectFiles(const std::vector<bool>& s)
{
	loadFileInfo();

	if (!files.select(infoFile.info, s))
		return false;

	stateChanged = true;

	if (state == ActiveState::Started)
	{
		lastError = files.prepareSelection();

		if (fileTransfer)
			fileTransfer->refreshSelection();

		return lastError == Status::Success;
	}

	return true;
}

bool mtt::Torrent::selectFile(uint32_t index, bool selected)
{
	loadFileInfo();

	if (!files.select(infoFile.info, index, selected))
		return false;

	if (state == ActiveState::Started)
	{
		lastError = files.prepareSelection();

		if (fileTransfer)
			fileTransfer->refreshSelection();

		return lastError == Status::Success;
	}

	return true;
}

void mtt::Torrent::setFilesPriority(const std::vector<mtt::Priority>& priority)
{
	loadFileInfo();

	if (files.selection.size() != priority.size())
		return;

	for (size_t i = 0; i < priority.size(); i++)
		files.selection[i].priority = priority[i];

	if (state == ActiveState::Started)
	{
		if (fileTransfer)
			fileTransfer->refreshSelection();
	}

	stateChanged = true;
}

bool mtt::Torrent::finished() const
{
	return files.progress.getPercentage() == 1;
}

bool mtt::Torrent::selectionFinished() const
{
	return files.progress.getSelectedPercentage() == 1;
}

uint64_t mtt::Torrent::getTimeAdded() const
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

	if (fileTransfer && infoFile.info.pieceSize)
	{
		float unfinishedPieces = fileTransfer->getUnfinishedPiecesDownloadSize() / (float)infoFile.info.pieceSize;
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

	if (fileTransfer && infoFile.info.pieceSize)
	{
		auto unfinishedPieces = fileTransfer->getUnfinishedPiecesDownloadSizeMap();

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
	return files.progress.getReceivedBytes(infoFile.info.pieceSize, infoFile.info.fullSize) + (fileTransfer ? fileTransfer->getUnfinishedPiecesDownloadSize() : 0);
}

size_t mtt::Torrent::downloadSpeed() const
{
	return fileTransfer ? fileTransfer->getDownloadSpeed() : 0;
}

uint64_t mtt::Torrent::uploaded() const
{
	return fileTransfer ? fileTransfer->getUploadSum() : 0;
}

size_t mtt::Torrent::uploadSpeed() const
{
	return fileTransfer ? fileTransfer->getUploadSpeed() : 0;
}

uint64_t mtt::Torrent::receivedBytes() const
{
	return fileTransfer ? fileTransfer->getDownloadSum() : 0;
}

uint64_t mtt::Torrent::dataLeft() const
{
	return infoFile.info.fullSize - downloaded();
}

mtt::Status mtt::Torrent::setLocationPath(const std::string& path, bool moveFiles)
{
	stateChanged = true;

	auto status = files.storage.setPath(path, moveFiles);

	if (!moveFiles)
		checkFiles(true);

	return status;
}
