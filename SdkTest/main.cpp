#include "Api\Core.h"
#include <windows.h>

int main()
{
	auto core = mttApi::Core::create();

	auto torrent = core->addFile("D:\\boruto.torrent");

	auto info = torrent->getFileInfo();

	torrent->start();

	for (int i = 0; i < 10; i++)
	{
		Sleep(1000);
		printf("Progress: %d Speed: %d\n", (int)torrent->downloaded(), (int)torrent->downloadSpeed());
	}

	torrent->stop();

	return 0;
}