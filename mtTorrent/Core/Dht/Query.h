#pragma once
#include "Dht/Table.h"
#include "utils/UdpAsyncComm.h"

namespace mtt
{
	namespace dht
	{
		class QueryListener
		{
		public:

			virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) = 0;
			virtual void findingPeersFinished(uint8_t* hash, uint32_t count) = 0;

			virtual UdpRequest sendMessage(Addr&, DataBuffer&, UdpResponseCallback response) = 0;
			virtual UdpRequest sendMessage(std::string& host, std::string& port, DataBuffer&, UdpResponseCallback response) = 0;
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

			struct DhtQuery
			{
				~DhtQuery();

				void start(uint8_t* hash, Table* table, QueryListener* dhtListener);
				void stop();

				bool finished();

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
				virtual void sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info) = 0;
				virtual void sendRequest(std::string& host, std::string& port, DataBuffer& data, RequestInfo& info) = 0;
				virtual bool onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request) = 0;

				Table* table;
				QueryListener* listener = nullptr;
			};

			struct FindPeers : public DhtQuery, public std::enable_shared_from_this<FindPeers>
			{
			protected:

				uint32_t MaxReturnedValues = 50;
				uint32_t foundCount = 0;

				virtual DataBuffer createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				virtual void sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info);
				virtual void sendRequest(std::string& host, std::string& port, DataBuffer& data, RequestInfo& info);
				virtual bool onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request) override;
				GetPeersResponse parseGetPeersResponse(DataBuffer& message);		
			};

			struct FindNode : public DhtQuery, public std::enable_shared_from_this<FindNode>
			{
				void startOne(uint8_t* hash, Addr& addr, Table* table, QueryListener* dhtListener, boost::asio::io_service* serviceIo);

				uint32_t resultCount = 0;
				
			protected:

				bool findClosest = true;

				virtual DataBuffer createRequest(uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				virtual void sendRequest(Addr& addr, DataBuffer& data, RequestInfo& info);
				virtual void sendRequest(std::string& host, std::string& port, DataBuffer& data, RequestInfo& info);
				virtual bool onResponse(UdpRequest comm, DataBuffer* data, RequestInfo request) override;
				FindNodeResponse parseFindNodeResponse(DataBuffer& message);
			};

			struct PingNodes : public std::enable_shared_from_this<PingNodes>
			{
				~PingNodes();

				void start(Addr& node, uint8_t bucketId, Table* table, QueryListener* dhtListener);
				void start(std::vector<Addr>& nodes, uint8_t bucketId, Table* table, QueryListener* dhtListener);
				void stop();

			protected:

				Table* table;
				QueryListener* listener;
				uint8_t bucketId;

				uint32_t MaxSimultaneousRequests = 5;

				std::mutex requestsMutex;
				std::vector<UdpRequest> requests;
				std::vector<Addr> nodesLeft;

				DataBuffer createRequest(uint16_t transactionId);

				struct PingInfo
				{
					uint16_t transactionId;
					Addr addr;
				};
				bool onResponse(UdpRequest comm, DataBuffer* data, PingInfo request);
				void sendRequest(Addr& addr);
			};
		}
	}
}