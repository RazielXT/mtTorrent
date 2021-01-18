#include "Torrent.h"
#include "Peers.h"
#include "FileTransfer.h"
#include "MetadataDownload.h"

mtt::Status mttApi::Torrent::start()
{
	return static_cast<mtt::Torrent*>(this)->start();
}

void mttApi::Torrent::stop()
{
	static_cast<mtt::Torrent*>(this)->stop();
}

mttApi::Torrent::ActiveState mttApi::Torrent::getActiveState() const
{
	return static_cast<const mtt::Torrent*>(this)->state;
}

mttApi::Torrent::State mttApi::Torrent::getState() const
{
	return static_cast<const mtt::Torrent*>(this)->getState();
}

mtt::Status mttApi::Torrent::getLastError() const
{
	return static_cast<const mtt::Torrent*>(this)->lastError;
}

float mttApi::Torrent::checkingProgress() const
{
	return static_cast<const mtt::Torrent*>(this)->checkingProgress();
}

void mttApi::Torrent::checkFiles()
{
	static_cast<mtt::Torrent*>(this)->checkFiles();
}

bool mttApi::Torrent::selectFiles(const std::vector<bool>& s)
{
	return static_cast<mtt::Torrent*>(this)->selectFiles(s);
}

bool mttApi::Torrent::selectFile(uint32_t index, bool selected)
{
	return static_cast<mtt::Torrent*>(this)->selectFile(index, selected);
}

void mttApi::Torrent::setFilesPriority(const std::vector<mtt::Priority>& p)
{
	return static_cast<mtt::Torrent*>(this)->setFilesPriority(p);
}

std::vector<mtt::FileSelection> mttApi::Torrent::getFilesSelection() const
{
	return static_cast<const mtt::Torrent*>(this)->files.selection;
}

std::string mttApi::Torrent::getLocationPath() const
{
	return static_cast<const mtt::Torrent*>(this)->files.storage.getPath();
}

mtt::Status mttApi::Torrent::setLocationPath(const std::string& path, bool moveFiles)
{
	return static_cast<mtt::Torrent*>(this)->setLocationPath(path, moveFiles);
}

const std::string& mttApi::Torrent::name() const
{
	return static_cast<const mtt::Torrent*>(this)->name();
}

const uint8_t* mttApi::Torrent::hash() const
{
	return static_cast<const mtt::Torrent*>(this)->hash();
}

float mttApi::Torrent::progress() const
{
	return static_cast<const mtt::Torrent*>(this)->progress();
}

float mttApi::Torrent::selectionProgress() const
{
	return static_cast<const mtt::Torrent*>(this)->selectionProgress();
}

uint64_t mttApi::Torrent::downloaded() const
{
	return static_cast<const mtt::Torrent*>(this)->downloaded();
}

uint64_t mttApi::Torrent::uploaded() const
{
	return static_cast<const mtt::Torrent*>(this)->uploaded();
}

bool mttApi::Torrent::finished() const
{
	return static_cast<const mtt::Torrent*>(this)->finished();
}

bool mttApi::Torrent::selectionFinished() const
{
	return static_cast<const mtt::Torrent*>(this)->selectionFinished();
}

const mtt::TorrentFileInfo& mttApi::Torrent::getFileInfo()
{
	auto torrent = static_cast<mtt::Torrent*>(this);
	torrent->loadFileInfo();

	return torrent->infoFile;
}

std::shared_ptr<mttApi::Peers> mttApi::Torrent::getPeers()
{
	return static_cast<mtt::Torrent*>(this)->peers;
}

std::shared_ptr<mttApi::FileTransfer> mttApi::Torrent::getFileTransfer()
{
	return std::static_pointer_cast<mttApi::FileTransfer>(static_cast<mtt::Torrent*>(this)->fileTransfer);
}

std::shared_ptr<mttApi::MagnetDownload> mttApi::Torrent::getMagnetDownload()
{
	return std::static_pointer_cast<mttApi::MagnetDownload>(static_cast<mtt::Torrent*>(this)->utmDl);
}

size_t mttApi::Torrent::getPiecesCount() const
{
	return static_cast<const mtt::Torrent*>(this)->files.progress.pieces.size();
}

bool mttApi::Torrent::getPiecesBitfield(uint8_t* dataBitfield, size_t& dataSize) const
{
	const auto& progress = static_cast<const mtt::Torrent*>(this)->files.progress;

	if (auto expected = progress.getBitfieldSize(); dataSize < expected)
	{
		dataSize = expected;
		return false;
	}

	return progress.toBitfield(dataBitfield, dataSize);
}

bool mttApi::Torrent::getReceivedPieces(uint32_t* dataPieces, size_t& dataSize) const
{
	auto& progress = static_cast<const mtt::Torrent*>(this)->files.progress;

	if (dataSize < progress.getReceivedPiecesCount() || !dataPieces)
	{
		dataSize = progress.getReceivedPiecesCount();
		return false;
	}

	size_t idx = 0;
	for (uint32_t i = 0; i < progress.pieces.size(); i++)
		if (progress.hasPiece(i))
			dataPieces[idx++] = i;

	dataSize = idx;
	return true;
}

std::vector<std::pair<float, uint32_t>> mttApi::Torrent::getFilesProgress()
{
	auto torrent = static_cast<mtt::Torrent*>(this);
	torrent->loadFileInfo();

	const auto& selection = torrent->files.selection;
	const auto& progress = torrent->files.progress;
	const auto& files = torrent->infoFile.info.files;

	if (selection.empty() || selection.size() != files.size())
		return {};

	std::vector<std::pair<float, uint32_t>> out;
	out.resize(selection.size());

	std::map<uint32_t, uint32_t> unfinished;
	if (torrent->fileTransfer)
		unfinished = torrent->fileTransfer->getUnfinishedPiecesDownloadSizeMap();

	float pieceSize = (float)torrent->infoFile.info.pieceSize;

	for (size_t i = 0; i < selection.size(); i++)
	{
		bool selected = selection[i].selected;
		auto& file = files[i];

		uint32_t received = 0;
		uint32_t unfinishedSize = 0;
		for (uint32_t p = file.startPieceIndex; p <= file.endPieceIndex; p++)
		{
			if (progress.hasPiece(p))
				received++;
			else if (selected)
			{
				if (auto u = unfinished.find(p); u != unfinished.end())
					unfinishedSize += u->second;
			}
		}

		uint32_t pieces = 1 + file.endPieceIndex - file.startPieceIndex;
		float receivedWhole = (float)received;
		if (unfinishedSize)
			receivedWhole += unfinishedSize / pieceSize;

		out[i] = { receivedWhole / pieces, received };
	}

	return std::move(out);
}

std::vector<uint64_t> mttApi::Torrent::getFilesAllocatedSize()
{
	auto torrent = static_cast<mtt::Torrent*>(this);
	torrent->loadFileInfo();

	return torrent->files.storage.getAllocatedSize();
}

