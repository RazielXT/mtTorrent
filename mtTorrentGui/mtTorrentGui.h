#pragma once

#include <QtWidgets/QMainWindow>
#include <Qtimer.h>
#include "ui_mtTorrentGui.h"
#include "TorrentsMgr.h"

class mtTorrentGui : public QMainWindow
{
    Q_OBJECT

public:
    mtTorrentGui(QWidget *parent = Q_NULLPTR);

private:

	TorrentsMgr torrents;

    Ui::mtTorrentGuiClass ui;

	QTimer updateTimer;

public Q_SLOTS:

	void updateTorrent();
	void addButtonPressed();
};
