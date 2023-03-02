#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"
#include "SourcesDataModel.h"

class SourcesTab
{
public:
	SourcesTab() = default;

	void init(Ui_mainWindowWidget&);
	void selectTorrent(mttApi::TorrentPtr t);
	void update();

private:

	void showContextMenu(const QPoint& pos);

	mttApi::TorrentPtr selected;

	QTableView* table;
	QWidget* sourcesTab;
	QTabWidget* tabs;

	SourcesDataModel* model;
};
