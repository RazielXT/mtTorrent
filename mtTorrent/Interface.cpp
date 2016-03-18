#include "Interface.h"

int gcount = 0;

using namespace Torrent;

int64_t getTimeSeconds()
{
	time_t timer;
	struct tm y2k = { 0 };
	double seconds;

	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time(&timer);  /* get current time; same as: timer = time(NULL)  */

	seconds = difftime(timer, mktime(&y2k));

	return static_cast<int64_t>(seconds);
}

std::string getTimestamp()
{
	return std::to_string(getTimeSeconds());
}

uint32_t Torrent::generateTransaction()
{
	int id = rand();
	return *reinterpret_cast<uint32_t*>(&id);
}

void Torrent::PeerInfo::setIp(uint32_t addr)
{
	ip = addr;

	uint8_t ipAddr[4];
	ipAddr[3] = *reinterpret_cast<uint8_t*>(&ip);
	ipAddr[2] = *(reinterpret_cast<uint8_t*>(&ip) + 1);
	ipAddr[1] = *(reinterpret_cast<uint8_t*>(&ip) + 2);
	ipAddr[0] = *(reinterpret_cast<uint8_t*>(&ip) + 3);

	ipStr = std::to_string(ipAddr[0]) + "." + std::to_string(ipAddr[1]) + "." + std::to_string(ipAddr[2]) + "." + std::to_string(ipAddr[3]);
}

bool Torrent::PiecesProgress::empty()
{
	return pieces.empty();
}

float Torrent::PiecesProgress::getPercentage()
{
	return piecesStartCount == 0 ? 1 : (pieces.size() / (float)piecesStartCount);
}

void Torrent::PiecesProgress::init(size_t size)
{
	piecesStartCount = size;
	
	for (size_t i = 0; i < piecesStartCount; i++)
	{
		pieces[i] = true;
	}
}

void Torrent::PiecesProgress::fromSelection(std::vector<File>& files)
{
	pieces.clear();

	for (auto& f : files)
	{
		for (size_t i = f.startPieceIndex; i <= f.startPieceIndex; i++)
		{
			pieces[i] = true;
		}
	}
}

void Torrent::PiecesProgress::reset(PiecesProgress& parent)
{
	pieces = parent.pieces;
	piecesStartCount = parent.piecesStartCount;
}

void Torrent::PiecesProgress::addPiece(size_t index)
{
	pieces[index] = true;
}

void Torrent::PiecesProgress::removePiece(size_t index)
{
	auto it = pieces.find(index);

	if (it != pieces.end())
	{
		pieces.erase(it);
	}
}

bool Torrent::PiecesProgress::hasPiece(size_t index)
{
	auto it = pieces.find(index);
	return it != pieces.end() && it->second;
}

void Torrent::PiecesProgress::fromBitfield(DataBuffer& bitfield, size_t piecesCount)
{
	pieces.clear();
	piecesStartCount = piecesCount;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;

		if(value)
			pieces[i] = true;
	}
}

DataBuffer Torrent::PiecesProgress::toBitfield()
{
	return{};
}

const std::map<size_t, bool>& Torrent::PiecesProgress::get()
{
	return pieces;
}

void DownloadedPiece::reset(size_t maxPieceSize)
{
	data.resize(maxPieceSize);
	dataSize = 0;
	receivedBlocks = 0;
}

void DownloadedPiece::addBlock(PieceBlock& block)
{
	receivedBlocks++;
	dataSize += block.info.length;
	memcpy(&data[0] + block.info.begin, block.data.data(), block.info.length);
}

bool Checkpoint::hit(int id, int msPeriod)
{
	static std::map<int, clock_t> checkpoints;

	auto& ch = checkpoints[id];

	auto current = clock();

	if (current - ch > msPeriod)
	{
		ch = current;
		return true;
	}
	else
		return false;
}
