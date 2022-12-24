#pragma once

#include <map>
#include <string>
#include "utils/DataBuffer.h"

namespace mtt
{
	class PeerCommunication;

	namespace ext
	{
		enum class Type
		{
			Handshake = 0,
			Pex,
			UtMetadata,
			UtHolepunch
		};

		struct Handshake
		{
			std::map<std::string, int> messageIds;

			uint8_t yourIp[4]{};
			std::string client;
			uint32_t metadataSize = 0;
		};

		struct RemoteExtension
		{
			void init(const char* name, const Handshake&, PeerCommunication*);
			bool enabled() const;

		protected:
			uint8_t messageId = 0;
			PeerCommunication* peer = nullptr;
		};
	};
}
