#include "State.h"
#include <fstream>
#include "Configuration.h"
#include <boost/filesystem.hpp>
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"

mtt::TorrentState::TorrentState(std::vector<uint8_t>& p) : pieces(p)
{

}

void mtt::TorrentState::saveState(const std::string& name)
{
	auto folderPath = mtt::config::getInternal().programFolderPath + mtt::config::getInternal().stateFolder + "\\" + name + ".state";

	std::ofstream file(folderPath, std::ios::binary);

	if (!file)
		return;

	BencodeWriter writer;

	writer.startArray();
	writer.addRawItem("12:downloadPath", downloadPath);
	writer.addRawItemFromBuffer("6:pieces", (const char*)pieces.data(), pieces.size());
	writer.addRawItem("13:lastStateTime", lastStateTime);
	writer.addRawItem("7:started", started);

	writer.startRawArrayItem("9:selection");
	for (auto& f : files)
	{
		writer.addNumber(f.selected);
	}
	writer.endArray();
	writer.endArray();

	file << writer.data;
}

bool mtt::TorrentState::loadState(const std::string& name)
{
	std::ifstream file(mtt::config::getInternal().programFolderPath + mtt::config::getInternal().stateFolder + "\\" + name + ".state", std::ios::binary);

	if (!file)
		return false;

	std::string data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	BencodeParser parser;

	if (!parser.parse((uint8_t*)data.data(), data.length()))
		return false;

	if (auto root = parser.getRoot())
	{
		downloadPath = root->getTxt("downloadPath");
		lastStateTime = (uint32_t)root->getBigInt("lastStateTime");
		started = root->getInt("started");
		if (auto pItem = root->getTxtItem("pieces"))
		{
			pieces.assign(pItem->data, pItem->data + pItem->size);
		}
		if (auto fList = root->getListItem("selection"))
		{
			files.clear();
			for (auto f : *fList)
			{
				files.push_back({f.getInt() != 0});
			}
		}
	}

	return true;
}

void mtt::TorrentsList::saveState()
{
	auto folderPath = mtt::config::getInternal().programFolderPath + mtt::config::getInternal().stateFolder + "\\list";

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

	file << writer.data;
}

bool mtt::TorrentsList::loadState()
{
	std::ifstream file(mtt::config::getInternal().programFolderPath + mtt::config::getInternal().stateFolder + "\\list", std::ios::binary);

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
