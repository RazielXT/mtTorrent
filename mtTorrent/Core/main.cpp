#ifdef _WIN32
#include <windows.h>

int __stdcall Main(HINSTANCE h, ULONG ulReason, PVOID pvReserved)
{
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
#endif
