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

		void start(IPeerListener* listener);
		void stop();

		void startConnecting();
		void stopConnecting();

		Status connect(const Addr& addr);
		size_t add(std::shared_ptr<PeerStream> stream, const BufferView& data);
		bool disconnect(PeerCommunication*, bool unwanted = false);
		void inspectConnectedPeers(std::function<void(const std::vector<PeerCommunication*>&)>);

		std::vector<TrackerInfo> getSourcesInfo();
		void refreshSource(const std::string& name);
		Status importTrackers(const std::vector<std::string>& urls);

		TrackerManager trackers;
		Torrent& torrent;

		uint32_t connectedCount() const;
		uint32_t receivedCount() const;
		std::vector<mtt::ConnectedPeerInfo> getConnectedPeersInfo() const;

		void reloadTorrentInfo();

	private:

		FileLog log;

		struct KnownPeer
		{
			bool operator==(const Addr& r);

			Addr address;
			PeerSource source;

			uint8_t pexFlags = {};
			bool unwanted = false;

			enum class State { Disconnected, Connecting, Connected };
			State state = State::Disconnected;

			Timestamp lastConnectionTime = 0;
			uint32_t connectionAttempts = 0;
		};

		uint32_t updateKnownPeers(const ext::PeerExchange::Message& pex);
		uint32_t updateKnownPeers(const std::vector<Addr>& peers, PeerSource source, const uint8_t* flags = nullptr);
		std::vector<KnownPeer> knownPeers;
		size_t currentConnectIdx = 0;

		mutable std::mutex peersMutex;

		std::shared_ptr<PeerCommunication> removePeer(PeerCommunication*, KnownPeer& info);
		void connect(const Addr& addr, uint32_t knownIdx);
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

		bool stopped() const;
		void connectNext();
		bool connecting = false;

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

		class NoListener : public mtt::IPeerListener
		{
		public:
			void handshakeFinished(PeerCommunication*) override {};
			void connectionClosed(PeerCommunication*, int code) override {};
			void messageReceived(PeerCommunication*, PeerMessage&) override {};
			void extendedHandshakeFinished(PeerCommunication*, const ext::Handshake&) override {};
			void extendedMessageReceived(PeerCommunication*, ext::Type, const BufferView&) override {};
		}
		nolistener;

		std::atomic<mtt::IPeerListener*> listener = nullptr;
		void setTargetListener(mtt::IPeerListener*);

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
