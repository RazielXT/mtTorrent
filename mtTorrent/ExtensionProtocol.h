#pragma once
#include "BencodeParser.h"

namespace mtt
{
	enum ExtendedMessageType
	{
		HandshakeEx = 0,
		PexEx,
		InvalidEx
	};

	struct PeerExchangeExtension
	{
		std::string added;
		std::string addedFlags;

		std::vector<PeerInfo> addedPeers;

		void load(BencodeParser::Object& data);
	};

	struct ExtensionProtocol
	{	
		ExtendedMessageType load(char id, DataBuffer& data);
		DataBuffer getExtendedHandshakeMessage();

		PeerExchangeExtension pex;

		struct
		{
			std::string peerClient;
			std::string myIp;
		} 
		extInfo;

	private:

		BencodeParser parser;
		std::map<int, ExtendedMessageType> messageIds;
	};
}
