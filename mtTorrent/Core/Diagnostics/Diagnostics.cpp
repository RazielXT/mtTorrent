#include "Diagnostics.h"
#include "../../utils/Filesystem.h"
#include <fstream>
#include <filesystem>
#include <limits>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <ctime>
#endif

void mtt::Diagnostics::Peer::serialize(Buffer& buffer) const
{
	std::lock_guard<std::mutex> guard(snapshotMutex);

	auto pos = buffer.size();
	buffer.resize(buffer.size() + snapshot.name.length() + 1 + 4 + snapshot.events.size() * sizeof(PeerEvent));

	auto ptr = buffer.data() + pos;

	memcpy(ptr, snapshot.name.data(), snapshot.name.length() + 1);
	ptr += snapshot.name.length() + 1;

	*reinterpret_cast<uint32_t*>(ptr) = (uint32_t)snapshot.events.size();
	ptr += 4;

	for (const auto& e : snapshot.events)
	{
		memcpy(ptr, &e, sizeof(PeerEvent));
		ptr += sizeof(PeerEvent);
	}
}

const uint8_t* mtt::Diagnostics::Peer::deserialize(const uint8_t* buffer)
{
	std::lock_guard<std::mutex> guard(snapshotMutex);

	auto ptr = buffer;

	snapshot.name = reinterpret_cast<const char*>(ptr);
	ptr += snapshot.name.size() + 1;

	uint32_t eventsCount = *reinterpret_cast<const uint32_t*>(ptr);
	ptr += 4;

	snapshot.events.resize(eventsCount);
	for (auto& e : snapshot.events)
	{
		memcpy(&e, ptr, sizeof(PeerEvent));
		ptr += sizeof(PeerEvent);
	}

	return ptr;
}

void mtt::Diagnostics::Peer::clear()
{
	std::lock_guard<std::mutex> guard(snapshotMutex);

	snapshot.events.clear();
}

int64_t startCounterTime = 0;

#ifdef _WIN32
double PCFreq = 0.0;

static int64_t getCounterTime()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return (int64_t)((li.QuadPart - startCounterTime) / PCFreq);
}

static void initCounterTime()
{
	LARGE_INTEGER li;
	if (QueryPerformanceFrequency(&li))
		PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	startCounterTime = li.QuadPart;
}
#else
static int64_t getCounterTime()
{
	return ((int64_t) ::clock()) - startCounterTime;
}

static void initCounterTime()
{
	startCounterTime = getCounterTime();
}
#endif // _WIN32

void mtt::Diagnostics::Peer::addSnapshotEvent(PeerEvent event)
{
	std::lock_guard<std::mutex> guard(snapshotMutex);

	event.time = getCounterTime();

	snapshot.events.push_back(event);
}

mtt::Diagnostics::PeerSnapshot& mtt::Diagnostics::PeerSnapshot::operator+=(const PeerSnapshot& rhs)
{
	if (name.empty())
		name = rhs.name;

	events.insert(events.end(), rhs.events.begin(), rhs.events.end());

	return *this;
}

mtt::Diagnostics::Storage::Storage()
{
	initCounterTime();

	timestart = (uint32_t)::time(0);

	folder = std::string(".") + pathSeparator + "Diagnostics" + pathSeparator;
}

void mtt::Diagnostics::Storage::addPeer(Peer& p)
{
	p.serialize(buffer);
	p.clear();
}

void mtt::Diagnostics::Storage::flush()
{
	if (buffer.empty())
		return;

	auto fullfolder = folder + std::to_string(timestart) + pathSeparator;

	std::filesystem::create_directories(fullfolder);

	std::ofstream file(fullfolder + "peers_" + std::to_string(index++), std::ios_base::binary);

	file.write((const char*)buffer.data(), buffer.size());

	buffer.clear();
}

std::vector<mtt::Diagnostics::PeerSnapshot> mtt::Diagnostics::Storage::loadPeers()
{
	std::map<std::string, mtt::Diagnostics::PeerSnapshot> peersMap;

	timestart = 0;
	index = 0;
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(folder, ec))
	{
		if (entry.is_directory())
		{
			auto timename = std::stoul(entry.path().filename().string());

			if (timename > timestart)
				timestart = timename;
		}
	}

	if (timestart == 0)
		return {};

	auto fullfolder = folder + std::to_string(timestart) + pathSeparator;

	std::ifstream file(fullfolder + "peers_" + std::to_string(index++), std::ios_base::binary);

	while (file)
	{
		file.seekg(0, std::ios::end);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);
		buffer.resize((size_t)size);
		file.read((char*)buffer.data(), size);

		const uint8_t* ptr = buffer.data();
		const uint8_t* endPtr = buffer.data() + size;

		while (ptr < endPtr)
		{
			Peer p;
			ptr = p.deserialize(ptr);
			peersMap[p.snapshot.name] += p.snapshot;
		}

		file = std::ifstream(fullfolder + "peers_" + std::to_string(index++), std::ios_base::binary);
	}

	std::vector<mtt::Diagnostics::PeerSnapshot> peers;
	peers.reserve(peersMap.size());

	for (auto& p : peersMap)
	{
		size_t i;
		for (i = 0; i < peers.size(); i++)
		{
			if (peers[i].events.front().time > p.second.events.front().time)
				break;
		}
		peers.insert(peers.begin() + i, p.second);
	}

	return std::move(peers);
}
