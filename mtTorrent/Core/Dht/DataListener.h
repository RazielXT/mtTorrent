#pragma once

#include "Interface.h"
#include "utils/UdpAsyncComm.h"

namespace mtt
{
	namespace dht
	{
		class DataListener
		{
		public:

			virtual void announceTokenReceived(const uint8_t* hash, std::string& token, udp::endpoint& source) = 0;

			virtual uint32_t onFoundPeers(const uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void findingPeersFinished(const uint8_t* hash, uint32_t count) = 0;

			virtual UdpRequest sendMessage(const Addr&, const DataBuffer&, UdpResponseCallback response) = 0;
			virtual void stopMessage(UdpRequest r) = 0;
			virtual void sendMessage(const udp::endpoint&, const DataBuffer&) = 0;
		};
	}
}