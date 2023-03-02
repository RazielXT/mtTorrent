#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"

class PeersDataModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit PeersDataModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	void update(mttApi::TorrentPtr);

	enum class Column
	{
		Address,
		Download,
		Upload,
		Progress,
		Client,
		State,
		Connection,
		Country,

		COUNT
	};

private:

	std::vector<mtt::ConnectedPeerInfo> peersInfo;
};
