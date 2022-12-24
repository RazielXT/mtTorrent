#include "ExtensionProtocol.h"

using namespace mtt::ext;

void RemoteExtension::init(const char* name, const Handshake& handshake, PeerCommunication* p)
{
	auto it = handshake.messageIds.find(name);

	if (it != handshake.messageIds.end())
	{
		messageId = (uint8_t)it->second;
		peer = p;
	}
}

bool RemoteExtension::enabled() const
{
	return messageId != 0;
}
