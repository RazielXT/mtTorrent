#pragma once
#include <windows.h>
#include <string>
#include "AppCore.h"

using namespace System;
using namespace System::Windows::Forms;

const int WM_MTT_ARGUMENT = WM_USER + 100;
const int MTT_ARGUMENT_START = 1;
const int MTT_ARGUMENT_END = 2;
const int MTT_SHOW_WINDOW = 3;
std::string argumentBuffer;

public ref class MyMessageFilter : System::Windows::Forms::IMessageFilter
{
public:

	MyMessageFilter(AppCore& c) : core(c) { }

	virtual bool PreFilterMessage(Message% objMessage)
	{
		if (objMessage.Msg == WM_MTT_ARGUMENT)
		{
			if (objMessage.LParam == (IntPtr)MTT_SHOW_WINDOW)
				core.showWindow();
			else if (objMessage.LParam == (IntPtr)MTT_ARGUMENT_END)
				core.processProgramArgument(gcnew System::String(argumentBuffer.data()));
			else
			{
				if (objMessage.LParam == (IntPtr)MTT_ARGUMENT_START)
					argumentBuffer.clear();

				argumentBuffer.push_back((char)objMessage.WParam.ToInt32());
			}

			return true;
		}

		return false;
	}

	AppCore& core;
};


namespace CrossProcess
{
	//read arguments from duplicate process instance
	void StartListening(AppCore& core)
	{
		MyMessageFilter^ filter = gcnew MyMessageFilter(core);
		Application::AddMessageFilter(filter);
	}

	//send arguments to existing process instance
	void SendArguments(HWND hMainWindow, cli::array<System::String^>^ args)
	{
		if (args->Length > 0)
		{
			auto arg = args[0];

			for (int i = 0; i < arg->Length; i++)
			{
				PostMessage(hMainWindow, WM_MTT_ARGUMENT, arg[i], i == 0 ? MTT_ARGUMENT_START : 0);
			}

			PostMessage(hMainWindow, WM_MTT_ARGUMENT, 0, MTT_ARGUMENT_END);
		}

		PostMessage(hMainWindow, WM_MTT_ARGUMENT, 0, MTT_SHOW_WINDOW);
	}
};