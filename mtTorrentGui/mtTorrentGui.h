#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mtTorrentGui.h"

class mtTorrentGui : public QMainWindow
{
    Q_OBJECT

public:
    mtTorrentGui(QWidget *parent = Q_NULLPTR);

private:
    Ui::mtTorrentGuiClass ui;
};
