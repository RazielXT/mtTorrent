#pragma once
#include <vector>
#include "Dht/Query.h"

namespace mtt
{
	namespace dht
	{
		class Communication
		{
		public:

			Communication(DhtListener& listener);
			~Communication();

			void findPeers(uint8_t* hash);
			void stopFindingPeers(uint8_t* hash);

		private:

			std::mutex peersQueriesMutex;
			std::vector<std::shared_ptr<Query::FindPeers>> peersQueries;

			Table table;

			ServiceThreadpool service;
			DhtListener& listener;	
		};
	}
}