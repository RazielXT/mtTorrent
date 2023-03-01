#include "Api/Files.h"
#include "Files.h"

float mttApi::Files::checkingProgress() const
{
	return static_cast<const mtt::Files*>(this)->checkingProgress();
}

void mttApi::Files::startCheck()
{
	static_cast<mtt::Files*>(this)->checkFiles();
}

std::vector<std::pair<float, uint32_t>> mttApi::Files::getProgress()
{
	return static_cast<mtt::Files*>(this)->getFilesProgress();
}

std::vector<uint64_t> mttApi::Files::getAllocatedSize()
{
	return static_cast<mtt::Files*>(this)->getFilesAllocatedSize();
}

mtt::Status mttApi::Files::select(const std::vector<bool>& s)
{
	return static_cast<mtt::Files*>(this)->selectFiles(s);
}

mtt::Status mttApi::Files::select(uint32_t index, bool selected)
{
	return static_cast<mtt::Files*>(this)->selectFile(index, selected);
}

void mttApi::Files::setPriority(const std::vector<mtt::Priority>& p)
{
	static_cast<mtt::Files*>(this)->setFilesPriority(p);
}

void mttApi::Files::setPriority(uint32_t index, mtt::Priority p)
{
	static_cast<mtt::Files*>(this)->setFilePriority(index, p);
}

std::vector<mtt::FileSelection> mttApi::Files::getSelection() const
{
	return static_cast<const mtt::Files*>(this)->getFilesSelection();
}

std::string mttApi::Files::getLocationPath() const
{
	return static_cast<const mtt::Files*>(this)->getLocationPath();
}

mtt::Status mttApi::Files::setLocationPath(const std::string& path, bool moveFiles)
{
	return static_cast<mtt::Files*>(this)->setLocationPath(path, moveFiles);
}

mtt::PathValidation mttApi::Files::validateCurrentPath() const
{
	return static_cast<const mtt::Files*>(this)->validateCurrentPath();
}

mtt::PathValidation mttApi::Files::validatePath(const std::string& p) const
{
	return static_cast<const mtt::Files*>(this)->validatePath(p);
}

size_t mttApi::Files::getPiecesCount() const
{
	return static_cast<const mtt::Files*>(this)->getPiecesCount();
}

size_t mttApi::Files::getSelectedPiecesCount() const
{
	return static_cast<const mtt::Files*>(this)->getSelectedPiecesCount();
}

size_t mttApi::Files::getReceivedPiecesCount() const
{
	return static_cast<const mtt::Files*>(this)->getReceivedPiecesCount();
}

bool mttApi::Files::getPiecesBitfield(mtt::Bitfield& data) const
{
	return static_cast<const mtt::Files*>(this)->getPiecesBitfield(data);
}
