#include "Torrent.h"
#include "utils/TorrentFileParser.h"
#include "MetadataDownload.h"
#include "Peers.h"
#include "Configuration.h"
#include "FileTransfer.h"
#include "State.h"
#include "utils/HexEncoding.h"
#include <filesystem>
#include "AlertsManager.h"

mtt::TorrentPtr mtt::Torrent::fromFile(mtt::TorrentFileInfo& fileInfo)
{
	mtt::TorrentPtr torrent = std::make_shared<Torrent>();
	torrent->infoFile = std::move(fileInfo);

	if (!torrent->infoFile.info.name.empty())
	{
		torrent->peers = std::make_shared<Peers>(torrent);
		torrent->fileTransfer = std::make_shared<FileTransfer>(torrent);
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

	torrent->peers = std::make_shared<Peers>(torrent);
	torrent->fileTransfer = std::make_shared<FileTransfer>(torrent);

	return torrent;
}

mtt::TorrentPtr mtt::Torrent::fromSavedState(std::string name)
{
	DataBuffer buffer;
	if (!Torrent::loadSavedTorrentFile(name, buffer))
		return nullptr;

	if (auto ptr = fromFile(mtt::TorrentFileParser::parse(buffer.data(), buffer.size())))
	{
		TorrentState state(ptr->files.progress.pieces);
		if (state.load(name))
		{
			ptr->files.storage.init(ptr->infoFile.info, state.downloadPath);

			if (ptr->files.selection.files.size() == state.files.size())
			{
				for (size_t i = 0; i < state.files.size(); i++)
				{
					auto& selection = ptr->files.selection.files[i];
					selection.selected = state.files[i].selected;
					selection.priority = state.files[i].priority;
				}
			}

			ptr->lastStateTime = state.lastStateTime;
			auto fileTime = ptr->files.storage.getLastModifiedTime();
			if (fileTime == 0)
				ptr->lastStateTime = 0;

			bool checked = ptr->lastStateTime != 0 && ptr->lastStateTime == fileTime;

			if (checked)
				ptr->files.progress.recheckPieces();
			else
				ptr->files.progress.removeReceived();

			if (state.started)
				ptr->start();
		}

		return ptr;
	}
	else
		TorrentState::remove(name);

	return nullptr;
}

void mtt::Torrent::save()
{
	if (!stateChanged)
		return;

	TorrentState saveState(files.progress.pieces);
	saveState.downloadPath = files.storage.getPath();
	saveState.lastStateTime = lastStateTime = files.storage.getLastModifiedTime();
	saveState.started = state == State::Started;

	for (auto& f : files.selection.files)
		saveState.files.push_back({ f.selected, f.priority });

	saveState.save(hashString());

	stateChanged = saveState.started;
}

void mtt::Torrent::saveTorrentFile(const char* data, size_t size)
{
	auto folderPath = mtt::config::getInternal().stateFolder + "\\" + hashString() + ".torrent";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	file.write(data, size);
}

void mtt::Torrent::saveTorrentFileFromUtm()
{
	if (utmDl)
	{
		auto fileData = infoFile.createTorrentFileData(utmDl->metadata.buffer.data(), utmDl->metadata.buffer.size());
		saveTorrentFile(fileData.data(), fileData.size());
	}
}

void mtt::Torrent::removeMetaFiles()
{
	auto path = mtt::config::getInternal().stateFolder + "\\" + hashString();
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
			auto loadedFile = mtt::TorrentFileParser::parse(buffer.data(), buffer.size());
			
			if (!loadedFile.info.name.empty())
			{
				auto newFile = infoFile.createTorrentFileData((const uint8_t*)loadedFile.file.infoStart, loadedFile.file.infoSize);
				saveTorrentFile(newFile.data(), newFile.size());
			}
		}

		peers->trackers.addTrackers(added);
	}

	return !added.empty();
}

bool mtt::Torrent::loadSavedTorrentFile(const std::string& hash, DataBuffer& out)
{
	auto filename = mtt::config::getInternal().stateFolder + "\\" + hash + ".torrent";
	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
		return false;

	out = DataBuffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

	return !out.empty();
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
			peers->reloadTorrentInfo();
			stateChanged = true;
			init();

			AlertsManager::Get().metadataAlert(AlertId::MetadataFinished, hash());
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
	AlertsManager::Get().torrentAlert(AlertId::TorrentAdded, hash());
}

bool mtt::Torrent::start()
{
#ifdef PEER_DIAGNOSTICS
	std::filesystem::path logDir = std::filesystem::u8path(".\\logs\\" + name());
	if (!std::filesystem::exists(logDir))
	{
		std::error_code ec;
		std::filesystem::create_directory(logDir, ec);
	}
#endif

	if (state == State::DownloadUtm)
	{
		state = State::Started;
		return true;
	}

	if (!checking)
	{
		auto filesTime = files.storage.getLastModifiedTime();

		if (filesTime != lastStateTime)
		{
			if (filesTime == 0)
			{
				files.progress.removeReceived();
				lastStateTime = 0;
			}
			else
				checkFiles([this](std::shared_ptr<PiecesCheck> ch)
					{
						if (!ch->rejected)
							start();
					});
		}

		files.progress.select(files.selection);
	}

	lastError = Status::E_InvalidInput;

	if (files.selection.files.empty())
		return false;

	lastError = files.prepareSelection();

	if (lastError != mtt::Status::Success)
		return false;

	service.start(2);

	state = State::Started;
	stateChanged = true;

	if (checking)
		return true;

	fileTransfer->start();

	return true;
}

void mtt::Torrent::stop()
{
	if (checking)
	{
		std::lock_guard<std::mutex> guard(checkStateMutex);

		if (checkState)
			checkState->rejected = true;

		checking = false;
	}

	if (state == mttApi::Torrent::State::Stopped)
		return;

	if (utmDl)
	{
		utmDl->stop();
	}

	if (fileTransfer)
	{
		fileTransfer->stop();
	}

	service.stop();

	state = State::Stopped;
	lastError = Status::Success;
	stateChanged = true;
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
			files.progress.select(files.selection);
			lastStateTime = files.storage.getLastModifiedTime();

			if (state == State::Started)
				start();
		}

		onFinish(check);
	};

	checking = true;
	std::lock_guard<std::mutex> guard(checkStateMutex);
	checkState = files.storage.checkStoredPiecesAsync(infoFile.info.pieces, service.io, checkFunc);
	return checkState;
}

void mtt::Torrent::checkFiles()
{
	if(!checking)
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

bool mtt::Torrent::selectFiles(const std::vector<bool>& s)
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

const uint8_t* mtt::Torrent::hash()
{
	return infoFile.info.hash;
}

std::string mtt::Torrent::hashString()
{
	return hexToString(infoFile.info.hash, 20);
}

std::string mtt::Torrent::name()
{
	return infoFile.info.name;
}

float mtt::Torrent::currentProgress()
{
	float progress = files.progress.getPercentage();

	if (fileTransfer && infoFile.info.pieceSize)
	{
		float unfinishedPieces = fileTransfer->getUnfinishedPiecesDownloadSize() / (float)infoFile.info.pieceSize;
		progress += unfinishedPieces / files.progress.pieces.size();
	}

	return progress;
}

float mtt::Torrent::currentSelectionProgress()
{
	float progress = files.progress.getSelectedPercentage();

	if (fileTransfer)
	{
		float unfinishedPieces = fileTransfer->getUnfinishedPiecesDownloadSize() / (float)infoFile.info.pieceSize;
		if(files.progress.selectedPieces)
			progress += unfinishedPieces / files.progress.selectedPieces;
	}

	return progress;
}

size_t mtt::Torrent::downloaded()
{
	return (size_t)(infoFile.info.fullSize * (double)files.progress.getPercentage()) + (fileTransfer ? fileTransfer->getUnfinishedPiecesDownloadSize() : 0);
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

void mtt::Torrent::setFilesPriority(const std::vector<mtt::Priority>& priority)
{
	if (files.selection.files.size() != priority.size())
		return;

	for (size_t i = 0; i < priority.size(); i++)
	{
		files.selection.files[i].priority = priority[i];
	}

	if (fileTransfer)
		fileTransfer->updatePiecesPriority();

	stateChanged = true;
}

mtt::Status mtt::Torrent::setLocationPath(const std::string& path)
{
	return files.storage.setPath(path, lastStateTime != 0);
}
