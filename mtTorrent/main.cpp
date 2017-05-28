#include <iostream>
#include <istream>
#include <ostream>
#include <string>
//#include <RiotRestApi.h>
#include "Core.h"

mtt::Core core;

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

int main(int argc, char* argv[])
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);

	try
	{
		if (auto t = core.getTorrent(core.addTorrent("D:\\hero.torrent")))
		{
			t->scheduler->selectFiles({ t->info.files.front() });
			t->start();
		}
	}
	catch (std::exception& e)
	{
		std::cout << "End exception: " << e.what() << "\n";
	}

	std::cout << "Count: " << std::to_string(gcount) << "\n";

	return 0;
}
#endif