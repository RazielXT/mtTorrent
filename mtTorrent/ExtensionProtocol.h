#pragma once
#include "BencodeParser.h"

namespace Torrent
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
		void setInfo(ClientInfo* client);
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

		ClientInfo* clientInfo;
		BencodeParser parser;
		std::map<int, ExtendedMessageType> messageIds;
	};
}
