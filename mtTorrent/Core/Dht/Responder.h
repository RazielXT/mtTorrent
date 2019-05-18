#pragma once
#include "Dht/Table.h"
#include "Dht/DataListener.h"
#include "utils/ScheduledTimer.h"
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

			bool handlePacket(udp::endpoint&, DataBuffer&);

			std::shared_ptr<Table> table;

		private:

			DataListener& listener;

			bool writeNodes(const char* hash, udp::endpoint& e, const mtt::BencodeParser::Object* requestData, PacketBuilder& out);
			bool writeValues(const char* infoHash, udp::endpoint& e, PacketBuilder& out);

			struct StoredValue
			{
				Addr addr;
				uint32_t timestamp;
			};

			std::mutex valuesMutex;
			std::map<NodeId, std::vector<StoredValue>> values;

			void announcedPeer(const char* infoHash, Addr& peer);
			void refreshStoredValues();

			std::mutex tokenMutex;
			uint32_t tokenSecret[2];

			bool isValidToken(uint32_t token, udp::endpoint& e);
			uint32_t getAnnounceToken(udp::endpoint& e);
			uint32_t getAnnounceToken(std::string& addr, uint32_t secret);
		};
	}
}
