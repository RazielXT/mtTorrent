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
#include "utils/TorrentFileParser.h"

mtt::Core core;

extern void InitLogTime();

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

std::string riotApiRequest()
{
#ifdef MTT_WITH_SSL
	char* server = "eun1.api.riotgames.com";
	char* requestUrl = "/lol/platform/v3/champion-rotations";
	auto token = "RGAPI-8c80aeff-96cb-41fa-b13f-65c5737edf72";

	// Create a context that uses the default paths for
	// finding CA certificates.
	asio::ssl::context ctx(asio::ssl::context::tlsv12);
	ctx.set_default_verify_paths();

	// Open a socket and connect it to the remote host.
	asio::io_service io_service;
	ssl_socket sock(io_service, ctx);
	tcp::resolver resolver(io_service);

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << requestUrl << " HTTP/1.1\r\n";
	request_stream << "Host: " << server << "\r\n";
	request_stream << "X-Riot-Token: " << token << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	return sendHttpsRequest(sock, resolver, request, server);
#else
	return "";
#endif
}

void mtt::Core::init()
{
#ifdef WINAPI
	::SetUnhandledExceptionFilter(OurCrashHandler);
#endif // WINAPI

	InitLogTime();

	mtt::config::load();

	dht = std::make_shared<dht::Communication>();

	if(mtt::config::getExternal().dht.enable)
		dht->start();

	bandwidth = std::make_unique<GlobalBandwidth>();

	listener = std::make_shared<IncomingPeersListener>([this](std::shared_ptr<TcpAsyncLimitedStream> s, const uint8_t* hash)
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
	bandwidth.reset();

	if (listener)
	{
		listener->stop();
	}

	saveTorrentList(torrents);

	for (auto& t : torrents)
	{
		auto state = t->state;
		t->stop();
		t->state = state;
		t->save();
		t->state = mttApi::Torrent::State::Stopped;
	}

	torrents.clear();
	listener.reset();

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
	size_t maxSize = 10 * 1024 * 1024;
	std::filesystem::path dir = std::filesystem::u8path(filename);

	if (!std::filesystem::exists(dir) || std::filesystem::file_size(dir) > maxSize)
		return { mtt::Status::E_InvalidPath, nullptr };

	std::ifstream file(filename, std::ios_base::binary);

	if (!file.good())
		return { mtt::Status::E_FileReadError, nullptr };

	DataBuffer buffer((
		std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	return addFile(buffer.data(), buffer.size());
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addFile(const uint8_t* data, size_t size)
{
	auto infoFile = mtt::TorrentFileParser::parse(data, size);

	if (auto t = getTorrent(infoFile.info.hash))
	{
		if (t->importTrackers(infoFile))
			return { mtt::Status::I_Merged, t };
		else
			return { mtt::Status::I_AlreadyExists, t };
	}

	auto torrent = Torrent::fromFile(infoFile);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	torrents.push_back(torrent);
	torrent->saveTorrentFile((const char*)data, size);
	torrent->checkFiles();
	saveTorrentList(torrents);

	return { mtt::Status::Success, torrent };
}

std::pair<mtt::Status, mtt::TorrentPtr> mtt::Core::addMagnet(const char* magnet)
{
	auto torrent = Torrent::fromMagnetLink(magnet);

	if (!torrent)
		return { mtt::Status::E_InvalidInput, nullptr };

	if (auto t = getTorrent(torrent->hash()))
	{
		if(t->importTrackers(torrent->infoFile))
			return { mtt::Status::I_Merged, t };
		else
			return { mtt::Status::I_AlreadyExists, t };
	}

	auto onMetadataUpdate = [this, torrent](Status s, mtt::MetadataDownloadState& state)
	{
		if (s == Status::Success && state.finished)
		{
			torrent->saveTorrentFileFromUtm();
			torrent->checkFiles();
		}
	};

	torrent->downloadMetadata(onMetadataUpdate);
	torrents.push_back(torrent);
	saveTorrentList(torrents);

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

			saveTorrentList(torrents);

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

mtt::GlobalBandwidth::GlobalBandwidth()
{
	BandwidthManager::Get().GetChannel("")->setLimit(mtt::config::getExternal().transfer.maxDownloadSpeed);
	BandwidthManager::Get().GetChannel("upload")->setLimit(mtt::config::getExternal().transfer.maxUploadSpeed);

	uint32_t bwTick = mtt::config::getInternal().bandwidthUpdatePeriodMs;

	bwTimer = ScheduledTimer::create(bwPool.io, [this, bwTick]()
		{
			BandwidthManager::Get().updateQuotas(bwTick);
			bwTimer->schedule(std::chrono::milliseconds(bwTick));
		});

	bwTimer->schedule(std::chrono::milliseconds(bwTick));

	config::registerOnChangeCallback(config::ValueType::Transfer, [this]()
		{
			BandwidthManager::Get().GetChannel("")->setLimit(mtt::config::getExternal().transfer.maxDownloadSpeed);
			BandwidthManager::Get().GetChannel("upload")->setLimit(mtt::config::getExternal().transfer.maxUploadSpeed);
		});
}
