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
#include "AddPeerForm.h"

#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

using namespace System;
using namespace System::Windows::Forms;

typedef mtt::Status(*IOCTL_FUNC)(mtBI::MessageId, const void*,void*);
IOCTL_FUNC IoctlFunc = nullptr;
HMODULE lib = nullptr;


bool selected = false;
uint8_t hash[20];
bool selectionChanged = false;

const char* getStringPtr(System::String^ str)
{
	return (const char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(str).ToPointer();
}

void applySettings(GuiLite::SettingsForm^ form)
{
	mtBI::SettingsInfo info;
	info.dhtEnabled = form->checkBoxDht->Checked;
	info.directory = getStringPtr(form->directoryTextBox->Text);
	info.maxConnections = (unsigned int)form->maxConnectionsNumeric->Value;
	info.udpPort = (unsigned int)form->udpPortNumeric->Value;
	info.tcpPort = (unsigned int)form->tcpPortNumeric->Value;

	IoctlFunc(mtBI::MessageId::SetSettings, &info, nullptr);
}

int chartTime = 0;

void initSpeedChart()
{
	auto chart = GuiLite::MainForm::instance->dlSpeedChart;
	chart->Series["DlSeries"]->XValueMember = "Time";
	chart->Series["DlSeries"]->YValueMembers = "Speed";
	chart->Series["DlSeries"]->ChartType = DataVisualization::Charting::SeriesChartType::Line;
	chart->Series["DlSeries"]->IsVisibleInLegend = false;
	chart->Series["DlSeries"]->Points->Clear();
	chart->Series["UpSeries"]->XValueMember = "Time";
	chart->Series["UpSeries"]->YValueMembers = "Speed";
	chart->Series["UpSeries"]->ChartType = DataVisualization::Charting::SeriesChartType::Line;
	chart->Series["UpSeries"]->IsVisibleInLegend = false;
	chart->Series["UpSeries"]->Points->Clear();
	chart->ChartAreas[0]->AxisX->MajorGrid->Enabled = false;
	chart->ChartAreas[0]->AxisY->MajorGrid->LineColor = Drawing::Color::LightGray;
	chartTime = 0;
}

void updateSpeedChart(float dlSpeed, float upSpeed)
{
	GuiLite::MainForm::instance->dlSpeedChart->Series["DlSeries"]->Points->AddXY(++chartTime, dlSpeed);
	GuiLite::MainForm::instance->dlSpeedChart->Series["UpSeries"]->Points->AddXY(chartTime, upSpeed);
}

mtBI::TorrentFilesSelection lastSelection;

void updateSelectionFormFooter()
{
	size_t selectedSize = 0;
	size_t fullsize = 0;
	uint32_t selectedCount = 0;

	for (auto& f : lastSelection.selection)
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
	txt += int(lastSelection.count).ToString();
	txt += " (";
	txt += int(selectedSize / (1024ll * 1024ll)).ToString();
	txt += " MB/";
	txt += int(fullsize / (1024ll * 1024ll)).ToString();
	txt += " MB)";

	GuiLite::FileSelectionForm::instance->infoLabel->Text = txt;
}

void fileSelectionChanged(int id, bool selected)
{
	if (id >= lastSelection.selection.size())
		return;

	lastSelection.selection[id].selected = selected;

	updateSelectionFormFooter();
}

void setAllFileSelection(bool selected)
{
	GuiLite::FileSelectionForm::instance->initialized = false;
	auto rows = GuiLite::FileSelectionForm::instance->filesGridView->Rows;
	int i = 0;

	for (auto& s : lastSelection.selection)
	{
		s.selected = selected;
		rows[i++]->Cells[1]->Value = selected;
	}
	GuiLite::FileSelectionForm::instance->initialized = true;
	updateSelectionFormFooter();
}

void fillFilesSelectionForm()
{
	mtBI::TorrentFilesSelection& info = lastSelection;
	info.count = 0;
	IoctlFunc(mtBI::MessageId::GetTorrentFilesSelection, hash, &info);

	if (info.count > 0)
		info.selection.resize(info.count);

	IoctlFunc(mtBI::MessageId::GetTorrentFilesSelection, hash, &info);

	auto panel = GuiLite::FileSelectionForm::instance->filesGridView;
	panel->Rows->Add(info.count);

	int i = 0;
	for (auto& f : info.selection)
	{
		auto row = gcnew cli::array<Object^>(4) {
			int(i).ToString(),
			f.selected,
			gcnew String(f.name.data),
			int(f.size / (1024ll * 1024ll)).ToString() + " MB"
		};

		panel->Rows[i]->SetValues(row);
		i++;
	}

	panel->Sort(panel->Columns[2], System::ComponentModel::ListSortDirection::Ascending);
	updateSelectionFormFooter();
}

void refreshTorrentInfo(uint8_t* hash)
{
	if (!IoctlFunc)
		return;

	mtBI::TorrentInfo info;
	IoctlFunc(mtBI::MessageId::GetTorrentInfo, hash, &info);

	if (info.filesCount > 0)
	{
		info.filenames.resize(info.filesCount);
		info.filesizes.resize(info.filesCount);
		IoctlFunc(mtBI::MessageId::GetTorrentInfo, hash, &info);
	}

	GuiLite::MainForm::instance->torrentInfoLabel->Clear();
	auto infoLines = GuiLite::MainForm::instance->torrentInfoLabel;

	infoLines->AppendText(gcnew String(info.name.data));
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText("Fullsize: ");
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText(int(info.fullsize/(1024ll*1024ll)).ToString());
	infoLines->AppendText(" MB");
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText(Environment::NewLine);
	infoLines->AppendText("Files: ");
	infoLines->AppendText(Environment::NewLine);

	if (info.filesCount)
	{
		auto files = gcnew array<String^>(info.filesCount);

		for (uint32_t i = 0; i < info.filesCount; i++)
		{
			files[i] = gcnew String(info.filenames[i].data) + " (" + int(info.filesizes[i] / (1024ll * 1024ll)).ToString() + " MB)";
		}
		
		Array::Sort(files);

		for (uint32_t i = 0; i < info.filesCount; i++)
		{
			infoLines->AppendText(files[i]);
			infoLines->AppendText(Environment::NewLine);
		}
	}

	GuiLite::MainForm::instance->selectButton->Visible = true;

	initSpeedChart();
}

void start()
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
	}
}

void addTorrent()
{
	OpenFileDialog^ openFileDialog = gcnew OpenFileDialog;

	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		auto filenamePtr = getStringPtr(openFileDialog->FileName);
		
		if (IoctlFunc(mtBI::MessageId::AddFromFile, filenamePtr, hash) == mtt::Status::Success)
		{
			selected = true;
			selectionChanged = true;
		}
	}
}

bool addTorrentFromMetadata(const char* magnetPtr)
{
	if (IoctlFunc(mtBI::MessageId::AddFromMetadata, magnetPtr, hash) != mtt::Status::Success)
		return false;

	selected = true;
	selectionChanged = true;

	return true;
}

void customAction(System::String^ action, System::String^ param)
{
	if (action == "AddPeer")
	{
		mtBI::AddPeerRequest request;
		memcpy(request.hash, hash, 20);
		request.addr = getStringPtr(param);

		IoctlFunc(mtBI::MessageId::AddPeer, &request, nullptr);
	}
}

void setSelected(bool v)
{
	selected = v;
	GuiLite::MainForm::instance->buttonStart->Enabled = selected;

	if (!selected)
	{
		GuiLite::MainForm::instance->getGrid()->ClearSelection();
	}
}

int magnetLinkSequence = 0;
uint32_t lastMagnetLinkLogCount = 0;

void onButtonClick(ButtonId id, System::String^ param)
{
	if (id == ButtonId::AddPeer)
	{
		if (!selected)
			return;

		GuiLite::AddPeerForm form;
		form.ShowDialog();
	}
	else if (id == ButtonId::TorrentDoubleClick)
	{
		if (!selected)
			return;

		mtBI::MagnetLinkProgress magnetProgress;
		if (!GuiLite::MagnetInputForm::instance && IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, hash, &magnetProgress) == mtt::Status::Success)
		{
			GuiLite::MagnetInputForm form;
			form.labelText->Text = "Getting info...";
			form.magnetFormButton->Text = "Logs";
			form.textBoxMagnet->Text = gcnew String(hexToString(hash, 20).data());
			magnetLinkSequence = 2;
			form.ShowDialog();
			GuiLite::MagnetInputForm::instance = nullptr;
			magnetLinkSequence = 0;
			lastMagnetLinkLogCount = 0;
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

		memcpy(request.hash, hash, 20);
		IoctlFunc(mtBI::MessageId::Remove, &request, nullptr);
	}
	else if (id == ButtonId::AddTorrentFile)
	{
		System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&addTorrent));
		newThread->SetApartmentState(System::Threading::ApartmentState::STA);
		newThread->Start();
	}
	else if (id == ButtonId::TorrentGrid)
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

			if (memcmp(hash, selectedHash, 20) != 0)
			{
				memcpy(hash, selectedHash, 20);
				selectionChanged = true;
			}
		}
	}
	else if (id == ButtonId::Start)
	{
		if (selected)
		{
			if (IoctlFunc(mtBI::MessageId::Start, hash, nullptr) == mtt::Status::Success)
				selectionChanged = true;
		}
	}
	else if (id == ButtonId::Stop)
	{
		if (selected)
		{
			if (IoctlFunc(mtBI::MessageId::Stop, hash, nullptr) == mtt::Status::Success)
				selectionChanged = true;
		}
	}
	else if (id == ButtonId::AddTorrentMagnet)
	{
		magnetLinkSequence = 1;
		GuiLite::MagnetInputForm form;
		form.ShowDialog();
		GuiLite::MagnetInputForm::instance = nullptr;
		magnetLinkSequence = 0;
		lastMagnetLinkLogCount = 0;
	}
	else if (id == ButtonId::Settings)
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
		}

		form.ShowDialog();
	}
	else if (id == ButtonId::SourceRefresh)
	{
		if (param)
		{
			mtBI::SourceId info;
			info.name = getStringPtr(param);
			memcpy(info.hash, hash, 20);
			IoctlFunc(mtBI::MessageId::RefreshSource, &info, nullptr);
		}
	}
	else if (id == ButtonId::SelectFiles)
	{
		GuiLite::FileSelectionForm form;
		form.ShowDialog();
		lastSelection.selection.clear();
		GuiLite::FileSelectionForm::instance = nullptr;
	}
	else if (GuiLite::MagnetInputForm::instance)
	{
		if (id == ButtonId::MagnetButton)
		{
			if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Add"))
			{
				if (GuiLite::MagnetInputForm::instance->textBoxMagnet->Text->Length == 0)
					return;

				if (magnetLinkSequence > 1)
					return;

				auto magnetPtr = getStringPtr(GuiLite::MagnetInputForm::instance->textBoxMagnet->Text);
				if (addTorrentFromMetadata(magnetPtr))
				{
					GuiLite::MagnetInputForm::instance->labelText->Text = "Getting info...";
					magnetLinkSequence = 2;

					GuiLite::MagnetInputForm::instance->magnetFormButton->Text = "Logs";
				}
				else
					GuiLite::MagnetInputForm::instance->labelText->Text = "Invalid magnet link";
			}
			else if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Logs"))
			{
				GuiLite::MagnetInputForm::instance->magnetFormButton->Enabled = false;
				GuiLite::MagnetInputForm::instance->logsTextBox->Visible = true;
				magnetLinkSequence = 3;
			}
		}
	}
	else if (GuiLite::FileSelectionForm::instance)
	{
		if (id == ButtonId::SelectionOk)
		{
			if (!lastSelection.selection.empty())
			{
				mtBI::TorrentFilesSelectionRequest selection;
				memcpy(selection.hash, hash, 20);
				for (auto& s : lastSelection.selection)
				{
					mtBI::FileSelectionRequest f;
					f.selected = s.selected;
					selection.selection.push_back(f);
				}

				IoctlFunc(mtBI::MessageId::SetTorrentFilesSelection, &selection, nullptr);
			}

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

int RefreshTimeCounter = 0;
void refreshUi()
{
	if (!IoctlFunc)
		return;

	if (magnetLinkSequence > 0)
	{
		if (magnetLinkSequence >= 2)
		{
			if (magnetLinkSequence == 3)
			{
				mtBI::MagnetLinkProgressLogs logs;
				logs.count = 0;
				if (IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, hash, &logs) == mtt::Status::Success && logs.count > 0)
				{
					if (lastMagnetLinkLogCount < logs.count)
					{
						logs.count = logs.count - lastMagnetLinkLogCount;
						logs.logs.resize(logs.count);
						logs.start = lastMagnetLinkLogCount;

						IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, hash, &logs);

						for (auto& l : logs.logs)
						{
							GuiLite::MagnetInputForm::instance->logsTextBox->Text += gcnew String(l.data);
							GuiLite::MagnetInputForm::instance->logsTextBox->Text += "\n";
						}

						lastMagnetLinkLogCount += logs.count;
					}
				}
			}

			mtBI::MagnetLinkProgress progress;
			if (IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, hash, &progress) == mtt::Status::Success)
			{
				if (progress.progress > 1.0f)
					progress.progress = 1.0f;

				GuiLite::MagnetInputForm::instance->progressBarMagnet->Value = (int)(progress.progress * 100);

				if (progress.finished)
				{
					magnetLinkSequence = 4;
					GuiLite::MagnetInputForm::instance->labelText->Text = "Finished";
				}
			}
			else
				GuiLite::MagnetInputForm::instance->labelText->Text = "Error";
		}
	}

	RefreshTimeCounter++;
	if (RefreshTimeCounter < 10 && !selectionChanged)
		return;

	RefreshTimeCounter = 0;

	if (selectionChanged)
	{
		refreshTorrentInfo(hash);
		selectionChanged = false;
	}

	mtBI::TorrentsList torrents;
	torrents.count = 0;
	if (IoctlFunc(mtBI::MessageId::GetTorrents, nullptr, &torrents) != mtt::Status::Success)
		return;

	if (torrents.count > 0)
	{
		torrents.list.resize(torrents.count);
		IoctlFunc(mtBI::MessageId::GetTorrents, nullptr, &torrents);
	}

	mtBI::TorrentStateInfo info;
	info.connectedPeers = 0;
	{
		auto torrentGrid = GuiLite::MainForm::instance->getGrid();
		adjustGridRowsCount(torrentGrid, torrents.count);

		for (uint32_t i = 0; i < torrents.count; i++)
		{
			auto& t = torrents.list[i];

			if (IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, t.hash, &info) == mtt::Status::Success)
			{
				String^ activeStatus;
				if (info.activeStatus != mtt::Status::Success)
				{
					if (info.activeStatus == mtt::Status::E_NotEnoughSpace)
						activeStatus = "Not enough space";
					else
						activeStatus = "Problem " + int(info.activeStatus).ToString();
				}
				else if (!t.active)
					activeStatus = "Stopped";
				else
					activeStatus = "Active";

				if (t.active)
					updateSpeedChart((info.downloadSpeed / 1024.f) / 1024.f, (info.uploadSpeed / 1024.f) / 1024.f);

				String^ speedInfo = float((info.downloadSpeed / 1024.f) / 1024.f).ToString("F");
				if (t.active && info.downloadSpeed > 0 && info.selectionProgress && info.selectionProgress < 1.0f && info.downloaded > 0)
				{
					speedInfo += " (";
					size_t leftBytes = ((size_t)(info.downloaded / info.selectionProgress)) - info.downloaded;
					size_t leftSeconds = leftBytes / info.downloadSpeed;
					TimeSpan time = System::TimeSpan::FromSeconds((double)leftSeconds);
					speedInfo += time.ToString("d\\d\\ hh\\hmm\\mss\\s")->TrimStart(' ', 'd', 'h', 'm', 's', '0');
					speedInfo += ")";
				}

				String^ name = gcnew String(info.name.data);
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

				auto row = gcnew cli::array< System::String^  >(10) {
					gcnew String(hexToString(t.hash, 20).data()),
						name, progress, activeStatus, speedInfo, float((info.uploadSpeed / 1024.f) / 1024.f).ToString("F"),
						int(info.connectedPeers).ToString(), int(info.foundPeers).ToString(),
						float((info.downloaded / 1024.f) / 1024.f).ToString("F"), float((info.uploaded / 1024.f) / 1024.f).ToString("F")
				};

				torrentGrid->Rows[i]->SetValues(row);
			}
		}
	}

	mtBI::TorrentPeersInfo peers;
	peers.count = 0;
	peers.peers.resize(info.connectedPeers);
	IoctlFunc(mtBI::MessageId::GetPeersInfo, hash, &peers);

	{
		auto peersGrid = GuiLite::MainForm::instance->getPeersGrid();
		String^ selectedPeer;
		if (peersGrid->SelectedRows->Count > 0)
			selectedPeer = (String^)peersGrid->SelectedRows[0]->Cells[0]->Value;
		adjustGridRowsCount(peersGrid, (int)peers.count);

		for (uint32_t i = 0; i < peers.count; i++)
		{
			auto& peerInfo = peers.peers[i];
			auto peerRow = gcnew cli::array< System::String^  >(6) {
				gcnew String(peerInfo.addr.data),
					float(peerInfo.dlSpeed / (1024.f * 1024)).ToString("F"), float(peerInfo.upSpeed / (1024.f * 1024)).ToString("F"),
					float(peerInfo.progress).ToString("P"),	gcnew String(peerInfo.client.data, 0, (int)strlen(peerInfo.client.data), System::Text::Encoding::UTF8),
					gcnew String(peerInfo.country.data)
			};

			peersGrid->Rows[i]->SetValues(peerRow);
		}

		if (peersGrid->SortedColumn)
			peersGrid->Sort(peersGrid->SortedColumn, peersGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
	
		if (peersGrid->RowCount > 0 && selectedPeer)
		{
			peersGrid->ClearSelection();
			for (uint32_t i = 0; i < peers.count; i++)
			{
				if (selectedPeer == (String^)peersGrid->Rows[i]->Cells[0]->Value)
					peersGrid->Rows[i]->Selected = true;
			}
		}
	}

	mtBI::SourcesInfo sources;
	sources.count = 0;
	IoctlFunc(mtBI::MessageId::GetSourcesInfo, hash, &sources);

	if (sources.count > 0)
	{
		sources.sources.resize(sources.count);
		IoctlFunc(mtBI::MessageId::GetSourcesInfo, hash, &sources);
	}

	{
		auto sourcesGrid = GuiLite::MainForm::instance->sourcesGrid;
		adjustGridRowsCount(sourcesGrid, (int)sources.count);

		for (uint32_t i = 0; i < sources.count; i++)
		{
			auto& source = sources.sources[i];
			auto timeTo = TimeSpan::FromSeconds(source.nextCheck);
			auto nextStr = timeTo.ToString("T");

			auto sourceRow = gcnew cli::array< System::String^  >(5) {
				gcnew String(source.name.data), gcnew String(source.status),
					int(source.peers).ToString(), nextStr, int(source.interval).ToString()
			};

			if (source.status[0] == 0)
			{
				sourceRow[2] = "";
				sourceRow[4] = "";
			}

			if (source.nextCheck == 0)
				sourceRow[3] = "";

			sourcesGrid->Rows[i]->SetValues(sourceRow);
		}

		if (sourcesGrid->SortedColumn)
			sourcesGrid->Sort(sourcesGrid->SortedColumn, sourcesGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
	}
}

void ProcessProgramArgument(System::String^ arg)
{
	auto magnetPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(arg).ToPointer();
	addTorrentFromMetadata(magnetPtr);
}

const int WM_MTT_ARGUMENT = WM_USER + 100;
const int MTT_ARGUMENT_START = 1;
const int MTT_ARGUMENT_END = 2;
std::string argumentBuffer;

public ref class MyMessageFilter : System::Windows::Forms::IMessageFilter
{
	public: 
	
		MyMessageFilter() { }

		virtual bool PreFilterMessage(Message% objMessage) 
		{
			if (objMessage.Msg == WM_MTT_ARGUMENT)
			{
				if (objMessage.LParam == (IntPtr)MTT_ARGUMENT_END)
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

	GuiLite::MainForm form;
	start();

	if (args->Length > 0)
		ProcessProgramArgument(args[0]);

	MyMessageFilter^ filter = gcnew MyMessageFilter();
	Application::AddMessageFilter(filter);

	Application::Run(%form);

	if(IoctlFunc)
		IoctlFunc(mtBI::MessageId::Deinit, nullptr, nullptr);

	if(lib)
		FreeLibrary(lib);
}

#pragma comment(lib, "user32.lib")
HWND GetExistingMainWindow()
{
	auto processes = System::Diagnostics::Process::GetProcessesByName("mtTorrent");
	for (int i = 0; i < processes->Length; i++)
	{
		if (processes[i]->MainWindowTitle == "mtTorrent")
			return (HWND)processes[i]->MainWindowHandle.ToInt64();
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
}

int Main(cli::array<System::String ^>^ args)
{
	if (HWND h = GetExistingMainWindow())
		OnDuplicate(h, args);
	else
		FormsMain(args);

	return 0;
}