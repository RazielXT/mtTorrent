#include "IncomingPeersListener.h"

std::string mttApi::Listener::getUpnpReadableInfo() const
{
	return static_cast<const mtt::IncomingPeersListener*>(this)->getUpnpReadableInfo();
}