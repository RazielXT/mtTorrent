#include "Storage.h"
#include <fstream>
#include <boost/filesystem.hpp>

mtt::Storage::Storage(size_t piecesz)
{
	pieceSize = piecesz;
}

void mtt::Storage::setPath(std::string p)
{
	path = p;
}

void mtt::Storage::storePiece(DownloadedPiece& piece)
{
	for (auto& s : selection.files)
	{
		auto& f = s.file;

		if (f.startPieceIndex <= piece.index && f.endPieceIndex >= piece.index)
		{
			storePiece(f, piece, pieceSize);
		}
	}
}

void mtt::Storage::exportFiles()
{
	for (auto& s : selection.files)
	{
		flush(s.file);
	}
}

void mtt::Storage::selectFiles(std::vector<mtt::File>& files)
{
	selection.files.clear();

	for (auto& f : files)
	{
		selection.files.push_back({ f });
	}
}

void mtt::Storage::saveProgress()
{

}

void mtt::Storage::loadProgress()
{

}

void mtt::Storage::storePiece(File& file, DownloadedPiece& piece, size_t normalPieceSize)
{
	/*auto& outBuffer = filesBuffer[file.id];

	size_t bufferStartOffset = (piece.index - file.startPieceIndex)*normalPieceSize;

	memcpy(&outBuffer[0] + bufferStartOffset, piece.data.data(), normalPieceSize);
	*/
	unsavedPieces[file.id].push_back(piece);

	if (unsavedPieces[file.id].size() > piecesCacheSize)
		flush(file);
}

void mtt::Storage::flush(File& file)
{
	auto path = getFullpath(file);
	std::string dirPath = path.substr(0, path.find_last_of('\\'));
	boost::filesystem::path dir(dirPath);
	if (!boost::filesystem::exists(dir))
	{
		if (!boost::filesystem::create_directory(dir))
			return;
	}

	auto& pieces = unsavedPieces[file.id];

	for (auto& p : pieces)
	{
		std::ofstream fileOut(path, std::ios_base::binary);
		auto piecePos = 
		fileOut.seekp(piecePos);
		fileOut.write(p.data.data(), p.dataSize);
	}

	unsavedPieces[file.id].clear();
}

std::string mtt::Storage::getFullpath(File& file)
{
	std::string filePath;

	for (auto& p : file.path)
	{
		if (!filePath.empty())
			filePath += "\\";

		filePath += p;
	}

	return path + filePath;
}

