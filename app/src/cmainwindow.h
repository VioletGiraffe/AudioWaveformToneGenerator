#pragma once

#include <QMainWindow>

namespace Ui {
class CMainWindow;
}

class CMainWindow : public QMainWindow
{
public:
	explicit CMainWindow(QWidget *parent = nullptr);
	~CMainWindow();

private:
	void setupInfoTab();
	void setupSignalTab();

	void outputDeviceSelected(int deviceIndex);

private:
	Ui::CMainWindow *ui;
};
