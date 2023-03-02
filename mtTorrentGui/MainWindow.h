#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "TorrentsList.h"
#include "CrossProcess.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QStringList params, QWidget *parent = nullptr);
    ~MainWindow();

public slots:
	void openSettings();
    void addMagnet();
    void addFile();
private:
    Ui::mainWindowWidget ui;

    TorrentsList torrents;

    std::shared_ptr<mttApi::Core> core;

    CrossProcess crossProcess;

    void addFilename(const QString&);

    void handleAppParams(const QStringList&);

    void saveSettingsFile();
    void loadSettingsFile();

    void closeEvent(QCloseEvent* event) override;
};
