#pragma once
#include <vector>
#include "Dht/Query.h"
#include "Dht/Responder.h"
#include "utils/ServiceThreadpool.h"
#include "utils/UdpAsyncComm.h"
#include "utils/ScheduledTimer.h"
#include <map>
#include "Listener.h"

namespace mtt
{
	namespace dht
	{
		class Communication : public DataListener
		{
		public:

			Communication();
			~Communication();

			static Communication& get();

			void start();
			void stop();

			void findPeers(const uint8_t* hash, ResultsListener* listener);
			void stopFindingPeers(const uint8_t* hash);

			void findNode(const uint8_t* hash);

			void pingNode(Addr& addr);

			void removeListener(ResultsListener* listener);

			void save();
			void load();

		protected:

			UdpCommPtr udp;
			bool onUnknownUdpPacket(udp::endpoint&, DataBuffer&);

			virtual uint32_t onFoundPeers(const uint8_t* hash, std::vector<Addr>& values) override;
			virtual void findingPeersFinished(const uint8_t* hash, uint32_t count) override;

			virtual UdpRequest sendMessage(Addr&, DataBuffer&, UdpResponseCallback response) override;
			virtual void sendMessage(udp::endpoint&, DataBuffer&) override;
			virtual void stopMessage(UdpRequest r) override;

			virtual void announceTokenReceived(const uint8_t* hash, std::string& token, udp::endpoint& source) override;

		private:

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