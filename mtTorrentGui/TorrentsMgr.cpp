#include "TorrentsMgr.h"
#include "..\mtTorrent\Interface.h"
#include <Windows.h>

HMODULE lib = nullptr;
mtIoctl libIoctl = nullptr;

TorrentsMgr::~TorrentsMgr()
{
	CloseHandle(lib);
}

void TorrentsMgr::init()
{
	lib = LoadLibrary(L"mtTorrent.dll");
	libIoctl = (mtIoctl)GetProcAddress(lib, "Ioctl");
}

bool TorrentsMgr::addTorrent(QString& filename, uint32_t& id)
{
	mttLib::AddTorrentFileParams params;
	params.path = filename.toUtf8().constData();

	if (libIoctl(mttLib::AddTorrentFile, &params) == mttLib::Ok)
	{
		id = params.id;
		return true;
	}

	return false;
}

