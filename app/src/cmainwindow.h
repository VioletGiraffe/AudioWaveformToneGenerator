#pragma once
#include "audio/caudiooutputwasapi.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include <QMainWindow>
#include <QChart>
#include <QTimer>
RESTORE_COMPILER_WARNINGS

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class CMainWindow;
}

class CMainWindow final : public QMainWindow
{
public:
	explicit CMainWindow(QWidget *parent = nullptr);
	~CMainWindow();

private:
	void setupChart();

	void newDeviceSelected();

	void displayDeviceInfo(const CAudioOutputWasapi::DeviceInfo& info);
	CAudioOutputWasapi::DeviceInfo selectedDeviceInfo();

// Slots
	void play();
	void stopPlayback();

private:
	Ui::CMainWindow *ui;

	CAudioOutputWasapi _audio;

	QChart _chart;

	QTimer _chartUpdateTimer;
};
