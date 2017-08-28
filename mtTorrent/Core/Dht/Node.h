#pragma once
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
			bool closerThan(NodeId& other, NodeId& target);
			bool closerToNodeThan(NodeId& distance, NodeId& target);
			NodeId distance(NodeId& r);
			uint8_t length();
			void setMax();

			static NodeId distance(const uint8_t* l, const uint8_t* r);
			static uint8_t length(uint8_t* data);
			bool operator==(const NodeId& r);
			bool operator==(const uint8_t* r);
		};

		bool operator<(const NodeId& l, const NodeId& r);

		struct NodeInfo
		{
			NodeId id;
			Addr addr;

			int parse(const char* buffer, bool v6);
			bool operator==(const NodeInfo& r);
		};
	}
}