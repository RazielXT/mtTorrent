#pragma once

#include "PeerCommunication2.h"
#include "DhtCommunication.h"

class BasicPeerListener : public mtt::IPeerListener
{
public:
	virtual void handshakeFinished() override
	{
	}

	virtual void connectionClosed() override
	{
	}

	virtual void messageReceived(mtt::PeerMessage&) override
	{
	}

	virtual void progressUpdated() override
	{
	}

	virtual void pieceReceived(mtt::DownloadedPiece* piece) override
	{
	}

	virtual void metadataPieceReceived(mtt::ext::UtMetadata::Message&) override
	{
	}

	virtual void pexReceived(mtt::ext::PeerExchange::Message&) override
	{
	}
};

class LocalWithTorrentFile : public BasicPeerListener, public mtt::dht::DhtListener
{
public:

	LocalWithTorrentFile();

	void testMetadataReceive();
	void testAsyncUdpRequest();
	void testAsyncDhtGetPeers();

private:

	bool failed = false;

	virtual void connectionClosed() override
	{
		failed = true;
	}

	mtt::ext::UtMetadata::Message utmMsg;
	virtual void metadataPieceReceived(mtt::ext::UtMetadata::Message&) override;

	void onUdpReceived(DataBuffer* data, PackedUdpRequest* source);
	DataBuffer udpResult;

	virtual uint32_t onFoundPeers(uint8_t* hash, std::vector<Addr>& values) override;
	virtual void findingPeersFinished(uint8_t* hash, uint32_t count) override;
	struct 
	{
		uint32_t finalCount = -1;
		std::vector<Addr> values;
	}
	dhtResult;

};