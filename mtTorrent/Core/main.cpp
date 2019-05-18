#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <Test.h>

#ifndef MTT_TEST_STANDALONE

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