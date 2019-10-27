#include "IncomingPeersListener.h"

std::string mttApi::Listener::getUpnpReadableInfo()
{
	return static_cast<mtt::IncomingPeersListener*>(this)->getUpnpReadableInfo();
}