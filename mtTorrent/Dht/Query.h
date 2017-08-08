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

		namespace Query
		{
			struct RequestInfo
			{
				NodeInfo node;
				uint16_t transactionId;
			};

			struct GenericQuery
			{
				~GenericQuery();

				void start(uint8_t* hash, Table* table, DhtListener* dhtListener, boost::asio::io_service* serviceIo);
				void stop();

				NodeId targetId;

			protected:

				uint32_t MaxCachedNodes = 32;
				uint32_t MaxSimultaneousRequests = 5;

				std::mutex requestsMutex;
				std::vector<UdpRequest> requests;

				std::mutex nodesMutex;
				std::vector<NodeInfo> receivedNodes;
				std::vector<NodeInfo> usedNodes;
				NodeId minDistance;

				virtual DataBuffer createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId) = 0;
				virtual void onResponse(DataBuffer* data, PackedUdpRequest* source, RequestInfo transactionId) = 0;

				Table* table;
				DhtListener* dhtListener = nullptr;
				boost::asio::io_service* serviceIo = nullptr;
			};

			struct FindPeers : public GenericQuery
			{
			protected:

				uint32_t MaxReturnedValues = 50;
				uint32_t foundCount = 0;

				virtual DataBuffer createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				virtual void onResponse(DataBuffer* data, PackedUdpRequest* source, RequestInfo transactionId) override;
				GetPeersResponse parseGetPeersResponse(DataBuffer& message);		
			};

			struct FindNode : public GenericQuery
			{
			protected:

				virtual DataBuffer createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				virtual void onResponse(DataBuffer* data, PackedUdpRequest* source, RequestInfo transactionId) override;
				FindNodeResponse parseFindNodeResponse(DataBuffer& message);
			};
		}
	}
}