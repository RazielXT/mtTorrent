#include "InfoTab.h"
#include <QtCharts/QtCharts>
#include "../mtTorrent/utils/HexEncoding.h"

void InfoTab::init(Ui_mainWindowWidget& ui)
{
	infoLabel = ui.labelGeneralInfo;
}

void InfoTab::selectTorrent(mttApi::TorrentPtr t)
{
	if (!t)
	{
		infoLabel->setText("");
		return;
	}

	QString info;
	
	if (!t->name().empty())
	{
		info += t->name().c_str();
		info += "\n\n";
	}
	
	auto& torrentFile = t->getMetadata();
	if (torrentFile.info.fullSize)
	{
		info += "Total size: \t" + utils::formatBytes(torrentFile.info.fullSize);
		info += "\n\n";

		info += "Save in: \t";
		info += t->getFiles().getLocationPath().c_str();
		info += "\n\n";
	}

	info += "Hash: \t";
	info += hexToString(t->hash(), 20).c_str();
	info += "\n\n";

	QString creationInfo;
	if (torrentFile.about.creationDate)
	{
		creationInfo += utils::formatTimestamp(torrentFile.about.creationDate);
	}
	if (!torrentFile.about.createdBy.empty())
	{
		creationInfo += "by ";
		creationInfo += torrentFile.about.createdBy.c_str();
	}
	if (!creationInfo.isEmpty())
	{
		info += "Created: \t" + creationInfo;
		info += "\n";
	}

	if (t->getTimeAdded())
	{
		info += "Added: \t" + utils::formatTimestamp(t->getTimeAdded());
	}

	infoLabel->setText(info);
}
