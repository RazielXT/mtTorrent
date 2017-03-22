#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <RiotRestApi.h>
#include "Communicator.h"

#ifndef STANDALONE

/*typedef void(*MSGFUNC)(UiMessage*);
extern "C"
{
__declspec(dllexport) void __cdecl StartUi(HWND*, HWND*);

__declspec(dllexport) void __cdecl SendMsg(UiMessage*);

__declspec(dllexport) void __cdecl SetMsgFunc(MSGFUNC);
}

MSGFUNC sendMsg;

__declspec(dllexport) void __cdecl SetMsgFunc(MSGFUNC func)
{
sendMsg = func;
}

__declspec(dllexport) void __cdecl SendMsg(UiMessage* msg)
{
FormsUpdate(msg);
}

__declspec(dllexport) void __cdecl StartUi(HWND* hwnd, HWND* parent)
{
FormsMain(hwnd, parent);
}*/

int __stdcall Main(HINSTANCE h, ULONG ulReason, PVOID pvReserved) {

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#else
int main(int argc, char* argv[])
{
	try
	{
		//riotApiRequest();

		Torrent::Communicator torrent;
		torrent.test();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}

	std::cout << "Count: " << std::to_string(gcount) << "\n";

	return 0;
}
#endif