#include "PeersTab.h"
#include "QSortFilterProxyModel"
#include "QMenu"
#include "AddPeerWindow.h"

class PeersTableItemDelegate : public guiUtils::CustomTableItemDelegate
{
public:
	QString displayText(const QVariant& value, const QLocale& locale) const override
	{
		auto column = index.column();

		if (column == (int)PeersDataModel::Column::Download || column == (int)PeersDataModel::Column::Upload)
			return utils::formatBytesSpeed(value.toULongLong());

		if (column == (int)PeersDataModel::Column::Progress)
		{
			if (auto pValue = value.toFloat())
				return utils::formatPercentage(pValue);
			return "";
		}

		return value.toString();
	}
};

void PeersTab::init(Ui_mainWindowWidget& ui)
{
	peersTab = ui.tabPeers;
	model = new PeersDataModel();
	auto filter = new QSortFilterProxyModel();
	filter->setSourceModel(model);

	table = ui.peersTable;
	table->setModel(filter);
	table->setItemDelegate(new PeersTableItemDelegate());

	table->connect(table, &QTableView::customContextMenuRequested, [this](const QPoint& pos) { showContextMenu(pos); });
	ui.tabWidget->connect(ui.tabWidget, &QTabWidget::currentChanged, [&](int index) { update(); });

	const int rowHeight = table->verticalHeader()->minimumSectionSize();
	table->verticalHeader()->setMaximumSectionSize(rowHeight);
	table->verticalHeader()->setDefaultSectionSize(rowHeight);
	table->horizontalHeader()->resizeSection((int)PeersDataModel::Column::Address, 300);
	table->horizontalHeader()->setSortIndicator((int)PeersDataModel::Column::Download, Qt::SortOrder::DescendingOrder);
}

void PeersTab::selectTorrent(mttApi::TorrentPtr t)
{
	selected = t;
	update();
}

void PeersTab::update()
{
	if (peersTab->isVisible())
	{
		model->update(selected);
	}
}

void PeersTab::showContextMenu(const QPoint& pos)
{
	if (!selected)
		return;

	QMenu contextMenu;
	contextMenu.setStyleSheet("font-size: 12px");
	contextMenu.addAction("Add peer", [this]() { AddPeerWindow(selected).exec(); });

	contextMenu.exec(table->viewport()->mapToGlobal(pos));
}
