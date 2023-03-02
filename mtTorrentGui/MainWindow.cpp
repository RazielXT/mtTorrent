#include "MainWindow.h"
#include <QFileIconProvider>
#include "SettingsWindow.h"
#include "MagnetWindow.h"
#include "NewTorrentWindow.h"
#include "Utils.h"

MainWindow::MainWindow(QStringList params, QWidget* parent)	: QMainWindow(parent), crossProcess(this)
{
	loadSettingsFile();

	core = mttApi::Core::create();
	{
		extern mttApi::Core* corePtr;
		extern TorrentsList* listPtr;
		corePtr = core.get();
		listPtr = &torrents;
	}

	ui.setupUi(this);

	connect(ui.buttonAddFile, &QPushButton::clicked, this, &MainWindow::addFile);
	connect(ui.buttonAddMagnet, &QPushButton::clicked, this, &MainWindow::addMagnet);
	connect(ui.buttonSettings, &QPushButton::clicked, this, &MainWindow::openSettings);

	torrents.init(ui);

	app::core().registerAlerts(mtt::Alerts::Category::Torrent | mtt::Alerts::Id::MetadataInitialized);

	QTimer* timer = new QTimer(ui.itemsTable);
	connect(timer, &QTimer::timeout, [this]()
		{
			auto alerts = app::core().popAlerts();
			for (auto& a : alerts)
			{
				if (a->id == mtt::Alerts::Id::TorrentAdded)
				{
					auto t = a->getAs<mtt::TorrentAlert>()->torrent;
					torrents.add(t);
				}
				else if (a->id == mtt::Alerts::Id::TorrentRemoved)
				{
					auto t = a->getAs<mtt::TorrentAlert>()->torrent;
					torrents.remove(t);
				}
				if (a->id == mtt::Alerts::Id::MetadataInitialized)
				{
					auto t = a->getAs<mtt::MetadataAlert>()->torrent;

					auto w = new NewTorrentWindow(t, this);
					w->setWindowModality(Qt::ApplicationModal);
					w->show();
				}
			}
		});
	timer->start(500);

	crossProcess.onMessage = [this](const QStringList& params)
	{
		{
			setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
			raise(); // for MacOS
			activateWindow(); // for Windows
		}

		handleAppParams(params);
	};
	crossProcess.startServer();

	if (!params.empty())
	{
		QTimer* timer = new QTimer(this);
		connect(timer, &QTimer::timeout, [this,params]()
			{
				handleAppParams(params);
			}
		);
		timer->setSingleShot(true);
		timer->start(500);
	}

	auto w = app::getSetting("width").toInt();
	auto h = app::getSetting("height").toInt();
	if (w && h)
		this->resize(w, h);
	if (app::getSetting("maximize").toBool())
		this->setWindowState(Qt::WindowMaximized);
	QList<int> sizes;
	sizes.append(app::getSetting("splitterUp").toInt());
	sizes.append(app::getSetting("splitterDown").toInt());
	if (sizes[0])
		ui.splitter->setSizes(sizes);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	torrents.deinit();

	bool maximized = this->windowState() == Qt::WindowMaximized;
	app::setSetting("maximize", maximized);
	if (!maximized)
	{
		app::setSetting("width", this->width());
		app::setSetting("height", this->height());
	}
	auto size = ui.splitter->sizes();
	app::setSetting("splitterUp", size[0]);
	app::setSetting("splitterDown", size[1]);

	QMainWindow::closeEvent(event);
}

MainWindow::~MainWindow()
{
	saveSettingsFile();
}

void MainWindow::openSettings()
{
	SettingsWindow w;
	w.exec();
}

void MainWindow::addMagnet()
{
	MagnetWindow w;
	w.exec();
}

void MainWindow::addFile()
{
	auto fileName = QFileDialog::getOpenFileName(nullptr, tr("Open torrent file"), {}, tr("Torrent File (*.torrent);;Any file (*.*)"));

	if (!fileName.isEmpty())
	{
		addFilename(fileName);
	}
}

void MainWindow::addFilename(const QString& fileName)
{
	auto [status, t] = app::core().addFile(fileName.toLocal8Bit().data());

	if (status != mtt::Status::Success)
	{
		QMessageBox msgBox;
		msgBox.setWindowTitle("Failed");
		msgBox.setStyleSheet(guiUtils::CenteredMessageBoxStyle());

		if (status == mtt::Status::I_AlreadyExists || status == mtt::Status::I_Merged)
		{
			msgBox.setInformativeText(t->name().c_str());

			if (status == mtt::Status::I_AlreadyExists)
				msgBox.setText("Torrent already exists");
			else if (status == mtt::Status::I_Merged)
				msgBox.setText("Torrent already exists, new trackers added");
		}
		else
		{
			msgBox.setText("Not valid torrent file");
			msgBox.setInformativeText(fileName);
		}

		msgBox.setStandardButtons(QMessageBox::Ok);
		msgBox.exec();
	}
}

void MainWindow::handleAppParams(const QStringList& params)
{
	for (const auto& p : params)
	{
		if (mtt::TorrentFileMetadata().parseMagnetLink(p.toStdString()) == mtt::Status::Success)
		{
			MagnetWindow::Show(p);
		}
		else
		{
			addFilename(p);
		}
	}
}

QMap<QString, QVariant> settings;

void MainWindow::saveSettingsFile()
{
	QSettings file("./data/ui.ini", QSettings::IniFormat);
	for (auto it = settings.constKeyValueBegin(); it != settings.constKeyValueEnd(); it++)
		file.setValue(it->first, it->second);
}

void MainWindow::loadSettingsFile()
{
	QSettings file("./data/ui.ini", QSettings::IniFormat);
	for (const auto& k : file.allKeys())
		settings[k] = file.value(k);
}
