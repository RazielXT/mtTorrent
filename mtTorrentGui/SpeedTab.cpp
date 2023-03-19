#include "SpeedTab.h"

void SpeedTab::init(Ui_mainWindowWidget& ui)
{
	chartView = ui.chartView;

	seriesDownload = new QLineSeries();
	seriesDownload->setName("Download");

	seriesUpload = new QLineSeries();
	seriesUpload->setName("Upload");

	QChart* chart = new QChart();
	chart->layout()->setContentsMargins(0, 0, 0, 0);
	chart->setBackgroundRoundness(0);
	chart->addSeries(seriesDownload);
	chart->addSeries(seriesUpload);

	axisX = new QValueAxis;
	axisY = new QValueAxis;
	{
		axisX->setLabelFormat("%ds");
		axisX->setTitleText("Time");
		chart->addAxis(axisX, Qt::AlignBottom);
		seriesDownload->attachAxis(axisX);
		seriesUpload->attachAxis(axisX);

		axisY->setLabelFormat("%.2f MB/s");
		axisY->setTitleText("Speed");
		chart->addAxis(axisY, Qt::AlignLeft);
		seriesDownload->attachAxis(axisY);
		seriesUpload->attachAxis(axisY);
	}

	chart->legend()->setVisible(true);
	chart->legend()->setAlignment(Qt::AlignTop);
	chart->legend()->setInteractive(true);

	chartView->setChart(chart);
	chartView->setRenderHint(QPainter::Antialiasing);

	chartView->hide();
}

void SpeedTab::selectTorrent(mttApi::TorrentPtr t)
{
	if (selected != t)
	{
		selected = t;
		clear();
	}

	if (t)
		chartView->show();
	else
		chartView->hide();
}

void SpeedTab::update()
{
	if (!selected)
		return;

	if (!selected->started())
	{
		clear();
		return;
	}

	qreal xValue = seriesDownload->count();

	qreal dlSpeed = selected->getFileTransfer().downloadSpeed() / (1024.f * 1024);
	qreal upSpeed = selected->getFileTransfer().uploadSpeed() / (1024.f * 1024);

	*seriesDownload << QPointF{ xValue, dlSpeed };
	*seriesUpload << QPointF{ xValue, upSpeed };

	maxX = std::max(maxX, xValue);
	axisX->setRange(0, maxX);
	maxY = std::max({ maxY,dlSpeed, upSpeed });
	axisY->setRange(0, maxY);
}

void SpeedTab::clear()
{
	if (maxX)
	{
		seriesDownload->clear();
		seriesUpload->clear();

		axisX->setRange(0, 0);
		axisY->setRange(0, 0);

		maxX = 0;
		maxY = 1 / 1024.f; //start without 0 height
	}
}
