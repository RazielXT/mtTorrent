#include "MagnetWindow.h"
#include "QTimer"
#include "QScrollBar"
#include "QClipboard"
#include "TorrentsList.h"

MagnetWindow::MagnetWindow(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.magnetAddButton, &QPushButton::clicked, this, &MagnetWindow::addMagnet);

	auto txt = QGuiApplication::clipboard()->text();
	if (mtt::TorrentFileMetadata().parseMagnetLink(txt.toStdString()) == mtt::Status::Success)
		ui.magnetInputText->setText(txt);
}

MagnetWindow::~MagnetWindow()
{}

void MagnetWindow::Show(mttApi::TorrentPtr t)
{
	MagnetWindow w;
	w.setTorrent(t);
	w.exec();
}

void MagnetWindow::Show(QString magnet)
{
	MagnetWindow w;
	w.setMagnet(magnet);
	w.exec();
}

void MagnetWindow::setTorrent(mttApi::TorrentPtr t)
{
	torrent = t;

	ui.magnetAddButton->hide();
	ui.magnetInputLabel->hide();
	ui.magnetInputText->setReadOnly(true);
	ui.magnetInputText->append("--------------------------");

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() { updateLogs(); });
	timer->start(500);

	updateLogs();
}

void MagnetWindow::setMagnet(QString magnet)
{
	ui.magnetInputText->setText(magnet);
	addMagnet();
}

void MagnetWindow::addMagnet()
{
	auto txt = ui.magnetInputText->toPlainText();
	if (txt.isEmpty())
		return;

	auto [status,t] = app::core().addMagnet(txt.toLocal8Bit().data());
	if (status != mtt::Status::Success)
	{
		ui.magnetInputLabel->setText("Invalid magnet link");
		ui.magnetInputLabel->setStyleSheet("QLabel { color : red; }");
		return;
	}

	autoClose = true;
	setTorrent(t);
}

void MagnetWindow::updateLogs()
{
	if (!torrent)
		return;

	if (auto info = torrent->getMetadataDownload())
	{
		std::vector<std::string> logs;
		logPos += info->getDownloadLog(logs, logPos);

		for (auto& l : logs)
		{
			ui.magnetInputText->append(l.c_str());
			ui.magnetInputText->verticalScrollBar()->setValue(ui.magnetInputText->verticalScrollBar()->maximum());
		}

		if (info->getState().finished && !ui.magnetInputLabel->isVisible())
		{
			ui.magnetInputLabel->setText("Finished");
			ui.magnetInputLabel->setStyleSheet("");
			ui.magnetInputLabel->setVisible(true);

			if (autoClose)
				close();
		}
	}
}
