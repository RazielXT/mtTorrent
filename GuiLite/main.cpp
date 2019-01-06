#include "MainForm.h"
#include "MagnetInputForm.h"
#include "../mtTorrent/Public/BinaryInterface.h"
#include "../mtTorrent/Public/Status.h"
#include "../mtTorrent/utils/HexEncoding.h"
#include <msclr/marshal_cppstd.h>
#include <windows.h>
#include <vcclr.h>
#include "SettingsForm.h"

using namespace System;
using namespace System::Windows::Forms;

typedef mtt::Status(*IOCTL_FUNC)(mtBI::MessageId, const void*,void*);
IOCTL_FUNC IoctlFunc = nullptr;
HMODULE lib = nullptr;

void applySettings(System::Object^ form)
{
	auto window = (GuiLite::SettingsForm^)form;
	mtBI::SettingsInfo info;
	info.dhtEnabled = window->checkBoxDht->Checked;
	auto dirPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(window->directoryTextBox->Text).ToPointer();
	info.directory.set(dirPtr);
	info.maxConnections = (unsigned int)window->maxConnectionsNumeric->Value;
	info.udpPort = (unsigned int)window->udpPortNumeric->Value;
	info.tcpPort = (unsigned int)window->tcpPortNumeric->Value;

	IoctlFunc(mtBI::MessageId::SetSettings, &info, nullptr);
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
}

void start()
{
	lib = LoadLibrary(L"mtTorrent.dll");

	if (lib)
	{
		IoctlFunc = (IOCTL_FUNC)GetProcAddress(lib, "Ioctl");
		IoctlFunc(mtBI::MessageId::Init, nullptr, nullptr);
	}
}

bool selected = false;
uint8_t hash[20];
bool selectionChanged = false;

void addTorrent()
{
	OpenFileDialog^ openFileDialog = gcnew OpenFileDialog;

	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		auto filenamePtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(openFileDialog->FileName).ToPointer();
		
		if (IoctlFunc(mtBI::MessageId::AddFromFile, filenamePtr, hash) == mtt::Status::Success)
		{
			selected = true;
			selectionChanged = true;
		}
	}
}

void setSelected(bool v)
{
	selected = v;
	GuiLite::MainForm::instance->buttonStart->Enabled = selected;
	GuiLite::MainForm::instance->buttonStart->Enabled = selected;

	if (!selected)
	{
		GuiLite::MainForm::instance->getGrid()->ClearSelection();
	}
}

int magnetLinkSequence = 0;
void onButtonClick(System::Object^ button)
{
	if (button == GuiLite::MainForm::instance->buttonAddTorrent)
	{
		System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&addTorrent));
		newThread->SetApartmentState(System::Threading::ApartmentState::STA);
		newThread->Start();
	}
	else if (button == GuiLite::MainForm::instance->getGrid())
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
	else if (button == GuiLite::MainForm::instance->buttonStart)
	{
		if (selected)
		{
			if (IoctlFunc(mtBI::MessageId::Start, hash, nullptr) == mtt::Status::Success)
				selectionChanged = true;
		}
	}
	else if (button == GuiLite::MainForm::instance->buttonStop)
	{
		if (selected)
		{
			if (IoctlFunc(mtBI::MessageId::Stop, hash, nullptr) == mtt::Status::Success)
				selectionChanged = true;
		}
	}
	else if (button == GuiLite::MainForm::instance->buttonAddMagnet)
	{
		magnetLinkSequence = 1;
		GuiLite::MagnetInputForm form;
		form.ShowDialog();
		GuiLite::MagnetInputForm::instance = nullptr;
		magnetLinkSequence = 0;
	}
	else if (button == GuiLite::MainForm::instance->buttonSettings)
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
	else if (GuiLite::MagnetInputForm::instance)
	{
		if (button == GuiLite::MagnetInputForm::instance->magnetFormButton)
		{
			if (GuiLite::MagnetInputForm::instance->magnetFormButton->Text->StartsWith("Add"))
			{
				if (GuiLite::MagnetInputForm::instance->textBoxMagnet->Text->Length == 0)
					return;

				if (magnetLinkSequence > 1)
					return;

				auto magnetPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(GuiLite::MagnetInputForm::instance->textBoxMagnet->Text).ToPointer();
				if (IoctlFunc(mtBI::MessageId::AddFromMetadata, magnetPtr, hash) == mtt::Status::Success)
				{
					GuiLite::MagnetInputForm::instance->magnetFormButton->Enabled = false;
					GuiLite::MagnetInputForm::instance->labelText->Text = "Getting info...";
					magnetLinkSequence = 2;
				}
				else
					GuiLite::MagnetInputForm::instance->labelText->Text = "Invalid magnet link";
			}
		}
	}
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
		if (magnetLinkSequence == 2)
		{
			mtBI::MagnetLinkProgress progress;
			if (IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, hash, &progress) == mtt::Status::Success)
			{
				GuiLite::MagnetInputForm::instance->progressBarMagnet->Value = (int)(progress.progress * 100);

				if (progress.finished)
				{
					magnetLinkSequence = 3;
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
				auto row = gcnew cli::array< System::String^  >(10) {
					gcnew String(hexToString(t.hash, 20).data()),
						gcnew String(info.name.data), info.checking ? "Checking " + float(info.checkingProgress).ToString("P") : float(info.progress).ToString("P"),
						t.active ? "Active" : "Stopped",
						float((info.downloadSpeed / 1024.f) / 1024.f).ToString("F"), float((info.uploadSpeed / 1024.f) / 1024.f).ToString("F"),
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
		adjustGridRowsCount(peersGrid, (int)peers.count);

		for (uint32_t i = 0; i < peers.count; i++)
		{
			auto& peerInfo = peers.peers[i];
			auto peerRow = gcnew cli::array< System::String^  >(5) {
				gcnew String(peerInfo.addr.data),
					float(peerInfo.dlSpeed / (1024.f * 1024)).ToString(), float(peerInfo.upSpeed / (1024.f * 1024)).ToString(), 
					float(peerInfo.progress).ToString("P"),	gcnew String(peerInfo.source)
			};

			peersGrid->Rows[i]->SetValues(peerRow);
		}

		if (peersGrid->SortedColumn)
			peersGrid->Sort(peersGrid->SortedColumn, peersGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
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

[STAThread]
void FormsMain(HWND* hwnd, HWND* parent)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	GuiLite::MainForm form;

	start();

	Application::Run(%form);

	if(lib)
		FreeLibrary(lib);
}

int Main()
{
	FormsMain(0, 0);

	return 0;
}