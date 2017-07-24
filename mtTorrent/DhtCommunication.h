#pragma once
#include <vector>
#include "Interface.h"
#include "ServiceThreadpool.h"
#include "UdpAsyncClient.h"

namespace mtt
{
	namespace dht
	{
		struct NodeId
		{
			uint8_t data[20];

			NodeId();
			NodeId(const char* buffer);
			void copy(const char* buffer);
			bool closerThan(NodeId& r, NodeId& target);
			bool closerThanThis(NodeId& distance, NodeId& target);
			NodeId distance(NodeId& r);
			uint8_t length();
			void setMax();
		};

		struct NodeInfo
		{
			NodeId id;
			Addr addr;

			size_t parse(char* buffer, bool v6);
			bool operator==(const NodeInfo& r);
		};

		struct MessageResponse
		{
			int result = 0;
		};

		struct GetPeersResponse : public MessageResponse
		{
			uint8_t id[20];
			std::string token;
			std::vector<NodeInfo> nodes;
			std::vector<Addr> values;
		};

		class DhtListener
		{
		public:

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) = 0;
		};

		class Communication
		{
		public:

			Communication(DhtListener& listener);
			~Communication();

			void findPeers(uint8_t* hash);
			void stopFindingPeers(uint8_t* hash);

		private:


			struct Query
			{
				~Query();

				uint32_t MaxReturnedValues = 50;
				uint32_t MaxCachedNodes = 64;
				uint32_t MaxSimultaneousConnections = 8;

				std::mutex requestsMutex;
				std::vector<UdpRequest> requests;

				std::mutex nodesMutex;
				std::vector<NodeInfo> receivedNodes;
				std::vector<NodeInfo> usedNodes;
				NodeId minDistance;
				NodeId targetIdNode;

				uint32_t foundCount = 0;

				DataBuffer createGetPeersRequest(uint8_t* hash, bool bothProtocols);
				void onGetPeersResponse(DataBuffer* data, PackedUdpRequest* source);
				GetPeersResponse parseGetPeersResponse(DataBuffer& message);
				void stop();
			};
			Query query;


			ServiceThreadpool service;
			DhtListener& listener;	
		};
	}
}