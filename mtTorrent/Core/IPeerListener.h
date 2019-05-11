#pragma once
#include "PeerMessage.h"
#include "ExtensionProtocol.h"

namespace mtt
{
	class PeerCommunication;

	class IPeerListener
	{
	public:

		virtual void handshakeFinished(PeerCommunication*) = 0;
		virtual void connectionClosed(PeerCommunication*, int code) = 0;

		virtual void messageReceived(PeerCommunication*, PeerMessage&) = 0;

		virtual void extHandshakeFinished(PeerCommunication*) = 0;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) = 0;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) = 0;
		virtual void progressUpdated(PeerCommunication*, uint32_t) = 0;
	};
}
