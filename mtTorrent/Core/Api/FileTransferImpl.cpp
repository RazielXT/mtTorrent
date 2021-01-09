#include "FileTransfer.h"

uint32_t mttApi::FileTransfer::getDownloadSpeed() const
{
	return static_cast<const mtt::FileTransfer*>(this)->getDownloadSpeed();
}

uint32_t mttApi::FileTransfer::getUploadSpeed() const
{
	return static_cast<const mtt::FileTransfer*>(this)->getUploadSpeed();
}

std::vector<mtt::ActivePeerInfo> mttApi::FileTransfer::getPeersInfo() const
{
	return static_cast<const mtt::FileTransfer*>(this)->getPeersInfo();
}

std::vector<uint32_t> mttApi::FileTransfer::getCurrentRequests() const
{
	return static_cast<const mtt::FileTransfer*>(this)->getCurrentRequests();
}
