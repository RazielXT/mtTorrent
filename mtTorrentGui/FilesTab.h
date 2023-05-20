#pragma once

#include "FilesDataModel.h"
#include "QSortFilterProxyModel"

class FilesTab
{
public:
	FilesTab() = default;

	void init(Ui_mainWindowWidget&);
	void deinit();

	void selectTorrent(mttApi::TorrentPtr t);
	void update();

private:

	void clear();
	bool cleared = false;

	mttApi::TorrentPtr selected;
	mtt::Bitfield bitfield;

	void filesClicked(const QModelIndex& index);
	QTreeView* filesTree = nullptr;
	FilesDataModel* model = nullptr;
	QSortFilterProxyModel* filter = nullptr;

	void redrawPiecesChart();
	QImage piecesImage;
	QLabel* piecesChart = nullptr;
	QLabel* piecesChartLabel = nullptr;

	QWidget* filesTab = nullptr;
};
