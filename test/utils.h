#pragma once

#include <string>
#include <windows.h>

void cleanTestFolder(std::string path);
bool copyTestFolder(std::string path, const char* folderName, int count);
HWND find_main_window(unsigned long process_id);
DWORD GetProcessByName(std::string name);
bool createDump(DWORD process, std::string path, const char* name);