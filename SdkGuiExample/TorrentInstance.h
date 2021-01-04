#include "mtTorrent/Api/Core.h"

class TorrentInstance
{
public:

	TorrentInstance();

	void draw();

private:

	void drawInfoWindow();

	void drawPeersWindow();

	void drawSourcesWindow();

	mttApi::TorrentPtr torrentPtr;
	std::shared_ptr<mttApi::Core> core;
};