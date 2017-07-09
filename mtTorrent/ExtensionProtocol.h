#pragma once
#include "BencodeParser.h"
#include <mutex>
#include "TorrentDefines.h"

namespace mtt
{
	namespace ext
	{
		enum MessageType
		{
			HandshakeEx = 0,
			PexEx,
			UtMetadataEx,
			InvalidEx
		};

		struct PeerExchange
		{
			struct Message
			{
				std::string added;
				std::string addedFlags;
				std::vector<PeerInfo> addedPeers;
			};

			Message load(BencodeParser::Object& data);
		};

		struct UtMetadata
		{
			enum MessageId
			{
				Request = 0,
				Data,
				Reject
			};

			struct Message
			{
				MessageId id;
				uint32_t piece;
				DataBuffer metadata;
				uint32_t size;
			};

			uint32_t size;
			Message load(BencodeParser::Object& data, const char* remainingData, size_t remainingSize);
		};

		struct ExtensionProtocol
		{
			MessageType load(char id, DataBuffer& data);
			DataBuffer getExtendedHandshakeMessage(bool enablePex = true, uint16_t metadataSize = 0);

			std::function<void(PeerExchange::Message&)> onPexMessage;
			std::function<void(UtMetadata::Message&)> onUtMetadataMessage;

			PeerExchange pex;
			UtMetadata utm;

		private:

			BencodeParser parser;
			std::map<int, MessageType> messageIds;
		};
	}
}
