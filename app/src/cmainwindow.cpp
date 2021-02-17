#include "cmainwindow.h"

#include "assert/advanced_assert.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include "ui_cmainwindow.h"

#include <QDebug>
RESTORE_COMPILER_WARNINGS

CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CMainWindow)
{
	ui->setupUi(this);

	connect(ui->cbSources, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, &CMainWindow::newDeviceSelected);

	// Scan all the available audio output devices and add all the appropriate ones to the combobox.
	for (const auto& info : _audio.devices())
		ui->cbSources->addItem(QString::fromStdWString(info.friendlyName), QString::fromStdWString(info.id));

	// Find and select the AVR if there is one.
	for (int i = 0; i < ui->cbSources->count(); ++i)
	{
		if (ui->cbSources->itemText(i).contains("AVR"))
		{
			ui->cbSources->setCurrentIndex(i);
			break;
		}
	}

	ui->btnPlay->setEnabled(ui->cbSources->count() > 0);

	// Handle parameter changes on the fly.
	connect(ui->cbChannel, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, [this]() {
		_audio.setChannelIndex(ui->cbChannel->currentData().toUInt());
	});

	connect(ui->sbToneFrequency, (void (QSpinBox::*)(int))&QSpinBox::valueChanged, this, [this](int value) {
		_audio.setFrequency(static_cast<float>(value));
	});

	// Play
	ui->btnPlay->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
	ui->btnPlay->setText({});
	connect(ui->btnPlay, &QPushButton::clicked, this, [this] {
		const auto deviceInfo = selectedDeviceInfo();
		auto fmt = _audio.mixFormat(deviceInfo.id);
		assert_r(fmt.sampleFormat == AudioFormat::Float);
		assert_and_return_r(ui->cbChannel->currentIndex() >= 0, );
		_audio.playTone(ui->cbSources->currentData().toString().toStdWString());
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


CAudioOutputWasapi::DeviceInfo CMainWindow::selectedDeviceInfo()
{
	if (ui->cbSources->currentIndex() < 0)
		return {};

	const auto selectedId = ui->cbSources->currentData().toString().toStdWString();
	for (auto&& deviceInfo : _audio.devices())
	{
		if (deviceInfo.id == selectedId)
			return deviceInfo;
	}

	return {};
}
