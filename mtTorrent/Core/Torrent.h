#pragma once

#include "Interface.h"
#include "utils/ServiceThreadpool.h"
#include "Files.h"
#include "Api/Torrent.h"
#include <functional>

namespace mtt
{
	class MetadataDownload;
	class FileTransfer;
	class Peers;

	class Torrent : public mttApi::Torrent, public std::enable_shared_from_this<Torrent>
	{
	public:

		Torrent();
		~Torrent();

		ActiveState state = ActiveState::Stopped;
		bool checking = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(mtt::TorrentFileInfo fileInfo);
		static TorrentPtr fromMagnetLink(std::string link);
		static TorrentPtr fromSavedState(std::string name);
		void downloadMetadata();
		State getState() const;

		mtt::Status start();
		enum class StopReason { Deinit, Manual, Internal };
		void stop(StopReason reason = StopReason::Manual);

		void checkFiles(bool all = false);
		float checkingProgress() const;

		bool selectFiles(const std::vector<bool>&);
		bool selectFile(uint32_t index, bool selected);
		void setFilesPriority(const std::vector<mtt::Priority>&);
		mtt::Status setLocationPath(const std::string& path, bool moveFiles);

		const std::string& name() const;
		float progress() const;
		float selectionProgress() const;
		uint64_t downloaded() const;
		size_t downloadSpeed() const;
		uint64_t uploaded() const;
		size_t uploadSpeed() const;
		uint64_t dataLeft() const;
		bool finished() const;
		bool selectionFinished() const;

		const uint8_t* hash() const;
		std::string hashString() const;

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::shared_ptr<Peers> peers;
		std::shared_ptr<FileTransfer> fileTransfer;
		std::shared_ptr<MetadataDownload> utmDl;

		void save();
		void saveTorrentFile(const char* data, size_t size);
		void removeMetaFiles();
		bool loadFileInfo();

		bool importTrackers(const mtt::TorrentFileInfo& fileInfo);

	protected:

		static bool loadSavedTorrentFile(const std::string& hash, DataBuffer& out);

		mutable std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;

		void refreshLastState();
		int64_t lastStateTime = 0;

		bool stateChanged = false;
		void initialize();

		std::mutex stateMutex;
		bool stopping = false;

		enum class LoadedState { Minimal, Full } loadedState = LoadedState::Minimal;
	};
}
