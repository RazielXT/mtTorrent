#include "utils.h"
#include <filesystem>
#include <DbgHelp.h>
#include <TlHelp32.h>

#pragma comment (lib, "dbghelp.lib")

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](char c) { return !isdigit(c); }) == s.end();
}

void cleanTestFolder(std::string path)
{
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		if (entry.is_directory())
		{
			auto path = entry.path().filename();

			if (is_number(path.string()))
			{
				std::filesystem::remove_all(entry.path());
			}
		}
	}
}

bool copyTestFolder(std::string path, const char* folderName, int count)
{
	for (int i = 0; i < count; i++)
	{
		std::error_code ec;
		std::filesystem::copy(path + folderName, path + std::to_string(i), std::filesystem::copy_options::recursive, ec);

		if (ec)
			return false;
	}

	return true;
}

struct handle_data {
	unsigned long process_id;
	HWND window_handle;
};

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
		return TRUE;
	data.window_handle = handle;
	return FALSE;
}

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)& data);
	return data.window_handle;
}

DWORD GetProcessByName(std::string name)
{
	DWORD pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);

	if (Process32First(snapshot, &process))
	{
		do
		{
			if (name == process.szExeFile)
			{
				pid = process.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &process));
	}

	CloseHandle(snapshot);

	return pid;
}

bool createDump(DWORD process, std::string path, const char* name)
{
	const DWORD Flags = MiniDumpWithFullMemory |
		MiniDumpWithFullMemoryInfo |
		MiniDumpWithHandleData |
		MiniDumpWithUnloadedModules |
		MiniDumpWithThreadInfo;

	auto fullpath = path + std::to_string(::time(0)) + "_" + name + ".dmp";

	HANDLE hFile = CreateFileA(fullpath.data(), GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile)
	{
		auto hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, process);
		BOOL Result = MiniDumpWriteDump(hProc,
			process,
			hFile,
			(MINIDUMP_TYPE)Flags,
			nullptr,
			nullptr,
			nullptr);

		CloseHandle(hProc);
		CloseHandle(hFile);

		if (Result)
		{
			return true;
		}
	}

	return false;
}
