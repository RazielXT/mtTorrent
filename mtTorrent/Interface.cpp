#include "Interface.h"

int gcount = 0;

using namespace Torrent;

std::string getTimestamp()
{
	time_t timer;
	struct tm y2k = { 0 };
	double seconds;

	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time(&timer);  /* get current time; same as: timer = time(NULL)  */

	seconds = difftime(timer, mktime(&y2k));

	return std::to_string(static_cast<int64_t>(seconds));
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
	return selectedPiecesCount == addedSelectedPiecesCount;
}

float Torrent::PiecesProgress::getPercentage()
{
	return selectedPiecesCount == 0 ? 1 : std::min(1.0f, addedSelectedPiecesCount / static_cast<float>(selectedPiecesCount));
}

float Torrent::PiecesProgress::getFullPercentage()
{
	return piecesCount == 0 ? 1 : addedPiecesCount / static_cast<float>(piecesCount);
}

void Torrent::PiecesProgress::init(size_t size)
{
	piecesCount = size;
	piecesProgress.resize(size);
}

void Torrent::PiecesProgress::setSelection(std::vector<File>& files)
{
	std::vector<Progress> newProgress(piecesCount);
	std::fill(newProgress.begin(), newProgress.end(), NotSelected);
	
	for (auto& f : files)
	{
		std::fill(newProgress.begin() + f.startPieceIndex, newProgress.begin() + f.endPieceIndex + 1, Selected);
	}

	addedSelectedPiecesCount = 0;
	selectedPiecesCount = 0;

	for (size_t i = 0; i < piecesCount; i++)
	{
		if (newProgress[i] == Selected)
			selectedPiecesCount++;

		if (newProgress[i] == Selected && piecesProgress[i] == Added)
			addedSelectedPiecesCount++;

		if(piecesProgress[i] != Added)
			piecesProgress[i] = newProgress[i];
	}
}

void Torrent::PiecesProgress::resetAdded()
{
	for (auto& p : piecesProgress)
	{
		if (p == Added)
			p = Selected;
	}
}

void Torrent::PiecesProgress::addPiece(size_t index)
{
	auto old = piecesProgress[index];

	if (old != Added)
	{
		piecesProgress[index] = Added;
		addedPiecesCount++;

		if(old == Selected)
			addedSelectedPiecesCount++;
	}
}

bool Torrent::PiecesProgress::hasPiece(size_t index)
{
	return piecesProgress[index] == Added;
}

bool Torrent::PiecesProgress::wantsPiece(size_t index)
{
	return piecesProgress[index] == Selected;
}

void Torrent::PiecesProgress::fromBitfield(DataBuffer& bitfield)
{
	addedPiecesCount = 0;

	for (int i = 0; i < piecesCount; i++)
	{
		size_t idx = static_cast<size_t>(i / 8.0f);
		unsigned char bitmask = 128 >> i % 8;

		bool value = (bitfield[idx] & bitmask) != 0;
		piecesProgress[i] = value ? Added : Selected;

		if (value)
			addedPiecesCount++;
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