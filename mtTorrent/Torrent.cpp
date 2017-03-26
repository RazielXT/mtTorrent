#include "Torrent.h"

mtt::Torrent::Torrent(std::string filepath)
{
	BencodeParser parser;
	if (!parser.parseFile(filepath.data()))
		return;

	static uint32_t idCounter = 1;
	id = idCounter++;

	info = parser.parseTorrentInfo();
	scheduler = std::make_shared<ProgressScheduler>(&info);
}

void mtt::Torrent::start()
{
	if (!comm)
		comm = new PeerMgr(&info, *scheduler);

	comm->start();
}

void mtt::Torrent::stop()
{
	if (comm)
		delete comm;
}

