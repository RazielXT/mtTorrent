#include "MainForm.h"
#include "MagnetInputForm.h"
#include "../mtTorrent/Public/BinaryInterface.h"
#include "../mtTorrent/Public/Status.h"
#include "../mtTorrent/utils/HexEncoding.h"
#include <msclr/marshal_cppstd.h>
#include <windows.h>
#include <vcclr.h>
#include "SettingsForm.h"
#include <WinUser.h>
#include "FileSelectionForm.h"
#include "ScheduleForm.h"
#include "AddPeerForm.h"

#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

using namespace System;
using namespace System::Windows::Forms;

typedef mtt::Status(*IOCTL_FUNC)(mtBI::MessageId, const void*,void*);
IOCTL_FUNC IoctlFunc = nullptr;
HMODULE lib = nullptr;

bool initialized = true;
bool selected = false;
uint8_t firstSelectedHash[20];
bool selectionChanged = false;
bool forceRefresh = true;

const char* getStringPtr(System::String^ str)
{
	return (const char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str).ToPointer();
}

const wchar_t* getWStringPtr(System::String^ str)
{
	return (const wchar_t*)System::Runtime::InteropServices::Marshal::StringToHGlobalUni(str).ToPointer();
}

void applySettings(GuiLite::SettingsForm^ form)
{
	mtBI::SettingsInfo info;
	info.dhtEnabled = form->checkBoxDht->Checked;
	info.directory = getStringPtr(form->directoryTextBox->Text);
	info.maxConnections = (unsigned int)form->maxConnectionsNumeric->Value;
	info.udpPort = (unsigned int)form->udpPortNumeric->Value;
	info.tcpPort = (unsigned int)form->tcpPortNumeric->Value;
	info.upnpEnabled = form->upnpMapCheckBox->Checked;

	IoctlFunc(mtBI::MessageId::SetSettings, &info, nullptr);
}

System::String^ getUpnpInfo()
{
	mtt::string info;

	IoctlFunc(mtBI::MessageId::GetUpnpInfo, nullptr, &info);

	return gcnew String(info.data ? info.data : "");
}

int chartTime = 0;
void updateSpeedChart(float dlSpeed, float upSpeed);
void initSpeedChart()
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

void updateSpeedChart(float dlSpeed, float upSpeed)
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
		speeds.reserve(MaxChartPointsPrecision/2);

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
				dlspeedTemp = (dlspeedTemp + (float)GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points[i]->YValues[0])/2;
				upspeedTemp = (upspeedTemp + (float)GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points[i]->YValues[0])/2;

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

TorrentCtxMenuInfo getTorrentContexMenuInfo()
{
	TorrentCtxMenuInfo info;

	mtBI::TorrentStateInfo state;
	info.active = IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, firstSelectedHash, &state) == mtt::Status::Success && state.started;

	mtBI::MagnetLinkProgressLogsRequest logsReq;
	logsReq.start = -1;
	memcpy(logsReq.hash, firstSelectedHash, 20);
	mtBI::MagnetLinkProgressLogsResponse logsResp{};
	info.utmLogs = IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, &logsReq, &logsResp) == mtt::Status::E_NoData && logsResp.fullcount > 0;

	mtBI::MagnetLinkProgress magnetProgress{};
	info.noInfo = IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, firstSelectedHash, &magnetProgress) == mtt::Status::Success && !magnetProgress.finished;

	return info;
}

mtBI::PiecesInfo progress;
mtt::array<uint8_t> lastBitfield;

void initPiecesChart()
{
	if (lastBitfield.size() != progress.bitfield.size() || selectionChanged)
	{
		lastBitfield.clear(progress.bitfield.size());

		auto chart = GuiLite::MainForm::instance->pieceChart;
		chart->Visible = true;

		chart->Series["HasSeries"]->Points->Clear();
		chart->Series["Request"]->Points->Clear();
		chart->ChartAreas[0]->AxisX->Interval = progress.piecesCount;
	}
}

void updatePiecesChart()
{
	auto chart = GuiLite::MainForm::instance->pieceChart;
	progress.bitfield.clear(progress.bitfield.size());

	if (IoctlFunc(mtBI::MessageId::GetPiecesInfo, &firstSelectedHash, &progress) == mtt::Status::Success && !progress.bitfield.empty())
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
				chart->Series["HasSeries"]->Points->AddXY(i, 0);
			}
		}

		chart->Series["Request"]->Points->Clear();

		for (size_t i = 0; i < progress.requests.size(); i++)
		{
			chart->Series["Request"]->Points->AddXY(progress.requests[i], 0);
		}

		std::swap(progress.bitfield, lastBitfield);
	}
	else
		chart->Visible = false;
}

struct
{
	mtBI::TorrentInfo info;
	unsigned char hash[20];
	bool added;
	bool priorityChanged = false;
}
fileSelection;

String^ formatBytes(size_t bytes)
{
	const char* type = " MB";

	auto sz = ((bytes / 1024.f) / 1024.f);
	if (sz < 1)
	{
		sz = (float)(bytes / (1024ll));
		type = " KB";

		if (sz == 0)
		{
			sz = (float)bytes;
			type = " B";
		}
	}
	else if (sz > 1024)
	{
		sz /= 1024.f;
		type = " GB";
	}

	auto str = gcnew String(sz.ToString("F"));
	str = str->TrimEnd('0')->TrimEnd('.', ',');

	return str + gcnew String(type);
}

String^ formatBytesSpeed(size_t bytes)
{
	return formatBytes(bytes) + "/s";
}

void updateSelectionFormFooter()
{
	size_t selectedSize = 0;
	size_t fullsize = 0;
	uint32_t selectedCount = 0;

	for (auto& f : fileSelection.info.files)
	{
		if (f.selected)
		{
			selectedCount++;
			selectedSize += f.size;
		}

		fullsize += f.size;
	}

	String^ txt = gcnew String("Selected ");
	txt += int(selectedCount).ToString();
	txt += "/";
	txt += int(fileSelection.info.files.size()).ToString();
	txt += " (";
	txt += formatBytes(selectedSize);
	txt += "/";
	txt += formatBytes(fullsize);
	txt += ")";

	GuiLite::FileSelectionForm::instance->infoLabel->Text = txt;
}

void fileSelectionChanged(int id, bool selected)
{
	if (id >= fileSelection.info.files.size())
		return;

	fileSelection.info.files[id].selected = selected;

	updateSelectionFormFooter();
}

void filePriorityChanged(const std::vector<int>& index, System::String^ value)
{
	uint8_t priority = 50;
	if (value == "Low")
		priority = 10;
	else if (value == "High")
		priority = 90;

	for (auto i : index)
	{
		fileSelection.info.files[i].priority = priority;
	}
	fileSelection.priorityChanged = true;
}

void setAllFileSelection(bool selected)
{
	GuiLite::FileSelectionForm::instance->initialized = false;
	auto rows = GuiLite::FileSelectionForm::instance->filesGridView->Rows;
	int i = 0;

	for (auto& s : fileSelection.info.files)
	{
		s.selected = selected;
		rows[i++]->Cells[1]->Value = selected;
	}
	GuiLite::FileSelectionForm::instance->initialized = true;
	updateSelectionFormFooter();
}

String^ formatPriorityToString(uint8_t p)
{
	if (p == 50)
		return "Normal";
	else if(p > 50)
		return "High";
	else //if (p < 50)
		return "Low";
}

void fillFilesSelectionForm()
{
	auto form = GuiLite::FileSelectionForm::instance;
	auto& info = fileSelection.info;

	if (IoctlFunc(mtBI::MessageId::GetTorrentInfo, fileSelection.hash, &info) != mtt::Status::Success || info.files.empty())
		return;

	auto list = form->filesGridView;
	list->Rows->Add((int)info.files.size());

	int i = 0;
	for (auto& f : info.files)
	{
		auto row = gcnew cli::array<Object^>(5) {
			int(i).ToString(),
			f.selected,
			gcnew String(f.name.data, 0, (int)f.name.length, System::Text::Encoding::UTF8),
			formatBytes(f.size),
			formatPriorityToString(f.priority)
		};

		list->Rows[i]->SetValues(row);
		i++;
	}

	list->Sort(list->Columns[2], System::ComponentModel::ListSortDirection::Ascending);
	updateSelectionFormFooter();

	form->Text = gcnew System::String(info.name.data);
	form->textBoxPath->Text = gcnew System::String(info.downloadLocation.data);

	form->labelError->Visible = false;
	form->checkBoxStart->Visible = fileSelection.added;
	fileSelection.priorityChanged = false;
}

void refreshTorrentInfo(uint8_t* hash)
{
	if (!IoctlFunc)
		return;

	mtBI::TorrentInfo info;
	IoctlFunc(mtBI::MessageId::GetTorrentInfo, hash, &info);

	GuiLite::MainForm::instance->torrentInfoLabel->Clear();
	auto infoLines = GuiLite::MainForm::instance->torrentInfoLabel;

	infoLines->AppendText(gcnew String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8));
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText("Total size: ");
	infoLines->AppendText(formatBytes(info.fullsize));
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText("Save in: \t");
	info.downloadLocation.append(info.name);
	infoLines->AppendText(gcnew String(info.downloadLocation.data, 0, (int)info.downloadLocation.length, System::Text::Encoding::UTF8));
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText("Hash: \t");
	auto hashStr = hexToString(hash, 20);
	for (int i = 0; i < 4; i++)
		hashStr.insert(8 + i * 8, 1, ' ');
	infoLines->AppendText(gcnew String(hashStr.data()));

	String^ creationStr = "";
	if (info.creationDate != 0)
	{
		creationStr = DateTimeOffset::FromUnixTimeSeconds(info.creationDate).ToString("MM/dd/yyyy");
		creationStr += " ";
	}

	if (info.createdBy.length != 0)
	{
		creationStr += "by ";
		creationStr += gcnew String(info.createdBy.data);
	}

	if (creationStr->Length > 0)
	{
		infoLines->AppendText(Environment::NewLine);
		infoLines->AppendText(Environment::NewLine);
		infoLines->AppendText("Created: \t");
		infoLines->AppendText(creationStr);
	}

	initSpeedChart();
}

void init()
{
	wchar_t pathBuffer[256];
	if (GetModuleFileName(NULL, pathBuffer, 256))
	{
		PathRemoveFileSpec(pathBuffer);
		SetCurrentDirectory(pathBuffer);
	}

	lib = LoadLibrary(L"mtTorrent.dll");

	if (lib)
	{
		IoctlFunc = (IOCTL_FUNC)GetProcAddress(lib, "Ioctl");
		IoctlFunc(mtBI::MessageId::Init, nullptr, nullptr);
		mtBI::RegisterAlertsRequest alertsRequest{ (uint32_t)mtt::AlertCategory::Torrent | (uint32_t)mtt::AlertCategory::Metadata };
		IoctlFunc(mtBI::MessageId::RegisterAlerts, &alertsRequest, nullptr);
	}
}

System::String^ getTorrentName(uint8_t* hash)
{
	mtBI::TorrentStateInfo info;

	if (selected && IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, hash, &info) == mtt::Status::Success)
		return gcnew System::String(info.name.data);
	else
		return "";
}

System::String^ getSelectedTorrentName()
{
	return getTorrentName(firstSelectedHash);
}

void handleTorrentAddResponse(mtt::Status status, uint8_t* hash)
{
	if (status == mtt::Status::I_AlreadyExists || status == mtt::Status::I_Merged)
	{
		auto name = getTorrentName(hash);

		if (status == mtt::Status::I_AlreadyExists)
			::MessageBox(NULL, L"Torrent already exists", getWStringPtr(name), MB_OK);
		else if (status == mtt::Status::I_Merged)
			::MessageBox(NULL, L"Torrent already exists, new trackers added", getWStringPtr(name), MB_OK);
	}
}

mtt::Status addTorrentFromFile(const char* filepath)
{
	uint8_t hash[20];
	auto status = IoctlFunc(mtBI::MessageId::AddFromFile, filepath, hash);
	handleTorrentAddResponse(status, hash);

	return status;
}

void addTorrent()
{
	OpenFileDialog^ openFileDialog = gcnew OpenFileDialog;
	openFileDialog->Filter = "Torrent files (*.torrent)|*.torrent|All files (*.*)|*.*";

	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		auto filenamePtr = getStringPtr(openFileDialog->FileName);
	
		addTorrentFromFile(filenamePtr);
	}
}

struct
{
	int magnetLinkSequence = 0;
	uint32_t lastMagnetLinkLogCount = 0;
	uint8_t hash[20];
}
magnet;

mtt::Status addTorrentFromMetadata(const char* magnetPtr)
{
	auto status = IoctlFunc(mtBI::MessageId::AddFromMetadata, magnetPtr, magnet.hash);
	handleTorrentAddResponse(status, magnet.hash);

	return status;
}

void setSelected(bool v)
{
	selected = v;
	GuiLite::MainForm::instance->buttonStart->Enabled = selected;

	if (!selected)
	{
		GuiLite::MainForm::instance->getGrid()->ClearSelection();

		GuiLite::MainForm::instance->torrentInfoLabel->Clear();
		GuiLite::MainForm::instance->dlSpeedChart->Visible = false;
	}
}

struct SelectedTorrent
{
	uint8_t hash[20];
};
std::vector<SelectedTorrent> getFullSelection()
{
	std::vector<SelectedTorrent> out;

	for (int i = 0; i < GuiLite::MainForm::instance->getGrid()->SelectedRows->Count; i++)
	{
		auto idStr = (String^)GuiLite::MainForm::instance->getGrid()->SelectedRows[i]->Cells[0]->Value;
		auto stdStr = msclr::interop::marshal_as<std::string>(idStr);
		SelectedTorrent t;
		decodeHexa(stdStr, t.hash);

		out.push_back(t);
	}
	
	return out;
}

void refreshSelection()
{
	if (GuiLite::MainForm::instance->getGrid()->SelectedRows->Count == 0)
	{
		setSelected(false);
	}
	else
	{
		setSelected(true);

		auto idStr = (String^)GuiLite::MainForm::instance->getGrid()->SelectedRows[0]->Cells[0]->Value;
		auto stdStr = msclr::interop::marshal_as<std::string>(idStr);
		uint8_t selectedHash[20];
		decodeHexa(stdStr, selectedHash);

		if (memcmp(firstSelectedHash, selectedHash, 20) != 0)
		{
			memcpy(firstSelectedHash, selectedHash, 20);
			selectionChanged = true;
		}
	}
}

void showFilesSelectionFormThread()
{
	GuiLite::FileSelectionForm form;
	form.ShowDialog();
	fileSelection.info.files.clear();
	GuiLite::FileSelectionForm::instance = nullptr;
}

void showFilesSelectionForm(const uint8_t* hash, bool added)
{
	if (GuiLite::FileSelectionForm::instance)
		return;

	memcpy(fileSelection.hash, hash, 20);
	fileSelection.added = added;

	System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&showFilesSelectionFormThread));
	newThread->SetApartmentState(System::Threading::ApartmentState::STA);
	newThread->Start();
}

void showSettingsFormThread()
{
	GuiLite::SettingsForm form;
	mtBI::SettingsInfo info;
	if (IoctlFunc(mtBI::MessageId::GetSettings, nullptr, &info) == mtt::Status::Success)
	{
		form.checkBoxDht->Checked = info.dhtEnabled;
		form.directoryTextBox->Text = gcnew String(info.directory.data);
		form.maxConnectionsNumeric->Value = info.maxConnections;
		form.udpPortNumeric->Value = info.udpPort;
		form.tcpPortNumeric->Value = info.tcpPort;
		form.upnpMapCheckBox->Checked = info.upnpEnabled;
	}

	form.ShowDialog();
}

struct ScheduledTorrent
{
	uint8_t hash[20];
	bool start = true;
	int countdown = 0;
};
std::vector<ScheduledTorrent> scheduledTorrents;

void ScheduleTorrentUpdateFunc(System::Object^ myObject, System::EventArgs^ myEventArgs)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		it->countdown--;

		if (it->countdown <= 0)
		{
			IoctlFunc(mtBI::MessageId::Start, it->hash, nullptr);
			scheduledTorrents.erase(it);
			break;
		}
	}

	if (scheduledTorrents.empty())
		GuiLite::MainForm::instance->scheduleTimer->Stop();
}

void scheduleTorrent(int seconds)
{
	ScheduledTorrent t;
	t.countdown = seconds;
	memcpy(t.hash, firstSelectedHash, 20);

	scheduledTorrents.push_back(t);

	if (!GuiLite::MainForm::instance->scheduleTimer)
	{
		GuiLite::MainForm::instance->scheduleTimer = gcnew System::Windows::Forms::Timer();
		GuiLite::MainForm::instance->scheduleTimer->Tick += gcnew EventHandler(ScheduleTorrentUpdateFunc);
		GuiLite::MainForm::instance->scheduleTimer->Interval = 1000;
	}

	GuiLite::MainForm::instance->scheduleTimer->Start();
}

int getSchedule(uint8_t* hash)
{
	for (auto& t : scheduledTorrents)
	{
		if (memcmp(t.hash, hash, 20) == 0)
			return t.countdown;
	}

	return 0;
}

void stopSchedule(uint8_t* hash)
{
	for (auto it = scheduledTorrents.begin(); it != scheduledTorrents.end(); it++)
	{
		if (memcmp(it->hash, hash, 20) == 0)
		{
			scheduledTorrents.erase(it);
			break;
		}
	}
}

void onButtonClick(ButtonId id, System::String^ param)
{
	if (id == ButtonId::AddPeerMenu)
	{
		if (!selected)
			return;

		GuiLite::AddPeerForm form;
		form.ShowDialog();
	}
	else if (id == ButtonId::AddPeer)
	{
		mtBI::AddPeerRequest request;
		memcpy(request.hash, firstSelectedHash, 20);
		request.addr = getStringPtr(param);

		IoctlFunc(mtBI::MessageId::AddPeer, &request, nullptr);
	}
	else if (id == ButtonId::OpenLocation)
	{
		mtBI::TorrentInfo info;
		if (IoctlFunc(mtBI::MessageId::GetTorrentInfo, firstSelectedHash, &info) == mtt::Status::Success && info.downloadLocation.length > 0)
		{
			auto path = gcnew String(info.downloadLocation.data);
			if (info.files.size() > 1)
				path += gcnew String(info.name.data);
			System::Diagnostics::Process::Start(path);
		}
	}
	else if (id == ButtonId::CheckFiles)
	{
		IoctlFunc(mtBI::MessageId::CheckFiles, &firstSelectedHash, nullptr);
	}
	else if (id == ButtonId::Schedule)
	{
		mtTorrentLiteGui::ScheduleForm form;
		form.ShowDialog();
	}
	else if (id == ButtonId::MagnetLogs)
	{
		if (!selected)
			return;

		mtBI::MagnetLinkProgress magnetProgress;
		memcpy(magnet.hash, firstSelectedHash, 20);
		if (!GuiLite::MagnetInputForm::instance && IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, magnet.hash, &magnetProgress) == mtt::Status::Success)
		{
			GuiLite::MagnetInputForm form;
			form.labelText->Text = "Getting info...";
			form.magnetFormButton->Text = "Logs";
			form.magnetFormButton->Enabled = false;
			form.logsTextBox->Visible = true;
			form.textBoxMagnet->Text = gcnew String(hexToString(magnet.hash, 20).data());
			form.textBoxMagnet->Enabled = true;
			magnet.magnetLinkSequence = 3;
			form.ShowDialog();
			GuiLite::MagnetInputForm::instance = nullptr;
			magnet.magnetLinkSequence = 0;
			magnet.lastMagnetLinkLogCount = 0;
		}
	}
	else if (id == ButtonId::Remove)
	{
		if (!selected)
			return;

		mtBI::RemoveTorrentRequest request;

		auto result = ::MessageBox(NULL, L"Delete files?", L"Torrent remove", MB_YESNOCANCEL);

		if (result == IDYES)
		{
			request.deleteFiles = true;
		}
		else if (result == IDNO)
		{
			request.deleteFiles = false;
		}
		else if (result == IDCANCEL)
		{
			return;
		}

		auto selection = getFullSelection();

		for (auto s : selection)
		{
			memcpy(request.hash, s.hash, 20);
			IoctlFunc(mtBI::MessageId::Remove, &request, nullptr);
		}

		forceRefresh = true;
	}
	else if (id == ButtonId::AddTorrentFile)
	{
		System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&addTorrent));
		newThread->SetApartmentState(System::Threading::ApartmentState::STA);
		newThread->Start();
	}
	else if (id == ButtonId::TorrentGrid)
	{
		refreshSelection();
	}
	else if (id == ButtonId::Start)
	{
		if (selected)
		{
			auto selection = getFullSelection();

			for (auto s : selection)
			{
				if (IoctlFunc(mtBI::MessageId::Start, s.hash, nullptr) == mtt::Status::Success)
					forceRefresh = true;

				stopSchedule(s.hash);
			}
		}
	}
	else if (id == ButtonId::Stop)
	{
		if (selected)
		{
			auto selection = getFullSelection();

			for (auto s : selection)
			{
				if (IoctlFunc(mtBI::MessageId::Stop, s.hash, nullptr) == mtt::Status::Success)
					forceRefresh = true;

				stopSchedule(s.hash);
			}
		}
	}
	else if (id == ButtonId::AddTorrentMagnet)
	{
		magnet.magnetLinkSequence = 1;
		GuiLite::MagnetInputForm form;
		form.ShowDialog();
		GuiLite::MagnetInputForm::instance = nullptr;
		magnet.magnetLinkSequence = 0;
		magnet.lastMagnetLinkLogCount = 0;
	}
	else if (id == ButtonId::Settings)
	{
		System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&showSettingsFormThread));
		newThread->SetApartmentState(System::Threading::ApartmentState::STA);
		newThread->Start();
	}
	else if (id == ButtonId::SourceRefresh)
	{
		if (param)
		{
			mtBI::SourceId info;
			info.name = getStringPtr(param);
			memcpy(info.hash, firstSelectedHash, 20);
			IoctlFunc(mtBI::MessageId::RefreshSource, &info, nullptr);
		}
	}
	else if (id == ButtonId::SelectFiles)
	{
		showFilesSelectionForm(firstSelectedHash, false);
	}
	else if (GuiLite::MagnetInputForm::instance)
	{
		if (id == ButtonId::MagnetButton)
		{
			if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Add"))
			{
				if (GuiLite::MagnetInputForm::instance->textBoxMagnet->Text->Length == 0)
					return;

				if (magnet.magnetLinkSequence > 1)
					return;

				auto magnetPtr = getStringPtr(GuiLite::MagnetInputForm::instance->textBoxMagnet->Text);
				auto status = addTorrentFromMetadata(magnetPtr);

				if (status == mtt::Status::Success)
				{
					GuiLite::MagnetInputForm::instance->textBoxMagnet->ReadOnly = true;
					GuiLite::MagnetInputForm::instance->labelText->Text = "Getting info...";
					magnet.magnetLinkSequence = 2;

					GuiLite::MagnetInputForm::instance->magnetFormButton->Text = "Logs";
				}
				else if (status == mtt::Status::I_AlreadyExists || status == mtt::Status::I_Merged)
				{
					GuiLite::MagnetInputForm::instance->Close();
				}
				else
					GuiLite::MagnetInputForm::instance->labelText->Text = "Invalid magnet link";
			}
			else if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Logs"))
			{
				GuiLite::MagnetInputForm::instance->magnetFormButton->Enabled = false;
				GuiLite::MagnetInputForm::instance->logsTextBox->Visible = true;
				magnet.magnetLinkSequence = 3;
			}
		}
	}
	else if (GuiLite::FileSelectionForm::instance)
	{
		if (id == ButtonId::SelectionOk)
		{
			auto form = GuiLite::FileSelectionForm::instance;

			if (form->textBoxPath->Text->Length > 0)
			{
				mtBI::TorrentSetPathRequest path;
				memcpy(path.hash, fileSelection.hash, 20);
				path.path = getStringPtr(form->textBoxPath->Text);
				if (IoctlFunc(mtBI::MessageId::SetTorrentPath, &path, nullptr) != mtt::Status::Success)
				{
					form->labelError->Text = "Error setting location path";
					form->labelError->Visible = true;
					return;
				}
			}

			if (!fileSelection.info.files.empty())
			{
				mtBI::TorrentFilesSelectionRequest selection;
				memcpy(selection.hash, fileSelection.hash, 20);
				selection.selection.reserve(fileSelection.info.files.size());

				for (auto& s : fileSelection.info.files)
				{
					mtBI::FileSelectionRequest f;
					f.selected = s.selected;
					selection.selection.add(f);
				}

				IoctlFunc(mtBI::MessageId::SetTorrentFilesSelection, &selection, nullptr);
			}

			if (fileSelection.priorityChanged)
			{
				mtBI::TorrentFilesPriorityRequest request;
				memcpy(request.hash, fileSelection.hash, 20);
				request.priority.reserve(fileSelection.info.files.size());

				for (auto& f : fileSelection.info.files)
				{
					request.priority.add(f.priority);
				}

				IoctlFunc(mtBI::MessageId::SetTorrentFilesPriority, &request, nullptr);
			}

			if (form->checkBoxStart->Visible && form->checkBoxStart->Checked)
				IoctlFunc(mtBI::MessageId::Start, fileSelection.hash, nullptr);

			GuiLite::FileSelectionForm::instance->Close();
		}
		else if (id == ButtonId::SelectionCancel)
		{
			GuiLite::FileSelectionForm::instance->Close();
		}
		else if (id == ButtonId::SelectionAll)
		{
			setAllFileSelection(true);
		}
		else if (id == ButtonId::SelectionNone)
		{
			setAllFileSelection(false);
		}
	}
}

void onButtonClick(ButtonId id)
{
	onButtonClick(id, nullptr);
}

void adjustGridRowsCount(System::Windows::Forms::DataGridView^ grid, int count)
{
	auto currentCount = grid->Rows->Count;
	if (currentCount > count)
	{
		if (count == 0)
			grid->Rows->Clear();
		else
			for (; currentCount > count; currentCount--)
			{
				grid->Rows->RemoveAt(currentCount - 1);
			}
	}
	else if (currentCount < count)
	{
		for (; currentCount < count; currentCount++)
		{
			grid->Rows->Add();
		}
	}
}

void checkAlerts()
{
	mtBI::AlertsList alertsRequest;
	if (IoctlFunc(mtBI::MessageId::PopAlerts, nullptr, &alertsRequest) == mtt::Status::Success && !alertsRequest.alerts.empty())
	{
		for (auto& alert : alertsRequest.alerts)
		{
			if (alert.id == mtt::AlertId::TorrentAdded)
			{
				showFilesSelectionForm(alert.hash, true);
			}
			else if (alert.id == mtt::AlertId::MetadataFinished)
			{
				if (memcmp(firstSelectedHash, alert.hash, 20) == 0)
					forceRefresh = true;
			}
		}
	}
}

bool lastInfoIncomplete = false;
int RefreshTimeCounter = 10;
void refreshUi()
{
	if (!IoctlFunc || !initialized)
		return;

	if (GuiLite::MagnetInputForm::instance)
	{
		if (magnet.magnetLinkSequence >= 2)
		{
			if (magnet.magnetLinkSequence == 3)
			{
					mtBI::MagnetLinkProgressLogsRequest logsRequest;
					logsRequest.start = magnet.lastMagnetLinkLogCount;
					memcpy(logsRequest.hash, magnet.hash, 20);
					mtBI::MagnetLinkProgressLogsResponse logsResponse;

					if (IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, &logsRequest, &logsResponse) == mtt::Status::Success)
					{
						for (auto& l : logsResponse.logs)
						{
							GuiLite::MagnetInputForm::instance->logsTextBox->Text += gcnew String(l.data);
							GuiLite::MagnetInputForm::instance->logsTextBox->Text += "\n";
						}

						magnet.lastMagnetLinkLogCount = (uint32_t)logsResponse.fullcount;
					}
			}

			mtBI::MagnetLinkProgress progress;
			auto status = IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, magnet.hash, &progress);
			if (status == mtt::Status::Success)
			{
				if (progress.progress > 1.0f)
					progress.progress = 1.0f;

				GuiLite::MagnetInputForm::instance->progressBarMagnet->Value = (int)(progress.progress * 100);

				if (progress.finished)
				{
					GuiLite::MagnetInputForm::instance->labelText->Text = "Finished";
		
					if(magnet.magnetLinkSequence == 2)
						GuiLite::MagnetInputForm::instance->Close();
					else
						magnet.magnetLinkSequence = 4;
				}
			}
			else
				GuiLite::MagnetInputForm::instance->labelText->Text = status == mtt::Status::E_NoData ? "No magnet data" : "Error";
		}
	}

	RefreshTimeCounter++;
	if (RefreshTimeCounter < 10 && !selectionChanged && !forceRefresh)
		return;

	RefreshTimeCounter = 0;

	refreshSelection();
	checkAlerts();

	mtBI::TorrentsList torrents;
	if (IoctlFunc(mtBI::MessageId::GetTorrents, nullptr, &torrents) != mtt::Status::Success)
		return;

	mtBI::TorrentStateInfo info;
	info.connectedPeers = 0;
	{
		bool selectionActive = false;
		bool selectionStopped = false;

		auto torrentGrid = GuiLite::MainForm::instance->getGrid();
		adjustGridRowsCount(torrentGrid, (int)torrents.list.size());

		for (size_t i = 0; i < torrents.list.size(); i++)
		{
			auto& t = torrents.list[i];

			if (IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, t.hash, &info) == mtt::Status::Success)
			{
				bool isSelected = (memcmp(t.hash, firstSelectedHash, 20) == 0);

				String^ activeStatus;
				if (info.activeStatus != mtt::Status::Success)
				{
					if (info.activeStatus == mtt::Status::E_NotEnoughSpace)
						activeStatus = "Not enough space";
					else
						activeStatus = "Problem " + int(info.activeStatus).ToString();
				}
				else if (!t.active)
				{
					int schedule = getSchedule(t.hash);

					if(schedule <= 0)
						activeStatus = "Stopped";
					else
					{
						activeStatus = "Scheduled (";
						TimeSpan time = System::TimeSpan::FromSeconds((double)schedule);
						activeStatus += time.ToString("d\\d\\ hh\\hmm\\mss\\s")->TrimStart(' ', 'd', 'h', 'm', 's', '0');
						activeStatus += ")";
					}
				}
				else
					activeStatus = "Active";

				if (isSelected)
				{
					if(t.active)
						updateSpeedChart((info.downloadSpeed / 1024.f) / 1024.f, (info.uploadSpeed / 1024.f) / 1024.f);
				}

				String^ speedInfo = "";
				if (t.active && info.downloadSpeed > 0 && info.selectionProgress && info.selectionProgress < 1.0f && info.downloaded > 0)
				{
					speedInfo = formatBytesSpeed(info.downloadSpeed);
					speedInfo += " (";
					size_t leftBytes = ((size_t)(info.downloaded / info.selectionProgress)) - info.downloaded;
					size_t leftSeconds = leftBytes / info.downloadSpeed;
					TimeSpan time = System::TimeSpan::FromSeconds((double)leftSeconds);
					speedInfo += time.ToString("d\\d\\ hh\\hmm\\mss\\s")->TrimStart(' ', 'd', 'h', 'm', 's', '0');
					speedInfo += ")";
				}

				String^ name = gcnew String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8);
				if (info.utmActive)
				{
					mtBI::MagnetLinkProgress magnetProgress;
					if (IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, t.hash, &magnetProgress) == mtt::Status::Success)
					{
						if (magnetProgress.progress > 1.0f)
							magnetProgress.progress = 1.0f;

						name = "Metadata download " + float(magnetProgress.progress).ToString("P");
					}
				}

				String^ progress;
				if (info.checking)
					progress = "Checking " + float(info.checkingProgress).ToString("P");
				else
				{
					progress = float(info.selectionProgress).ToString("P");

					if(info.progress != info.selectionProgress)
						progress += " (" + float(info.progress).ToString("P") + ")";
				}

				auto row = gcnew cli::array< System::String^  >(12) {
					gcnew String(hexToString(t.hash, 20).data()),
						name, progress, activeStatus, speedInfo,
						int(info.downloadSpeed).ToString(), t.active ? formatBytesSpeed(info.uploadSpeed) : "", int(info.uploadSpeed).ToString(),
						(t.active || info.connectedPeers) ? int(info.connectedPeers).ToString() : "",
						(t.active || info.foundPeers) ? int(info.foundPeers).ToString() : "",
						formatBytes(info.downloaded), formatBytes(info.uploaded)
				};

				torrentGrid->Rows[i]->SetValues(row);

				if(isSelected)
				{
					if (lastInfoIncomplete && !info.utmActive)
						selectionChanged = true;

					lastInfoIncomplete = info.utmActive;
				}

				if (torrentGrid->Rows[i]->Selected)
				{
					if (t.active)
						selectionActive = true;
					else
						selectionStopped = true;
				}

				if (torrentGrid->SortedColumn)
					torrentGrid->Sort(torrentGrid->SortedColumn, torrentGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
			}
		}

		GuiLite::MainForm::instance->buttonStart->Enabled = selectionStopped;
		GuiLite::MainForm::instance->buttonStop->Enabled = selectionActive;
		GuiLite::MainForm::instance->buttonRemove->Enabled = selected;
	}

	if (forceRefresh)
		refreshSelection();

	if (selectionChanged)
		refreshTorrentInfo(firstSelectedHash);

	auto activeTab = GuiLite::MainForm::instance->getActiveTab();

	if(selectionChanged || activeTab == GuiLite::MainForm::TabType::Peers)
	{
		mtBI::TorrentPeersInfo peersInfo;
		IoctlFunc(mtBI::MessageId::GetPeersInfo, firstSelectedHash, &peersInfo);

		auto peersGrid = GuiLite::MainForm::instance->getPeersGrid();
		String^ selectedPeer;
		if (peersGrid->SelectedRows->Count > 0)
			selectedPeer = (String^)peersGrid->SelectedRows[0]->Cells[0]->Value;
		adjustGridRowsCount(peersGrid, (int)peersInfo.peers.size());

		for (uint32_t i = 0; i < peersInfo.peers.size(); i++)
		{
			auto& peerInfo = peersInfo.peers[i];
			auto peerRow = gcnew cli::array< System::String^  >(8) {
				gcnew String(peerInfo.addr.data),
					formatBytesSpeed(peerInfo.dlSpeed), int(peerInfo.dlSpeed).ToString(), formatBytesSpeed(peerInfo.upSpeed), int(peerInfo.upSpeed).ToString(),
					float(peerInfo.progress).ToString("P"),	gcnew String(peerInfo.client.data, 0, (int)peerInfo.client.length, System::Text::Encoding::UTF8),
					gcnew String(peerInfo.country.data)
			};

			peersGrid->Rows[i]->SetValues(peerRow);
		}

		if (peersGrid->SortedColumn)
			peersGrid->Sort(peersGrid->SortedColumn, peersGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
	
		if (peersGrid->RowCount > 0 && selectedPeer)
		{
			peersGrid->ClearSelection();
			for (uint32_t i = 0; i < peersInfo.peers.size(); i++)
			{
				if (selectedPeer == (String^)peersGrid->Rows[i]->Cells[0]->Value)
					peersGrid->Rows[i]->Selected = true;
			}
		}
	}

	if (selectionChanged || activeTab == GuiLite::MainForm::TabType::Sources)
	{
		mtBI::SourcesInfo sourcesInfo;
		IoctlFunc(mtBI::MessageId::GetSourcesInfo, firstSelectedHash, &sourcesInfo);

		auto sourcesGrid = GuiLite::MainForm::instance->sourcesGrid;
		adjustGridRowsCount(sourcesGrid, (int)sourcesInfo.sources.size());

		for (size_t i = 0; i < sourcesInfo.sources.size(); i++)
		{
			auto& source = sourcesInfo.sources[i];
			auto nextCheck = (source.nextCheck == 0) ? gcnew String("") : TimeSpan::FromSeconds(source.nextCheck).ToString("T");
			char status[11];

			if(source.status == mtBI::SourceInfo::Ready)
				memcpy(status, "Ready", 6);
			else if (source.status == mtBI::SourceInfo::Connecting)
				memcpy(status, "Connecting", 11);
			else if (source.status == mtBI::SourceInfo::Announcing)
				memcpy(status, "Announcing", 11);
			else if (source.status == mtBI::SourceInfo::Announced)
				memcpy(status, "Announced", 10);
			else if (source.status == mtBI::SourceInfo::Offline)
				memcpy(status, "Offline", 8);
			else
				memcpy(status, "Stopped", 8);

			auto sourceRow = gcnew cli::array< System::String^  >(7) {
				gcnew String(source.name.data), gcnew String(status),
					int(source.peers).ToString(), int(source.seeds).ToString(), int(source.leechers).ToString(), nextCheck, int(source.interval).ToString()
			};

			if (source.status == mtBI::SourceInfo::Stopped)
			{
				sourceRow[2] = "";
				sourceRow[3] = "";
				sourceRow[4] = "";
				sourceRow[6] = "";
			}
			else if (source.status != mtBI::SourceInfo::Announced)
			{
				if(source.peers == 0)
					sourceRow[2] = "";

				sourceRow[3] = "";
				sourceRow[4] = "";
			}

			sourcesGrid->Rows[i]->SetValues(sourceRow);
		}

		if (sourcesGrid->SortedColumn)
			sourcesGrid->Sort(sourcesGrid->SortedColumn, sourcesGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
	}

	if(selectionChanged || activeTab == GuiLite::MainForm::TabType::Progress)
		updatePiecesChart();

	selectionChanged = false;
	forceRefresh = false;
}

void ProcessProgramArgument(System::String^ arg)
{
	auto inputPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(arg).ToPointer();
	if (addTorrentFromMetadata(inputPtr) != mtt::Status::E_InvalidInput)
		return;

	if (addTorrentFromFile(inputPtr) != mtt::Status::E_InvalidInput)
		return;

	std::wstring str = L"Couldnt process input: ";
	str += (wchar_t*)System::Runtime::InteropServices::Marshal::StringToHGlobalUni(arg).ToPointer();

	::MessageBox(NULL, str.data(), L"Invalid input", MB_OK);
}

void deinit()
{
	initialized = false;

	if (IoctlFunc)
		IoctlFunc(mtBI::MessageId::Deinit, nullptr, nullptr);

	if (lib)
		FreeLibrary(lib);
}

const int WM_MTT_ARGUMENT = WM_USER + 100;
const int MTT_ARGUMENT_START = 1;
const int MTT_ARGUMENT_END = 2;
const int MTT_SHOW_WINDOW = 3;
std::string argumentBuffer;

public ref class MyMessageFilter : System::Windows::Forms::IMessageFilter
{
	public: 
	
		MyMessageFilter() { }

		virtual bool PreFilterMessage(Message% objMessage) 
		{
			if (objMessage.Msg == WM_MTT_ARGUMENT)
			{
				if (objMessage.LParam == (IntPtr)MTT_SHOW_WINDOW)
				{
					GuiLite::MainForm::instance->Show();
					GuiLite::MainForm::instance->WindowState = FormWindowState::Normal;
				}
				else if (objMessage.LParam == (IntPtr)MTT_ARGUMENT_END)
					ProcessProgramArgument(gcnew System::String(argumentBuffer.data()));
				else
				{
					if (objMessage.LParam == (IntPtr)MTT_ARGUMENT_START)
						argumentBuffer.clear();

					argumentBuffer.push_back((char)objMessage.WParam.ToInt32());
				}

				return true;
			}

			return false;
		}
};

[STAThread]
void FormsMain(cli::array<System::String ^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	init();
	GuiLite::MainForm form;

	if (args->Length > 0)
		ProcessProgramArgument(args[0]);

	MyMessageFilter^ filter = gcnew MyMessageFilter();
	Application::AddMessageFilter(filter);

	Application::Run(%form);

	deinit();
}

HWND siblingWindowId;
DWORD siblingProcessId;
static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
	int length = GetWindowTextLength(hWnd);
	wchar_t* buffer = new wchar_t[length + 1];
	GetWindowText(hWnd, buffer, length + 1);

	if (length != 0 && !IsWindowVisible(hWnd) && wcsncmp(buffer, L"mtTorrent", length) == 0)
	{
		DWORD processId = 0;
		GetWindowThreadProcessId(hWnd, &processId);

		if(processId == siblingProcessId)
			siblingWindowId = hWnd;
	}
	return TRUE;
}

HWND getHiddenWindowHandle(int parentProcessID)
{
	siblingProcessId = (DWORD)parentProcessID;
	siblingWindowId = 0;
	EnumWindows(enumWindowCallback, NULL);
	return siblingWindowId;
}

#pragma comment(lib, "user32.lib")
HWND GetExistingMainWindow()
{
	auto p = System::Diagnostics::Process::GetCurrentProcess();
	auto location = System::Reflection::Assembly::GetExecutingAssembly()->Location;

	auto processes = System::Diagnostics::Process::GetProcessesByName(p->ProcessName);
	for (int i = 0; i < processes->Length; i++)
	{
		if (processes[i]->Id != p->Id && processes[i]->MainModule->FileName == location)
		{
			HWND window = (HWND)processes[i]->MainWindowHandle.ToInt64();

			if (!window)
				window = getHiddenWindowHandle(processes[i]->Id);

			return window;
		}
	}

	return 0;
}

void OnDuplicate(HWND hMainWindow, cli::array<System::String ^>^ args)
{
	if (args->Length > 0)
	{
		auto arg = args[0];

		for (int i = 0; i < arg->Length; i++)
		{
			PostMessage(hMainWindow, WM_MTT_ARGUMENT, arg[i], i == 0 ? MTT_ARGUMENT_START : 0);
		}

		PostMessage(hMainWindow, WM_MTT_ARGUMENT, 0, MTT_ARGUMENT_END);
	}

	PostMessage(hMainWindow, WM_MTT_ARGUMENT, 0, MTT_SHOW_WINDOW);
}

int Main(cli::array<System::String ^>^ args)
{
	if (HWND h = GetExistingMainWindow())
		OnDuplicate(h, args);
	else
		FormsMain(args);

	return 0;
}