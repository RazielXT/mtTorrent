#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"

class SourcesDataModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit SourcesDataModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	void update(mttApi::TorrentPtr);

	enum class Column
	{
		Name,
		State,
		Peers,
		Seeds,
		Leeches,
		NextCheck,
		Interval,

		COUNT
	};

	std::string sourceRefreshName(int idx);

private:

	std::vector<mtt::TrackerInfo> sourcesInfo;
	size_t sourcesSize = 0;
};
