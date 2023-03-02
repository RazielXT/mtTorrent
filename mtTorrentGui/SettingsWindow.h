#pragma once

#include <QDialog>
#include "ui_SettingsWindow.h"
#include "mtTorrent/Api/Configuration.h"

class SettingsWindow : public QDialog
{
	Q_OBJECT

public:
	SettingsWindow(QWidget *parent = nullptr);
	~SettingsWindow();
public slots:
	void browseFolder();
	void closeButton();
	void saveButton();
private:
	Ui::SettingsWindowClass ui;

	mtt::config::External config;
};
