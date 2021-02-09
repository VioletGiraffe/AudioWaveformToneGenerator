#pragma once
#include "audio/caudiooutput.h"

DISABLE_COMPILER_WARNINGS
#include <QMainWindow>
RESTORE_COMPILER_WARNINGS

class QAudioDeviceInfo;

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

	void displayDeviceInfo(const QAudioDeviceInfo& info);
	QAudioDeviceInfo deviceInfoById(uint devInfoId);
	QAudioDeviceInfo selectedDeviceInfo();

private:
	Ui::CMainWindow *ui;

	CAudioOutput _audio;
};
