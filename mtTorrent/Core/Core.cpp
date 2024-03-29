#include "Core.h"
#include "Peers.h"
#include "MetadataDownload.h"
#include "Downloader.h"
#include "Configuration.h"
#include "Dht/Communication.h"
#include "utils/TcpAsyncServer.h"
#include "IncomingPeersListener.h"
#include "State.h"
#include "AlertsManager.h"
#include "utils/HexEncoding.h"
#include "utils/TorrentFileParser.h"
#include "Logging.h"

#ifdef WINAPI

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

#endif // WINAPI

void mtt::Core::init()
{
#ifdef WINAPI
	::SetUnhandledExceptionFilter(OurCrashHandler);
#endif // WINAPI

	WRITE_GLOBAL_LOG(General, "Init start");

	mtt::config::load();

	dht = std::make_shared<dht::Communication>(udpComm);

	bandwidth = std::make_unique<GlobalBandwidth>();

	listener = std::make_unique<IncomingPeersListener>();

	utp.init();

	TorrentsList list;
	list.load();

	if (!list.torrents.empty())
		initListeners();

	for (auto& t : list.torrents)
	{
		auto tPtr = Torrent::fromSavedState(t.name);

		if (!tPtr)
			continue;

		if (auto t = getTorrent(tPtr->hash()))
			continue;

		torrents.push_back(tPtr);
		listener->addTorrent(tPtr->hash());
	}

	WRITE_GLOBAL_LOG(General, "Init end");
}

static void saveTorrentList(const std::vector<mtt::TorrentPtr>& torrents)
{
	mtt::TorrentsList list;
	for (auto& t : torrents)
	{
		list.torrents.push_back({ t->hashString() });
	}

	list.save();
}

void mtt::Core::deinit()
{
	WRITE_GLOBAL_LOG(General, "Deinit start");

	bandwidth.reset();

	if (listener)
	{
		listener->stop();
	}

	{
		std::lock_guard<std::mutex> guard(torrentsMtx);
		saveTorrentList(torrents);

		for (auto& t : torrents)
		{
			t->stop(Torrent::StopReason::Deinit);
		}
		torrents.clear();
	}
	listener.reset();

	if (dht)
	{
		dht->stop();
		dht.reset();
	}

	utp.stop();
	udpComm.deinit();

	mtt::config::save();

	mtt::AlertsManager::Get().popAlerts();

	WRITE_GLOBAL_LOG(General, "Deinit end");
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addFile(const char* filename)
{
	const std::size_t maxSize = 50 * 1024 * 1024;
	std::filesystem::path dir = Storage::utf8Path(filename);

	if (!std::filesystem::exists(dir) || std::filesystem::file_size(dir) > maxSize)
		return { mtt::Status::E_InvalidPath, nullptr };

	std::ifstream file(dir, std::ios_base::binary);

	if (!file.good())
		return { mtt::Status::E_FileReadError, nullptr };

	DataBuffer buffer((
		std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	return addFile(buffer.data(), buffer.size());
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addFile(const uint8_t* data, std::size_t size)
{
	auto infoFile = mtt::TorrentFileParser::parse(data, size);

	if (auto t = getTorrent(infoFile.info.hash))
	{
		if (t->peers->importTrackers(infoFile.announceList) == Status::Success)
			return { mtt::Status::I_Merged, t };

		return { mtt::Status::I_AlreadyExists, t };
	}

	if (infoFile.info.pieces.empty())
		return { mtt::Status::E_InvalidInput, nullptr };

	auto torrent = Torrent::fromFile(std::move(infoFile));

	{
		std::lock_guard<std::mutex> guard(torrentsMtx);
		torrents.push_back(torrent);
		saveTorrentList(torrents);
	}
	initListeners();

	listener->addTorrent(torrent->hash());
	torrent->saveTorrentFile(data, size);

	AlertsManager::Get().torrentAlert(Alerts::Id::TorrentAdded, *torrent);
	AlertsManager::Get().metadataAlert(Alerts::Id::MetadataInitialized, *torrent);

	return { mtt::Status::Success, torrent };
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addMagnet(const char* magnet)
{
	auto torrent = Torrent::fromMagnetLink(magnet);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	if (auto t = getTorrent(torrent->hash()))
	{
		if (t->peers->importTrackers(torrent->infoFile.announceList) == Status::Success)
			return { mtt::Status::I_Merged, t };

		return { mtt::Status::I_AlreadyExists, t };
	}

	{
		std::lock_guard<std::mutex> guard(torrentsMtx);
		torrents.push_back(torrent);
		saveTorrentList(torrents);
	}
	initListeners();

	listener->addTorrent(torrent->hash());
	torrent->downloadMetadata();

	AlertsManager::Get().torrentAlert(Alerts::Id::TorrentAdded, *torrent);

	return { mtt::Status::Success, torrent };
}

mtt::TorrentPtr mtt::Core::getTorrent(const uint8_t* hash) const
{
	std::lock_guard<std::mutex> guard(torrentsMtx);

	for (auto t : torrents)
	{
		if (memcmp(t->hash(), hash, 20) == 0)
			return t;
	}

	return nullptr;
}

mtt::Status mtt::Core::removeTorrent(const uint8_t* hash, bool deleteFiles)
{
	std::lock_guard<std::mutex> guard(torrentsMtx);

	for (auto it = torrents.begin(); it != torrents.end(); it++)
	{
		if (memcmp((*it)->hash(), hash, 20) == 0)
		{
			auto t = *it;
			if (deleteFiles && t->loadFileInfo())
			{
				t->files.storage.deleteAll();
			}

			t->stop();
			t->removeMetaFiles();

			listener->removeTorrent((*it)->hash());
			torrents.erase(it);

			saveTorrentList(torrents);

			AlertsManager::Get().torrentAlert(Alerts::Id::TorrentRemoved, *t);

			return Status::Success;
		}
	}

	return Status::E_InvalidInput;
}

void mtt::Core::initListeners()
{
	if (listening)
		return;

	listening = true;

	udpComm.setBindPort(mtt::config::getExternal().connection.udpPort);
	config::registerOnChangeCallback(config::ValueType::Connection, [this]()
		{
			udpComm.setBindPort(mtt::config::getExternal().connection.udpPort);
		});

	udpComm.listen([this](udp::endpoint& e, std::vector<BufferView>& b)
		{
			utp.onUdpPacket(e, b);
			dht->onUdpPacket(e, b);
		});

	dht->init();

	listener->start([this](std::shared_ptr<PeerStream> s, const BufferView& data, const uint8_t* hash)
		{
			auto t = getTorrent(hash);
			return t ? t->peers->add(s, data) : 0;
		});
}

mtt::Status mtt::Core::removeTorrent(const char* hash, bool deleteFiles)
{
	uint8_t hexa[20];
	decodeHexa(hash, hexa);

	return removeTorrent(hexa, deleteFiles);
}

mtt::GlobalBandwidth::GlobalBandwidth()
{
	BandwidthManager::Get().GetChannel("")->setLimit(mtt::config::getExternal().transfer.maxDownloadSpeed);
	BandwidthManager::Get().GetChannel("upload")->setLimit(mtt::config::getExternal().transfer.maxUploadSpeed);

	uint32_t bwTick = mtt::config::getInternal().bandwidthUpdatePeriodMs;
	bwPool.start(1);

	bwTimer = ScheduledTimer::create(bwPool.io, [this, bwTick]()
		{
			BandwidthManager::Get().updateQuotas(bwTick);
			return ScheduledTimer::Duration(bwTick);
		});

	bwTimer->schedule(ScheduledTimer::Duration(bwTick));

	config::registerOnChangeCallback(config::ValueType::Transfer, [this]()
		{
			BandwidthManager::Get().GetChannel("")->setLimit(mtt::config::getExternal().transfer.maxDownloadSpeed);
			BandwidthManager::Get().GetChannel("upload")->setLimit(mtt::config::getExternal().transfer.maxUploadSpeed);
		});
}

mtt::GlobalBandwidth::~GlobalBandwidth()
{
	bwTimer->disable();
	bwTimer = nullptr;

	BandwidthManager::Get().close();
}
