#include <iostream>
#include <istream>
#include <ostream>
#include <string>
//#include <RiotRestApi.h>
#include <Test.h>
#include "Global/BinaryInterface.h"
#include "Core.h"
#include "Peers.h"

#ifndef STANDALONE
#include "Windows.h"

mtt::Core core;

extern "C"
{
__declspec(dllexport) mtt::Status __cdecl Ioctl(mtBI::MessageId id, void* data)
{
	if (id == mtBI::MessageId::Start)
		core.start();
	else if (id == mtBI::MessageId::GetTorrentStateInfo)
	{
		auto resp = (mtBI::TorrentStateInfo*) data;
		resp->name.set(core.torrent->name());
		resp->connectedPeers = core.torrent->peers->connectedCount();
		resp->foundPeers = core.torrent->peers->receivedCount();
		resp->downloaded = core.torrent->downloaded();
		resp->downloadSpeed = core.torrent->downloadSpeed();
		resp->progress = core.torrent->currentProgress();
	}
	else if (id == mtBI::MessageId::GetPeersInfo)
	{
		auto resp = (mtBI::TorrentPeersInfo*) data;
		auto peers = core.torrent->peers->getConnectedInfo();
		resp->count = (uint32_t)std::min(resp->peers.size(), peers.size());
		for (size_t i = 0; i < resp->count; i++)
		{
			auto& peer = peers[i];
			auto& out = resp->peers[i];
			out.addr.set(peer.addr.toString());
			out.progress = peer.percentage;
			out.speed = peer.lastSpeed;

			if(peers[i].source == mtt::PeerSource::Tracker)
				memcpy(out.source, "Tracker", 8);
			else if (peers[i].source == mtt::PeerSource::Pex)
				memcpy(out.source, "Pex", 4);
			else if (peers[i].source == mtt::PeerSource::Dht)
				memcpy(out.source, "Dht", 4);
			else
				memcpy(out.source, "Manual", 7);
		}
	}
	else if (id == mtBI::MessageId::GetTorrentInfo)
	{
		auto resp = (mtBI::TorrentInfo*) data;
		resp->name.set(core.torrent->infoFile.info.name);
		resp->fullsize = core.torrent->infoFile.info.fullSize;
		resp->filesCount = (uint32_t)core.torrent->infoFile.info.files.size();

		if (resp->filenames.size() == resp->filesCount)
		{
			for (size_t i = 0; i < resp->filenames.size(); i++)
			{
				resp->filenames[i].set(core.torrent->infoFile.info.files[i].path.back());
				resp->filesizes[i] = core.torrent->infoFile.info.files[i].size;
			}
		}
	}
	else
		return mtt::Status::E_InvalidInput;

	return mtt::Status::Success;
}
}

int __stdcall Main(HINSTANCE h, ULONG ulReason, PVOID pvReserved) {

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
		//init
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#else

BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	//if(CEvent == CTRL_CLOSE_EVENT)
	//	MessageBox(NULL, "Program being closed!", "CEvent", MB_OK);
	
	return TRUE;
}

int main(int argc, char* argv[])
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

	try
	{
		TorrentTest test;
		test.start();
	}
	catch (std::exception& e)
	{
		std::cout << "End exception: " << e.what() << "\n";
	}

	return 0;
}
#endif