#include "cmainwindow.h"

#include "assert/advanced_assert.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include "ui_cmainwindow.h"

#include <QDebug>
#include <QHash>
RESTORE_COMPILER_WARNINGS

static uint deviceInfoId(const CAudioOutputWasapi::DeviceInfo& info)
{
	return ::qHash(QString::fromStdWString(info.id));
}

CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CMainWindow)
{
	ui->setupUi(this);

	connect(ui->cbSources, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, &CMainWindow::newDeviceSelected);

	// Scan all the available audio output devices and add all the appropriate ones to the combobox.
	for (const auto& info : _audio.devices())
		ui->cbSources->addItem(QString::fromStdWString(info.friendlyName), deviceInfoId(info));

	// Find and select the AVR if there is one.
	for (int i = 0; i < ui->cbSources->count(); ++i)
	{
		if (ui->cbSources->itemText(i).contains("AVR"))
		{
			ui->cbSources->setCurrentIndex(i);
			break;
		}
	}

	// Play
	ui->btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	ui->btnPlay->setText({});
	connect(ui->btnPlay, &QPushButton::clicked, this, [this] {
		const auto deviceInfo = selectedDeviceInfo();
		auto fmt = _audio.mixFormat(deviceInfo.id);
		assert_r(fmt.sampleFormat == AudioFormat::Float);
		assert_and_return_r(ui->cbChannel->currentIndex() >= 0, );
		_audio.playTone(Signal{ static_cast<float>(ui->sbToneFrequency->value()), ui->cbChannel->currentData().toUInt()});
	});

	// Stop
	ui->btnStopAudio->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
	ui->btnStopAudio->setText({});
	connect(ui->btnStopAudio, &QPushButton::clicked, this, [this] {
		_audio.stopPlayback();
	});
}

CMainWindow::~CMainWindow()
{
	delete ui;
}

void CMainWindow::newDeviceSelected()
{
	const auto info = selectedDeviceInfo();
	displayDeviceInfo(info);

	const auto fmt = _audio.mixFormat(info.id);
	ui->sbToneFrequency->setMaximum(fmt.sampleRate / 2);

	ui->cbChannel->clear();
	for (const auto& ch: fmt.channels)
		ui->cbChannel->addItem(QString::fromStdString(ch.name), ch.index);
	ui->cbChannel->setCurrentIndex(0);
}

void CMainWindow::displayDeviceInfo(const CAudioOutputWasapi::DeviceInfo& info)
{
	const auto fmt = _audio.mixFormat(info.id);

	QString infoText = "Channel count: " + QString::number(fmt.channels.size()) + '\n';
	infoText += "Sample rate: " + QString::number(fmt.sampleRate) + '\n';
	infoText += "Sample size: " + QString::number(fmt.bitsPerSample) + '\n';
	infoText += "Sample type: " + QString{fmt.sampleFormat == AudioFormat::PCM ? "PCM" : "float"} + '\n';

	ui->infoText->setPlainText(infoText);
}

CAudioOutputWasapi::DeviceInfo CMainWindow::deviceInfoById(uint devInfoId)
{
	for (auto&& deviceInfo : _audio.devices())
	{
		if (deviceInfoId(deviceInfo) == devInfoId)
			return deviceInfo;
	}

	return {};
}

CAudioOutputWasapi::DeviceInfo CMainWindow::selectedDeviceInfo()
{
	return ui->cbSources->currentIndex() != -1 ? deviceInfoById(ui->cbSources->currentData().toUInt()) : CAudioOutputWasapi::DeviceInfo{};
}
