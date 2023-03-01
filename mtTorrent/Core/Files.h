#pragma once

#include "State.h"
#include "Storage.h"
#include "PiecesProgress.h"
#include "Api/Files.h"

namespace mtt
{
	class Files : public mttApi::Files
	{
	public:

		Files(Torrent&);

		void initialize();
		void initialize(TorrentState& state);

		Status start();
		void stop();

		uint64_t getLastModifiedTime();

		float checkingProgress() const;
		void checkFiles();

		std::vector<uint64_t> getFilesAllocatedSize();
		std::string getLocationPath() const;
		mtt::Status setLocationPath(const std::string& path, bool moveFiles);
		mtt::PathValidation validateCurrentPath() const;
		mtt::PathValidation validatePath(const std::string&) const;

		std::vector<mtt::FileSelection> getFilesSelection() const;
		mtt::Status selectFiles(const std::vector<bool>&);
		mtt::Status selectFile(uint32_t index, bool selected);
		void setFilesPriority(const std::vector<mtt::Priority>&);
		void setFilePriority(uint32_t index, mtt::Priority);

		std::vector<std::pair<float, uint32_t>> getFilesProgress();

		std::size_t getPiecesCount() const;
		std::size_t getSelectedPiecesCount() const;
		std::size_t getReceivedPiecesCount() const;
		bool getPiecesBitfield(mtt::Bitfield&) const;

		PiecesProgress progress;
		DownloadSelection selection;
		Storage storage;

		bool checking = false;

	private:

		uint64_t lastFileTime = 0;
		void checkFiles(const std::vector<bool>& pieces);

		mutable std::mutex checkStateMutex;
		std::shared_ptr<mtt::PiecesCheck> checkState;

		void select(uint32_t idx, bool selected);

		Torrent& torrent;
	};
}
