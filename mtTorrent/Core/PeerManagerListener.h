#pragma once
#include "IPeerListener.h"

namespace mtt
{
	class PeerManagerListener : public IPeerListener
	{
		virtual void onConnected(std::shared_ptr<PeerCommunication>, Addr&) = 0;
		virtual void onConnectFail(Addr&) = 0;
		virtual void onAddrReceived(std::vector<Addr>&) = 0;
	};
}
