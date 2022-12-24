#pragma once

#include "IPeerListener.h"
#include "TrackerManager.h"
#include "Dht/Listener.h"
#include "Api/Peers.h"
#include "PexExtension.h"

namespace mtt
{
	class Peers : public mttApi::Peers, public mtt::IPeerListener
	{
	public:

		Peers(Torrent& torrent);

		using PeersUpdateCallback = std::function<void(Status, PeerSource)>;
		void start(PeersUpdateCallback onPeersUpdated, IPeerListener* peerListener);
		void stop();

		void connectNext(uint32_t count);
		void connect(const Addr& addr);
		size_t add(std::shared_ptr<PeerStream> stream, const BufferView& data);
		std::shared_ptr<PeerCommunication> disconnect(PeerCommunication*);

		std::vector<TrackerInfo> getSourcesInfo();
		void refreshSource(const std::string& name);

		TrackerManager trackers;
		Torrent& torrent;

		uint32_t connectedCount() const;
		uint32_t receivedCount() const;
		std::vector<std::shared_ptr<PeerCommunication>> getActivePeers() const;

		void reloadTorrentInfo();

	private:

		FileLog log;

		enum class PeerQuality { Unknown, Closed, Offline, Connecting, Bad, Normal, Good };
		struct KnownPeer
		{
			bool operator==(const Addr& r);

			Addr address;
			uint8_t pexFlags = {};
			PeerSource source;
			PeerQuality lastQuality = PeerQuality::Unknown;
			Timestamp lastConnectionTime = 0;
			uint32_t connectionAttempts = 0;
		};

		uint32_t updateKnownPeers(const ext::PeerExchange::Message& pex);
		uint32_t updateKnownPeers(const std::vector<Addr>& peers, PeerSource source, const uint8_t* flags = nullptr);
		uint32_t updateKnownPeers(const Addr& addr, PeerSource source);
		std::vector<KnownPeer> knownPeers;
		mutable std::mutex peersMutex;

		std::shared_ptr<PeerCommunication> disconnect(PeerCommunication*, KnownPeer* info);
		void connect(uint32_t idx);
		struct ActivePeer
		{
			std::shared_ptr<PeerCommunication> comm;
			uint32_t idx;
		};
		std::vector<ActivePeer> activeConnections;
		KnownPeer* getKnownPeer(PeerCommunication* p);
		ActivePeer* getActivePeer(PeerCommunication* p);
		ActivePeer* getActivePeer(const Addr&);

		std::shared_ptr<ScheduledTimer> refreshTimer;
		void update();

		struct
		{
			TrackerInfo info;
			ext::PeerExchange::LocalData local;
		}
		pex;

		TrackerInfo remoteInfo;

		class DhtSource : public dht::ResultsListener
		{
		public:

			DhtSource(Peers&, Torrent&);

			void start();
			void stop();

			void update();
			void findPeers();

			TrackerInfo info;

		private:
			uint32_t dhtFoundPeers(const uint8_t* hash, const std::vector<Addr>& values) override;
			void dhtFindingPeersFinished(const uint8_t* hash, uint32_t count) override;

			Peers& peers;
			Torrent& torrent;

			int cfgCallbackId = -1;
		}
		dht;

		void handshakeFinished(mtt::PeerCommunication*) override;
		void connectionClosed(mtt::PeerCommunication*, int) override;
		void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override;
		void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) override;
		void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView&) override;

		std::mutex listenerMtx;
		mtt::IPeerListener* listener = nullptr;
		void setTargetListener(mtt::IPeerListener*);

		PeersUpdateCallback updateCallback;

		struct HolepunchState
		{
			Addr target;
			PeerCommunication* negotiator = {};
			Timestamp time{};
		};
		std::vector<HolepunchState> holepunchStates;
		std::mutex holepunchMutex;

		void evaluatePossibleHolepunch(PeerCommunication*, const KnownPeer&);
		void handleHolepunchMessage(PeerCommunication*, const ext::UtHolepunch::Message&);
	};
}
