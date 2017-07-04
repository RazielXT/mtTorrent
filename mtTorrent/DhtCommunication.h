#pragma once
#include <vector>
#include "Network.h"

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
		};

		struct NodeAddr
		{
			NodeAddr();
			NodeAddr(char* buffer, bool v6);

			std::string str;
			std::vector<uint8_t> addrBytes;
			uint16_t port;

			bool isIpv6();
			size_t parse(char* buffer, bool v6);
		};

		struct NodeInfo
		{
			NodeId id;
			NodeAddr addr;

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
			std::vector<NodeAddr> values;
		};

		class Communication
		{
		public:

			void test();

		private:

			GetPeersResponse parseGetPeersResponse(DataBuffer& message);
		};
	}
}