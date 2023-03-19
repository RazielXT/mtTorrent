#include "FileTransfer.h"

uint32_t mttApi::FileTransfer::downloadSpeed() const
{
	return static_cast<const mtt::FileTransfer*>(this)->downloader.downloadSpeed;
}

uint64_t mttApi::FileTransfer::downloaded() const
{
	return static_cast<const mtt::FileTransfer*>(this)->downloader.downloaded;
}

uint32_t mttApi::FileTransfer::uploadSpeed() const
{
	return static_cast<const mtt::FileTransfer*>(this)->uploader->uploadSpeed;
}

uint64_t mttApi::FileTransfer::uploaded() const
{
	return static_cast<const mtt::FileTransfer*>(this)->uploader->uploaded;
}

std::vector<uint32_t> mttApi::FileTransfer::getCurrentRequests() const
{
	return static_cast<const mtt::FileTransfer*>(this)->downloader.getCurrentRequests();
}
