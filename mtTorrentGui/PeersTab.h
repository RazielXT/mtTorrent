#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"
#include "PeersDataModel.h"

class PeersTab
{
public:
	PeersTab() = default;

	void init(Ui_mainWindowWidget&);
	void selectTorrent(mttApi::TorrentPtr t);
	void update();

private:

	void showContextMenu(const QPoint& pos);

	mttApi::TorrentPtr selected;

	QTableView* table;
	QWidget* peersTab;

	PeersDataModel* model;
};
