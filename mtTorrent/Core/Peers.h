#pragma once

#include "IPeerListener.h"
#include "TrackerManager.h"
#include "Dht/Listener.h"
#include "Api/Peers.h"

namespace mtt
{
	class Peers : public mttApi::Peers
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

		class PeersListener : public mtt::IPeerListener, public std::enable_shared_from_this<PeersListener>
		{
		public:
			PeersListener(Peers*);
			void handshakeFinished(mtt::PeerCommunication*) override;
			void connectionClosed(mtt::PeerCommunication*, int) override;
			void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override;
			void extHandshakeFinished(mtt::PeerCommunication*) override;
			void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override;
			void pexReceived(mtt::PeerCommunication*, mtt::ext::PeerExchange::Message&) override;
			void progressUpdated(mtt::PeerCommunication*, uint32_t) override;
			void setTarget(mtt::IPeerListener*);
			void setParent(Peers*);

			std::mutex mtx;
			mtt::IPeerListener* target = nullptr;
			Peers* peers;
		};

		std::shared_ptr<PeersListener> peersListener;

		PeersUpdateCallback updateCallback;

		enum class PeerQuality { Unknown, Closed, Offline, Connecting, Bad, Normal, Good };
		struct KnownPeer
		{
			bool operator==(const Addr& r);

			Addr address;
			PeerSource source;
			PeerQuality lastQuality = PeerQuality::Unknown;
			Timestamp lastConnectionTime = 0;
			uint32_t connectionAttempts = 0;
		};

		uint32_t updateKnownPeers(const std::vector<Addr>& peers, PeerSource source);
		uint32_t updateKnownPeers(const Addr& addr, PeerSource source);
		std::vector<KnownPeer> knownPeers;
		mutable std::mutex peersMutex;

		void connect(uint32_t idx);
		std::shared_ptr<PeerCommunication> getActivePeer(const Addr&);
		struct ActivePeer
		{
			std::shared_ptr<PeerCommunication> comm;
			uint32_t idx;
		};
		std::vector<ActivePeer> activeConnections;
		KnownPeer* getKnownPeer(PeerCommunication* p);
		ActivePeer* getActivePeer(PeerCommunication* p);

		TrackerInfo pexInfo;
		TrackerInfo remoteInfo;

		class DhtSource : public dht::ResultsListener
		{
		public:

			DhtSource(Peers&, Torrent&);

			void start();
			void stop();

			void findPeers();

			TrackerInfo info;

		private:
			uint32_t dhtFoundPeers(const uint8_t* hash, const std::vector<Addr>& values) override;
			void dhtFindingPeersFinished(const uint8_t* hash, uint32_t count) override;

			std::shared_ptr<ScheduledTimer> dhtRefreshTimer;
			std::mutex timerMtx;

			Peers& peers;
			Torrent& torrent;

			int cfgCallbackId = -1;
		}
		dht;
	};
}
