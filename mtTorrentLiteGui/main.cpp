#include "MainForm.h"
#include "Utils/Utils.h"
#include "CrossProcessComm.h"

using namespace System::Windows::Forms;

AppCore core;

[STAThread]
void FormsMain(cli::array<System::String ^>^ args)
{
	if (Environment::OSVersion->Version->Major >= 6)
		SetProcessDPIAware();

	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	core.init();
	GuiLite::MainForm form;

	if (args->Length > 0)
		core.processProgramArgument(args[0]);

	CrossProcess::StartListening(core);
	Application::Run(%form);

	core.deinit();
}

int Main(cli::array<System::String ^>^ args)
{
	if (HWND h = GetExistingMainWindow())
		CrossProcess::SendArguments(h, args);
	else
		FormsMain(args);

	return 0;
}