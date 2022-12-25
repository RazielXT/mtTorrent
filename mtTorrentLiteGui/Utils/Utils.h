#pragma once

#include <string>
#include <cstdint>
#include <windows.h>

std::string getUtf8String(System::String^ str);

System::String^ formatBytes(uint64_t bytes);
System::String^ formatBytesSpeed(uint64_t bytes);

System::String^ GetClipboardText();

void adjustGridRowsCount(System::Windows::Forms::DataGridView^ grid, int count);

void RegisterMagnetAssociation(long long parent);

HWND GetExistingMainWindow();