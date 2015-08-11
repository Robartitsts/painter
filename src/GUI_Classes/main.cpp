#include "stdafx.h"

#include "Painter.h"
#include "StartWindow.h"
#include <windows.h>



int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	//console window//
	AllocConsole();

	SetConsoleTitleA("Robot Artist v3 (8/11/15)");
	freopen("conin$", "r", stdin);
	freopen("conout$", "w", stdout);
	freopen("conout$", "w", stderr);
	//console window//

	//actual program//
	StartWindow w;
	w.show();
	w.raise();
	//actual program//

	return a.exec();
}
