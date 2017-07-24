#pragma once
#include "BencodeParser.h"
#include <mutex>

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
				std::vector<Addr> addedPeers;
			};

			void load(BencodeParser::Object& data);
			std::function<void(PeerExchange::Message&)> onPexMessage;
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

			uint32_t size = 0;

			void load(BencodeParser::Object& data, const char* remainingData, size_t remainingSize);
			DataBuffer createMetadataRequest(uint32_t index);

			std::function<void(UtMetadata::Message&)> onUtMetadataMessage;
		};

		struct ExtensionProtocol
		{
			struct
			{
				bool enabled = false;
				std::string yourIp;
				std::string client;
			}
			state;

			bool isSupported(MessageType type);

			MessageType load(char id, DataBuffer& data);
			DataBuffer getExtendedHandshakeMessage(bool enablePex = true, uint16_t metadataSize = 0);

			PeerExchange pex;
			UtMetadata utm;

		private:

			BencodeParser parser;
			std::map<int, MessageType> messageIds;
		};
	}
}
