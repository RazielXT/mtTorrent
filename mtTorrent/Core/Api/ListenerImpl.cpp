#include "IncomingPeersListener.h"

std::string mttApi::PortListener::getUpnpReadableInfo() const
{
	return static_cast<const mtt::IncomingPeersListener*>(this)->getUpnpReadableInfo();
}
