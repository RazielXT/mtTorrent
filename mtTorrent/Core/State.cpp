#include "State.h"
#include <fstream>
#include "Configuration.h"
#include "utils/BencodeWriter.h"
#include "utils/BencodeParser.h"

mtt::TorrentState::TorrentState(std::vector<uint8_t>& p) : pieces(p)
{

}

void mtt::TorrentState::save(const std::string& name)
{
	auto folderPath = mtt::config::getInternal().stateFolder + "\\" + name + ".state";

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
		writer.startArray();
		writer.addNumber(f.selected);
		writer.addNumber((size_t)f.priority);
		writer.endArray();
	}
	writer.endArray();
	writer.endArray();

	file << writer.data;
}

bool mtt::TorrentState::load(const std::string& name)
{
	std::ifstream file(mtt::config::getInternal().stateFolder + "\\" + name + ".state", std::ios::binary);

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
		lastStateTime = (int64_t)root->getBigInt("lastStateTime");
		started = root->getInt("started");
		if (auto pItem = root->getTxtItem("pieces"))
		{
			if(pieces.size() == pItem->size)
				pieces.assign(pItem->data, pItem->data + pItem->size);
		}
		if (auto fList = root->getListItem("selection"))
		{
			files.clear();
			for (auto& f : *fList)
			{
				if (f.isList())
				{
					auto params = f.getFirstItem();
					files.push_back({ params->getInt() != 0, (Priority)params->getNextSibling()->getInt() });
				}
				else
					files.push_back({ false, Priority::Normal });
			}
		}
	}

	return true;
}

void mtt::TorrentState::remove(const std::string& name)
{
	auto fullName = mtt::config::getInternal().stateFolder + "\\" + name + ".state";
	std::remove(fullName.data());
}

void mtt::TorrentsList::save()
{
	auto folderPath = mtt::config::getInternal().stateFolder + "\\list";

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

bool mtt::TorrentsList::load()
{
	std::ifstream file(mtt::config::getInternal().stateFolder + "\\list", std::ios::binary);

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
