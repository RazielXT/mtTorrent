#pragma once

#include "ExtensionProtocol.h"
#include "PeerMessage.h"

namespace mtt
{
	class PeerCommunication;

	class IPeerListener
	{
	public:

		virtual void handshakeFinished(PeerCommunication*) = 0;
		virtual void connectionClosed(PeerCommunication*, int code) = 0;

		virtual void messageReceived(PeerCommunication*, PeerMessage&) = 0;

		virtual void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) = 0;
		virtual void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView& data) = 0;
	};
}
