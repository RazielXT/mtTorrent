#include "MainForm.h"
#include "../mtTorrent/Global/BinaryInterface.h"
#include <windows.h>
#include <vcclr.h>

using namespace System;
using namespace System::Windows::Forms;

typedef void(*IOCTL_FUNC)(mtBI::MessageId, void*);
IOCTL_FUNC IoctlFunc = nullptr;
HMODULE lib = nullptr;

void refreshTorrentInfo()
{
	if (!IoctlFunc)
		return;

	mtBI::TorrentInfo info;
	IoctlFunc(mtBI::MessageId::GetTorrentInfo, &info);

	if (info.filesCount > 0)
	{
		info.filenames.resize(info.filesCount);
		info.filesizes.resize(info.filesCount);
		IoctlFunc(mtBI::MessageId::GetTorrentInfo, &info);
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
		IoctlFunc(mtBI::MessageId::Start, nullptr);

		refreshTorrentInfo();
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

void refreshUi()
{
	if (!IoctlFunc)
		return;

	mtBI::TorrentStateInfo info;
	IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, &info);

	{
		auto row = gcnew cli::array< System::String^  >(6) {
			gcnew String(info.name.data), float(info.progress).ToString("P"),
				float(info.downloadSpeed / (1024.f * 1024)).ToString("F"), float(info.downloaded / (1024.f * 1024)).ToString("F"),
				int(info.connectedPeers).ToString(), int(info.foundPeers).ToString()
		};

		auto torrentGrid = GuiLite::MainForm::instance->getGrid();
		adjustGridRowsCount(torrentGrid, 1);
		torrentGrid->Rows[0]->SetValues(row);
	}

	mtBI::TorrentPeersInfo peers;
	peers.peers.resize(info.connectedPeers);
	IoctlFunc(mtBI::MessageId::GetPeersInfo, &peers);

	{
		auto peersGrid = GuiLite::MainForm::instance->getPeersGrid();
		adjustGridRowsCount(peersGrid, (int)peers.count);

		for (uint32_t i = 0; i < peers.count; i++)
		{
			auto& peerInfo = peers.peers[i];
			auto peerRow = gcnew cli::array< System::String^  >(4) {
				gcnew String(peerInfo.addr.data),
					float(peerInfo.speed / (1024.f * 1024)).ToString(), float(peerInfo.progress).ToString("P"),
					gcnew String(peerInfo.source)
			};

			peersGrid->Rows[i]->SetValues(peerRow);
		}

		if (peersGrid->SortedColumn)
			peersGrid->Sort(peersGrid->SortedColumn, peersGrid->SortOrder == SortOrder::Ascending ? System::ComponentModel::ListSortDirection::Ascending : System::ComponentModel::ListSortDirection::Descending);
	}

	mtBI::SourcesInfo sources;
	sources.count = 0;
	IoctlFunc(mtBI::MessageId::GetSourcesInfo, &sources);

	if (sources.count > 0)
	{
		sources.sources.resize(sources.count);
		IoctlFunc(mtBI::MessageId::GetSourcesInfo, &sources);
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