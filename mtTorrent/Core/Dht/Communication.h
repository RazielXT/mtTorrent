#pragma once
#include <vector>
#include "Dht/Query.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncComm.h"

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

			void pingNode(Addr& addr, uint8_t* hash);

		protected:

			bool onNewUdpPacket(UdpRequest, DataBuffer*);

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;
			virtual UdpRequest sendMessage(Addr&, DataBuffer&, UdpResponseCallback response) override;
			virtual UdpRequest sendMessage(std::string& host, std::string& port, DataBuffer&, UdpResponseCallback response) override;

			UdpAsyncComm udpMgr;

		private:

			std::mutex peersQueriesMutex;
			std::vector<std::shared_ptr<Query::FindPeers>> peersQueries;

			Table table;
			ServiceThreadpool service;
			ResultsListener& listener;
		};
	}
}