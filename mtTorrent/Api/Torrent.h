#pragma once

#include "Public/Status.h"
#include "Api/Interface.h"
#include "Api/FileTransfer.h"
#include "Api/Peers.h"
#include <memory>


namespace mttApi
{
	class Torrent
	{
	public:

		API_EXPORT bool start();
		API_EXPORT void stop();

		enum class State
		{
			Stopped,
			Started,
			DownloadUtm,
		};

		API_EXPORT State getStatus();
		API_EXPORT mtt::Status getLastError();
		API_EXPORT float checkingProgress();

		API_EXPORT mtt::DownloadSelection getFilesSelection();
		API_EXPORT bool selectFiles(std::vector<bool>&);
		API_EXPORT std::string getLocationPath();
		API_EXPORT mtt::Status setLocationPath(const std::string path);

		API_EXPORT std::string name();
		API_EXPORT float currentProgress();
		API_EXPORT float currentSelectionProgress();
		API_EXPORT size_t downloaded();
		API_EXPORT size_t uploaded();
		API_EXPORT size_t dataLeft();
		API_EXPORT bool finished();
		API_EXPORT bool selectionFinished();

		API_EXPORT const mtt::TorrentFileInfo& getFileInfo();
		API_EXPORT std::shared_ptr<Peers> getPeers();
		API_EXPORT std::shared_ptr<mttApi::FileTransfer> getFileTransfer();

		API_EXPORT void getPiecesBitfield(std::vector<uint8_t>& bitfield);
		API_EXPORT void getPiecesReceivedList(std::vector<uint32_t>& list);

		API_EXPORT bool getMetadataDownloadState(mtt::MetadataDownloadState& state);
		API_EXPORT bool getMetadataDownloadLog(std::vector<std::string>& logs, size_t logStart = 0);
		API_EXPORT size_t getMetadataDownloadLogSize();
	};
}
