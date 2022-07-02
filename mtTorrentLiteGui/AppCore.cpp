#include "MainForm.h"
#include "ScheduleForm.h"
#include "AppCore.h"
#include "AppCoreCallbacks.h"
#include "AddPeerForm.h"
#include <codecvt>

#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

AppCore::AppCore() : fileSelection(*this), magnetProgress(*this), scheduler(*this), torrentsView(*this), settings(*this)
{

}

extern AppCore core;
static void UiUpdateFunc(System::Object^ myObject, System::EventArgs^ myEventArgs)
{
	core.update();
}

bool AppCore::init()
{
	wchar_t pathBuffer[256];
	if (GetModuleFileName(NULL, pathBuffer, 256))
	{
		PathRemoveFileSpec(pathBuffer);
		SetCurrentDirectory(pathBuffer);
	}

	if (!loadLibrary())
		return false;

	registerAlerts();

	GuiLite::MainForm::instance->updateTimer = gcnew System::Windows::Forms::Timer();
	GuiLite::MainForm::instance->updateTimer->Tick += gcnew System::EventHandler(UiUpdateFunc);
	GuiLite::MainForm::instance->updateTimer->Interval = 100;
	GuiLite::MainForm::instance->updateTimer->Start();

	return true;
}

void AppCore::deinit()
{
	unloadLibrary();
}

//forms callbacks
extern AppCore core;
static void addTorrentFromFileThread()
{
	core.addTorrentFromFile();
}
static void showSettingsFormThread()
{
	core.settings.showSettingsForm();
}

System::String^ AppCore::getTorrentName(uint8_t* hash)
{
	mtBI::TorrentStateInfo info;

	if (selected && IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, hash, &info) == mtt::Status::Success)
		return gcnew System::String(info.name.data);
	else
		return "";
}

TorrentCtxMenuInfo AppCore::getTorrentContexMenuInfo()
{
	TorrentCtxMenuInfo info;

	mtBI::TorrentStateInfo state;
	info.active = IoctlFunc(mtBI::MessageId::GetTorrentStateInfo, firstSelectedHash, &state) == mtt::Status::Success && state.started;

	mtBI::MagnetLinkProgressLogsRequest logsReq;
	logsReq.start = -1;
	memcpy(logsReq.hash, firstSelectedHash, 20);
	mtBI::MagnetLinkProgressLogsResponse logsResp{};
	info.utmLogs = IoctlFunc(mtBI::MessageId::GetMagnetLinkProgressLogs, &logsReq, &logsResp) == mtt::Status::Success && logsResp.fullcount > 0;

	mtBI::MagnetLinkProgress magnetProgress{};
	info.noInfo = IoctlFunc(mtBI::MessageId::GetMagnetLinkProgress, firstSelectedHash, &magnetProgress) == mtt::Status::Success && !magnetProgress.finished;

	info.scheduled = !info.active && (scheduler.isQueued(firstSelectedHash) || scheduler.isScheduled(firstSelectedHash));

	return info;
}

void AppCore::addTorrentFromFile()
{
	System::Windows::Forms::OpenFileDialog^ openFileDialog = gcnew System::Windows::Forms::OpenFileDialog;
	openFileDialog->Filter = "Torrent files (*.torrent)|*.torrent|All files (*.*)|*.*";

	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		array<System::Byte>^ encodedBytes = System::Text::Encoding::UTF8->GetBytes(openFileDialog->FileName + "\0");
		pin_ptr<System::Byte> pinnedBytes = &encodedBytes[0];
		auto utf8Bytes = reinterpret_cast<char*>(pinnedBytes);
		addTorrentFromFile(utf8Bytes);
	}
}

void AppCore::handleTorrentAddResponse(mtt::Status status, uint8_t* hash)
{
	if (status == mtt::Status::I_AlreadyExists || status == mtt::Status::I_Merged)
	{
		auto name = getTorrentName(hash);

		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv_utf8_w;
		std::wstring unicode_codepoints = conv_utf8_w.from_bytes(getUtf8String(name));

		if (status == mtt::Status::I_AlreadyExists)
			::MessageBox(NULL, L"Torrent already exists", unicode_codepoints.data(), MB_OK);
		else if (status == mtt::Status::I_Merged)
			::MessageBox(NULL, L"Torrent already exists, new trackers added", unicode_codepoints.data(), MB_OK);
	}
}

void AppCore::removeTorrent(uint8_t* hash, bool removeFiles)
{
	mtBI::RemoveTorrentRequest request;
	request.deleteFiles = removeFiles;
	memcpy(request.hash, hash, 20);

	IoctlFunc(mtBI::MessageId::Remove, &request, nullptr);
	forceRefresh = true;
}

void AppCore::processProgramArgument(System::String^ arg)
{
	if (HWND windowHandle = (HWND)GuiLite::MainForm::instance->Handle.ToInt64())
		SetFocus(windowHandle);

	auto inputPtr = (char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(arg).ToPointer();
	if (magnetProgress.addTorrentFromMetadata(inputPtr) != mtt::Status::E_InvalidInput)
		return;

	if (addTorrentFromFile(inputPtr) != mtt::Status::E_InvalidInput)
		return;

	std::wstring str = L"Couldnt process input: ";
	str += (wchar_t*)System::Runtime::InteropServices::Marshal::StringToHGlobalUni(arg).ToPointer();

	::MessageBox(NULL, str.data(), L"Invalid input", MB_OK);
}

void AppCore::showWindow()
{
	GuiLite::MainForm::instance->Activate();

	if (GuiLite::MainForm::instance->WindowState == System::Windows::Forms::FormWindowState::Minimized)
	{
		ShowWindow((HWND)GuiLite::MainForm::instance->Handle.ToInt64(), SW_RESTORE);
	}
	else
	{
		ShowWindow((HWND)GuiLite::MainForm::instance->Handle.ToInt64(), SW_MINIMIZE);
		ShowWindow((HWND)GuiLite::MainForm::instance->Handle.ToInt64(), SW_RESTORE);
	}
}

mtt::Status AppCore::addTorrentFromFile(const char* filepath)
{
	uint8_t hash[20];
	auto status = IoctlFunc(mtBI::MessageId::AddFromFile, filepath, hash);
	handleTorrentAddResponse(status, hash);

	return status;
}

void AppCore::registerAlerts()
{
	mtBI::RegisterAlertsRequest alertsRequest{ (uint32_t)mtt::AlertCategory::Torrent | (uint32_t)mtt::AlertCategory::Metadata };
	IoctlFunc(mtBI::MessageId::RegisterAlerts, &alertsRequest, nullptr);
}

void AppCore::checkAlerts()
{
	mtBI::AlertsList alertsRequest;
	if (IoctlFunc(mtBI::MessageId::PopAlerts, nullptr, &alertsRequest) == mtt::Status::Success && !alertsRequest.alerts.empty())
	{
		for (auto& alert : alertsRequest.alerts)
		{
			if (alert.id == mtt::AlertId::TorrentAdded)
			{
				torrentsView.select(alert.hash);
				selectionChanged = true;

				fileSelection.showFilesSelectionForm(alert.hash, true);
			}
			else if (alert.id == mtt::AlertId::TorrentFinished)
			{
				scheduler.torrentFinished(alert.hash);
			}
			else if (alert.id == mtt::AlertId::MetadataFinished)
			{
				if (memcmp(firstSelectedHash, alert.hash, 20) == 0)
				{
					selectionChanged = true;
					forceRefresh = true;
				}
			}
		}
	}
}

void AppCore::update()
{
	if (!IoctlFunc)
		return;

	magnetProgress.update();

	//100ms timer, refresh UI once per second or after change
	RefreshTimeCounter++;
	if (RefreshTimeCounter < 10 && !selectionChanged && !forceRefresh)
		return;

	RefreshTimeCounter = 0;

	checkAlerts();

	torrentsView.update();

	selectionChanged = false;
	forceRefresh = false;
}

void AppCore::stopRunningTorrents()
{
	auto selection = core.torrentsView.getAllSelectedTorrents();

	for (auto s : selection)
	{
		if (core.IoctlFunc(mtBI::MessageId::Stop, s.hash, nullptr) == mtt::Status::Success)
			core.forceRefresh = true;

		core.scheduler.stop(s.hash);
	}
}

void AppCore::onButtonClick(ButtonId id, System::String^ param)
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
		request.addr = getUtf8String(param);

		IoctlFunc(mtBI::MessageId::AddPeer, &request, nullptr);
	}
	else if (id == ButtonId::OpenLocation)
	{
		mtBI::TorrentInfo info;
		if (IoctlFunc(mtBI::MessageId::GetTorrentInfo, firstSelectedHash, &info) == mtt::Status::Success && info.downloadLocation.length > 0)
		{
			auto path = gcnew System::String(info.downloadLocation.data, 0, (int)info.downloadLocation.length, System::Text::Encoding::UTF8);
			if (info.files.size() > 1)
				path += gcnew System::String(info.name.data, 0, (int)info.name.length, System::Text::Encoding::UTF8);
			try
			{
				System::Diagnostics::Process::Start(path);
			}
			catch (System::Exception^ ex)
			{
				::MessageBoxA(NULL, getUtf8String(ex->Message).data(), "Error", MB_OK);
			}
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
	else if (id == ButtonId::QueueNext)
	{
		auto selection = torrentsView.getAllSelectedTorrents();

		for (auto& s : selection)
		{
			scheduler.queueTorrent(s.hash, true);
		}
	}
	else if (id == ButtonId::QueueLast)
	{
		auto selection = torrentsView.getAllSelectedTorrents();

		for (auto& s : selection)
		{
			scheduler.queueTorrent(s.hash, false);
		}
	}
	else if (id == ButtonId::StopAfterFinish)
	{
		auto selection = torrentsView.getAllSelectedTorrents();

		for (auto& s : selection)
			scheduler.stopAfterFinish(s.hash);
	}
	else if (id == ButtonId::StopSchedule)
	{
		auto selection = torrentsView.getAllSelectedTorrents();

		for (auto& s : selection)
			scheduler.stop(s.hash);
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

		auto selection = torrentsView.getAllSelectedTorrents();

		for (auto s : selection)
		{
			memcpy(request.hash, s.hash, 20);
			IoctlFunc(mtBI::MessageId::Remove, &request, nullptr);
		}
	}
	else if (id == ButtonId::AddTorrentFile)
	{
		System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&addTorrentFromFileThread));
		newThread->SetApartmentState(System::Threading::ApartmentState::STA);
		newThread->Start();
	}
	else if (id == ButtonId::TorrentGrid)
	{
		torrentsView.refreshSelection();
	}
	else if (id == ButtonId::Start)
	{
		if (selected)
		{
			auto selection = torrentsView.getAllSelectedTorrents();

			for (auto s : selection)
			{
				IoctlFunc(mtBI::MessageId::Start, s.hash, nullptr);

				scheduler.stop(s.hash);
			}
		}
	}
	else if (id == ButtonId::Stop)
	{
		if (selected)
		{
			System::Threading::Thread^ newThread = gcnew System::Threading::Thread(gcnew System::Threading::ThreadStart(&stopRunningTorrents));
			newThread->SetApartmentState(System::Threading::ApartmentState::STA);
			newThread->Start();
		}
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
			info.name = getUtf8String(param);
			memcpy(info.hash, firstSelectedHash, 20);
			IoctlFunc(mtBI::MessageId::RefreshSource, &info, nullptr);
		}
	}
	else
	{
		fileSelection.onButtonClick(id);
		magnetProgress.onButtonClick(id);
	}

	forceRefresh = true;
}

System::String^ AppCore::getUpnpInfo()
{
	mtt::string info;

	IoctlFunc(mtBI::MessageId::GetUpnpInfo, nullptr, &info);

	return gcnew System::String(info.data ? info.data : "");
}
