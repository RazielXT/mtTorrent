#pragma once
#include "include\cef_v8.h"

class MyV8Handler : public CefV8Handler {
public:
	MyV8Handler() {}

	virtual bool Execute(const CefString& name,
		CefRefPtr<CefV8Value> object,
		const CefV8ValueList& arguments,
		CefRefPtr<CefV8Value>& retval,
		CefString& exception) OVERRIDE {
		if (name == "myfunc") {
			// Return my string value.
			retval = CefV8Value::CreateString("My Value!");
			return true;
		}

		// Function does not exist.
		return false;
	}

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(MyV8Handler);
};