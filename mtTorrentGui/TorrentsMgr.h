#pragma once
#include "QString"

class TorrentsMgr
{
public:

	~TorrentsMgr();

	void init();

	bool addTorrent(QString& filename, uint32_t& id);
};