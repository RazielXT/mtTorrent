#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"

class FilesDataModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit FilesDataModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;

	void select(mttApi::TorrentPtr);
	void updateFilesState();
	int getFileIdx(const QModelIndex& index);

	enum class Column
	{
		Name,
		Size,
		Priority,
		Progress,
		Pieces,
		Remaining,
		Active,

		COUNT
	};

	std::function<void()> onCheckChange;

	const QMap<int, QString> priorityMap = { {mtt::PriorityLow, "Low"}, {mtt::PriorityNormal, "Normal"}, {mtt::PriorityHigh, "High"} };

private:

	struct FileColumnInfo
	{
		size_t pieces = 0;
		float progress = 0;
		size_t remaining = 0;
		size_t active = 0;
	};

	std::vector<FileColumnInfo> filesState;

	mttApi::TorrentPtr selected;

	class TreeItem
	{
	public:
		std::vector<int> children;
		int parentIdx = -1;
		int idx = 0;
		int row = 0;

		int fileIdx = -1;
		QString name;
		uint64_t size = 0;
		Qt::CheckState check = Qt::CheckState::Checked;
		int priority = 1;
	};

	std::vector<TreeItem> items;
	const int RootItemIdx = 0;

	void createTreeItemsHierarchy();
	void switchItemCheck(TreeItem&);
	void setItemCheck(TreeItem&, Qt::CheckState);
	void refreshParentItemCheck(int, bool emitChanges = true);

	void emitCheckChange(const TreeItem& item);
};
