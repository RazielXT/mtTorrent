#include "TorrentsDataModel.h"

TorrentsDataModel::TorrentsDataModel(QObject* parent) : QAbstractTableModel(parent)
{
	torrents = app::core().getTorrents();
	timestamps.resize(torrents.size());
}

int TorrentsDataModel::rowCount(const QModelIndex& /*parent*/) const
{
	return (int)torrents.size();
}

int TorrentsDataModel::columnCount(const QModelIndex& /*parent*/) const
{
	return (int)Column::COUNT;
}

QVariant TorrentsDataModel::data(const QModelIndex& index, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (index.row() >= torrents.size())
			return {};

		auto& t = torrents[index.row()];

		switch ((Column)index.column())
		{
		case Column::Name:
			return t->name().c_str();
		case Column::State:
			return utils::statusString(t);
		case Column::Progress:
			return t->selectionProgress();
		case Column::Download:
			return t->getFileTransfer().downloadSpeed();
		case Column::Upload:
			return t->getFileTransfer().uploadSpeed();
		case Column::Peers:
			return t->getPeers().connectedCount();
		case Column::Downloaded:
			return (qulonglong)t->finishedBytes();
		case Column::Uploaded:
			return (qulonglong)t->getFileTransfer().uploaded();
		case Column::Added:
			return (qulonglong)t->getTimeAdded();
		}
	}
	else if (role == Qt::TextAlignmentRole && index.column() != (int)Column::Name)
	{
		return QVariant{ Qt::AlignHCenter | Qt::AlignVCenter };
	}
	return {};
}

static const char* columnNames[] = { "Name", "State", "Progress", "Download", "Upload", "Peers", "Downloaded", "Uploaded", "Added on" };

Q_INVOKABLE QVariant TorrentsDataModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			return columnNames[section];
		}
		if (role == Qt::TextAlignmentRole)
		{
			return QVariant{ Qt::AlignHCenter | Qt::AlignVCenter };
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void TorrentsDataModel::update()
{
	int rowsMinMax[2] = {-1,-1};
	bool stateChanged = false;

	for (size_t i = 0; i < torrents.size(); i++)
	{
		auto& t = torrents[i];

		auto state = t->getStateTimestamp();
		bool tStateChange = timestamps[i] != state || t->getState() == mttApi::Torrent::State::CheckingFiles;

		if (tStateChange || t->started() || t == forceRefresh)
		{
			timestamps[i] = state;
			stateChanged |= tStateChange;

			if (rowsMinMax[0] == -1)
				rowsMinMax[0] = (int)i;
			rowsMinMax[1] = (int)i;
		}
	}

	forceRefresh = nullptr;

	if (rowsMinMax[1] != -1)
	{
		QModelIndex editIndex = createIndex(rowsMinMax[0], stateChanged ? 0 : (int)Column::Progress);
		QModelIndex editIndexEnd = createIndex(rowsMinMax[1], (int)Column::COUNT - 1);
		emit dataChanged(editIndex, editIndexEnd, { Qt::DisplayRole });
	}
}

void TorrentsDataModel::add(mttApi::TorrentPtr t)
{
	beginInsertRows({}, torrents.size(), torrents.size());
	torrents.push_back(t);
	timestamps.emplace_back();
	endInsertRows();
}

void TorrentsDataModel::remove(mttApi::TorrentPtr t)
{
	size_t i = 0;
	for (auto it = torrents.begin(); it != torrents.end(); it++)
	{
		if (*it == t)
		{
			beginRemoveRows({}, i, i);
			torrents.erase(it);
			timestamps.erase(timestamps.begin() + i);
			endRemoveRows();

			break;
		}
		i++;
	}
}
