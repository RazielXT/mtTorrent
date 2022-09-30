#pragma once

#include "utils/BencodeParser.h"
#include "utils/TcpAsyncStream.h"
#include "Utp/UtpManager.h"
#include <map>

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
				std::string addedFlags;
				std::vector<Addr> addedPeers;
			};

			void load(BencodeParser::Object* data);
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

			void load(BencodeParser::Object* data, const char* remainingData, std::size_t remainingSize);
			DataBuffer createMetadataRequest(uint32_t index);

			std::function<void(UtMetadata::Message&)> onUtMetadataMessage;
		};

		struct ExtensionProtocol
		{
			struct
			{
				bool sentHandshake = false;
				bool enabled = false;
				std::string yourIp;
				std::string client;
			}
			state;

			bool isSupported(MessageType type);

			bool requestMetadataPiece(uint32_t index);

			PeerExchange pex;

			UtMetadata utm;

			void sendHandshake();

			MessageType load(char id, const DataBuffer& data);
			DataBuffer createExtendedHandshakeMessage(bool enablePex = true, uint16_t metadataSize = 0);

			std::function<void(const DataBuffer&)> write;

			std::map<int, MessageType> messageIds;
		};
	}
}
