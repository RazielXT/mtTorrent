#include <filesystem>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
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
	for (const auto & entry : std::filesystem::directory_iterator(path))
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
	EnumWindows(enum_windows_callback, (LPARAM)&data);
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
		}
		while (Process32Next(snapshot, &process));
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

bool closeTestApp(std::string path, const char* dumpPath)
{
	DWORD process = GetProcessByName(path);

	if (process)
	{
		auto hwnd = find_main_window(process);

		if (hwnd)
			SendMessage(hwnd, WM_CLOSE, 0, 0);

		DWORD waitTime = 0;
		while (GetProcessByName(path) == process)
		{
			waitTime += 200;

			if (waitTime > 5000)
			{
				createDump(process, dumpPath, "waiting");
				return false;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		return true;
	}

	return false;
}

bool runTest(std::string path, const char* appExecutable, int id, const char* dumpPath)
{
	int runTimeSeconds = 5;

	std::string fullPath = path + std::to_string(id) + "/";

	SetCurrentDirectory(fullPath.data());

	fullPath += appExecutable;

	HINSTANCE result = ShellExecute(0, "open", fullPath.data(), 0, 0, SW_SHOWNORMAL);

	std::this_thread::sleep_for(std::chrono::seconds(runTimeSeconds));

	return closeTestApp(appExecutable, dumpPath);
}

void registerDumpCreate(const char* dumpPath, const char* appExecutable)
{
	HKEY hKey;
	LPCTSTR sk = TEXT("SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting");

	LONG openRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, KEY_ALL_ACCESS, &hKey);

	if (openRes == ERROR_SUCCESS)
	{
		HKEY lhKey;
		RegCreateKey(hKey, "LocalDumps", &lhKey);
		HKEY elhKey;
		RegCreateKey(lhKey, appExecutable, &elhKey);

		openRes = RegSetValueExA(elhKey, "DumpFolder", 0, REG_SZ, (const BYTE*)dumpPath, (DWORD)strlen(dumpPath) + 1);

		RegCloseKey(elhKey);
		RegCloseKey(lhKey);
		RegCloseKey(hKey);
	}
}

void unregisterDumpCreate(const char* appExecutable)
{
	HKEY hKey;
	LPCTSTR sk = TEXT("SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps");

	LONG openRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, KEY_ALL_ACCESS, &hKey);

	if (openRes == ERROR_SUCCESS)
	{
		auto r = RegDeleteKeyEx(hKey, appExecutable, KEY_WOW64_64KEY, 0);
		RegCloseKey(hKey);
	}
}

int main()
{
	int testCount = 1;
	const char* testPath = "D:/test/";
	const char* dumpPath = "D:/test/";
	const char* appFolder = "mtt";
	const char* appExecutable = "mtTorrent.exe";

	cleanTestFolder(testPath);
	copyTestFolder(testPath, appFolder, testCount);

	registerDumpCreate(dumpPath, appExecutable);

	for (int i = 0; i < testCount; i++)
	{
		if(!runTest(testPath, appExecutable, i, dumpPath))
			break;
	}

	unregisterDumpCreate(appExecutable);

	return 0;
}