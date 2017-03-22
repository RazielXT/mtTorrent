#include "mtTorrentGui.h"

mtTorrentGui::mtTorrentGui(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	QStringList headers;
	headers << tr("Torrent") << tr("Peers/Seeds") << tr("Progress")
		<< tr("Down rate") << tr("Up rate") << tr("Status");

	ui.tableWidget->setColumnCount(headers.size());
	ui.tableWidget->setHorizontalHeaderLabels(headers);
	ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
}
