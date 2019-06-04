#include "mttLoader.h"
#include "utils.h"
#include <chrono>
#include <thread>
#include <iostream>

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

bool runApplicationTest(std::string path, const char* appExecutable, const char* arguments, int runTime, int id, const char* dumpPath)
{
	std::string fullPath = path + std::to_string(id) + "/";

	SetCurrentDirectory(fullPath.data());

	fullPath += appExecutable;

	HINSTANCE result = ShellExecute(0, "open", fullPath.data(), arguments, 0, SW_SHOWNORMAL);

	std::this_thread::sleep_for(std::chrono::seconds(runTime));

	return closeTestApp(appExecutable, dumpPath);
}

bool basicLoadTest(std::string path, const char* libFile, int runTime, int id, const char* dumpPath)
{
	std::string fullPath = path + std::to_string(id) + "/";

	SetCurrentDirectory(fullPath.data());

	if (!mtt::load())
		return false;

	uint8_t hash[20];
	mtt::IoctlFunc(mtBI::MessageId::AddFromMetadata, "6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I", hash);

	std::this_thread::sleep_for(std::chrono::seconds(runTime));

	mtt::unload();

	return true;
}

void registerDumpCreate(const char* dumpPath, const char* appExecutable)
{
	HKEY hKey;
	LPCTSTR sk = TEXT("SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps");

	LONG openRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, sk, 0, KEY_ALL_ACCESS, &hKey);

	if (openRes == ERROR_SUCCESS)
	{
		HKEY elhKey;
		RegCreateKey(hKey, appExecutable, &elhKey);
		openRes = RegSetValueExA(elhKey, "DumpFolder", 0, REG_SZ, (const BYTE*)dumpPath, (DWORD)strlen(dumpPath) + 1);
		RegCloseKey(elhKey);

		RegCreateKey(hKey, "mtTorrentTest.exe", &elhKey);
		openRes = RegSetValueExA(elhKey, "DumpFolder", 0, REG_SZ, (const BYTE*)dumpPath, (DWORD)strlen(dumpPath) + 1);
		RegCloseKey(elhKey);

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
		RegDeleteKeyEx(hKey, appExecutable, KEY_WOW64_64KEY, 0);
		RegDeleteKeyEx(hKey, "mtTorrentTest.exe", KEY_WOW64_64KEY, 0);
		RegCloseKey(hKey);
	}
}

int main(int argc, char** argv)
{
	std::string testCase = "app";
	int testCount = 10;
	std::string testPath = "E:/tests/";
	std::string appArguments = "6QBN6XVGKV7CWOT5QXKDYWF3LIMUVK4I";
	int runTime = 10;
	bool sameInstance = true;

	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (i + 1 < argc)
			{
				if (strcmp(argv[i], "case") == 0)
				{
					testCase = argv[i + 1];
				}
				else if (strcmp(argv[i], "count") == 0)
				{
					testCount = std::stoi(argv[i + 1]);
				}
				else if (strcmp(argv[i], "path") == 0)
				{
					testPath = argv[i + 1];
				}
				else if (strcmp(argv[i], "runTime") == 0)
				{
					runTime = std::stoi(argv[i + 1]);
				}
				else if (strcmp(argv[i], "sameInstance") == 0)
				{
					sameInstance = argv[i + 1][0] != '0';
				}
				else if (strcmp(argv[i], "appArguments") == 0)
				{
					appArguments = argv[i + 1];
				}
				else
					continue;

				i++;
			}
		}
	}

	const char* dumpPath = testPath.data();
	const char* appFolder = "mtTorrent";
	const char* libFile = "mtTorrent.dll";
	const char* appExecutable = "mtTorrent.exe";

	std::cout << "Test case: " << testCase << "\n";
	std::cout << "Test run time: " << runTime << "\n";
	std::cout << "Tests count: " << testCount << "\n";
	std::cout << "Same instance: " << sameInstance << "\n";

	std::cout << "Preparing folders...\n";
	cleanTestFolder(testPath);
	copyTestFolder(testPath, appFolder, sameInstance ? 1 : testCount);

	registerDumpCreate(dumpPath, appExecutable);

	int i = 0;
	for (; i < testCount; i++)
	{
		std::cout << "Running test " << i << "\n";

		int instanceId = sameInstance ? 0 : i;

		if (testCase == "all" || testCase == "basicLoad")
		{
			if (!basicLoadTest(testPath, libFile, runTime, instanceId, dumpPath))
				break;
		}

		if (testCase == "all" || testCase == "app")
		{
			if (!runApplicationTest(testPath, appExecutable, appArguments.empty() ? nullptr : appArguments.data(), runTime, instanceId, dumpPath))
				break;
		}
	}

	unregisterDumpCreate(appExecutable);

	if (i < testCount)
		std::cout << "Testing finished prematurely\n";
	else
		std::cout << "Testing finished\n\n";

	return 0;
}