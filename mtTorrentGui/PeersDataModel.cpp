#include "PeersDataModel.h"

PeersDataModel::PeersDataModel(QObject* parent /*= nullptr*/)
{
}

int PeersDataModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return (int)peersInfo.size();
}

int PeersDataModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return (int)Column::COUNT;
}

QString PeerStateString(const mtt::PeerState& state)
{
	if (state.action == mtt::PeerState::Disconnected)
		return "Closing";
	if (state.action == mtt::PeerState::Established)
		return state.amInterested ? "Requesting" : "Idle";

	return "Connecting";
}

QVariant PeersDataModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (role == Qt::DisplayRole)
	{
		if (index.row() >= peersInfo.size())
			return {};

		auto& info = peersInfo[index.row()];

		switch ((Column)index.column())
		{
		case Column::Address:
			return info.address.c_str();
		case Column::Download:
			return info.stats.downloadSpeed;
		case Column::Upload:
			return info.stats.uploadSpeed;
		case Column::Progress:
			return info.percentage;
		case Column::Client:
			return info.client.c_str();
		case Column::State:
			return PeerStateString(info.state);
		case Column::Connection:
			return utils::connectionFlagsString(info.flags);
		case Column::Country:
			return info.country.c_str();
		}
	}
	else if (role == Qt::TextAlignmentRole && index.column() != (int)Column::Address)
	{
		return QVariant{ Qt::AlignHCenter | Qt::AlignVCenter };
	}

	return {};
}

static const char* columnNames[] = { "Address", "Download", "Upload", "%", "Client", "State", "Connection", "Country" };

QVariant PeersDataModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
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

void PeersDataModel::update(mttApi::TorrentPtr t)
{
	if (!t)
	{
		if (!peersInfo.empty())
		{
			beginRemoveRows({}, 0, peersInfo.size() - 1);
			peersInfo.clear();
			endRemoveRows();
		}

		return;
	}

	//eval precisely which peer rows add/remove
	//expect added peers always appended

	auto currentPeers = t->getPeers().getConnectedPeersInfo();

	const size_t NoIdx = 9999;
	size_t addedStartIdx = NoIdx;
	std::vector<std::pair<size_t, size_t>> removed;

	auto& oldPeers = peersInfo;
	size_t currentIdxPos = 0;
	for (int i = 0; i < oldPeers.size(); i++)
	{
		if (currentIdxPos == currentPeers.size() || currentPeers[currentIdxPos].address != oldPeers[i].address)
		{
			if (!removed.empty() && removed.back().second + 1 == i)
				removed.back().second = i;
			else
				removed.emplace_back(i, i);
		}
		else
			currentIdxPos++;
	}
	if (currentIdxPos != currentPeers.size())
		addedStartIdx = currentIdxPos;

	if (!removed.empty())
	{
		for (auto it = removed.rbegin(); it != removed.rend(); it++)
		{
			beginRemoveRows({}, it->first, it->second);
			peersInfo.resize(peersInfo.size() - (it->second - it->first + 1));//rowCount
			endRemoveRows();
		}
	}
	if (addedStartIdx != NoIdx)
	{
		beginInsertRows({}, addedStartIdx, currentPeers.size() - 1);
		peersInfo.resize(currentPeers.size());//rowCount
		endInsertRows();
	}

	peersInfo = std::move(currentPeers);

	if (!peersInfo.empty())
	{
		QModelIndex bottomRight, topLeft;
		{
			topLeft = createIndex(0, (int)Column::Download);
			bottomRight = createIndex(peersInfo.size() - 1, (int)Column::Connection);
		}
		emit dataChanged(topLeft, bottomRight, QList<int>({ Qt::DisplayRole }));
	}
}
