#include "MainForm.h"
#include <windows.h>
#include <vcclr.h>

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
void FormsMain(HWND* hwnd, HWND* parent)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	GuiLite::MainForm form;

	Application::Run(%form);
}

int Main()
{
	FormsMain(0, 0);

	return 0;
}