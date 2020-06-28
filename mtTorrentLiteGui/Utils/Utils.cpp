#include "Utils.h"
#include <windows.h>

using namespace System;
using namespace System::Windows::Forms;

std::string getUtf8String(System::String^ str)
{
	array<Byte>^ encodedBytes = System::Text::Encoding::UTF8->GetBytes(str + "\0");
	pin_ptr<Byte> pinnedBytes = &encodedBytes[0];
	return reinterpret_cast<char*>(pinnedBytes);
}

String^ formatBytes(uint64_t bytes)
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

String^ formatBytesSpeed(uint64_t bytes)
{
	return formatBytes(bytes) + "/s";
}

System::String^ GetClipboardText()
{
	System::String^ text = "";
	if (OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData)
		{
			char* pszText = static_cast<char*>(GlobalLock(hData));
			if (pszText)
				text = gcnew System::String(pszText);

			GlobalUnlock(hData);
		}
		CloseClipboard();
	}
	return text;
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

void RegisterMagnetAssociation(long long parent)
{
	auto reg = Microsoft::Win32::RegistryKey::OpenBaseKey(Microsoft::Win32::RegistryHive::ClassesRoot, Microsoft::Win32::RegistryView::Registry64)->OpenSubKey("Magnet\\shell\\open\\command", true);

	if (reg)
	{
		auto commandPath = gcnew String("\"");
		commandPath += System::Reflection::Assembly::GetExecutingAssembly()->Location;
		commandPath += "\" \"%1\"";

		reg->SetValue("", commandPath);

		::MessageBox((HWND)parent, L"Magnet links will now be opened with mtTorrent", L"Magnet link associated", MB_OK);
	}
}


HWND siblingWindowId = {};
DWORD siblingProcessId = {};
static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
	int length = GetWindowTextLength(hWnd);
	wchar_t* buffer = new wchar_t[length + 1];
	GetWindowText(hWnd, buffer, length + 1);

	if (length != 0 && !IsWindowVisible(hWnd) && wcsncmp(buffer, L"mtTorrent", length) == 0)
	{
		DWORD processId = 0;
		GetWindowThreadProcessId(hWnd, &processId);

		if (processId == siblingProcessId)
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
