#pragma once

#include "IPeerListener.h"
#include "TrackerManager.h"
#include "Dht/Listener.h"
#include "LogFile.h"

namespace mtt
{
	enum class PeerSource
	{
		Tracker,
		Pex,
		Dht,
		Manual,
		Remote
	};

	class Peers;
	namespace dht
	{
		class ResultsListener;
	}

	class PeersAnalyzer : public IPeerListener, public dht::ResultsListener
	{
	public:

		PeersAnalyzer(Peers&);

		void start();
		void stop();

		virtual void handshakeFinished(PeerCommunication*) override;
		virtual void connectionClosed(PeerCommunication*, int) override;
		virtual void messageReceived(PeerCommunication*, PeerMessage&) override;
		virtual void extHandshakeFinished(PeerCommunication*) override;
		virtual void metadataPieceReceived(PeerCommunication*, ext::UtMetadata::Message&) override;
		virtual void pexReceived(PeerCommunication*, ext::PeerExchange::Message&) override;
		virtual void progressUpdated(PeerCommunication*) override;

		uint32_t getDownloadSpeed(PeerCommunication*);
		uint32_t getDownloadSpeed();
		uint32_t getUploadSpeed();
		size_t getUploadSum();
		size_t uploaded = 0;

		Peers& peers;
		IPeerListener* peerListener = nullptr;

		std::mutex measureMutex;
		std::shared_ptr<ScheduledTimer> speedMeasureTimer;
		std::vector<std::pair<PeerCommunication*, std::pair<size_t, size_t>>> lastSpeedMeasure;
		void updateMeasures();

		void evalCurrentPeers();
		uint32_t secondsFromLastPeerEval = 0;

		void checkForDhtPeers();
		uint32_t secondsFromLastDhtCheck = 5*60;

		virtual uint32_t dhtFoundPeers(uint8_t* hash, std::vector<Addr>& values);
		virtual void dhtFindingPeersFinished(uint8_t* hash, uint32_t count);

		LogFile log;
	};

	class Peers
	{
		friend PeersAnalyzer;
	public:

		Peers(TorrentPtr torrent);

		using PeersUpdateCallback = std::function<void(Status, PeerSource)>;
		void start(PeersUpdateCallback onPeersUpdated, IPeerListener* peerListener);
		void stop();

		void connectNext(uint32_t count);
		std::shared_ptr<PeerCommunication> connect(Addr& addr);
		std::shared_ptr<PeerCommunication> getPeer(PeerCommunication*);
		void add(std::shared_ptr<TcpAsyncStream> stream);
		std::shared_ptr<PeerCommunication> disconnect(PeerCommunication*);

		bool active = false;

		TrackerInfo getSourceInfo(const std::string& source);
		std::vector<TrackerInfo> getSourcesInfo();
		uint32_t getSourcesCount();
		void refreshSource(const std::string& name);

		TrackerManager trackers;

		uint32_t connectedCount();
		uint32_t receivedCount();

		PeersAnalyzer analyzer;

		struct PeerInfo
		{
			Addr address;
			PeerSource source;
			uint32_t downloadSpeed = 0;
			uint32_t uploadSpeed = 0;
			float percentage = 0;
		};
		std::vector<PeerInfo> getConnectedInfo();
	private:

		PeersUpdateCallback updateCallback;

		enum class PeerQuality {Unknown, Potential, Offline, Unwanted, Bad, Normal, Good};
		struct KnownPeer
		{
			bool operator==(const Addr& r);

			PeerInfo info;

			PeerQuality lastQuality = PeerQuality::Unknown;
			size_t downloaded = 0;
			size_t uploaded = 0;
			int connections = 0;
		};

		uint32_t updateKnownPeers(std::vector<Addr>& peers, PeerSource source);
		uint32_t updateKnownPeers(Addr& peers, PeerSource source);
		std::vector<KnownPeer> knownPeers;
		std::mutex peersMutex;

		std::shared_ptr<PeerCommunication> connect(uint32_t idx);
		std::shared_ptr<PeerCommunication> getActivePeer(Addr&);
		struct ActivePeer
		{
			std::shared_ptr<PeerCommunication> comm;
			uint32_t idx;
			uint32_t timeConnected;
			uint32_t timeLastPiece = 0;
		};
		std::vector<ActivePeer> activeConnections;
		mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(PeerCommunication* p);
		mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(mtt::Peers::ActivePeer* p);
		mtt::Peers::ActivePeer* mtt::Peers::getActivePeer(PeerCommunication* p);

		TorrentPtr torrent;

		TrackerInfo dhtInfo;
		TrackerInfo pexInfo;
	};
}
