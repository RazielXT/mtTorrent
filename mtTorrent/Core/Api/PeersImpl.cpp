#include "Peers.h"

std::vector<mtt::TrackerInfo> mttApi::Peers::getSourcesInfo()
{
	return static_cast<mtt::Peers*>(this)->getSourcesInfo();
}

void mttApi::Peers::refreshSource(const std::string& name)
{
	static_cast<mtt::Peers*>(this)->refreshSource(name);
}

mtt::Status mttApi::Peers::addTrackers(const std::vector<std::string>& urls)
{
	return static_cast<mtt::Peers*>(this)->importTrackers(urls);
}

uint32_t mttApi::Peers::connectedCount() const
{
	return static_cast<const mtt::Peers*>(this)->connectedCount();
}

uint32_t mttApi::Peers::receivedCount() const
{
	return static_cast<const mtt::Peers*>(this)->receivedCount();
}

mtt::Status mttApi::Peers::connect(const char* address)
{
	return static_cast<mtt::Peers*>(this)->connect(Addr::fromString(address));
}
{
	static_cast<mtt::Peers*>(this)->connect(Addr::fromString(address));
}
