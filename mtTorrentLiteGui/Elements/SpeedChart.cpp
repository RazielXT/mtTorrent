#include "SpeedChart.h"
#include "MainForm.h"
#include <vector>

using namespace System;
using namespace System::Windows::Forms;

SpeedChart::SpeedChart()
{

}

void SpeedChart::resetChart()
{
	auto chart = GuiLite::MainForm::instance->dlSpeedChart;
	chart->Visible = true;
	chart->Series["DlSeries"]->XValueMember = "Time";
	chart->Series["DlSeries"]->YValueMembers = "Speed";
	chart->Series["DlSeries"]->ChartType = DataVisualization::Charting::SeriesChartType::Line;
	//chart->Series["DlSeries"]->IsVisibleInLegend = false;
	chart->Series["DlSeries"]->Points->Clear();
	chart->Series["UpSeries"]->XValueMember = "Time";
	chart->Series["UpSeries"]->YValueMembers = "Speed";
	chart->Series["UpSeries"]->ChartType = DataVisualization::Charting::SeriesChartType::Line;
	//chart->Series["UpSeries"]->IsVisibleInLegend = false;
	chart->Series["UpSeries"]->Points->Clear();
	chart->ChartAreas[0]->AxisX->MajorGrid->Enabled = false;
	chart->ChartAreas[0]->AxisY->MajorGrid->LineColor = Drawing::Color::LightGray;
	chartTime = 0;
}

void SpeedChart::update(float dlSpeed, float upSpeed)
{
	const int MaxChartPointsPrecision = 1000;

	int currentPointsCount = GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points->Count;
	if (currentPointsCount >= MaxChartPointsPrecision)
	{
		float dlspeedTemp = 0, upspeedTemp = 0, timeTemp = 0;
		struct ChartSpeedVals
		{
			float time;
			float dlSpeed;
			float upspeed;
		};
		std::vector<ChartSpeedVals> speeds;
		speeds.reserve(MaxChartPointsPrecision / 2);

		for (int i = 0; i < currentPointsCount; i++)
		{
			if (i % 2 == 0)
			{
				dlspeedTemp = (float)GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points[i]->YValues[0];
				upspeedTemp = (float)GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points[i]->YValues[0];
				timeTemp = (float)GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points[i]->XValue;
			}
			else
			{
				dlspeedTemp = (dlspeedTemp + (float)GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points[i]->YValues[0]) / 2;
				upspeedTemp = (upspeedTemp + (float)GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points[i]->YValues[0]) / 2;

				speeds.push_back({ timeTemp, dlspeedTemp, upspeedTemp });
			}
		}

		GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points->Clear();
		GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points->Clear();

		for (auto& speed : speeds)
		{
			GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points->AddXY(speed.time, speed.dlSpeed);
			GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points->AddXY(speed.time, speed.upspeed);
		}
	}

	GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points->AddXY(++chartTime, dlSpeed);
	GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points->AddXY(chartTime, upSpeed);
}
