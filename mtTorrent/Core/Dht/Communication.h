#pragma once
#include <vector>
#include "Dht/Query.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncComm.h"
#include "utils/ScheduledTimer.h"

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

		class Communication : public QueryListener
		{
		public:

			Communication();
			~Communication();

			void findPeers(uint8_t* hash, ResultsListener* listener);
			void stopFindingPeers(uint8_t* hash);

			void findNode(uint8_t* hash);
			std::shared_ptr<Query::FindNode> fnQ;

			void pingNode(Addr& addr, uint8_t* hash);

			void removeListener(ResultsListener* listener);

			std::string save();
			void load(std::string&);

		protected:

			bool onNewUdpPacket(UdpRequest, DataBuffer*);

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;

			virtual UdpRequest sendMessage(Addr&, DataBuffer&, UdpResponseCallback response) override;

			UdpAsyncComm udpMgr;

		private:

			struct QueryInfo
			{
				std::shared_ptr<Query::DhtQuery> q;
				ResultsListener* listener;
			};

			std::mutex peersQueriesMutex;
			std::vector<QueryInfo> peersQueries;

			Table table;
			ServiceThreadpool service;

			void loadDefaultRoots();

			std::shared_ptr<ScheduledTimer> refreshTimer;
			void refreshTable();
		};
	}
}