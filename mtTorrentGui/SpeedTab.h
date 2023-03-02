#pragma once

#include "ui_MainWindow.h"
#include "Utils.h"
#include <QtCharts/QtCharts>

class SpeedTab
{
public:
	SpeedTab() = default;

	void init(Ui_mainWindowWidget&);
	void selectTorrent(mttApi::TorrentPtr t);
	void update();

private:
	mttApi::TorrentPtr selected;

	QLineSeries* seriesUpload;
	QLineSeries* seriesDownload;
	QValueAxis* axisX;
	QValueAxis* axisY;

	qreal maxX = 0;
	qreal maxY = 0;

	QWidget* speedTab;
	QChartView* chartView;
	void clear();
};
