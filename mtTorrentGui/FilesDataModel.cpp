#include "FilesDataModel.h"
#include "QTimer"
#include "TorrentsList.h"

FilesDataModel::FilesDataModel(QObject* parent /*= nullptr*/)
{
}

QVariant FilesDataModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
	if (!selected)
		return {};

	auto& item = items[index.internalId()];

	if (role == Qt::DisplayRole)
	{
		if (item.fileIdx == -1 && index.column() > (int)Column::Size)
			return {};

		switch ((Column)index.column())
		{
			case Column::Name:
				return item.name;
			case Column::Size:
				return (qulonglong)item.size;
		}

		if (item.fileIdx == -1)
			return {};

		auto& state = filesState[item.fileIdx];

		switch ((Column)index.column())
		{
		case Column::Priority:
			return item.priority;
		case Column::Progress:
			return state.progress;
		case Column::Pieces:
			return (quint64)state.pieces;
		case Column::Remaining:
			return (quint64)state.remaining;
		case Column::Active:
			return (quint64)state.active;
		}
	}
	if ((Column)index.column() == Column::Name)
	{
		if (role == Qt::CheckStateRole)
		{
			return item.check;
		}
		if (role == Qt::DecorationRole)
		{
			QFileIconProvider& icons = utils::getIconProvider();

			if (item.fileIdx == -1)
				return icons.icon(QFileIconProvider::Folder);

			return icons.icon(QFileInfo(item.name));
		}
	}
	if (role == Qt::EditRole && (Column)index.column() == Column::Priority)
	{
		return item.priority;
	}

	return {};
}

int FilesDataModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	return (int)Column::COUNT;
}

int FilesDataModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (parent.column() > 0 || items.empty())
		return 0;

	const TreeItem* parentPtr = nullptr;

	if (!parent.isValid())
		parentPtr = &items[RootItemIdx];
	else
		parentPtr = &items[parent.internalId()];

	return (int)parentPtr->children.size();
}

QModelIndex FilesDataModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (!hasIndex(row, column, parent))
		return {};

	const TreeItem* parentPtr = nullptr;

	if (!parent.isValid())
		parentPtr = &items[RootItemIdx];
 	else
		parentPtr = &items[parent.internalId()];

	if (parentPtr && parentPtr->children.size() > row)
		return createIndex(row, column, parentPtr->children[row]);

	return {};
}

QModelIndex FilesDataModel::parent(const QModelIndex& child) const
{
	if (!child.isValid())
		return {};

	auto& childItem = items[child.internalId()];

	if (childItem.parentIdx == RootItemIdx || childItem.parentIdx == -1)
		return {};

	return createIndex(items[childItem.parentIdx].row, 0, childItem.parentIdx);
}

static const char* columnNames[] = { "Name", "Size", "Priority", "Progress", "Pieces", "Remaining", "Active" };

QVariant FilesDataModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if (orientation == Qt::Horizontal)
	{
		if (role == Qt::DisplayRole)
		{
			return columnNames[section];
		}
	}

	return QAbstractItemModel::headerData(section, orientation, role);
}

void FilesDataModel::updateFilesState()
{
	if (!selected)
		return;

	auto progress = selected->getFiles().getProgress();
	auto active = selected->getFileTransfer().getCurrentRequests();
	auto& files = selected->getFilesInfo();
	filesState.resize(progress.size());

	std::set<int> changed;
	for (size_t i = 0; i < progress.size(); i++)
	{
		auto& file = files[i];

		FileColumnInfo state;
		state.pieces = file.endPieceIndex - file.startPieceIndex + 1;
		state.progress = progress[i].first;
		state.remaining = state.pieces - progress[i].second;
		state.active = 0;
		for (auto a : active)
			if (a >= file.startPieceIndex && a <= file.endPieceIndex)
				state.active++;

		auto& saved = filesState[i];
		if (state.progress != saved.progress || state.active != saved.active || state.remaining != saved.remaining)
		{
			saved = state;
			changed.insert(i);
		}
	}

	if (!changed.empty())
		for (auto& item : items)
		{
			if (item.fileIdx != -1 && changed.find(item.fileIdx) != changed.end())
			{
				QModelIndex bottomRight, topLeft;
				{
					topLeft = createIndex(item.row, (int)Column::Progress, item.idx);
					bottomRight = createIndex(item.row, (int)Column::Active, item.idx);
				}
				emit dataChanged(topLeft, bottomRight, QList<int>({ Qt::DisplayRole }));
			}
		}
}

void FilesDataModel::createTreeItemsHierarchy()
{
	auto selection = selected->getFiles().getSelection();
	auto& files = selected->getFilesInfo();
	items.clear();
	items.reserve(files.size());
	items.push_back({}); //root

	for (int idx = 0; idx < files.size(); idx++)
	{
		auto& file = files[idx];

		auto paths = file.path;
		paths.push_back(file.name);

		auto parentIdx = RootItemIdx;
		for (auto& p : paths)
		{
			bool found = false;
			for (auto childIdx : items[parentIdx].children)
			{
				if (items[childIdx].name == p.data())
				{
					parentIdx = childIdx;
					found = true;
					break;
				}
			}

			if (!found)
			{
				items.push_back({});
				auto newItemIdx = (int)items.size() - 1;
				items[parentIdx].children.push_back(newItemIdx);

				auto& newItem = items.back();
				newItem.name = p.data();
				newItem.parentIdx = parentIdx;
				newItem.row = (int)items[parentIdx].children.size() - 1;
				newItem.idx = newItemIdx;
				parentIdx = newItemIdx;
			}

			items[parentIdx].size += file.size;
		}

		auto& fileItem = items.back();
		fileItem.fileIdx = idx;
		fileItem.check = selection[idx].selected ? Qt::Checked : Qt::Unchecked;
		fileItem.priority = (int)selection[idx].priority;
	}

	for (auto& i : items)
	{
		if (i.fileIdx == -1)
			refreshParentItemCheck(i.idx, false);
	}
}

void FilesDataModel::select(mttApi::TorrentPtr s)
{
	if (!s || !s->hasMetadata())
	{
		beginResetModel();
		s = nullptr;
		items.clear();
		endResetModel();

		filesState.clear();
	}
	else
	{
		beginResetModel();
		selected = s;
		createTreeItemsHierarchy();
		endResetModel();

		updateFilesState();
	}
}

int FilesDataModel::getFileIdx(const QModelIndex& index)
{
	auto idx = index.internalId();
	if (idx <= items.size())
	{
		return items[idx].fileIdx;
	}

	return -1;
}

bool FilesDataModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	auto row = index.row();

	if (role == Qt::CheckStateRole)
	{
		auto& item = items[index.internalId()];
		switchItemCheck(item);
		app::list().refresh(selected);
	}
	else if (role == Qt::EditRole && index.column() == (int)Column::Priority)
	{
		auto& item = items[index.internalId()];
		item.priority = priorityMap.key(value.toString(), mtt::PriorityNormal);
		selected->getFiles().setPriority((uint32_t)item.fileIdx, (mtt::Priority)item.priority);
	}

	return true;
}

Qt::ItemFlags FilesDataModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return {};

	Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (index.column() == (int)Column::Name)
		flags |= Qt::ItemIsUserCheckable;
	if (index.column() == (int)Column::Priority)
		flags |= Qt::ItemIsEditable;

	return flags;
}

void FilesDataModel::switchItemCheck(TreeItem& item)
{
	auto c = (item.check == Qt::Unchecked) ? Qt::Checked : Qt::Unchecked;
	setItemCheck(item, c);
	refreshParentItemCheck(item.parentIdx);

	if (onCheckChange)
		onCheckChange();
}

void FilesDataModel::setItemCheck(TreeItem& item, Qt::CheckState ch)
{
	item.check = ch;
	emitCheckChange(item);

	if (item.fileIdx != -1)
	{
		selected->getFiles().select((uint32_t)item.fileIdx, ch == Qt::Checked);
	}
	else
	{
		for (auto i : item.children)
		{
			setItemCheck(items[i], ch);
		}
	}
}

void FilesDataModel::refreshParentItemCheck(int parentIdx, bool emitChanges)
{
	bool allChecked = true;
	bool allUnchecked = true;

	while (parentIdx != RootItemIdx)
	{
		auto& parent = items[parentIdx];

		for (auto i : parent.children)
		{
			if (items[i].check != Qt::Checked)
				allChecked = false;
			else if (items[i].check != Qt::Unchecked)
				allUnchecked = false;
			
			if (!allChecked && !allUnchecked)
				break;
		}
		if (allChecked)
			parent.check = Qt::Checked;
		else if (allUnchecked)
			parent.check = Qt::Unchecked;
		else
			parent.check = Qt::PartiallyChecked;

		if (emitChanges)
			emitCheckChange(parent);

		parentIdx = parent.parentIdx;
	}
}

void FilesDataModel::emitCheckChange(const TreeItem& item)
{
	QModelIndex bottomRight, topLeft;
	{
		topLeft = createIndex(item.row, 0, item.idx);
		bottomRight = topLeft;
	}
	emit dataChanged(topLeft, bottomRight, QList<int>({ Qt::CheckStateRole }));
}
