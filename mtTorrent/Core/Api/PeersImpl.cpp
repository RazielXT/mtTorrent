#include "Peers.h"

std::vector<mtt::TrackerInfo> mttApi::Peers::getSourcesInfo()
{
	return static_cast<mtt::Peers*>(this)->getSourcesInfo();
}

uint32_t mttApi::Peers::getSourcesCount()
{
	return static_cast<mtt::Peers*>(this)->getSourcesCount();
}

void mttApi::Peers::refreshSource(const std::string& name)
{
	return static_cast<mtt::Peers*>(this)->refreshSource(name);
}

uint32_t mttApi::Peers::connectedCount()
{
	return static_cast<mtt::Peers*>(this)->connectedCount();
}

uint32_t mttApi::Peers::receivedCount()
{
	return static_cast<mtt::Peers*>(this)->receivedCount();
}

void mttApi::Peers::connect(const char* address)
{
	static_cast<mtt::Peers*>(this)->connect(Addr(address));
}
