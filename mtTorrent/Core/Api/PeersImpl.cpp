#include "Peers.h"

std::vector<mtt::TrackerInfo> mttApi::Peers::getSourcesInfo()
{
	return static_cast<mtt::Peers*>(this)->getSourcesInfo();
}

void mttApi::Peers::refreshSource(const std::string& name)
{
	return static_cast<mtt::Peers*>(this)->refreshSource(name);
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
