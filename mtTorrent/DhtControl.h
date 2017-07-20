#pragma once
#include <functional>
#include "Interface.h"

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

		struct NodeInfo
		{
			NodeId id;
			Addr addr;

			size_t parse(char* buffer, bool v6);
			bool operator==(const NodeInfo& r);
		};

		class Control
		{
		public:

			void deinit();

			void getPeers(std::function<bool()> callback, NodeId& id);

			void addNode(NodeInfo& node);

			void stateChanged(NodeId& id);
		};
	}
}
