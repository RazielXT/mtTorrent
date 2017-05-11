#pragma once
#include "include\cef_v8.h"
#include <future>

class MyV8Handler : public CefV8Handler {
public:
	MyV8Handler(CefRefPtr<CefBrowser> b) : browser(b) {}

	CefRefPtr<CefBrowser> browser;

	struct LoadedV8Func
	{
		CefRefPtr<CefV8Value> func;
		CefRefPtr<CefV8Context> ctx;
	};
	std::map<std::string, LoadedV8Func> functions;

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE 
	{
		if (name == "requestAppend") 
		{
			sendIpc();
			// Return my string value.
			//retval = CefV8Value::CreateString("My Value!");
			return true;
		}
		if (name == "register") 
		{
			if (arguments.size() == 1 && arguments[0]->IsFunction()) 
			{
				functions[arguments[0]->GetFunctionName().ToString()] = { arguments[0] , CefV8Context::GetCurrentContext() };
				return true;
			}
		}

		// Function does not exist.
		return false;
	}

	void sendIpc()
	{
		// Create the message object.
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("appendRequest");

		// Retrieve the argument list object.
		CefRefPtr<CefListValue> args = msg->GetArgumentList();

		// Populate the argument values.
		args->SetString(0, "request");

		// Send the process message to the render process.
		// Use PID_BROWSER instead when sending a message to the browser process.
		browser->SendProcessMessage(PID_BROWSER, msg);
	}

	bool executeFunc(const char* funcName, CefString str)
	{
		auto it = functions.find(funcName);
		if (it == functions.end())
			return false;

		auto& f = it->second;

		CefV8ValueList args;
		if(!str.empty())
			args.push_back(CefV8Value::CreateString(str));

		CefRefPtr<CefV8Value> retval;
		CefRefPtr<CefV8Exception> exception;
		if (f.func->ExecuteFunctionWithContext(f.ctx, NULL, args)) {
			if (exception.get()) 
			{
				return false;
			}
			else 
			{
				return true;
			}
		}

		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(MyV8Handler);
};