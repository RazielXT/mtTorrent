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

		bool started = false;
		bool checking = false;
		Status lastError = Status::Success;

		static TorrentPtr fromFile(mtt::TorrentFileInfo fileInfo);
		static TorrentPtr fromMagnetLink(std::string link);
		static TorrentPtr fromSavedState(std::string name);
		void downloadMetadata();

		bool isActive() const;
		State getState() const;
		TimePoint getStateTimestamp() const;

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
		uint64_t receivedBytes() const;
		uint64_t dataLeft() const;
		bool finished() const;
		bool selectionFinished() const;

		const uint8_t* hash() const;
		std::string hashString() const;
		Timestamp getTimeAdded() const;

		Files files;
		TorrentFileInfo infoFile;
		ServiceThreadpool service;

		std::unique_ptr<Peers> peers;
		std::unique_ptr<FileTransfer> fileTransfer;
		std::unique_ptr<MetadataDownload> utmDl;

		void save();
		void saveTorrentFile(const uint8_t* data, std::size_t size);
		void removeMetaFiles();
		bool loadFileInfo();


	protected:

		static bool loadSavedTorrentFile(const std::string& hash, DataBuffer& out);

		mutable std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;

		void refreshLastState();
		uint64_t lastFileTime = 0;
		Timestamp addedTime = 0;

		bool stateChanged = false;
		TimePoint activityTime;
		void initialize();

		std::mutex stateMutex;
		bool stopping = false;

		enum class LoadedState { Minimal, Full } loadedState = LoadedState::Minimal;
	};
}
