#pragma once

#include "Interface.h"

namespace mttApi
{
	class Dht
	{
	public:

		/*
			dht is started and any stop action hasnt finished
		*/
		API_EXPORT bool active() const;

		/*
			Get currently stored nodes count
		*/
		API_EXPORT uint32_t getNodesCount() const;
	};
}
