#include "SettingsWindow.h"
#include "QFileDialog"
#include "QFileIconProvider"
#include "Utils.h"
#include "QSettings"
#include "QMessageBox"
#include "QTimer"

SettingsWindow::SettingsWindow(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.browseFolderButton, &QPushButton::clicked, this, &SettingsWindow::browseFolder);
	connect(ui.buttonsBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &SettingsWindow::closeButton);
	connect(ui.buttonsBox->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &SettingsWindow::saveButton);

#ifdef _WIN32
	connect(ui.associateButton, &QPushButton::clicked, this, [this]()
		{
			QSettings settings(R"(HKEY_CURRENT_USER\Software\Classes\Magnet\shell\open\command)", QSettings::NativeFormat);
			QString command = "\"" + QCoreApplication::applicationFilePath();
			command.replace("/", "\\");
			command += R"(" "%1")";
			settings.setValue("Default", command);
			settings.sync();

			auto status = settings.status();

			QMessageBox msg;
			msg.setStyleSheet(guiUtils::CenteredMessageBoxStyle());
			msg.setWindowTitle("Magnet link associated");
			msg.setText("Magnet links will now be opened with mtTorrent");
			msg.exec();
		});
#else
	ui.associateButton->hide();
#endif

	ui.browseFolderButton->setIcon(utils::getIconProvider().icon(QFileIconProvider::Folder));

	config = mtt::config::getExternal();

	ui.defaultFolderInput->setText(config.files.defaultDirectory.c_str());

	ui.listenPortSpinBox->setValue((int)config.connection.tcpPort);
	ui.maxConnectionsSpinBox->setValue((int)config.connection.maxTorrentConnections);
	ui.maxDlSpinBox->setValue((int)config.transfer.maxDownloadSpeed * 1024);
	ui.maxUpSpinBox->setValue((int)config.transfer.maxUploadSpeed * 1024);
	ui.upnpCheckBox->setChecked(config.connection.upnpPortMapping);

	ui.dhtCheckBox->setChecked(config.dht.enabled);
	ui.utpCheckBox->setChecked(config.connection.enableUtpIn);
	ui.encryptionComboBox->addItems({ "Refuse", "Allow", "Require" });
	ui.encryptionComboBox->setCurrentIndex((int)config.connection.encryption);

	auto updateFunc = [this]()
	{
		auto upnpInfo = app::core().getPortListener().getUpnpReadableInfo();
		if (upnpInfo.empty())
			ui.upnpInfoLabel->hide();
		else
		{
			ui.upnpInfoLabel->setText(upnpInfo.c_str());
			ui.upnpInfoLabel->show();
		}

		auto dhtNodes = app::core().getDht().getNodesCount();
		if (dhtNodes == 0)
			ui.dhtInfoLabel->hide();
		else
		{
			ui.dhtInfoLabel->setText("Current DHT nodes count " + QString::number(dhtNodes));
			ui.dhtInfoLabel->show();
		}
	};

	auto timer = new QTimer(ui.upnpInfoLabel);
	connect(timer, &QTimer::timeout, updateFunc);

	timer->start(1000);
	updateFunc();
}

SettingsWindow::~SettingsWindow()
{}

void SettingsWindow::browseFolder()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::Directory);

	if (dialog.exec())
	{
		auto path  = dialog.selectedFiles()[0];

		ui.defaultFolderInput->setText(path);
	}
}

void SettingsWindow::closeButton()
{
	close();
}

void SettingsWindow::saveButton()
{
	config.files.defaultDirectory = ui.defaultFolderInput->text().toStdString();

	config.connection.tcpPort = config.connection.udpPort = (uint32_t)ui.listenPortSpinBox->value();
	config.connection.maxTorrentConnections = (uint32_t)ui.maxConnectionsSpinBox->value();
	config.transfer.maxDownloadSpeed = (uint32_t) ui.maxDlSpinBox->value() / 1024;
	config.transfer.maxUploadSpeed = (uint32_t)ui.maxUpSpinBox->value() / 1024;
	config.connection.upnpPortMapping = (uint32_t)ui.upnpCheckBox->isChecked();

	config.dht.enabled = ui.dhtCheckBox->isChecked();
	config.connection.enableUtpIn = config.connection.enableUtpOut = ui.utpCheckBox->isChecked();
	config.connection.encryption = (mtt::config::Encryption)ui.encryptionComboBox->currentIndex();

	mtt::config::setValues(config);

	close();
}
