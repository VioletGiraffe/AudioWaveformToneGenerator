#include <QApplication>
#include "cmainwindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	CMainWindow wnd;
	wnd.show();
	return app.exec();
}
