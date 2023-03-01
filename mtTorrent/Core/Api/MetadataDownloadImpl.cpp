#include "MetadataDownload.h"
#include "Api/Interface.h"

mtt::MetadataDownloadState mttApi::MetadataDownload::getState() const
{
	return static_cast<const mtt::MetadataDownload*>(this)->state;
}

size_t mttApi::MetadataDownload::getDownloadLog(std::vector<std::string>& logs, std::size_t logStart) const
{
	auto utm = static_cast<const mtt::MetadataDownload*>(this);

	auto events = utm->getEvents(logStart);
	for (auto& e : events)
	{
		logs.push_back(e.toString());
	}

	return events.size();
}

size_t mttApi::MetadataDownload::getDownloadLogSize() const
{
	return static_cast<const mtt::MetadataDownload*>(this)->getEventsCount();
}
