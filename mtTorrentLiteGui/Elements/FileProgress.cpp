#include "FileProgress.h"
#include "MainForm.h"
#include "../AppCore.h"
#include "../Utils/Utils.h"

FileProgress::FileProgress(AppCore& c) : core(c)
{

}

void FileProgress::update()
{
	refreshing = true;
	selectionChanged = false;
	updatePiecesChart();
	updateFilesProgress();
	refreshing = false;
}

void FileProgress::fileSelectionChanged(int rowIndex)
{
	if (refreshing)
		return;

	auto row = GuiLite::MainForm::instance->filesProgressGridView->Rows[rowIndex];

	mtBI::TorrentFileSelectionRequest request;
	request.index = (uint32_t)System::Convert::ToInt32(row->Cells[0]->Value->ToString());
	request.selected = System::Convert::ToBoolean(row->Cells[1]->Value->ToString());
	memcpy(request.hash, core.firstSelectedHash, 20);

	core.IoctlFunc(mtBI::MessageId::SetTorrentFileSelection, &request, nullptr);
}

void FileProgress::initPiecesChart()
{
	if (lastBitfield.size() != progress.bitfield.size() || core.selectionChanged)
	{
		System::GC::Collect();
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

		selectionChanged = true;
	}
}

uint32_t FileProgress::getProgressChartIndex(uint32_t pieceIdx)
{
	if (ProgressChartIntervalSize < MaxProgressChartIntervalSize)
		return pieceIdx;

	float coef = ProgressChartIntervalSize / (float)progress.piecesCount;

	return (uint32_t)(pieceIdx * coef);
}

void FileProgress::updatePiecesChart()
{
	auto chart = GuiLite::MainForm::instance->pieceChart;
	progress.bitfield.clear(progress.bitfield.size());
	activePieces.clear();

	if (core.IoctlFunc(mtBI::MessageId::GetPiecesInfo, &core.firstSelectedHash, &progress) == mtt::Status::Success && !progress.bitfield.empty())
	{
		initPiecesChart();

		chart->Titles[0]->Text = int(progress.receivedCount).ToString() + "/" + int(progress.piecesCount).ToString();

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
		for (size_t i = 0; i < progress.requests.size(); i++)
		{
			chart->Series["Request"]->Points->AddXY(getProgressChartIndex(progress.requests[i]), 0);
		}

		activePieces.assign(progress.requests.data(), progress.requests.data() + progress.requests.size());

		std::swap(progress.bitfield, lastBitfield);
	}
	else
		chart->Visible = false;
}

void FileProgress::updateFilesProgress()
{
	auto filesGrid = GuiLite::MainForm::instance->filesProgressGridView;

	if (selectionChanged)
	{
		mtBI::TorrentInfo info;
		if (core.IoctlFunc(mtBI::MessageId::GetTorrentInfo, &core.firstSelectedHash, &info) == mtt::Status::Success)
		{
			adjustGridRowsCount(filesGrid, (int)info.files.size());

			for (uint32_t i = 0; i < info.files.size(); i++)
			{
				auto& file = info.files[i];
				auto row = gcnew cli::array< System::String^ >(9) {
					int(i).ToString(), file.selected ? "true" : "false",
						gcnew System::String(file.name.data, 0, (int)file.name.length, System::Text::Encoding::UTF8),
						"", formatBytes(file.size), int(file.size).ToString(), "", "", ""
				};

				filesGrid->Rows[i]->SetValues(row);
			}

			//filesGrid->Sort(filesGrid->Columns[1], System::ComponentModel::ListSortDirection::Ascending);
			filesGrid->ClearSelection();
		}
	}

	mtBI::TorrentFilesProgress progressInfo;
	if (core.IoctlFunc(mtBI::MessageId::GetTorrentFilesProgress, &core.firstSelectedHash, &progressInfo) == mtt::Status::Success)
	{
		if (selectionChanged)
		{
			fileFinishedPieces.clear();
			fileFinishedPieces.resize(progressInfo.files.size(), -1);
		}

		for (int i = 0; i < filesGrid->Rows->Count; i++)
		{
			int idx = System::Convert::ToInt32(filesGrid->Rows[i]->Cells[0]->Value->ToString());

			if (idx < (int)progressInfo.files.size())
			{
				const auto& file = progressInfo.files[idx];
				uint32_t piecesCount = 1 + file.pieceEnd - file.pieceStart;

				//percentage column
				filesGrid->Rows[i]->Cells[3]->Value = float(file.progress).ToString("P");

				//pieces size column
				if (selectionChanged)
					filesGrid->Rows[i]->Cells[6]->Value = int(piecesCount).ToString();

				//remaining pieces column
				if (fileFinishedPieces[idx] != file.receivedPieces)
				{
					fileFinishedPieces[idx] = file.receivedPieces;
					filesGrid->Rows[i]->Cells[7]->Value = int(piecesCount - file.receivedPieces).ToString();
				}

				//active pieces column
				size_t active = 0;
				for (auto pIdx : activePieces)
				{
					if (pIdx >= file.pieceStart && pIdx <= file.pieceEnd)
						active++;
				}
				filesGrid->Rows[i]->Cells[8]->Value = int(active).ToString();
			}
		}

		if (filesGrid->SelectedRows->Count > 0)
		{
			int idx = System::Convert::ToInt32(filesGrid->SelectedRows[0]->Cells[0]->Value->ToString());
			if (!selectionChanged)
			{
				auto chart = GuiLite::MainForm::instance->pieceChart;
				chart->Series["Request"]->Points->Clear();

				for (uint32_t i = progressInfo.files[idx].pieceStart; i <= progressInfo.files[idx].pieceEnd; i++)
				{
					chart->Series["Request"]->Points->AddXY(getProgressChartIndex(i), 0);
				}

				filesGrid->ClearSelection();
			}
		}
	}
}
