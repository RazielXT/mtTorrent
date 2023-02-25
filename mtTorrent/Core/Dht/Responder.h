#pragma once
#include "Dht/Table.h"
#include "Dht/DataListener.h"
#include "utils/BencodeParser.h"
#include "utils/PacketHelper.h"
#include <map>

namespace mtt
{
	namespace dht
	{
		class Responder
		{
		public:

			Responder(DataListener&);

			void refresh();

			bool handlePacket(const udp::endpoint&, const BufferView&);

			std::shared_ptr<Table> table;

		private:

			DataListener& listener;

			bool writeNodes(const char* hash, const udp::endpoint& e, const mtt::BencodeParser::Object* requestData, PacketBuilder& out);
			bool writeValues(const char* infoHash, const udp::endpoint& e, PacketBuilder& out);

			struct StoredValue
			{
				Addr addr;
				Timestamp timestamp;
			};

			std::mutex valuesMutex;
			std::map<NodeId, std::vector<StoredValue>> values;

			void announcedPeer(const char* infoHash, const Addr& peer);
			void refreshStoredValues();

			std::mutex tokenMutex;
			uint32_t tokenSecret[2];

			bool isValidToken(uint32_t token, const udp::endpoint& e);
			uint32_t getAnnounceToken(const udp::endpoint& e);
			uint32_t getAnnounceToken(const std::string& addr, uint32_t secret);
		};
	}
}
