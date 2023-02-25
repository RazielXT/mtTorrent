#pragma once
#include "Dht/Table.h"
#include "Dht/DataListener.h"

namespace mtt
{
	namespace dht
	{
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

		struct PingMessage : public MessageResponse
		{
			uint8_t id[20];
		};

		namespace Query
		{
			struct RequestInfo
			{
				NodeInfo node;
				uint16_t transactionId;
				uint8_t minDistance;
			};

			struct DhtQuery
			{
				DhtQuery();
				~DhtQuery();

				void start(const uint8_t* hash, std::shared_ptr<Table> table, DataListener* dhtListener);
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

				virtual DataBuffer createRequest(const uint8_t* hash, bool bothProtocols, uint16_t transactionId) = 0;
				virtual void sendRequest(const Addr& addr, const DataBuffer& data, RequestInfo& info) = 0;
				virtual bool onResponse(UdpRequest comm, const BufferView& data, RequestInfo request) = 0;

				std::shared_ptr<Table> table;
				DataListener* listener = nullptr;
			};

			struct GetPeers : public DhtQuery, public std::enable_shared_from_this<GetPeers>
			{
			protected:

				uint32_t MaxReturnedValues = 50;
				uint32_t foundCount = 0;

				DataBuffer createRequest(const uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				void sendRequest(const Addr& addr, const DataBuffer& data, RequestInfo& info);
				bool onResponse(UdpRequest comm, const BufferView& data, RequestInfo request) override;
				GetPeersResponse parseGetPeersResponse(const BufferView& message);
			};

			struct FindNode : public DhtQuery, public std::enable_shared_from_this<FindNode>
			{
				void startOne(const uint8_t* hash, Addr& addr, std::shared_ptr<Table> table, DataListener* dhtListener);

				uint32_t resultCount = 0;
				
			protected:

				bool findClosest = true;

				DataBuffer createRequest(const uint8_t* hash, bool bothProtocols, uint16_t transactionId) override;
				void sendRequest(const Addr& addr, const DataBuffer& data, RequestInfo& info);
				bool onResponse(UdpRequest comm, const BufferView& data, RequestInfo request) override;
				FindNodeResponse parseFindNodeResponse(const BufferView& message);
			};

			struct PingNodes : public std::enable_shared_from_this<PingNodes>
			{
				~PingNodes();

				void start(const Addr& addr, std::shared_ptr<Table> table, DataListener* dhtListener);
				void start(std::vector<NodeInfo>& nodes, std::shared_ptr<Table> table, DataListener* dhtListener);
				void stop();

			protected:

				std::shared_ptr<Table> table;
				DataListener* listener = nullptr;

				uint32_t MaxSimultaneousRequests = 5;

				std::mutex requestsMutex;
				std::vector<UdpRequest> requests;
				std::vector<NodeInfo> nodesLeft;

				DataBuffer createRequest(uint16_t transactionId);
				void sendRequest(const NodeInfo&, bool unknown);

				struct PingInfo
				{
					uint16_t transactionId;
					NodeInfo node;
					bool unknown;
				};
				bool onResponse(UdpRequest comm, const BufferView& data, PingInfo request);
				PingMessage parseResponse(const BufferView& message);
			};

			void AnnouncePeer(const uint8_t* infohash, const std::string& token, const udp::endpoint& target, DataListener* dhtListener);
		}
	}
}