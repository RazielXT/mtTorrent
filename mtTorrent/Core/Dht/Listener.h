#pragma once
#include <vector>
#include "utils/Network.h"

namespace mtt
{
	namespace dht
	{
		class ResultsListener
		{
		public:

			virtual uint32_t dhtFoundPeers(uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void dhtFindingPeersFinished(uint8_t* hash, uint32_t count) = 0;
		};
	}
}