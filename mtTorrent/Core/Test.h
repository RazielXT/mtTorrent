#pragma once

#include "PeerCommunication.h"
#include "Dht/Communication.h"
#include <mutex>

class BasicPeerListener : public mtt::IPeerListener
{
public:
	virtual void handshakeFinished(mtt::PeerCommunication*) override
	{
	}

	virtual void connectionClosed(mtt::PeerCommunication*, int) override
	{
	}

	virtual void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override
	{
	}

	virtual void progressUpdated(mtt::PeerCommunication*, uint32_t) override
	{
	}

	virtual void extHandshakeFinished(mtt::PeerCommunication*) override
	{
	}

	virtual void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override
	{
	}

	virtual void pexReceived(mtt::PeerCommunication*, mtt::ext::PeerExchange::Message&) override
	{
	}
};

class TorrentTest : public BasicPeerListener, public mtt::dht::ResultsListener
{
public:

	TorrentTest();

	void testMetadataReceive();
	void testAsyncDhtUdpRequest();
	void testAsyncDhtGetPeers();
	void testTrackers();
	void testStorageLoad();
	void testStorageCheck();
	void testGetCountry();
	void testPeerListen();
	void testDhtTable();
	void testTorrentFileSerialization();
	void bigTestGetTorrentFileByLink();
	void idealMagnetLinkTest();

	void start();

private:

	bool failed = false;
	virtual void connectionClosed(mtt::PeerCommunication*, int) override;

	mtt::ext::UtMetadata::Message utmMsg;
	virtual void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override;

	struct 
	{
		uint32_t finalCount = -1;

		std::mutex resultMutex;
		std::vector<Addr> values;
	}
	dhtResult;

	virtual uint32_t dhtFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
	virtual void dhtFindingPeersFinished(uint8_t* hash, uint32_t count) override;

	virtual void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage& msg) override
	{
		if (onPeerMsg)
			onPeerMsg(msg);
	};

	std::function<void(mtt::PeerMessage&)> onPeerMsg;
};