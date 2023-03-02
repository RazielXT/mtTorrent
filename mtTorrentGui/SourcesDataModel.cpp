#include "SourcesDataModel.h"

SourcesDataModel::SourcesDataModel(QObject* parent /*= nullptr*/)
{
}

int SourcesDataModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return (int)sourcesInfo.size();
}

int SourcesDataModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return (int)Column::COUNT;
}

static const char* getTrackerStateName(mtt::TrackerState s)
{
	if (s == mtt::TrackerState::Connected || s == mtt::TrackerState::Alive)
		return "Ready";
	else if (s == mtt::TrackerState::Connecting)
		return "Connecting";
	else if (s == mtt::TrackerState::Announcing)
		return "Announcing";
	else if (s == mtt::TrackerState::Announced)
		return "Announced";
	else if (s == mtt::TrackerState::Offline)
		return "Offline";
	else
		return "Stopped";
}

QVariant SourcesDataModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (role == Qt::DisplayRole)
	{
		if (index.row() >= sourcesInfo.size())
			return {};

		auto& info = sourcesInfo[index.row()];

		switch ((Column)index.column())
		{
		case Column::Name:
			return info.hostname.c_str();
		case Column::State:
			return getTrackerStateName(info.state);
		case Column::Peers:
			return info.peers;
		case Column::Seeds:
			return info.seeds;
		case Column::Leeches:
			return info.leeches;
		case Column::NextCheck:
			return (qulonglong)info.nextAnnounce;
		case Column::Interval:
			return info.announceInterval;
		}
	}
	else if (role == Qt::TextAlignmentRole && index.column() != (int)Column::Name)
	{
		return QVariant{ Qt::AlignHCenter | Qt::AlignVCenter };
	}

	return {};
}

static const char* columnNames[] = { "Name", "State", "Peers", "Seeds", "Leeches", "NextCheck", "Interval" };

QVariant SourcesDataModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
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

void SourcesDataModel::update(mttApi::TorrentPtr t)
{
	if (!t)
	{
		if (!sourcesInfo.empty())
		{
			beginRemoveRows({}, 0, sourcesInfo.size() - 1);
			sourcesInfo.clear();
			sourcesSize = 0;
			endResetModel();
		}
		return;
	}

	sourcesInfo = t->getPeers().getSourcesInfo();
	if (sourcesInfo.size() != sourcesSize)
	{
		beginResetModel();
		sourcesSize = sourcesInfo.size();
		endResetModel();
	}

	if (!sourcesInfo.empty())
	{
		//convert duration just once for all
		{
			auto currentTime = (mtt::Timestamp)::time(0);
			for (auto& info : sourcesInfo)
				info.nextAnnounce = info.nextAnnounce < currentTime ? 0 : info.nextAnnounce - currentTime;
		}

		QModelIndex bottomRight, topLeft;
		{
			topLeft = createIndex(0, (int)Column::State);
			bottomRight = createIndex(sourcesInfo.size() - 1, (int)Column::Interval);
		}
		emit dataChanged(topLeft, bottomRight, QList<int>({ Qt::DisplayRole }));
	}
}

std::string SourcesDataModel::sourceRefreshName(int idx)
{
	if (idx < sourcesInfo.size() && sourcesInfo[idx].nextAnnounce != 0)
		return sourcesInfo[idx].hostname;
	return {};
}
