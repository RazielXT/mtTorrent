#include <QItemDelegate>
#include "QApplication"
#include "QDateTime"

// TorrentViewDelegate is used to draw the progress bars.
class TorrentViewDelegate : public QItemDelegate
{
	Q_OBJECT

		mutable QTime timer;
public:
	inline TorrentViewDelegate(QWidget* parent) : QItemDelegate(parent) { timer.start(); }

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		const QModelIndex &index) const override
	{
		if (index.column() != 2) {
			QItemDelegate::paint(painter, option, index);
			return;
		}

		// Set up a QStyleOptionProgressBar to precisely mimic the
		// environment of a progress bar.
		QStyleOptionProgressBar progressBarOption;
		progressBarOption.state = QStyle::State_Enabled;
		progressBarOption.direction = QApplication::layoutDirection();
		progressBarOption.rect = option.rect;
		progressBarOption.fontMetrics = QApplication::fontMetrics();
		progressBarOption.minimum = 0;
		progressBarOption.maximum = 100;
		progressBarOption.textAlignment = Qt::AlignCenter;
		progressBarOption.textVisible = true;

		// Set the progress and text values of the style option.
		int progress = timer.restart();
		progressBarOption.progress = progress < 0 ? 0 : progress;
		progressBarOption.text = QString::asprintf("%d%%", progressBarOption.progress);

		// Draw the progress bar onto the view.
		QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
	}
};

//#include "TorrentViewDelegate.moc"