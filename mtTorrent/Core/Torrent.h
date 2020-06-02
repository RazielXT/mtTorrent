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

		Torrent();

		State state = State::Stopped;
		bool checking = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(mtt::TorrentFileInfo& fileInfo);
		static TorrentPtr fromMagnetLink(std::string link);
		static TorrentPtr fromSavedState(std::string name);
		void downloadMetadata(std::function<void(Status, MetadataDownloadState&)> callback);

		bool start();
		enum class StopReason { Deinit, Manual, Internal };
		void stop(StopReason reason = StopReason::Manual);

		void checkFiles();
		std::shared_ptr<PiecesCheck> checkFiles(std::function<void(std::shared_ptr<PiecesCheck>)> onFinish);
		float checkingProgress();

		bool selectFiles(const std::vector<bool>&);
		void setFilesPriority(const std::vector<mtt::Priority>&);
		mtt::Status setLocationPath(const std::string& path);

		std::string name();
		float currentProgress();
		float currentSelectionProgress();
		uint64_t downloaded();
		size_t downloadSpeed();
		uint64_t uploaded();
		size_t uploadSpeed();
		uint64_t dataLeft();
		bool finished();
		bool selectionFinished();

		const uint8_t* hash();
		std::string hashString();

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::shared_ptr<Peers> peers;
		std::shared_ptr<FileTransfer> fileTransfer;
		std::shared_ptr<MetadataDownload> utmDl;

		void save();
		void saveTorrentFile(const char* data, size_t size);
		void saveTorrentFileFromUtm();
		void removeMetaFiles();

		bool importTrackers(const mtt::TorrentFileInfo& fileInfo);

	protected:

		static bool loadSavedTorrentFile(const std::string& hash, DataBuffer& out);

		std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;
		uint64_t lastStateTime = 0;
		bool stateChanged = false;
		void init();
	};
}
