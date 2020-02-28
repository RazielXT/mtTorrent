#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Files.h"
#include <functional>
#include "Api/Torrent.h"

namespace mtt
{
	class MetadataDownload;
	class FileTransfer;
	class Peers;

	class Torrent : public mttApi::Torrent
	{
	public:

		State state = State::Stopped;
		bool checking = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(mtt::TorrentFileInfo& fileInfo);
		static TorrentPtr fromFile(std::string filepath);
		static TorrentPtr fromMagnetLink(std::string link);
		static TorrentPtr fromSavedState(std::string name);
		void downloadMetadata(std::function<void(Status, MetadataDownloadState&)> callback);

		bool start();
		void stop();

		void checkFiles();
		std::shared_ptr<PiecesCheck> checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish);
		float checkingProgress();
		bool filesChecked();

		bool selectFiles(const std::vector<bool>&);
		void setFilesPriority(const std::vector<mtt::Priority>&);

		std::string name();
		float currentProgress();
		float currentSelectionProgress();
		size_t downloaded();
		size_t downloadSpeed();
		size_t uploaded();
		size_t uploadSpeed();
		size_t dataLeft();
		bool finished();
		bool selectionFinished();

		const uint8_t* hash();
		std::string hashString();

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::shared_ptr<Peers> peers;
		std::shared_ptr<FileTransfer> fileTransfer;
		std::unique_ptr<MetadataDownload> utmDl;

		void save();
		void saveTorrentFile();
		void removeMetaFiles();

	protected:

		std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;
		uint64_t lastStateTime = 0;
		bool stateChanged = false;
		void init();
	};
}
