#include "MetadataDownload.h"

mtt::MetadataDownloadState mttApi::MagnetDownload::getState()
{
	return static_cast<mtt::MetadataDownload*>(this)->state;
}

size_t mttApi::MagnetDownload::getDownloadLog(std::vector<std::string>& logs, size_t logStart)
{
	auto utm = static_cast<mtt::MetadataDownload*>(this);

	auto events = utm->getEvents(logStart);
	for (auto& e : events)
	{
		logs.push_back(e.toString());
	}

	return events.size();
}

size_t mttApi::MagnetDownload::getDownloadLogSize()
{
	return static_cast<mtt::MetadataDownload*>(this)->getEventsCount();
}
