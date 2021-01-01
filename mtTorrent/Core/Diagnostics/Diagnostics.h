#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <map>

namespace mtt
{
	namespace Diagnostics
	{
		using Buffer = std::vector<uint8_t>;

		using TimeOffset = int64_t;

		enum class PeerEventType : uint8_t
		{
			Connect, Connected, RemoteConnected, Disconnect, ReceiveMessage, Interested, Choke, RequestPiece, Piece
		};

		struct PeerEvent
		{
			PeerEventType type;

			uint32_t idx;
			uint32_t subIdx;
			//uint32_t sz;

			TimeOffset time;
		};

		struct PeerSnapshot
		{
			std::string name;
			std::vector<PeerEvent> events;

			PeerSnapshot& operator+=(const PeerSnapshot&);
		};

		class Peer
		{
		public:

			void serialize(Buffer&) const;
			const uint8_t* deserialize(const uint8_t*);

			void clear();
			void addSnapshotEvent(PeerEvent event);

			PeerSnapshot snapshot;
			mutable std::mutex snapshotMutex;
		};

		class Storage
		{
		public:
			
			Storage();

			void addPeer(Peer&);
			void flush();

			std::vector<PeerSnapshot> loadPeers();

			std::string folder;

			uint32_t timestart = 0;

		public:
			Buffer buffer;
			uint32_t index = 0;
		};
	}
}
