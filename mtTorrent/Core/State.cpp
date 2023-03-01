#include "State.h"
#include "Configuration.h"
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"
#include "utils/Filesystem.h"
#include <fstream>

mtt::TorrentState::TorrentState(std::vector<uint8_t>& p) : pieces(p)
{

}

void mtt::TorrentState::save(const std::string& name)
{
	auto folderPath = mtt::config::getInternal().stateFolder + pathSeparator + name + ".state";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	BencodeWriter writer;

	writer.startMap();

	writer.startMapItem("info");
	writer.addRawItem("4:name", info.name);
	writer.addRawItem("9:pieceSize", info.pieceSize);
	writer.addRawItem("8:fullSize", info.fullSize);
	writer.endMap();

	writer.addRawItem("12:downloadPath", downloadPath);
	writer.addRawItemFromBuffer("6:pieces", (const char*)pieces.data(), pieces.size());
	writer.addRawItem("13:lastStateTime", lastStateTime);
	writer.addRawItem("9:addedTime", addedTime);
	writer.addRawItem("7:started", started);
	writer.addRawItem("8:uploaded", uploaded);
	writer.addRawItem("10:downloaded", downloaded);
	writer.addRawItem("7:version", version);

	writer.startRawArrayItem("10:unfinished");
	for (auto& p : unfinishedPieces)
	{
		writer.startMap();
		writer.addRawItemFromBuffer("6:blocks", (const char*)p.blocksState.data(), p.blocksState.size());
		writer.addRawItem("9:remaining", p.remainingBlocks);
		writer.addRawItem("5:index", p.index);
		writer.addRawItem("10:downloaded", p.downloadedSize);
		writer.endMap();
	}
	writer.endArray();

	writer.startRawArrayItem("9:selection");
	for (auto& s : selection)
	{
		writer.startArray();
		writer.addNumber(s.selected);
		writer.addNumber((size_t)s.priority);
		writer.endArray();
	}
	writer.endArray();

	writer.endMap();

	file.write((const char*)writer.data.data(), writer.data.size());
}

bool mtt::TorrentState::load(const std::string& name)
{
	std::ifstream file(mtt::config::getInternal().stateFolder + pathSeparator + name + ".state", std::ios::binary);

	if (!file)
		return false;

	std::string data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	BencodeParser parser;

	if (!parser.parse((uint8_t*)data.data(), data.length()))
		return false;

	if (auto root = parser.getRoot())
	{
		if (auto pInfo = root->getDictObject("info"))
		{
			info.name = pInfo->getTxt("name");
			info.pieceSize = (uint32_t)pInfo->getInt("pieceSize");
			info.fullSize = (uint32_t)pInfo->getBigInt("fullSize");
		}

		downloadPath = root->getTxt("downloadPath");
		lastStateTime = root->getBigInt("lastStateTime");
		addedTime = (Timestamp)root->getBigInt("addedTime");
		started = root->getInt("started");
		uploaded = root->getBigInt("uploaded");
		downloaded = root->getBigInt("downloaded");
		version = root->getInt("version");

		if (auto pItem = root->getTxtItem("pieces"))
		{
			pieces.assign(pItem->data, pItem->data + pItem->size);
		}
		if (auto uList = root->getListObject("unfinished"))
		{
			unfinishedPieces.clear();
			for (auto& u : *uList)
			{
				if (u.isMap())
				{
					if (auto todoItem = u.getTxtItem("blocks"))
					{
						mtt::PieceState state;

						state.index = (uint32_t)u.getInt("index");
						state.remainingBlocks = (uint32_t)u.getInt("remaining");
						state.downloadedSize = (uint32_t)u.getInt("downloaded");
						state.blocksState.assign(todoItem->data, todoItem->data + todoItem->size);

						if (!state.blocksState.empty() && state.downloadedSize)
							unfinishedPieces.emplace_back(std::move(state));
					}
				}
			}
		}
		if (auto sList = root->getListObject("selection"))
		{
			selection.clear();
			for (const auto& s : *sList)
			{
				if (s.isList())
				{
					auto params = s.getFirstItem();
					selection.push_back({ params->getInt() != 0, (Priority)params->getNextSibling()->getInt() });
				}
				else
					selection.push_back({ false, PriorityNormal });
			}
		}
	}

	return true;
}

void mtt::TorrentState::remove(const std::string& name)
{
	auto fullName = mtt::config::getInternal().stateFolder + pathSeparator + name + ".state";
	std::remove(fullName.data());
}

void mtt::TorrentsList::save()
{
	auto folderPath = mtt::config::getInternal().stateFolder + pathSeparator + "list";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	BencodeWriter writer;

	writer.startArray();
	for (auto& t : torrents)
	{
		writer.startMap();
		writer.addRawItem("4:name", t.name);
		writer.endMap();
	}
	writer.endArray();

	file.write((const char*)writer.data.data(), writer.data.size());
}

bool mtt::TorrentsList::load()
{
	std::ifstream file(mtt::config::getInternal().stateFolder + pathSeparator + "list", std::ios::binary);

	if (!file)
		return false;

	std::string data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	BencodeParser parser;
	if (!parser.parse((uint8_t*)data.data(), data.length()))
		return false;

	torrents.clear();

	if (auto root = parser.getRoot())
	{
		for (auto& it : *root)
		{
			TorrentInfo info;
			info.name = it.getTxt("name");

			torrents.push_back(info);
		}
	}

	return true;
}
