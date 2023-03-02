#pragma once

#include <QDialog>
#include "ui_MagnetWindow.h"
#include "Utils.h"

class MagnetWindow : public QDialog
{
	Q_OBJECT

public:
	MagnetWindow(QWidget *parent = nullptr);
	~MagnetWindow();

	static void Show(mttApi::TorrentPtr t);
	static void Show(QString);

public slots:
	void addMagnet();

private:
	Ui::MagnetWindowClass ui;

	void setTorrent(mttApi::TorrentPtr t);
	void setMagnet(QString);
	void updateLogs();
	size_t logPos = 0;
	mttApi::TorrentPtr torrent;

	bool autoClose = false;
};
