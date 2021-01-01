#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

#include "JsonDiagnostics.h"
#include <fstream>
#include <algorithm>

int main()
{
	wchar_t pathBuffer[256];
	if (GetModuleFileName(NULL, pathBuffer, 256))
	{
		PathRemoveFileSpec(pathBuffer);
		SetCurrentDirectory(pathBuffer);
	}

	JsonDiagnostics diagnostics;
	auto js = diagnostics.generateJson();

	std::vector<size_t> quotes;
	for (size_t i = 0; i < js.size(); i++)
	{
		if (js[i] == '"')
			quotes.push_back(i);
	}
	for (auto it = quotes.rbegin(); it != quotes.rend(); it++)
	{
		js.insert(js.begin() + *it, '\\');
	}

	std::ofstream jsData("report.js");
	jsData << "var input = \"";
	jsData << js;
	jsData << "\";";

	return 0;
}