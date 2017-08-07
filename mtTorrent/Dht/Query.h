#pragma once
#include "Dht/Table.h"
#include "ServiceThreadpool.h"
#include "UdpAsyncClient.h"

namespace mtt
{
	namespace dht
	{
		class DhtListener
		{
		public:

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) = 0;
		};

		struct MessageResponse
		{
			int result = 0;
			uint16_t transaction = 0;
		};

		struct GetPeersResponse : public MessageResponse
		{
			uint8_t id[20];
			std::string token;
			std::vector<NodeInfo> nodes;
			std::vector<Addr> values;
		};

		struct GetPeersRequest
		{
			uint8_t id[20];
			uint8_t target[20];
		};

		struct FindNodeResponse : public MessageResponse
		{
			uint8_t id[20];
			std::vector<NodeInfo> nodes;
		};

		struct FindNodeRequest
		{
			uint8_t id[20];
			uint8_t target[20];
		};

		struct PingMessage
		{
			uint8_t id[20];
		};

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

			static DataBuffer createGetPeersRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId);
			void onGetPeersResponse(DataBuffer* data, PackedUdpRequest* source, uint16_t transactionId);
			GetPeersResponse parseGetPeersResponse(DataBuffer& message);
			uint16_t createTransactionId();

			void start();
			void stop();

			DhtListener* dhtListener = nullptr;
			boost::asio::io_service* serviceIo = nullptr;
		};
	}
}