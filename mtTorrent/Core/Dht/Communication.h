#pragma once

#include <vector>
#include <map>
#include "Api/Dht.h"
#include "Dht/Query.h"
#include "Dht/Responder.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncComm.h"
#include "utils/ScheduledTimer.h"
#include "Listener.h"

namespace mtt
{
	namespace dht
	{
		class Communication : public DataListener, public mttApi::Dht
		{
		public:

			Communication(UdpAsyncComm&);
			~Communication();

			static Communication& get();

			void init();
			void start();
			void stop();
			bool active() const;

			void findPeers(const uint8_t* hash, ResultsListener* listener);
			void stopFindingPeers(const uint8_t* hash);

			void findNode(const uint8_t* hash);

			void pingNode(const Addr& addr);

			void removeListener(ResultsListener* listener);

			void save();
			void load();

			bool onUdpPacket(udp::endpoint&, std::vector<BufferView>&);

			uint32_t getNodesCount() const;

		protected:

			UdpAsyncComm& udp;

			uint32_t onFoundPeers(const uint8_t* hash, const std::vector<Addr>& values) override;
			void findingPeersFinished(const uint8_t* hash, uint32_t count) override;

			UdpRequest sendMessage(const Addr&, const DataBuffer&, UdpResponseCallback response) override;
			void sendMessage(const udp::endpoint&, const DataBuffer&) override;
			void stopMessage(UdpRequest r) override;

			void announceTokenReceived(const uint8_t* hash, const std::string& token, const udp::endpoint& source) override;

			struct QueryInfo
			{
				std::shared_ptr<Query::DhtQuery> q;
				ResultsListener* listener;
			};

			std::mutex peersQueriesMutex;
			std::vector<QueryInfo> peersQueries;

			std::shared_ptr<Table> table;
			Responder responder;
			ServiceThreadpool service;

			void loadDefaultRoots();

			std::shared_ptr<ScheduledTimer> refreshTimer;
			void refreshTable();
		};
	}
}