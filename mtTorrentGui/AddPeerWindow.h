#pragma once

#include <QDialog>
#include "ui_AddPeerWindow.h"
#include "Utils.h"

class AddPeerWindow : public QDialog
{
	Q_OBJECT

public:
	AddPeerWindow(mttApi::TorrentPtr t, QWidget *parent = nullptr);
	~AddPeerWindow();

private:
	Ui::AddPeerWindowClass ui;

	mttApi::TorrentPtr torrent;
};
