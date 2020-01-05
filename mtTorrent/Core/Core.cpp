#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "utils/TcpAsyncServer.h"
#include "IncomingPeersListener.h"
#include "State.h"
#include "utils/HexEncoding.h"

mtt::Core core;

extern void InitLogTime();


#include <DbgHelp.h>
#pragma comment (lib, "dbghelp.lib")

LONG WINAPI OurCrashHandler(EXCEPTION_POINTERS* pException)
{
	MINIDUMP_EXCEPTION_INFORMATION  M;
	M.ThreadId = GetCurrentThreadId();
	M.ExceptionPointers = pException;
	M.ClientPointers = 0;

	CHAR    Dump_Path[MAX_PATH];
	GetModuleFileName(NULL, Dump_Path, sizeof(Dump_Path));
	lstrcpy(Dump_Path + lstrlen(Dump_Path) - 3, "dmp");

	HANDLE hFile = CreateFile(Dump_Path, GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
	{
		//std::cerr << ("Failed to write dump: Invalid dump file");
	}
	else
	{
		const DWORD Flags = MiniDumpWithFullMemory |
			MiniDumpWithFullMemoryInfo |
			MiniDumpWithHandleData |
			MiniDumpWithUnloadedModules |
			MiniDumpWithThreadInfo;

		BOOL Result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hFile,
			(MINIDUMP_TYPE)Flags,
			&M,
			nullptr,
			nullptr);

		CloseHandle(hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void mtt::Core::init()
{
	::SetUnhandledExceptionFilter(OurCrashHandler);

	InitLogTime();

	mtt::config::load();

	dht = std::make_shared<dht::Communication>();

	if(mtt::config::getExternal().dht.enable)
		dht->start();

	listener = std::make_shared<IncomingPeersListener>([this](std::shared_ptr<TcpAsyncStream> s, const uint8_t* hash)
	{
		auto t = getTorrent(hash);
		if (t)
		{
			t->peers->add(s);
		}
	}
	);

	TorrentsList list;
	list.load();

	for (auto& t : list.torrents)
	{
		auto tPtr = Torrent::fromSavedState(t.name);

		if(!tPtr)
			continue;

		if (auto t = getTorrent(tPtr->hash()))
			continue;

		torrents.push_back(tPtr);
	}

	config::registerOnChangeCallback(config::ValueType::Dht, [this]()
		{
			if (mtt::config::getExternal().dht.enable)
				dht->start();
			else
				dht->stop();
		});
}

void mtt::Core::deinit()
{
	if (listener)
	{
		listener->stop();
		listener.reset();
	}

	TorrentsList list;
	for (auto& t : torrents)
	{
		list.torrents.push_back({ t->hashString() });
		t->stop();
	}

	list.save();
	torrents.clear();

	if (dht)
	{
		dht->stop();
		dht.reset();
	}

	UdpAsyncComm::Deinit();

	mtt::config::save();
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addFile(const char* filename)
{
	auto torrent = Torrent::fromFile(filename);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	if (auto t = getTorrent(torrent->hash()))
		return { mtt::Status::I_AlreadyExists, torrent };

	torrents.push_back(torrent);
	torrent->saveTorrentFile();
	torrent->checkFiles();

	return { mtt::Status::Success, torrent };
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addFile(const uint8_t* data, size_t size)
{
	auto torrent = Torrent::fromFileData(data, size);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	if (auto t = getTorrent(torrent->hash()))
		return { mtt::Status::I_AlreadyExists, torrent };

	torrents.push_back(torrent);
	torrent->saveTorrentFile();
	torrent->checkFiles();

	return { mtt::Status::Success, torrent };
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addMagnet(const char* magnet)
{
	auto torrent = Torrent::fromMagnetLink(magnet);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	if (auto t = getTorrent(torrent->hash()))
		return { mtt::Status::I_AlreadyExists, torrent };

	auto onMetadataUpdate = [this, torrent](Status s, mtt::MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished)
		{
			torrent->saveTorrentFile();
			torrent->checkFiles();
		}
	};

	torrent->downloadMetadata(onMetadataUpdate);
	torrents.push_back(torrent);

	return { mtt::Status::Success, torrent };
}

mtt::TorrentPtr mtt::Core::getTorrent(const uint8_t* hash)
{
	for (auto t : torrents)
	{
		if (memcmp(t->hash(), hash, 20) == 0)
			return t;
	}

	return nullptr;
}

mtt::TorrentPtr mtt::Core::getTorrent(const char* hash)
{
	uint8_t hexa[20];
	decodeHexa(hash, hexa);

	return getTorrent(hexa);
}

mtt::Status mtt::Core::removeTorrent(const uint8_t* hash, bool deleteFiles)
{
	for (auto it = torrents.begin(); it != torrents.end(); it++)
	{
		if (memcmp((*it)->hash(), hash, 20) == 0)
		{
			auto t = *it;
			t->stop();

			t->removeMetaFiles();

			torrents.erase(it);
	
			if (deleteFiles)
			{
				t->files.storage.deleteAll();
			}

			return Status::Success;
		}
	}

	return Status::E_InvalidInput;
}

mtt::Status mtt::Core::removeTorrent(const char* hash, bool deleteFiles)
{
	uint8_t hexa[20];
	decodeHexa(hash, hexa);

	return removeTorrent(hexa, deleteFiles);
}
