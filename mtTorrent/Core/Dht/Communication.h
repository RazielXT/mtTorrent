#pragma once
#include <vector>
#include "Dht/Query.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncMgr.h"

namespace mtt
{
	namespace dht
	{
		class ResultsListener
		{
		public:

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) = 0;
		};

		class Communication : public DhtListener
		{
		public:

			Communication(ResultsListener&);
			~Communication();

			void findPeers(uint8_t* hash);
			void stopFindingPeers(uint8_t* hash);

		protected:

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;
			virtual UdpConnection sendMessage(Addr&, DataBuffer&, UdpConnectionCallback response) override;
			virtual UdpConnection sendMessage(std::string& host, std::string& port, DataBuffer&, UdpConnectionCallback response) override;

			UdpAsyncMgr udpMgr;

		private:

			std::mutex peersQueriesMutex;
			std::vector<std::shared_ptr<Query::FindPeers>> peersQueries;

			Table table;
			ServiceThreadpool service;
			ResultsListener& listener;
		};
	}
}