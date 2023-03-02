#include "MainWindow.h"
#include <QtWidgets/QApplication>

#ifdef _WIN32
#include <Shlwapi.h>

void initEnvironment()
{
	wchar_t pathBuffer[256];
	if (GetModuleFileName(NULL, pathBuffer, 256))
	{
		PathRemoveFileSpec(pathBuffer);
		SetCurrentDirectory(pathBuffer);
	}
}
#else
void initEnvironment()
{
}
#endif

int main(int argc, char *argv[])
{
	QStringList params;
	for (int i = 1; i < argc; i++)
		params.append(argv[i]);
	
	if (CrossProcess::TrySendParams(params))
		return 0;

    initEnvironment();

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/new/res/icons/mtt.ico"));

    MainWindow w(params);
    w.show();
    return a.exec();
}
