#include "PiecesProgress.h"
#include "MainForm.h"
#include "../AppCore.h"
#include "../Utils/Utils.h"

PiecesProgress::PiecesProgress(AppCore& c) : core(c)
{

}

void PiecesProgress::update()
{
	updateFilesProgress();
	updatePiecesChart();
}

void PiecesProgress::initPiecesChart()
{
	if (lastBitfield.size() != progress.bitfield.size() || core.selectionChanged)
	{
		lastBitfield.clear(progress.bitfield.size());

		auto chart = GuiLite::MainForm::instance->pieceChart;
		chart->Visible = true;

		chart->Series["HasSeries"]->Points->Clear();
		chart->Series["Request"]->Points->Clear();
		ProgressChartIntervalSize = min(progress.piecesCount, MaxProgressChartIntervalSize);
		chart->ChartAreas[0]->AxisX->Interval = ProgressChartIntervalSize;

		for (uint32_t i = 0; i < ProgressChartIntervalSize; i++)
		{
			chart->Series["HasSeries"]->Points->AddXY(i, 0);
			chart->Series["HasSeries"]->Points[i]->IsEmpty = true;
		}
	}
}

uint32_t PiecesProgress::getProgressChartIndex(uint32_t pieceIdx)
{
	if (ProgressChartIntervalSize < MaxProgressChartIntervalSize)
		return pieceIdx;

	float coef = ProgressChartIntervalSize / (float)progress.piecesCount;

	return (uint32_t)(pieceIdx * coef);
}

void PiecesProgress::highlightChartPieces(uint32_t from, uint32_t to)
{
	piecesHighlight = { from, to };
}

void PiecesProgress::updatePiecesChart()
{
	auto chart = GuiLite::MainForm::instance->pieceChart;
	progress.bitfield.clear(progress.bitfield.size());

	if (core.IoctlFunc(mtBI::MessageId::GetPiecesInfo, &core.firstSelectedHash, &progress) == mtt::Status::Success && !progress.bitfield.empty())
	{
		initPiecesChart();

		chart->Titles[0]->Text = "Pieces: " + int(progress.receivedCount).ToString() + "/" + int(progress.piecesCount).ToString();

		for (uint32_t i = 0; i < progress.piecesCount; i++)
		{
			size_t idx = static_cast<size_t>(i / 8.0f);
			unsigned char bitmask = 128 >> i % 8;

			bool value = (progress.bitfield[idx] & bitmask) != 0;
			bool lastValue = (lastBitfield[idx] & bitmask) != 0;

			if (value && !lastValue)
			{
				chart->Series["HasSeries"]->Points[getProgressChartIndex(i)]->IsEmpty = false;
			}
		}

		chart->Series["Request"]->Points->Clear();

		if (piecesHighlight.from == -1)
		{
			for (size_t i = 0; i < progress.requests.size(); i++)
			{
				chart->Series["Request"]->Points->AddXY(getProgressChartIndex(progress.requests[i]), 0);
			}
		}
		else
		{
			for (uint32_t i = piecesHighlight.from; i < piecesHighlight.to; i++)
			{
				chart->Series["Request"]->Points->AddXY(getProgressChartIndex(i), 0);
			}
			piecesHighlight.from = -1;
		}

		std::swap(progress.bitfield, lastBitfield);
	}
	else
		chart->Visible = false;
}

void PiecesProgress::updateFilesProgress()
{
	auto filesGrid = GuiLite::MainForm::instance->filesProgressGridView;

	if (core.selectionChanged)
	{
		mtBI::TorrentInfo info;
		if (core.IoctlFunc(mtBI::MessageId::GetTorrentInfo, &core.firstSelectedHash, &info) == mtt::Status::Success)
		{
			adjustGridRowsCount(filesGrid, (int)info.files.size());

			for (uint32_t i = 0; i < info.files.size(); i++)
			{
				auto& file = info.files[i];
				auto row = gcnew cli::array< System::String^ >(5) {
					int(i).ToString(),
						gcnew System::String(file.name.data, 0, (int)file.name.length, System::Text::Encoding::UTF8),
						"", formatBytes(file.size), int(file.size).ToString()
				};

				filesGrid->Rows[i]->SetValues(row);
			}

			filesGrid->Sort(filesGrid->Columns[1], System::ComponentModel::ListSortDirection::Ascending);
			filesGrid->ClearSelection();
		}
	}

	mtBI::TorrentFilesProgress progressInfo;
	if (core.IoctlFunc(mtBI::MessageId::GetTorrentFilesProgress, &core.firstSelectedHash, &progressInfo) == mtt::Status::Success)
	{
		for (int i = 0; i < filesGrid->Rows->Count; i++)
		{
			int idx = System::Convert::ToInt32(filesGrid->Rows[i]->Cells[0]->Value->ToString());

			if (idx < (int)progressInfo.files.size())
				filesGrid->Rows[i]->Cells[2]->Value = float(progressInfo.files[idx].progress).ToString("P");
		}

		if (filesGrid->SelectedRows->Count > 0)
		{
			int idx = System::Convert::ToInt32(filesGrid->SelectedRows[0]->Cells[0]->Value->ToString());
			if (!core.selectionChanged)
				highlightChartPieces(progressInfo.files[idx].pieceStart, progressInfo.files[idx].pieceEnd);
			filesGrid->ClearSelection();
		}
	}
}
