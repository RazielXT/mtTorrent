#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"

class TorrentsDataModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit TorrentsDataModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	void update();

	enum class Column
	{
		Name,
		State,
		Progress,
		Download,
		Upload,
		Peers,
		Downloaded,
		Uploaded,
		Added,

		COUNT
	};

	std::vector<mttApi::TorrentPtr> torrents;
	std::vector<mttApi::Torrent::TimePoint> timestamps;

	mttApi::TorrentPtr forceRefresh;

	void add(mttApi::TorrentPtr);
	void remove(mttApi::TorrentPtr);
};
