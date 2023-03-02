#include "SourcesTab.h"
#include "QSortFilterProxyModel"
#include "QMenu"

class SourcesTableItemDelegate : public guiUtils::CustomTableItemDelegate
{
public:
	QString displayText(const QVariant& value, const QLocale& locale) const override
	{
		auto column = index.column();

		if (column == (int)SourcesDataModel::Column::Name || column == (int)SourcesDataModel::Column::State)
			return value.toString();
		if (column == (int)SourcesDataModel::Column::NextCheck)
			return utils::formatDuration(value.toUInt());

		auto number = value.toUInt();
		return number ? QString::number(number) : "";
	}
};

void SourcesTab::init(Ui_mainWindowWidget& ui)
{
	sourcesTab = ui.tabSources;
	model = new SourcesDataModel();
	auto filter = new QSortFilterProxyModel();
	filter->setSourceModel(model);

	table = ui.sourcesTable;
	table->setModel(filter);
	table->horizontalHeader()->resizeSection((int)SourcesDataModel::Column::Name, 300);
	table->setItemDelegate(new SourcesTableItemDelegate());

	table->connect(table, &QTableView::customContextMenuRequested, [this](const QPoint& pos) { showContextMenu(pos); });
	ui.tabWidget->connect(ui.tabWidget, &QTabWidget::currentChanged, [&](int index) { update(); });

 	const int rowHeight = table->verticalHeader()->minimumSectionSize();
	table->verticalHeader()->setMaximumSectionSize(rowHeight);
	table->verticalHeader()->setDefaultSectionSize(rowHeight);
}

void SourcesTab::selectTorrent(mttApi::TorrentPtr t)
{
	selected = t;
	update();
}

void SourcesTab::update()
{
	if (sourcesTab->isVisible())
	{
		model->update(selected);
	}
}

void SourcesTab::showContextMenu(const QPoint& pos)
{
	auto idx = table->indexAt(pos);

	if (idx.isValid())
	{
		auto name = model->sourceRefreshName(idx.row());
		if (!name.empty())
		{
			QMenu contextMenu;
			contextMenu.setStyleSheet("font-size: 12px");
			contextMenu.addAction(("Refresh " + name).c_str(), [this, name]()
				{
					selected->getPeers().refreshSource(name);
				});

			contextMenu.exec(table->viewport()->mapToGlobal(pos));
		}
	}
}
