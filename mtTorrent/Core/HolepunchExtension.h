#pragma once

#include "ExtensionProtocol.h"
#include "utils/Network.h"

namespace mtt
{
	namespace ext
	{
		struct UtHolepunch : public RemoteExtension
		{
			enum MessageId
			{
				Rendezvous,
				Connect,
				Error
			};

			enum ErrorCode
			{
				E_None,
				E_NoSuchPeer,
				E_NotConnected,
				E_NoSupport,
				E_NoSelf
			};

			struct Message
			{
				MessageId id;
				Addr addr;
				ErrorCode e;
			};

			static bool Load(const BufferView& data, Message& msg);

			void sendConnect(const Addr&);
			void sendRendezvous(const Addr&);
			void sendError(const Addr&, ErrorCode);

		private:
			void send(const Addr&, MessageId, ErrorCode);
		};
	}
}
