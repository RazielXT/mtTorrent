#include "Files.h"
#include "Torrent.h"
#include "Configuration.h"
#include "AlertsManager.h"
#include "FileTransfer.h"

mtt::Files::Files(Torrent& t) : torrent(t), storage(t.infoFile.info)
{
}

void mtt::Files::initialize()
{
	auto& info = torrent.infoFile.info;
	selection.clear();
	for (auto& f : info.files)
	{
		selection.push_back({ true, PriorityNormal });
	}

	progress.init(info.pieces.size());
}

void mtt::Files::initialize(TorrentState& state)
{
	storage.init(state.downloadPath);
	lastFileTime = state.lastStateTime;

	selection = std::move(state.selection);

	progress.calculatePieces();
}

mtt::Status mtt::Files::start()
{
	if (checking)
		return Status::Success;

	const auto& files = torrent.getFilesInfo();

	std::vector<bool> wantedChecks;
	for (size_t i = 0; i < files.size(); i++)
	{
		auto fileTime = storage.getLastModifiedTime(i);

		if (lastFileTime < fileTime)
		{
			if (wantedChecks.empty())
				wantedChecks.resize(progress.pieces.size());

			for (uint32_t p = files[i].startPieceIndex; p <= files[i].endPieceIndex; p++)
				wantedChecks[p] = true;
		}

		if (!fileTime)
		{
			for (uint32_t p = files[i].startPieceIndex; p <= files[i].endPieceIndex; p++)
				progress.removePiece(p);
		}
	}

	if (!wantedChecks.empty())
	{
		lastFileTime = 0;
		checkFiles(wantedChecks);
		return Status::Success;
	}

	if (!torrent.selectionFinished())
	{
		auto s = storage.preallocateSelection(selection);
		if (s != Status::Success)
			return s;
		lastFileTime = std::max(lastFileTime, storage.lastAllocationTime);
	}

	return Status::Success;
}

void mtt::Files::stop()
{
	if (checking)
	{
		std::lock_guard<std::mutex> guard(checkStateMutex);

		if (checkState)
			checkState->rejected = true;

		checking = false;
	}
}

void mtt::Files::select(uint32_t idx, bool selected)
{
	if (selection[idx].selected != selected)
	{
		selection[idx].selected = selected;

		auto& files = torrent.infoFile.info.files;
		uint32_t startPieceIndex = files[idx].startPieceIndex;
		uint32_t endPieceIndex = files[idx].endPieceIndex;

		if (!selected)
		{
			//keep start if selected by other file
			{
				int i = int(idx) - 1;
				while (i >= 0 && files[i].endPieceIndex == startPieceIndex)
				{
					if (selection[i].selected)
					{
						startPieceIndex++;
						break;
					}
					i--;
				}
			}
			//keep end if selected by other file
			{
				uint32_t i = idx + 1;
				while (i < files.size() && files[i].startPieceIndex == endPieceIndex)
				{
					if (selection[i].selected)
					{
						if (endPieceIndex == 0)
							startPieceIndex++; //force skip all
						else
							endPieceIndex--;

						break;
					}
					i--;
				}
			}
		}

		progress.select(startPieceIndex, endPieceIndex, selected);
	}
}

std::vector<mtt::FileSelection> mtt::Files::getFilesSelection() const
{
	return selection;
}

mtt::Status mtt::Files::selectFiles(const std::vector<bool>& s)
{
	auto& files = torrent.getFilesInfo();

	if (files.size() < s.size())
		return Status::E_InvalidInput;

	for (size_t i = 0; i < s.size(); i++)
		select((uint32_t)i, s[i]);

	if (torrent.started)
	{
		auto s = storage.preallocateSelection(selection);
		if (s != Status::Success)
		{
			torrent.lastError = s;
			torrent.stop(Torrent::StopReason::Internal);
			return s;
		}
		lastFileTime = std::max(lastFileTime, storage.lastAllocationTime);
	}

	torrent.refreshSelection();

	return Status::Success;
}

mtt::Status mtt::Files::selectFile(uint32_t index, bool selected)
{
	auto& files = torrent.getFilesInfo();

	if (files.size() <= index)
		return Status::E_InvalidInput;

	select(index, selected);

	if (torrent.started && selected)
	{
		auto s = storage.preallocateFile(index);
		if (s != Status::Success)
		{
			torrent.lastError = s;
			torrent.stop(Torrent::StopReason::Internal);
			return s;
		}
		lastFileTime = std::max(lastFileTime, storage.lastAllocationTime);
	}

	torrent.refreshSelection();

	return Status::Success;
}

void mtt::Files::setFilesPriority(const std::vector<mtt::Priority>& priority)
{
	if (selection.size() != priority.size())
		return;

	for (size_t i = 0; i < priority.size(); i++)
		selection[i].priority = priority[i];

	torrent.refreshSelection();
}

void mtt::Files::setFilePriority(uint32_t index, mtt::Priority priority)
{
	if (selection.size() <= index)
		return;

	if (selection[index].priority != priority)
	{
		selection[index].priority = priority;

		torrent.refreshSelection();
	}
}

uint64_t mtt::Files::getLastModifiedTime()
{
	lastFileTime = storage.getLastModifiedTime();
	return lastFileTime;
}

std::vector<std::pair<float, uint32_t>> mtt::Files::getFilesProgress()
{
	const auto& files = torrent.getFilesInfo();

	if (files.empty())
		return {};

	std::vector<std::pair<float, uint32_t>> out;
	out.reserve(files.size());

	auto unfinished = torrent.fileTransfer->downloader.getUnfinishedPiecesDownloadSizeMap();
	auto pieceSize = (float)torrent.infoFile.info.pieceSize;

	for (const auto& file : files)
	{
		uint32_t received = 0;
		uint32_t unfinishedSize = 0;
		for (uint32_t p = file.startPieceIndex; p <= file.endPieceIndex; p++)
		{
			if (progress.hasPiece(p))
				received++;
			else
			{
				if (auto u = unfinished.find(p); u != unfinished.end())
					unfinishedSize += u->second;
			}
		}

		uint32_t pieces = 1 + file.endPieceIndex - file.startPieceIndex;
		auto receivedWhole = (float)received;
		if (unfinishedSize)
			receivedWhole += unfinishedSize / pieceSize;

		out.emplace_back(receivedWhole / pieces, received);
	}

	return out;
}

std::vector<uint64_t> mtt::Files::getFilesAllocatedSize()
{
	torrent.loadFileInfo();
	return storage.getAllocatedSize();
}

std::string mtt::Files::getLocationPath() const
{
	return storage.getPath();
}

mtt::Status mtt::Files::setLocationPath(const std::string& path, bool moveFiles)
{
	auto status = storage.setPath(path, moveFiles);

	if (status == Status::Success)
	{
		torrent.refreshSelection();

		if (!moveFiles)
			checkFiles();
	}

	return status;
}

mtt::PathValidation mtt::Files::validateCurrentPath() const
{
	return storage.validatePath(selection);
}

mtt::PathValidation mtt::Files::validatePath(const std::string& path) const
{
	return storage.validatePath(selection, path);
}

std::size_t mtt::Files::getPiecesCount() const
{
	return progress.pieces.size();
}

std::size_t mtt::Files::getSelectedPiecesCount() const
{
	return progress.selectedPieces;
}

std::size_t mtt::Files::getReceivedPiecesCount() const
{
	return progress.getReceivedPiecesCount();
}

bool mtt::Files::getPiecesBitfield(mtt::Bitfield& data) const
{
	if (getPiecesCount() == 0)
		return false;

	progress.toBitfield(data);
	return true;
}

float mtt::Files::checkingProgress() const
{
	std::lock_guard<std::mutex> guard(checkStateMutex);

	if (checkState)
		return checkState->piecesChecked / (float)checkState->piecesCount;

	return 1;
}

void mtt::Files::checkFiles()
{
	if (checking)
		return;

	std::vector<bool> wantedChecks(progress.pieces.size(), true);
	checkFiles(wantedChecks);
}

void mtt::Files::checkFiles(const std::vector<bool>& wantedChecks)
{
	std::lock_guard<std::mutex> guard(checkStateMutex);

	if (checking)
		return;

	checking = true;

	auto request = checkState = std::make_shared<mtt::PiecesCheck>(progress);
	request->piecesCount = (uint32_t)progress.pieces.size();

	auto localOnFinish = [this, request]()
	{
		{
			std::lock_guard<std::mutex> guard(checkStateMutex);
			checking = false;
			checkState.reset();
		}

		if (!request->rejected)
			lastFileTime = storage.getLastModifiedTime();

		torrent.refreshAfterChecking(*request);
	};

	torrent.prepareForChecking();
	progress.removeReceived(wantedChecks);
	if (!lastFileTime)
		torrent.fileTransfer->downloader.unfinishedPieces.clear();

	const uint32_t WorkersCount = 4;
	torrent.service.start(WorkersCount);

	auto finished = std::make_shared<uint32_t>(0);

	for (uint32_t i = 0; i < WorkersCount; i++)
		torrent.service.post([WorkersCount, i, finished, localOnFinish, request, wantedChecks, this]()
			{
				storage.checkStoredPieces(*request, torrent.infoFile.info.pieces, WorkersCount, i, wantedChecks);

				if (++(*finished) == WorkersCount)
					localOnFinish();
			});
}
