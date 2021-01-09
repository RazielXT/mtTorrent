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

void mttApi::Torrent::setFilesPriority(const std::vector<mtt::Priority>& p)
{
	return static_cast<mtt::Torrent*>(this)->setFilesPriority(p);
}

mtt::DownloadSelection mttApi::Torrent::getFilesSelection() const
{
	return static_cast<const mtt::Torrent*>(this)->files.selection;
}

std::string mttApi::Torrent::getLocationPath() const
{
	return static_cast<const mtt::Torrent*>(this)->files.storage.getPath();
}

mtt::Status mttApi::Torrent::setLocationPath(const std::string& path)
{
	return static_cast<mtt::Torrent*>(this)->setLocationPath(path);
}

const std::string& mttApi::Torrent::name() const
{
	return static_cast<const mtt::Torrent*>(this)->name();
}

float mttApi::Torrent::currentProgress() const
{
	return static_cast<const mtt::Torrent*>(this)->currentProgress();
}

float mttApi::Torrent::currentSelectionProgress() const
{
	return static_cast<const mtt::Torrent*>(this)->currentSelectionProgress();
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

const mtt::TorrentFileInfo& mttApi::Torrent::getFileInfo() const
{
	return static_cast<const mtt::Torrent*>(this)->infoFile;
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

bool mttApi::Torrent::getPiecesBitfield(uint8_t* dataBitfield, size_t dataSize)
{
	return static_cast<mtt::Torrent*>(this)->files.progress.toBitfield(dataBitfield, dataSize);
}

bool mttApi::Torrent::getReceivedPieces(uint32_t* dataPieces, size_t& dataSize)
{
	auto& progress = static_cast<mtt::Torrent*>(this)->files.progress;

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

std::vector<float> mttApi::Torrent::getFilesProgress() const
{
	auto& selection = static_cast<const mtt::Torrent*>(this)->files.selection;
	auto& progress = static_cast<const mtt::Torrent*>(this)->files.progress;

	if (selection.files.empty())
		return {};


	std::vector<float> out;
	out.resize(selection.files.size());

	for (size_t i = 0; i < selection.files.size(); i++)
	{
		auto& file = selection.files[i].info;
		uint32_t piecesCount = file.endPieceIndex - file.startPieceIndex + 1;
		uint32_t receivedPieces = 0;

		for (uint32_t p = file.startPieceIndex; p <= file.endPieceIndex; p++)
		{
			if (progress.hasPiece(p))
				receivedPieces++;
		}

		out[i] = receivedPieces / (float)piecesCount;
	}

	return out;
}

std::vector<uint64_t> mttApi::Torrent::getFilesAllocatedSize() const
{
	return static_cast<const mtt::Torrent*>(this)->files.storage.getAllocatedSize();
}

