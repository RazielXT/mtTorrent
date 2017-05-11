// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "simple_app.h"

#include <string>

#include "simple_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "MyV8Handler.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {
  }

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = NULL;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

}  // namespace

SimpleApp::SimpleApp() {
}

void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

#if defined(OS_WIN) || defined(OS_LINUX)
  // Create the browser using the Views framework if "--use-views" is specified
  // via the command-line. Otherwise, create the browser using the native
  // platform framework. The Views framework is currently only supported on
  // Windows and Linux.
  const bool use_views = command_line->HasSwitch("use-views");
#else
  const bool use_views = false;
#endif

  // SimpleHandler implements browser-level callbacks.
  CefRefPtr<SimpleHandler> handler(new SimpleHandler(use_views));

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;

  std::string url = "file://D:/Hobby/mtTorrent/Data/index.html";

  if (use_views) {
    // Create the BrowserView.
    CefRefPtr<CefBrowserView> browser_view = CefBrowserView::CreateBrowserView(handler, url, browser_settings, NULL, NULL);

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new SimpleWindowDelegate(browser_view));
  } 
  else
  {
    // Information used when creating the native window.
    CefWindowInfo window_info;
    window_info.SetAsPopup(NULL, "cefsimple");

	window_info.style = 0x14CF0000;
	window_info.ex_style = 0x80100;

    // Create the first browser window.
    CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, NULL);
  }
}

//from browser to renderer
bool SimpleApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,	CefProcessId source_process,	CefRefPtr<CefProcessMessage> message) 
{
	const std::string& message_name = message->GetName();
	auto args = message->GetArgumentList();

	if (message_name == "appendResponse")
	{
		v8Handler->executeFunc("doAppend", args->GetString(0));

		return true;
	}

	return false;
}

void SimpleApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	if (v8Handler)
	{
		for (auto it = v8Handler->functions.begin(); it != v8Handler->functions.end();)
			if (it->second.ctx->IsSame(context))
				v8Handler->functions.erase(it++);
			else
				++it;
	}
}

void SimpleApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	CEF_REQUIRE_RENDERER_THREAD();

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();

	// Create an instance of my CefV8Handler object.
	v8Handler = new MyV8Handler(browser);

	// Create the "myfunc" function.
	CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("requestAppend", v8Handler);
	// Add the "myfunc" function to the "window" object.
	object->SetValue("requestAppend", func, V8_PROPERTY_ATTRIBUTE_NONE);


	object->SetValue("register", CefV8Value::CreateFunction("register", v8Handler), V8_PROPERTY_ATTRIBUTE_NONE);
}