#pragma once

#include "PeerCommunication2.h"

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

class LocalWithTorrentFile : public BasicPeerListener
{
	bool failed = false;

	virtual void connectionClosed() override
	{
		failed = true;
	}

	mtt::ext::UtMetadata::Message utmMsg;
	virtual void metadataPieceReceived(mtt::ext::UtMetadata::Message&) override;

public:

	void run();
};