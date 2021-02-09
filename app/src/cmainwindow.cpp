#include "cmainwindow.h"

#include "assert/advanced_assert.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include "ui_cmainwindow.h"

#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QDebug>
#include <QHash>
RESTORE_COMPILER_WARNINGS

static uint deviceInfoId(const QAudioDeviceInfo& info)
{
	return qHash(info.deviceName()) ^ qHash(info.realm());
}

CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CMainWindow)
{
	ui->setupUi(this);

	connect(ui->cbSources, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, &CMainWindow::newDeviceSelected);

	// Scan all the available audio output devices and add all the appropriate ones to the combobox.
	for (const auto& info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
	{
		if (const auto realm = info.realm(); realm != "default")
			ui->cbSources->addItem(info.deviceName() + " - " + realm, deviceInfoId(info));
	}

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
		auto fmt = deviceInfo.preferredFormat();
		assert_r(fmt.codec() == "audio/pcm");
		//assert_and_return_r(deviceInfo.isFormatSupported(fmt), );
		assert_and_return_r(ui->cbChannel->currentIndex() >= 0, );
		_audio.playTone(ui->sbToneFrequency->value(), 10000, deviceInfo, fmt, ui->cbChannel->currentData().toInt());
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

static QString sampleTypeName(const QAudioFormat::SampleType st)
{
	switch (st)
	{
	case QAudioFormat::SignedInt:
		return "signed int";
	case QAudioFormat::UnSignedInt:
		return "unsigned int";
	case QAudioFormat::Float:
		return "float";
	case QAudioFormat::Unknown:
		return "unknown";
	default:
		return "invalid!";
	}
}

static QString byteOrderName(const QAudioFormat::Endian endian)
{
	switch (endian)
	{
	case QAudioFormat::BigEndian:
		return "big endian";
	case QAudioFormat::LittleEndian:
		return "little endian";
	default:
		return "invalid!";
	}
}

void CMainWindow::newDeviceSelected()
{
	const auto info = selectedDeviceInfo();
	displayDeviceInfo(info);

	const auto fmt = info.preferredFormat();
	ui->sbToneFrequency->setMaximum(fmt.sampleRate() / 2);

	ui->cbChannel->clear();
	for (auto&& ch : Channel::fromFormat(fmt))
		ui->cbChannel->addItem(ch.name, ch.index);
	ui->cbChannel->setCurrentIndex(0);
}

void CMainWindow::displayDeviceInfo(const QAudioDeviceInfo& info)
{
	if (info.isNull())
	{
		ui->infoText->clear();
		return;
	}

	QString infoText = "Byte orders:\n";
	for (const auto value : info.supportedByteOrders())
		infoText += '\t' + byteOrderName(value) + '\n';

	infoText += "Channel counts:\n";
	for (const auto value : info.supportedChannelCounts())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample rates:\n";
	for (const auto value : info.supportedSampleRates())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample sizes:\n";
	for (const auto value : info.supportedSampleSizes())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample types:\n";
	for (const auto value : info.supportedSampleTypes())
		infoText += '\t' + sampleTypeName(value) + '\n';

	infoText += "Codecs:\n";
	for (const auto& value : info.supportedCodecs())
		infoText += '\t' + value + '\n';

	infoText = infoText + "Preferred format: "
		+ byteOrderName(info.preferredFormat().byteOrder()) + '\t'
		+ QString::number(info.preferredFormat().channelCount()) + '\t'
		+ QString::number(info.preferredFormat().sampleRate()) + '\t'
		+ QString::number(info.preferredFormat().sampleSize()) + '\t'
		+ sampleTypeName(info.preferredFormat().sampleType()) + '\t'
		+ info.preferredFormat().codec();

	ui->infoText->setPlainText(infoText);
}

QAudioDeviceInfo CMainWindow::deviceInfoById(uint devInfoId)
{
	for (auto&& deviceInfo : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
	{
		if (deviceInfoId(deviceInfo) == devInfoId)
			return deviceInfo;
	}

	return {};
}

QAudioDeviceInfo CMainWindow::selectedDeviceInfo()
{
	return ui->cbSources->currentIndex() != -1 ? deviceInfoById(ui->cbSources->currentData().toUInt()) : QAudioDeviceInfo{};
}
