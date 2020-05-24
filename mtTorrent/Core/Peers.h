#pragma once

#include "IPeerListener.h"
#include "TrackerManager.h"
#include "Dht/Listener.h"
#include "LogFile.h"
#include "Api/Peers.h"

namespace mtt
{
	class Peers : public mttApi::Peers
	{
	public:

		Peers(TorrentPtr torrent);

		using PeersUpdateCallback = std::function<void(Status, PeerSource)>;
		void start(PeersUpdateCallback onPeersUpdated, IPeerListener* peerListener);
		void stop();

		void connectNext(uint32_t count);
		std::shared_ptr<PeerCommunication> connect(Addr& addr);
		std::shared_ptr<PeerCommunication> getPeer(PeerCommunication*);
		size_t add(std::shared_ptr<TcpAsyncStream> peer, const BufferView& data);
		std::shared_ptr<PeerCommunication> disconnect(PeerCommunication*);

		bool active = false;

		std::vector<TrackerInfo> getSourcesInfo();
		void refreshSource(const std::string& name);

		TrackerManager trackers;

		uint32_t connectedCount();
		uint32_t receivedCount();
		std::vector<std::shared_ptr<PeerCommunication>> getActivePeers();

		void reloadTorrentInfo();

	private:

		enum LogEvent : uint8_t { Connect, RemoteConnect, Remove, ConnectPeers };
#ifdef PEER_DIAGNOSTICS
		struct LogEventParams
		{
			LogEvent e;
			char info;
			uint16_t id2;
			uint32_t id;
			long time;
		};
		std::vector<LogEventParams> logevents;
		std::mutex logmtx;
		void addLogEvent(LogEvent e, Addr& id, char info = 0);
		void saveLogEvents();
#else
		void addLogEvent(LogEvent, Addr&, char) {}
		void saveLogEvents() {}
#endif

		TorrentPtr torrent;

		class PeersListener : public mtt::IPeerListener, public std::enable_shared_from_this<PeersListener>
		{
		public:
			PeersListener(Peers*);
			virtual void handshakeFinished(mtt::PeerCommunication*) override;
			virtual void connectionClosed(mtt::PeerCommunication*, int) override;
			virtual void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override;
			virtual void extHandshakeFinished(mtt::PeerCommunication*) override;
			virtual void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override;
			virtual void pexReceived(mtt::PeerCommunication*, mtt::ext::PeerExchange::Message&) override;
			virtual void progressUpdated(mtt::PeerCommunication*, uint32_t) override;
			void setTarget(mtt::IPeerListener*);
			void setParent(Peers*);

			std::mutex mtx;
			mtt::IPeerListener* target = nullptr;
			Peers* peers;
		};

		std::shared_ptr<PeersListener> peersListener;

		PeersUpdateCallback updateCallback;

		enum class PeerQuality {Unknown, Closed, Offline, Connecting, Bad, Normal, Good};
		struct KnownPeer
		{
			bool operator==(const Addr& r);

			Addr address;
			PeerSource source;
			PeerQuality lastQuality = PeerQuality::Unknown;
			uint32_t lastConnectionTime = 0;
			uint32_t connectionAttempts = 0;
		};

		uint32_t updateKnownPeers(const std::vector<Addr>& peers, PeerSource source);
		uint32_t updateKnownPeers(Addr& peers, PeerSource source);
		std::vector<KnownPeer> knownPeers;
		std::mutex peersMutex;

		std::shared_ptr<PeerCommunication> connect(uint32_t idx);
		std::shared_ptr<PeerCommunication> getActivePeer(Addr&);
		struct ActivePeer
		{
			std::shared_ptr<PeerCommunication> comm;
			uint32_t idx;
		};
		std::vector<ActivePeer> activeConnections;
		mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(PeerCommunication* p);
		mtt::Peers::ActivePeer* mtt::Peers::getActivePeer(PeerCommunication* p);

		TrackerInfo pexInfo;

		class DhtSource : public dht::ResultsListener
		{
		public:

			DhtSource(Peers&, TorrentPtr);

			void start();
			void stop();

			void findPeers();

			TrackerInfo info;

		private:
			virtual uint32_t dhtFoundPeers(const uint8_t* hash, std::vector<Addr>& values) override;
			virtual void dhtFindingPeersFinished(const uint8_t* hash, uint32_t count) override;

			std::shared_ptr<ScheduledTimer> dhtRefreshTimer;
			std::mutex timerMtx;

			Peers& peers;
			TorrentPtr torrent;

			int cfgCallbackId = -1;
		}
		dht;
	};
}
