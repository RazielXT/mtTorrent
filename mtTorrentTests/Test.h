#pragma once

#include "PeerCommunication.h"
#include "Dht/Communication.h"
#include <mutex>

class TorrentTest
{
public:

	TorrentTest();

	void testMetadataReceive();
	void testAsyncDhtUdpRequest();
	void testAsyncDhtGetPeers();
	void testTrackers();
	void testStorageLoad();
	void testStorageCheck();
	void testDumpStoredPiece();
	void testPeerListen();
	void testDhtTable();
	void testTorrentFileSerialization();
	void bigTestGetTorrentFileByLink();
	void idealMagnetLinkTest();

	void start();
};