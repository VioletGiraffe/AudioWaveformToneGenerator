#include "cmainwindow.h"
#include "assert/advanced_assert.h"
#include "system/win_utils.hpp"

#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
	AdvancedAssert::setLoggingFunc([](const char* msg) {
		qInfo() << msg;
	});

	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	QApplication app(argc, argv);
	CO_INIT_HELPER(COINIT_APARTMENTTHREADED);

	CMainWindow wnd;
	wnd.show();
	return app.exec();
}
