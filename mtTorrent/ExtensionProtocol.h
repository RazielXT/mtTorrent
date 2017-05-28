#pragma once
#include "BencodeParser.h"
#include <mutex>
#include "TorrentDefines.h"

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

		std::vector<PeerInfo> readPexPeers();
		void load(BencodeParser::Object& data);
		std::mutex dataMutex;
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
