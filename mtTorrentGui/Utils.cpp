#include "Utils.h"
#include "QComboBox"
#include "QMessageBox"
#include "QSettings"

QFileIconProvider& utils::getIconProvider()
{
	static QFileIconProvider provider;
	return provider;
}

QString utils::formatBytes(uint64_t bytes)
{
	const char* type = " MB";

	auto sz = ((bytes / 1024.f) / 1024.f);
	if (sz < 1)
	{
		sz = (float)(bytes / (1024ll));
		type = " KB";

		if (sz == 0)
		{
			sz = (float)bytes;
			type = " B";
		}
	}
	else if (sz > 1024)
	{
		sz /= 1024.f;
		type = " GB";
	}

	QString str = QString::number(sz, 'f', 2);
	while (str.back() == '0')
		str.chop(1);
	if (str.back() == '.')
		str.chop(1);

	return str + QString(type);
}

QString utils::formatBytesSpeed(uint64_t bytes)
{
	return bytes ? (formatBytes(bytes) + "/s") : "";
}

QString utils::formatPercentage(float p)
{
	p *= 100;
	QString str = QString::number(p, 'f', 2);
	while (str.back() == '0')
		str.chop(1);
	if (str.back() == '.')
		str.chop(1);
	str += "%";
	return str;
}

QString utils::formatTimestamp(uint64_t tm)
{
	return QDateTime::fromSecsSinceEpoch(tm).toString("dd-MMM-yy H:mm");
}

QString utils::formatDuration(uint64_t seconds)
{
	QString timeTxt;
	if (seconds)
	{
		timeTxt = QString::number(seconds % 60) + "s";
		seconds /= 60;
		if (auto minutes = seconds % 60)
			timeTxt = QString::number(minutes) + "m" + timeTxt;
		seconds /= 60;
		if (auto hours = seconds)
			timeTxt = QString::number(hours) + "h" + timeTxt;
	}
	return timeTxt;
}

mttApi::Core* corePtr = nullptr;
TorrentsList* listPtr = nullptr;

mttApi::Core& app::core()
{
	return *corePtr;
}

TorrentsList& app::list()
{
	return *listPtr;
}

QString utils::statusString(const mttApi::TorrentPtr& t)
{
	QString activeStatus;
	auto state = t->getState();

	if (state == mttApi::Torrent::State::CheckingFiles)
	{
		activeStatus = "Checking " + utils::formatPercentage(t->getFiles().checkingProgress());
	}
	else if (state == mttApi::Torrent::State::DownloadingMetadata)
	{
		activeStatus = "Metadata download";
	}
	else if (state == mttApi::Torrent::State::Stopped)
	{
		auto lastErr = t->getLastError();

		if (lastErr != mtt::Status::Success)
		{
			if (lastErr == mtt::Status::E_NotEnoughSpace)
				activeStatus = "Not enough space";
			else if (lastErr == mtt::Status::E_InvalidPath)
				activeStatus = "Invalid path";
			else
				activeStatus = "Problem " + QString::number((int)lastErr);
		}
		else
			activeStatus = "Stopped";
	}
	else if (state == mttApi::Torrent::State::Stopping)
		activeStatus = "Stopping";
	else
		activeStatus = "Active";

	return activeStatus;
}

QString utils::connectionFlagsString(uint32_t flags)
{
	auto connectionInfo = QString(flags & mtt::PeerFlags::Tcp ? "TCP" : "UTP");
	if (flags & mtt::PeerFlags::RemoteConnection)
		connectionInfo += " R";
	if (flags & mtt::PeerFlags::Encrypted)
		connectionInfo += " E";
	if (flags & mtt::PeerFlags::Holepunch)
		connectionInfo += " H";
	return connectionInfo;
}

extern QMap<QString,QVariant> settings;

void app::setSetting(QString name, QVariant value)
{
	settings[name] = value;
}

QVariant app::getSetting(QString name, const QVariant& defaultValue)
{
	return settings.value(name, defaultValue);
}

void guiUtils::CustomTableItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem itemOption(option);
	if (itemOption.state & QStyle::State_HasFocus)
		itemOption.state = itemOption.state ^ QStyle::State_HasFocus;
	QStyledItemDelegate::paint(painter, itemOption, index);
}

void guiUtils::CustomTableItemDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& i) const
{
	index = i;
	QStyledItemDelegate::initStyleOption(option, i);
}

guiUtils::ComboBoxItemDelegate::ComboBoxItemDelegate(QMap<int, QString> o, QObject* parent)
	: QStyledItemDelegate(parent), options(o)
{
}

guiUtils::ComboBoxItemDelegate::~ComboBoxItemDelegate()
{
}

QString guiUtils::ComboBoxItemDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
	return options[value.toInt()];
}

QWidget* guiUtils::ComboBoxItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	auto cb = new QComboBox(parent);
	const int row = index.row();
	for (auto it = options.constKeyValueBegin(); it != options.constKeyValueEnd(); it++)
		cb->addItem(it->second, it->first);

	return cb;
}

void guiUtils::ComboBoxItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	auto cb = qobject_cast<QComboBox*>(editor);
	// get the index of the text in the combobox that matches the current value of the item
	auto cbIndexData = index.data(Qt::EditRole).toInt();
	const int cbIndex = cb->findData(cbIndexData);
	// if it is valid, adjust the combobox
	if (cbIndex >= 0)
		cb->setCurrentIndex(cbIndex);
}

void guiUtils::ComboBoxItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	auto cb = qobject_cast<QComboBox*>(editor);
	model->setData(index, cb->currentText(), Qt::EditRole);
}

QString guiUtils::CenteredMessageBoxStyle()
{
	return "QLabel{min-width:250 px; font-size: 12px;} QPushButton{ font-size: 12px; } QDialogButtonBox { qproperty-centerButtons: true; }";
}
