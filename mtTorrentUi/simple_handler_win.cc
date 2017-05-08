// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "tests/cefsimple/simple_handler.h"

#include <string>
#include <windows.h>

#include "include/cef_browser.h"
#include <thread>

void SimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                                        const CefString& title) {
  CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
  SetWindowText(hwnd, std::wstring(title).c_str());
}

LPCWSTR kParentWndProc = L"CefParentWndProc";

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC hParentWndProc = reinterpret_cast<WNDPROC>(::GetPropW(hWnd, kParentWndProc));

	// Handle your messages
	if (uMsg == WM_NCHITTEST)
	{
		LRESULT hit = DefWindowProc(hWnd, uMsg, wParam, lParam);
		if (hit == HTCLIENT) 
		{
			hit = HTCAPTION;
		}
		return hit;
	}

	return CallWindowProc(hParentWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK SubclassedWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC hParentWndProc = reinterpret_cast<WNDPROC>(::GetPropW(hWnd, kParentWndProc));

	// Handle your messages
	if (uMsg == WM_NCHITTEST)
	{
		auto hit = CallWindowProc(hParentWndProc, hWnd, uMsg, wParam, lParam);
		return HTTRANSPARENT;// hit == HTCLIENT ? HTCAPTION : hit;
	}

	return CallWindowProc(hParentWndProc, hWnd, uMsg, wParam, lParam);
}

void SubclassWindow(HWND hWnd, HRGN hRegion) {
	HANDLE hParentWndProc = ::GetPropW(hWnd, kParentWndProc);
	if (hParentWndProc) {
		return;
	}

	SetLastError(0);
	LONG_PTR hOldWndProc = SetWindowLongPtr(
		hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassedWindowProc));
	if (hOldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
		return;
	}

	::SetPropW(hWnd, kParentWndProc, reinterpret_cast<HANDLE>(hOldWndProc));
	//::SetPropW(hWnd, kDraggableRegion, reinterpret_cast<HANDLE>(hRegion));
}

BOOL CALLBACK SubclassWindowsProc2(HWND hwnd, LPARAM lParam) {
	SubclassWindow(hwnd, reinterpret_cast<HRGN>(lParam));
	return TRUE;

}
BOOL CALLBACK SubclassWindowsProc(HWND hwnd, LPARAM lParam) {

	SubclassWindowsProc2(hwnd, NULL);
	//std::thread([hwnd]() { Sleep(2000);  ::EnumChildWindows(hwnd, SubclassWindowsProc2, NULL); }).detach();
	
	return TRUE;
}

void SimpleHandler::EnableDrag(CefRefPtr<CefBrowser> browser)
{
	HWND windowHnd = browser->GetHost()->GetWindowHandle();
	::EnumChildWindows(windowHnd, SubclassWindowsProc, NULL);


	SetLastError(0);
	LONG_PTR hOldWndProc = SetWindowLongPtr(windowHnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MainWindowProc));
	if (hOldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
		return;
	}
	::SetPropW(windowHnd, kParentWndProc, reinterpret_cast<HANDLE>(hOldWndProc));
}