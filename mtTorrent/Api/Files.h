#pragma once

#include "Interface.h"

namespace mttApi
{
	class Files
	{
	public:

		/*
			progress of last files checking task (0-1)
		*/
		API_EXPORT float checkingProgress() const;
		/*
			start check of existing files, if not already checking
		*/
		API_EXPORT void startCheck();
		/*
			get download progress of files, sorted by order in torrent file, % (including unfinished pieces) and finished pieces
		*/
		API_EXPORT std::vector<std::pair<float, uint32_t>> getProgress();
		/*
			get current allocated sizes on disk in bytes, sorted by order in torrent file
		*/
		API_EXPORT std::vector<uint64_t> getAllocatedSize();

		/*
			get current selection info, sorted by order in torrent file
		*/
		API_EXPORT std::vector<mtt::FileSelection> getSelection() const;
		/*
			select/deselect files to download, sorted by order in torrent file
		*/
		API_EXPORT mtt::Status select(const std::vector<bool>&);
		/*
			select/deselect file to download, index sorted by order in torrent file
		*/
		API_EXPORT mtt::Status select(uint32_t index, bool selected);
		/*
			set priority of files download, sorted by order in torrent file
		*/
		API_EXPORT void setPriority(const std::vector<mtt::Priority>&);
		/*
			set priority of files download, sorted by order in torrent file
		*/
		API_EXPORT void setPriority(uint32_t index, mtt::Priority);
		/*
			get current download location path
		*/
		API_EXPORT std::string getLocationPath() const;
		/*
			change download location path, moving existing files if wanted
			if new path contains torrent files and moveFiles is true returns E_NotEmpty
			if new path contains torrent files and moveFiles is false they are inherited
		*/
		API_EXPORT mtt::Status setLocationPath(const std::string& path, bool moveFiles);
		/*
			validate current path status, considering currently selected files
		*/
		API_EXPORT mtt::PathValidation validateCurrentPath() const;
		/*
			validate path status considering currently selected files
		*/
		API_EXPORT mtt::PathValidation validatePath(const std::string&) const;

		/*
			get count of all pieces
		*/
		API_EXPORT std::size_t getPiecesCount() const;
		/*
			get count of selected for download pieces
		*/
		API_EXPORT std::size_t getSelectedPiecesCount() const;
		/*
			get count of received pieces
		*/
		API_EXPORT std::size_t getReceivedPiecesCount() const;
		/*
			get pieces progress as bitfield
		*/
		API_EXPORT bool getPiecesBitfield(mtt::Bitfield&) const;

	protected:

		Files() = default;
		Files(const Files&) = delete;
	};
}
