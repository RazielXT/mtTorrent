#pragma once

#include <QDialog>
#include "ui_NewTorrentWindow.h"
#include "mtTorrent/Api/Interface.h"
#include "FilesDataModel.h"

class NewTorrentWindow : public QDialog
{
	Q_OBJECT

public:
	NewTorrentWindow(mttApi::TorrentPtr, QWidget *parent = nullptr);
	~NewTorrentWindow();

public slots:
	void browseFolder();
	void cancelButton();
	void okButton();

private:
	Ui::NewTorrentWindowClass ui;
	FilesDataModel* model;
	mttApi::TorrentPtr torrent;

	void refreshSummary();
	bool validateFolder();

	mtt::PathValidation validation;
};
