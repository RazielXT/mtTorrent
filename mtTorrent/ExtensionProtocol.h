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
		UtMetadataEx,
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

	struct UtMetadataExtension
	{
		int size = 0;
		uint32_t remainingPiecesFlag = 0;
		DataBuffer metadata;

		bool isFull();
		void setSize(int size);
		void load(BencodeParser::Object& data, const char* remainingData, size_t remainingSize);
	};

	struct ExtensionProtocol
	{	
		ExtendedMessageType load(char id, DataBuffer& data);
		DataBuffer getExtendedHandshakeMessage(bool enablePex = true, uint16_t metadataSize = 0);

		PeerExchangeExtension pex;
		UtMetadataExtension utm;

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
