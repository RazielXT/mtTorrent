#pragma once

#include "Interface.h"

namespace mttApi
{
	class Listener
	{
	public:

		/*
			Get info about UPNP mapping, enabled by setting connection.upnpPortMapping
		*/
		API_EXPORT std::string getUpnpReadableInfo() const;

	};
}
