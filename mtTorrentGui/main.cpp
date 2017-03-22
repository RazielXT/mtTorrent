#include "mtTorrentGui.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    mtTorrentGui w;
    w.show();
    return a.exec();
}
