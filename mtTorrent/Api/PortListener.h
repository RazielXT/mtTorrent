#pragma once

#include "Interface.h"

namespace mttApi
{
	class PortListener
	{
	public:

		/*
			Get info about UPNP mapping, enabled by setting connection.upnpPortMapping
		*/
		API_EXPORT std::string getUpnpReadableInfo() const;

	protected:

		PortListener() = default;
		PortListener(const PortListener&) = delete;
	};
}
