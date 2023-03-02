#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"

class InfoTab
{
public:
	InfoTab() = default;

	void init(Ui_mainWindowWidget&);
	void selectTorrent(mttApi::TorrentPtr t);

private:
	QLabel* infoLabel = nullptr;
};
