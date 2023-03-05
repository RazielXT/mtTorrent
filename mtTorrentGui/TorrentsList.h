#pragma once

#include "ui_MainWindow.h"
#include "FilesTab.h"
#include "SpeedTab.h"
#include "Utils.h"
#include "InfoTab.h"
#include "PeersTab.h"
#include "TorrentsDataModel.h"
#include "SourcesTab.h"

class TorrentsList
{
public:
	TorrentsList();

	void init(Ui_mainWindowWidget&);
	void deinit();

	void add(mttApi::TorrentPtr);
	void remove(mttApi::TorrentPtr);
	void select(mttApi::TorrentPtr);
	void refresh(mttApi::TorrentPtr);
	void refresh();

	mttApi::TorrentPtr getTorrent(const QModelIndex& idx) const;

private:

	InfoTab infoTab;
	FilesTab filesTab;
	SpeedTab speedTab;
	PeersTab peersTab;
	SourcesTab sourcesTab;

	void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void showContextMenu(const QPoint& pos);
	void changeTorrentDirectory(const mttApi::TorrentPtr& t);

	QTableView* table = nullptr;
	Ui_mainWindowWidget* window = nullptr;
	TorrentsDataModel* model = nullptr;
	QSortFilterProxyModel* filter = nullptr;

	void selectTorrent(int idx);

	void start();
	void stop();
	void remove();
	void refreshButtons();

	QList<int> getSelectionIndex() const;
};
