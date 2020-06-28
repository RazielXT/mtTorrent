#pragma once
#include "../../mtTorrent/Public/BinaryInterface.h"
#include "../../mtTorrent/Public/Status.h"
#include <windows.h>

class MttLibrary
{
public:

	~MttLibrary() {};

	bool loadLibrary()
	{
		lib = LoadLibrary(L"mtTorrent.dll");

		if (lib)
		{
			IoctlFunc = (IOCTL_FUNC)GetProcAddress(lib, "Ioctl");
			IoctlFunc(mtBI::MessageId::Init, nullptr, nullptr);
		}

		return lib != nullptr;
	}

	void unloadLibrary()
	{
		if (IoctlFunc)
			IoctlFunc(mtBI::MessageId::Deinit, nullptr, nullptr);

		if (lib)
			FreeLibrary(lib);
	}

	typedef mtt::Status(*IOCTL_FUNC)(mtBI::MessageId, const void*, void*);
	IOCTL_FUNC IoctlFunc = nullptr;

private:

	HMODULE lib = nullptr;
};