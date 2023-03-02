#include "FilesTab.h"
#include "Utils.h"
#include "QTimer"

void FilesTab::init(Ui_mainWindowWidget& ui)
{
	filesTree = ui.filesTree;
	filesTab = ui.tabFiles;

	model = new FilesDataModel();
	filter = new QSortFilterProxyModel();
	filter->setSourceModel(model);
	filesTree->setModel(filter);
	filesTree->header()->resizeSection((int)FilesDataModel::Column::Name, 350);
	filesTree->setEditTriggers(QAbstractItemView::CurrentChanged);

	filesTree->setItemDelegateForColumn((int)FilesDataModel::Column::Size, new guiUtils::BytesItemDelegate());
	filesTree->setItemDelegateForColumn((int)FilesDataModel::Column::Priority, new guiUtils::ComboBoxItemDelegate(model->priorityMap));
	filesTree->setItemDelegateForColumn((int)FilesDataModel::Column::Progress, new guiUtils::PercentageItemDelegate());

	piecesChart = ui.piecesChartWidget;
	piecesChart->setScaledContents(true);
	piecesChart->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	piecesChartLabel = ui.piecesChartLabel;

	filesTree->connect(filesTree, &QTreeView::clicked, [this](const QModelIndex& index) { filesClicked(index); });
}

void FilesTab::update()
{
	if (filesTab->isVisible())
	{
		if (selected)
		{
			redrawPiecesChart();
			model->updateFilesState();
			cleared = false;
		}
		else if (!cleared)
		{
			clear();
			cleared = true;
		}
	}
}

void FilesTab::clear()
{
	if (!piecesImage.width())
		piecesImage = QImage(1, 1, QImage::Format_RGB32);

	piecesImage.fill(QColorConstants::Svg::lightgray);

	auto px = QPixmap::fromImage(piecesImage);
	piecesChart->setPixmap(px.scaled(piecesChart->width(), 1, Qt::IgnoreAspectRatio, Qt::FastTransformation));

	piecesChartLabel->setText("");
}

void FilesTab::selectTorrent(mttApi::TorrentPtr t)
{
	selected = t;
	model->select(selected);

	if (t)
		filesTree->expandAll();
	else
		clear();
}

int piecesHighlight[2] = { 0,0 };

void FilesTab::filesClicked(const QModelIndex& index)
{
	auto fileIdx = model->getFileIdx(filter->mapToSource(index));

	if (fileIdx != -1)
	{
		auto& file = selected->getFilesInfo()[fileIdx];

		piecesHighlight[0] = file.startPieceIndex;
		piecesHighlight[1] = file.endPieceIndex;
		redrawPiecesChart();
		auto stopHighlight = [this](int myStart) 
		{
			if (myStart == piecesHighlight[0])
			{
				piecesHighlight[0] = piecesHighlight[1] = 0;
				redrawPiecesChart();
			}
		};

		QTimer* timer = new QTimer(piecesChart);
		auto myStart = piecesHighlight[0];
		piecesChart->connect(timer, &QTimer::timeout, std::bind(stopHighlight, myStart));
		timer->setSingleShot(true);
		timer->start(500);
	}
}

void FilesTab::redrawPiecesChart()
{
	size_t piecesCount = selected->getMetadata().info.pieces.size();
	uint32_t finishedPieces = 0;

	if (piecesImage.width() != piecesCount)
		piecesImage = QImage(piecesCount ? (int)piecesCount : 1, 1, QImage::Format_RGB32);

	piecesImage.fill(QColorConstants::Svg::lightgray);

	QPainter painter(&piecesImage);

	//in progress - orange
	{
		painter.setPen(QColor(255, 140, 0));
		auto active = selected->getFileTransfer().getCurrentRequests();
		for (auto a : active)
			painter.drawPoint(QPoint(a, 0));
	}
	//done - green
	{
		painter.setPen(QColor(17, 175, 45));
		if (selected->getFiles().getPiecesBitfield(bitfield))
		{
			for (size_t i = 0; i < piecesCount; i++)
			{
				if (bitfield.has(i))
				{
					finishedPieces++;
					painter.drawPoint(QPoint(i, 0));
				}
			}
		}
	}
	//highlight - blue
	{
		painter.setPen(QColor(0, 120, 215));
		for (int i = piecesHighlight[0]; i < piecesHighlight[1]; i++)
		{
			if (i < piecesCount)
				painter.drawPoint(QPoint(i, 0));
		}
	}

	piecesChartLabel->setText(QString::number(finishedPieces) + "/" + QString::number(piecesCount));

	auto px = QPixmap::fromImage(piecesImage);
	piecesChart->setPixmap(px.scaled(piecesChart->width(), 1, Qt::IgnoreAspectRatio, Qt::FastTransformation));
}
