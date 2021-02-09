#include "cmainwindow.h"
#include "assert/advanced_assert.h"

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
	CMainWindow wnd;
	wnd.show();
	return app.exec();
}
