#pragma once

#include "IPeerListener.h"
#include "TrackerManager.h"
#include "Dht/Listener.h"

namespace mtt
{
	enum class PeerSource
	{
		Tracker,
		Pex,
		Dht,
		Manual
	};

	class Peers;
	namespace dht
	{
		class ResultsListener;
	}

	class PeerStatistics : public IPeerListener, public dht::ResultsListener
	{
	public:

		PeerStatistics(Peers&);

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

		Peers& peers;
		IPeerListener* peerListener = nullptr;

		std::shared_ptr<ScheduledTimer> speedMeasureTimer;
		std::vector<std::pair<PeerCommunication*, size_t>> lastSpeedMeasure;
		void updateMeasures();

		void evalCurrentPeers();
		uint32_t secondsFromLastPeerEval = 0;

		void checkForDhtPeers();
		uint32_t secondsFromLastDhtCheck = 5*60;

		virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values);
		virtual void findingPeersFinished(uint8_t* hash, uint32_t count);
	};

	class Peers
	{
		friend PeerStatistics;
	public:

		Peers(TorrentPtr torrent);

		using PeersUpdateCallback = std::function<void(Status, const std::string& source)>;
		void start(PeersUpdateCallback onPeersUpdated, IPeerListener* peerListener);
		void stop();

		void connectNext(uint32_t count);
		std::shared_ptr<PeerCommunication> connect(Addr& addr);
		void disconnect(PeerCommunication*);

		bool active = false;

		TrackerInfo getSourceInfo(const std::string& source);
		std::vector<TrackerInfo> getSourcesInfo();
		uint32_t getSourcesCount();

		TrackerManager trackers;

		uint32_t connectedCount();
		uint32_t receivedCount();

		PeerStatistics statistics;

		struct PeerInfo
		{
			Addr addr;
			PeerSource source;
			uint32_t lastSpeed = 0;
			float percentage = 0;
		};
		std::vector<PeerInfo> getConnectedInfo();
	private:

		PeersUpdateCallback updateCallback;

		enum class PeerQuality {Unknown, Potential, Offline, Unwanted, Bad, Normal, Good};
		struct KnownPeer
		{
			bool operator==(const Addr& r);

			Addr address;
			PeerSource source;
			PeerQuality lastQuality = PeerQuality::Unknown;
			size_t downloaded = 0;
			uint32_t lastSpeed = 0;
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
		};
		std::vector<ActivePeer> activeConnections;
		mtt::Peers::KnownPeer* mtt::Peers::getKnownPeer(PeerCommunication* p);
		mtt::Peers::ActivePeer* mtt::Peers::getActivePeer(PeerCommunication* p);

		TorrentPtr torrent;

		TrackerInfo dhtInfo;
		TrackerInfo pexInfo;
	};
}
