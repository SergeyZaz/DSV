#include <QApplication>
#include "zmainwindow.h"
#include "windows.h"

int main(int argc, char *argv[])
{
#ifdef _WIN32
#ifdef _DEBUG
				AllocConsole();
				freopen("CONOUT$","wt", stdout);
				freopen("CONOUT$","wt", stderr);
				freopen("CONIN$","rt", stdin);
				GetStdHandle(STD_OUTPUT_HANDLE);
#endif
#endif
				
    QApplication app(argc, argv);

    ZMainWindow mainWin;
    mainWin.showMaximized();

    return app.exec();
}
