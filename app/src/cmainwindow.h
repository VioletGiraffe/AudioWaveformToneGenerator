#pragma once
#include "audio/caudiooutputwasapi.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include <QMainWindow>
RESTORE_COMPILER_WARNINGS

namespace Ui {
class CMainWindow;
}

class CMainWindow : public QMainWindow
{
public:
	explicit CMainWindow(QWidget *parent = nullptr);
	~CMainWindow();

private:
	void newDeviceSelected();

	void displayDeviceInfo(const CAudioOutputWasapi::DeviceInfo& info);
	CAudioOutputWasapi::DeviceInfo selectedDeviceInfo();

private:
	Ui::CMainWindow *ui;

	CAudioOutputWasapi _audio;
};
