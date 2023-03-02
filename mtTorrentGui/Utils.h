#pragma once

#include "QString"
#include "QFileIconProvider"
#include "mtTorrent/Api/Core.h"
#include "QStyledItemDelegate"
#include "QList"

class TorrentsList;

namespace app
{
	mttApi::Core& core();

	TorrentsList& list();

	QVariant getSetting(QString name, const QVariant& defaultValue = QVariant());
	void setSetting(QString name, QVariant value);
}

namespace utils
{
	QString formatBytes(uint64_t bytes);
	QString formatBytesSpeed(uint64_t bytes);
	QString formatPercentage(float p);
	QString formatTimestamp(uint64_t tm);
	QString formatDuration(uint64_t seconds);

	QFileIconProvider& getIconProvider();

	QString statusString(const mttApi::TorrentPtr& t);
	QString connectionFlagsString(uint32_t);
}

namespace guiUtils
{
	QString CenteredMessageBoxStyle();

	class CustomTableItemDelegate : public QStyledItemDelegate
	{
	protected:
		void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
		mutable QModelIndex index;
	};

	class BytesItemDelegate : public QStyledItemDelegate
	{
	public:
		QString displayText(const QVariant& value, const QLocale& locale) const override
		{
			return utils::formatBytes(value.toULongLong());
		}
	};

	class PercentageItemDelegate : public QStyledItemDelegate
	{
	public:
		QString displayText(const QVariant& value, const QLocale& locale) const override
		{
			return utils::formatPercentage(value.toFloat());
		}
	};

	class ComboBoxItemDelegate : public QStyledItemDelegate
	{
		Q_OBJECT
	public:
		ComboBoxItemDelegate(QMap<int,QString> options, QObject* parent = nullptr);
		~ComboBoxItemDelegate();

		QString displayText(const QVariant& value, const QLocale& locale) const override;
		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
		void setEditorData(QWidget* editor, const QModelIndex& index) const override;
		void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
	private:
		QMap<int, QString> options;
	};
}
