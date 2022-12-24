#pragma once

#include "ExtensionProtocol.h"
#include "utils/Network.h"
#include "Api/Interface.h"
#include <set>

namespace mtt
{
	class PeerCommunication;

	namespace ext
	{
		namespace PeerExchange
		{
			const static uint32_t UpdateFreq = 60;

			enum Flags
			{
				SupportsEncryption = 0x1,
				Seed = 0x2,
				SupportsUtp = 0x4,
				SupportsUth = 0x8
			};

			struct Message
			{
				std::vector<Addr> addedPeers;
				BufferView addedFlags;
				std::vector<Addr> added6Peers;
				BufferView added6Flags;
				std::vector<Addr> droppedPeers;
			};

			struct LocalData
			{
				LocalData();

				void update(const std::vector<PeerCommunication*>&);

				bool empty() const;
				DataBuffer getPexData() const;
				DataBuffer createFirstPexData() const;

				size_t hash = 0;

			private:
				DataBuffer pexData;
				Timestamp dataTime;
				uint32_t droppedDataOffset[2] = {};

				std::set<Addr> dataPeers;
				mutable std::mutex dataMutex;
			};

			struct Remote : public RemoteExtension
			{
				bool load(const BufferView& data, Message& msg);
				void update(const LocalData& data);
				bool wasConnected(const Addr&) const;

			private:
				size_t lastHash = 0;
				uint32_t lastSendTime = 0;

				std::set<Addr> peers;
				mutable std::mutex peersMutex;
			};
		}
	}
}
