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
	return static_cast<uint32_t>(rand());
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

bool Torrent::PiecesProgress::finished()
{
	return piecesSelection.size() - unwantedPiecesCount == 0;
}

float Torrent::PiecesProgress::getPercentage()
{
	return piecesSelection.size() == 0 ? 1 : std::min(1.0f, addedSelectedPiecesCount / (float)selectedPiecesCount);
}

void Torrent::PiecesProgress::init(size_t size)
{
	piecesCount = size;
	
	for (size_t i = 0; i < piecesCount; i++)
	{
		piecesSelection[i] = true;
	}
}

void Torrent::PiecesProgress::setSelection(std::vector<File>& files)
{
	std::map<size_t, bool> newProgress;
	for (size_t i = 0; i < piecesCount; i++)
	{
		newProgress[i] = false;
	}
	
	for (auto& f : files)
	{
		for (size_t i = f.startPieceIndex; i <= f.startPieceIndex; i++)
		{
			newProgress[i] = true;
		}
	}

	unwantedPiecesCount = 0;

	for (size_t i = 0; i < piecesCount; i++)
	{
		if (!newProgress[i])
			unwantedPiecesCount++;

		if (newProgress[i] && piecesSelection.find(i) == piecesSelection.end())
			continue;
		else
			piecesSelection[i] = newProgress[i];
	}
}

void Torrent::PiecesProgress::resetAdded(PiecesProgress& parent)
{
	selectedPiecesCount = 0;
	addedSelectedPiecesCount = 0;

	for (size_t i = 0; i < piecesCount; i++)
	{
		auto& p = piecesSelection[i];
		auto parentPiece = parent.piecesSelection[i];

		if (p == Added && parentPiece == Selected)
		{
			selectedPiecesCount++;
			p = Selected;
		}		
	}
}

void Torrent::PiecesProgress::closePiece(size_t index)
{
	auto it = piecesSelection.find(index);

	if (it != piecesSelection.end())
	{
		piecesSelection.erase(it);
	}
}

bool Torrent::PiecesProgress::closedPiece(size_t index)
{
	return piecesSelection.find(index) == piecesSelection.end();
}

bool Torrent::PiecesProgress::hasPreparedPiece(size_t index)
{
	auto it = piecesSelection.find(index);
	return it != piecesSelection.end() && it->second;
}

void Torrent::PiecesProgress::fromBitfield(DataBuffer& bitfield)
{
	unwantedPiecesCount = 0;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;
		piecesSelection[i] = value;

		if (!value)
			unwantedPiecesCount++;
	}
}

DataBuffer Torrent::PiecesProgress::toBitfield()
{
	return{};
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
