#pragma once
#include <vector>
#include "Dht/Query.h"
#include "Dht/Responder.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncComm.h"
#include "utils/ScheduledTimer.h"
#include <map>

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

		class Communication : public DataListener
		{
		public:

			Communication();
			~Communication();

			void findPeers(uint8_t* hash, ResultsListener* listener);
			void stopFindingPeers(uint8_t* hash);

			void findNode(uint8_t* hash);

			void pingNode(Addr& addr);

			void removeListener(ResultsListener* listener);

			std::string save();
			void load(std::string&);

		protected:

			UdpAsyncComm udpMgr;
			bool onUnknownUdpPacket(udp::endpoint&, DataBuffer&);

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;

			virtual UdpRequest sendMessage(Addr&, DataBuffer&, UdpResponseCallback response) override;
			virtual void sendMessage(udp::endpoint&, DataBuffer&) override;

			virtual void announceTokenReceived(uint8_t* hash, std::string& token, udp::endpoint& source) override;	

		private:

			struct QueryInfo
			{
				std::shared_ptr<Query::DhtQuery> q;
				ResultsListener* listener;
			};

			std::mutex peersQueriesMutex;
			std::vector<QueryInfo> peersQueries;

			Table table;
			Responder responder;
			ServiceThreadpool service;

			void loadDefaultRoots();

			std::shared_ptr<ScheduledTimer> refreshTimer;
			void refreshTable();
		};
	}
}