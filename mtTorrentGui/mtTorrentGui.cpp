#include "mtTorrentGui.h"
#include "TorrentDelegate.h"
#include "qfiledialog.h"
#include "QMessageBox.h"

mtTorrentGui::mtTorrentGui(QWidget *parent)
	: QMainWindow(parent), updateTimer(this)
{
	ui.setupUi(this);

	torrents.init();

	QStringList headers;
	headers << tr("Torrent") << tr("Peers/Seeds") << tr("Progress")
		<< tr("Down rate") << tr("Up rate") << tr("Status");

	ui.treeWidget->setColumnCount(headers.size());
//	ui.tableWidget->horizontalHeader()->sor
	ui.treeWidget->setSortingEnabled(true);
	ui.treeWidget->setHeaderLabels(headers);
	//ui.treeWidget->setsetStretchLastSection(true);
	ui.treeWidget->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
	ui.treeWidget->setAlternatingRowColors(true);
	ui.treeWidget->setRootIsDecorated(false);

	ui.treeWidget->setItemDelegate(new TorrentViewDelegate(this));

	updateTorrent();

	ui.treeWidget->startTimer(1000);
	connect(ui.treeWidget, SIGNAL(timeout()), this, SLOT(updateTorrent()));
}

int i = 0;

void mtTorrentGui::updateTorrent()
{	
	QTreeWidgetItem* item = new QTreeWidgetItem();
	item->setText(0, tr("0/0"));
	item->setText(1, tr("0/0"));
	item->setText(2, QString::number(i++));
	item->setText(3, "0.0 KB/s");
	item->setText(4, "0.0 KB/s");
	item->setText(5, tr("Idle"));

	ui.treeWidget->addTopLevelItem(item);
}

void mtTorrentGui::addButtonPressed()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Images (*.png *.xpm *.jpg)"));

	QStringList fileNames;
	if (dialog.exec())
		fileNames = dialog.selectedFiles();

	for (auto& f : fileNames)
	{
		uint32_t id;

		if (torrents.addTorrent(f, id))
		{

		}
		else
		{
			auto indx = f.lastIndexOf('/');
			auto filename = f.mid(indx > 0 ? indx+1 : indx);
			QMessageBox::warning(this, tr("Add failed"), tr("Loading of torrent file ") + filename + " failed.", QMessageBox::Ok);
		}
	}
}
