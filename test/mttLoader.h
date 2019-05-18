#pragma once
#include "../mtTorrent/Public/BinaryInterface.h"
#include "../mtTorrent/Public/Status.h"
#include "Windows.h"

typedef mtt::Status(*IOCTL_FUNC)(mtBI::MessageId, const void*, void*);

namespace mtt
{
	IOCTL_FUNC IoctlFunc = nullptr;
	HMODULE lib = nullptr;

	bool load()
	{
		lib = LoadLibraryA("mtTorrent.dll");

		if (lib)
		{
			IoctlFunc = (IOCTL_FUNC)GetProcAddress(lib, "Ioctl");
			IoctlFunc(mtBI::MessageId::Init, nullptr, nullptr);
		}

		return lib != nullptr;
	}

	void unload()
	{
		if (IoctlFunc)
			IoctlFunc(mtBI::MessageId::Deinit, nullptr, nullptr);

		if (lib)
			FreeLibrary(lib);
	}
}
