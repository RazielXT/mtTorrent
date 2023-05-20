#include "TorrentsList.h"
#include "QSortFilterProxyModel"
#include "QtWidgets/QHeaderView"
#include "QtWidgets/QStyledItemDelegate"
#include "QTimer"
#include "QMenu"
#include "QMessageBox"
#include "MagnetWindow.h"

TorrentsList::TorrentsList()
{
}

class TorrentsListItemDelegate : public guiUtils::CustomTableItemDelegate
{
public:
	TorrentsListItemDelegate(TorrentsList& l) : list(l) {};
	TorrentsList& list;
	QString displayText(const QVariant& value, const QLocale& locale) const override
	{
		auto column = index.column();
		switch ((TorrentsDataModel::Column)column)
		{
		case TorrentsDataModel::Column::Progress:
		{
			auto selectedProgress = value.toFloat();
			QString txt = utils::formatPercentage(selectedProgress);
			auto t = list.getTorrent(index);
			if (t->getFiles().getPiecesCount() != t->getFiles().getSelectedPiecesCount())
			{
				auto fullProgress = t->progress();
				if (fullProgress != selectedProgress)
					txt += " (" + utils::formatPercentage(fullProgress) + ")";
			}

			return txt;
		}
		case TorrentsDataModel::Column::Download:
		{
			auto downloadSpeed = value.toULongLong();
			auto txt = utils::formatBytesSpeed(downloadSpeed);
			if (downloadSpeed > 512)
			{
				auto t = list.getTorrent(index);
				if (!t->selectionFinished() && t->getState() == mttApi::Torrent::State::Active)
				{
					auto downloaded = t->finishedSelectedBytes();
					uint64_t leftBytes = ((uint64_t)(downloaded / t->selectionProgress())) - downloaded;
					if (uint64_t leftTime = leftBytes / downloadSpeed)
						txt += " (" + utils::formatDuration(leftTime) + ")";
				}
			}
			return txt;
		}
		case TorrentsDataModel::Column::Upload:
			return utils::formatBytesSpeed(value.toULongLong());
		case TorrentsDataModel::Column::Peers:
		{
			auto t = list.getTorrent(index);
			if (value.toUInt() || t->started())
			{
				return value.toString() + " (" + QString::number(t->getPeers().receivedCount()) + ")";
			}
			return "";
		}
		case TorrentsDataModel::Column::Downloaded:
		case TorrentsDataModel::Column::Uploaded:
			return utils::formatBytes(value.toULongLong());
		case TorrentsDataModel::Column::Added:
			return utils::formatTimestamp(value.toULongLong());
		case TorrentsDataModel::Column::Name:
		case TorrentsDataModel::Column::State:
		default:
			return value.toString();
		}
	}
};

void TorrentsList::init(Ui_mainWindowWidget& mainWindow)
{
	filesTab.init(mainWindow);
	speedTab.init(mainWindow);
	infoTab.init(mainWindow);
	peersTab.init(mainWindow);
	sourcesTab.init(mainWindow);

	table = mainWindow.itemsTable;
	window = &mainWindow;

	table->connect(table, &QTableView::customContextMenuRequested, [this](const QPoint& pos) { showContextMenu(pos); });

	model = new TorrentsDataModel();
	filter = new QSortFilterProxyModel();
	filter->setSourceModel(model);
	table->setModel(filter);
	table->horizontalHeader()->setSectionsMovable(true);
	table->setColumnWidth((int)TorrentsDataModel::Column::Name, 300);
	table->setColumnWidth((int)TorrentsDataModel::Column::Peers, 100);
	table->setColumnWidth((int)TorrentsDataModel::Column::Downloaded, 80);
	table->setColumnWidth((int)TorrentsDataModel::Column::Uploaded, 80);
	table->setItemDelegate(new TorrentsListItemDelegate(*this));

	table->connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& selected, const QItemSelection& deselected) { selectionChanged(selected, deselected); });

	auto timer = new QTimer(table);
	table->connect(timer, &QTimer::timeout, [this]()
		{
			refresh();
		});
	timer->start(1000);

	window->tabWidget->connect(window->tabWidget, &QTabWidget::currentChanged, [&](int index)
		{
			filesTab.update();
			speedTab.update();
			peersTab.update();
			sourcesTab.update();
		});

	window->buttonStart->connect(window->buttonStart, &QPushButton::clicked, [this](bool checked) { start(); });
	window->buttonStop->connect(window->buttonStop, &QPushButton::clicked, [this](bool checked) { stop(); });
	window->buttonRemove->connect(window->buttonRemove, &QPushButton::clicked, [this](bool checked) { remove(); });

	const int rowHeight = table->verticalHeader()->minimumSectionSize();
	table->verticalHeader()->setMaximumSectionSize(rowHeight);
	table->verticalHeader()->setDefaultSectionSize(rowHeight);

	auto sortColumn = app::getSetting("torrentSortColumn");
	auto sortOrder = app::getSetting("torrentSortOrder");
	if (sortColumn.isValid())
		table->horizontalHeader()->setSortIndicator(sortColumn.toInt(), (Qt::SortOrder)sortOrder.toInt());
	else
		table->horizontalHeader()->setSortIndicator((int)TorrentsDataModel::Column::Added, Qt::DescendingOrder);
	auto columnSizes = app::getSetting("columnSizes").toStringList();
	if (columnSizes.size() == (int)TorrentsDataModel::Column::COUNT)
		for (int i = 0; i < columnSizes.size(); i++)
			if (auto sz = columnSizes[i].toInt())
				table->setColumnWidth(i, sz);
}

void TorrentsList::deinit()
{
	app::setSetting("torrentSortColumn", table->horizontalHeader()->sortIndicatorSection());
	app::setSetting("torrentSortOrder", (int)table->horizontalHeader()->sortIndicatorOrder());

	filesTab.deinit();

	QStringList columnSizes;
	for (int i = 0; i < (int)TorrentsDataModel::Column::COUNT; i++)
		columnSizes.append(QString::number(table->columnWidth(i)));
	app::setSetting("columnSizes", columnSizes);
}

void TorrentsList::add(mttApi::TorrentPtr t)
{
	model->add(t);
}

void TorrentsList::remove(mttApi::TorrentPtr t)
{
	model->remove(t);
}

void TorrentsList::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	refreshButtons();

	if (selected.empty() && deselected.empty())
		return;

	QModelIndex index;

	if (!selected.empty())
		index = selected.first().indexes().front();
	else
		index = table->currentIndex();

	if (index.isValid())
	{
		auto idx = filter->mapToSource(index).row();
		selectTorrent(idx);
	}
	else
		selectTorrent(-1);
}

void TorrentsList::showContextMenu(const QPoint& pos)
{
	auto idx = table->indexAt(pos);

	if (idx.isValid())
	{
		auto t = getTorrent(idx);

		QMenu contextMenu(table);
		contextMenu.setStyleSheet("font-size: 12px");

		auto actionOpenDir = contextMenu.addAction("Open directory", [t]() { QDesktopServices::openUrl(QUrl::fromLocalFile(t->getFiles().getLocationPath().c_str())); });
		actionOpenDir->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_DirLinkIcon));

		auto actionChangeDir = contextMenu.addAction("Change directory", [t, this]() { changeTorrentDirectory(t); });
		actionChangeDir->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_DirIcon));

		if (t->getMetadataDownload())
		{
			auto actionMagnetLogs = contextMenu.addAction("Magnet logs", [t]() { MagnetWindow::Show(t); });
			actionMagnetLogs->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_FileDialogDetailedView));
		}

		if (t->hasMetadata())
		{
			auto actionCheck = contextMenu.addAction("Check files", [t]() { t->getFiles().startCheck(); });
			actionCheck->setIcon(QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_FileDialogContentsView));
		}

		contextMenu.exec(table->viewport()->mapToGlobal(pos));
	}
}

void TorrentsList::changeTorrentDirectory(const mttApi::TorrentPtr& t)
{
	QFileDialog dialog(table);
	dialog.setFileMode(QFileDialog::Directory);

	auto originalPath = QString(t->getFiles().getLocationPath().c_str());
	dialog.setDirectory(originalPath);

	if (dialog.exec())
	{
		auto path = dialog.selectedFiles()[0];
		if (originalPath == path)
			return;

		auto validation = t->getFiles().validatePath(path.toStdString());

		QString errorTxt;
		if (validation.status != mtt::Status::Success)
		{
			if (validation.status == mtt::Status::E_NotEnoughSpace)
			{
				errorTxt = "Not enough space, Available: ";
				errorTxt += utils::formatBytes(validation.availableSpace);
				errorTxt += ", Needed: ";
				errorTxt += utils::formatBytes(validation.missingSize);
			}
			else
				errorTxt = "Invalid path";
		}
		else
		{
			auto status = t->getFiles().setLocationPath(path.toStdString(), true);

			if (status == mtt::Status::E_NotEmpty)
			{
				QMessageBox msgBox;
				msgBox.setText("Target directory already contains files, inherit?");
				msgBox.setWindowTitle("Change directory");
				msgBox.setStyleSheet(guiUtils::CenteredMessageBoxStyle());
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
				if (msgBox.exec() == QMessageBox::Yes)
					status = t->getFiles().setLocationPath(path.toStdString(), false);
			}

			if (status != mtt::Status::Success)
			{
				errorTxt = "Problem changing directory";
			}
		}

		if (!errorTxt.isEmpty())
		{
			QMessageBox msgBox;
			msgBox.setText(errorTxt);
			msgBox.setWindowTitle("Change directory");
			msgBox.setStyleSheet(guiUtils::CenteredMessageBoxStyle());
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.exec();
		}
		else
			infoTab.selectTorrent(t);
	}
}

void TorrentsList::select(mttApi::TorrentPtr selected)
{
	int idx = 0;
	for (auto& t : model->torrents)
	{
		if (t == selected)
		{
			QModelIndex index = model->index(idx, 0);
			index = filter->mapFromSource(index);
			table->setCurrentIndex(index);
			return;
		}
		idx++;
	}
}

void TorrentsList::refresh(mttApi::TorrentPtr t)
{
	model->forceRefresh = t;
	infoTab.selectTorrent(t);
}

void TorrentsList::refresh()
{
	model->update();

	filesTab.update();
	speedTab.update();
	peersTab.update();
	sourcesTab.update();

	refreshButtons();
}

mttApi::TorrentPtr TorrentsList::getTorrent(const QModelIndex& idx) const
{
	return model->torrents[filter->mapToSource(idx).row()];
}

void TorrentsList::selectTorrent(int idx)
{
	mttApi::TorrentPtr selected;

	if (idx != -1 && idx < model->torrents.size())
		selected = model->torrents[idx];

	infoTab.selectTorrent(selected);
	filesTab.selectTorrent(selected);
	speedTab.selectTorrent(selected);
	peersTab.selectTorrent(selected);
	sourcesTab.selectTorrent(selected);
}

void TorrentsList::refreshButtons()
{
	bool selectionStopped = false;
	bool selectionActive = false;

	auto selection = getSelectionIndex();
	bool hasSelection = !selection.isEmpty();

	if (hasSelection)
	{
		for (auto& idx : selection)
		{
			if (idx < model->torrents.size())
			{
				auto state = model->torrents[idx]->getState();
				if (state != mttApi::Torrent::State::Stopping && state != mttApi::Torrent::State::Stopped)
					selectionActive = true;
				else
					selectionStopped = true;
			}
		}
	}

	window->buttonStart->setEnabled(selectionStopped);
	window->buttonStop->setEnabled(selectionActive);
	window->buttonRemove->setEnabled(hasSelection);
}

void TorrentsList::start()
{
	auto selection = getSelectionIndex();
	if (!selection.isEmpty())
	{
		for (auto idx : selection)
		{
			if (idx < model->torrents.size())
			{
				model->torrents[idx]->start();
			}
		}

		model->update();
	}

	refreshButtons();
}

void TorrentsList::stop()
{
	auto selection = getSelectionIndex();
	if (!selection.isEmpty())
	{
		for (auto idx : selection)
		{
			if (idx < model->torrents.size())
			{
				model->torrents[idx]->stop();
			}
		}

		model->update();
	}

	refreshButtons();
}

void TorrentsList::remove()
{
	QMessageBox msgBox;
	msgBox.setText("Delete files?");
	msgBox.setWindowTitle("Remove");
	msgBox.setStyleSheet(guiUtils::CenteredMessageBoxStyle());
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	int ret = msgBox.exec();

	if (ret != QMessageBox::Cancel)
	{
		bool deleteFiles = ret == QMessageBox::Yes;
		auto selection = getSelectionIndex();

		for (auto idx : selection)
		{
			if (idx < model->torrents.size())
			{
				app::core().removeTorrent(model->torrents[idx], deleteFiles);
			}
		}
	}
}

QList<int> TorrentsList::getSelectionIndex() const
{
	auto selection = table->selectionModel()->selectedRows();
	QList<int> indices;
	indices.reserve(selection.size());

	for (auto& s : selection)
	{
		indices.append(filter->mapToSource(s).row());
	}
	return indices;
}
