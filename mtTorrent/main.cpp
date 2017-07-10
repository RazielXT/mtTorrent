#include <iostream>
#include <istream>
#include <ostream>
#include <string>
//#include <RiotRestApi.h>
#include "PeerCommunication2.h"

#ifndef STANDALONE

extern "C"
{
__declspec(dllexport) void __cdecl Ioctl(mttLib::MessageId id, void* data)
{
	//core.ioctlMsg(id, data);
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

int main(int argc, char* argv[])
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

	try
	{
		mtt::TorrentInfo torrent;
		boost::asio::io_service io;
		BasicPeerListener listener;

		mtt::PeerCommunication2 peer(torrent, listener, io);
	}
	catch (std::exception& e)
	{
		std::cout << "End exception: " << e.what() << "\n";
	}

	return 0;
}
#endif