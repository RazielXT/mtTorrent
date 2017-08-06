#pragma once

#include "PeerCommunication.h"
#include "DhtCommunication.h"
#include <mutex>

class BasicPeerListener : public mtt::IPeerListener
{
public:
	virtual void handshakeFinished(mtt::PeerCommunication*) override
	{
	}

	virtual void connectionClosed(mtt::PeerCommunication*) override
	{
	}

	virtual void messageReceived(mtt::PeerCommunication*, mtt::PeerMessage&) override
	{
	}

	virtual void progressUpdated(mtt::PeerCommunication*) override
	{
	}

	virtual void pieceReceiveFinished(mtt::PeerCommunication*, mtt::DownloadedPiece* piece) override
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

class TorrentTest : public BasicPeerListener, public mtt::dht::DhtListener
{
public:

	TorrentTest();

	void testMetadataReceive();
	void testAsyncUdpRequest();
	void testAsyncDhtGetPeers();
	void testTrackers();
	void testStorageLoad();
	void testStorageCheck();
	void testGetCountry();
	void testPeerListen();

	void start();
private:

	bool failed = false;
	virtual void connectionClosed(mtt::PeerCommunication*) override;

	mtt::ext::UtMetadata::Message utmMsg;
	virtual void metadataPieceReceived(mtt::PeerCommunication*, mtt::ext::UtMetadata::Message&) override;

	void onUdpReceived(DataBuffer* data, PackedUdpRequest* source);
	DataBuffer udpResult;

	virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
	virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;
	struct 
	{
		uint32_t finalCount = -1;

		std::mutex resultMutex;
		std::vector<Addr> values;
	}
	dhtResult;

};