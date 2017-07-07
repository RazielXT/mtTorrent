#pragma once
#include <functional>
#include "DhtCommunication.h"

namespace mtt
{
	namespace dht
	{
		class Control
		{
		public:

			void deinit();

			void getPeers(std::function<bool()> callback, NodeId& id);

			void stateChanged(NodeId& id);
		};
	}
}
