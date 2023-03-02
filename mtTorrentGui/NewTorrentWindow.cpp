#include "NewTorrentWindow.h"
#include "QPushButton"
#include "QSortFilterProxyModel"
#include "mtTorrent/Api/Configuration.h"
#include "QFileDialog"
#include "QStorageInfo"
#include "QMessageBox"
#include "TorrentsList.h"

NewTorrentWindow::NewTorrentWindow(mttApi::TorrentPtr t, QWidget *parent)
	: QDialog(parent), torrent(t)
{
	ui.setupUi(this);
	setWindowTitle(torrent->name().c_str());

	connect(ui.buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &NewTorrentWindow::cancelButton);
	connect(ui.buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &NewTorrentWindow::okButton);

	connect(ui.changeFolderButton, &QPushButton::clicked, this, &NewTorrentWindow::browseFolder);
	ui.changeFolderButton->setIcon(utils::getIconProvider().icon(QFileIconProvider::Folder));

	model = new FilesDataModel();
	auto filter = new QSortFilterProxyModel();
	filter->setSourceModel(model);

	auto filesTree = ui.filesTree;
	filesTree->setModel(filter);
	filesTree->header()->resizeSection((int)FilesDataModel::Column::Name, 350);
	filesTree->setEditTriggers(QAbstractItemView::CurrentChanged);

	filesTree->setItemDelegateForColumn((int)FilesDataModel::Column::Size, new guiUtils::BytesItemDelegate());
	filesTree->setItemDelegateForColumn((int)FilesDataModel::Column::Priority, new guiUtils::ComboBoxItemDelegate(model->priorityMap));
	
	filesTree->hideColumn((int)FilesDataModel::Column::Progress);
	filesTree->hideColumn((int)FilesDataModel::Column::Pieces);
	filesTree->hideColumn((int)FilesDataModel::Column::Remaining);
	filesTree->hideColumn((int)FilesDataModel::Column::Active);

	model->select(torrent);
	filesTree->expandAll();

	ui.folderInput->setText(t->getFiles().getLocationPath().c_str());
	validateFolder();

	model->onCheckChange = [this]()
		{
			validateFolder();
		};
}

NewTorrentWindow::~NewTorrentWindow()
{}

void NewTorrentWindow::browseFolder()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::Directory);

	if (dialog.exec())
	{
		auto path = dialog.selectedFiles()[0];

		ui.folderInput->setText(path);
		validateFolder();
	}
}

void NewTorrentWindow::cancelButton()
{
	app::core().removeTorrent(torrent, true);
	close();
}

void NewTorrentWindow::okButton()
{
	if (!validateFolder())
		return;

	auto status = torrent->getFiles().setLocationPath(ui.folderInput->text().toStdString(), false);

	if (status != mtt::Status::Success)
	{
		ui.errorLabel->setText(status == mtt::Status::E_InvalidPath ? "Invalid path" : "Error setting destination folder");
		ui.errorLabel->show();
	}
	else
	{
		if (ui.startCheckBox->isChecked())
			torrent->start();
	}

	app::list().select(torrent);

	close();
}

void NewTorrentWindow::refreshSummary()
{
	QString txt = "Selected " + utils::formatBytes(validation.requiredSize) + "/" + utils::formatBytes(torrent->getMetadata().info.fullSize);
	txt += ", Free space: " + utils::formatBytes(validation.availableSpace);
	ui.summaryLabel->setText(txt);
}

bool NewTorrentWindow::validateFolder()
{
	validation = torrent->getFiles().validatePath(ui.folderInput->text().toStdString());
	refreshSummary();

	if (validation.status != mtt::Status::Success)
	{
		QString errorTxt;
		if (validation.status == mtt::Status::E_NotEnoughSpace)
		{
			errorTxt = "Not enough space, Available: ";
			errorTxt += utils::formatBytes(validation.availableSpace);
			errorTxt += ", Needed: ";
			errorTxt += utils::formatBytes(validation.missingSize);
		}
		else
			errorTxt = "Invalid path";

		ui.errorLabel->setText(errorTxt);
		ui.errorLabel->show();
		return false;
	}

	ui.errorLabel->hide();
	return true;
}
