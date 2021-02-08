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

	connect(ui->cbSources, (void (QComboBox::*)(int)) & QComboBox::currentIndexChanged, this, [this] (int) {
		const auto info	= selectedDeviceInfo();
		displayDeviceInfo(info);
		ui->sbToneFrequency->setMaximum(info.preferredFormat().sampleRate() / 2);
	});

	{
		for (const auto& info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
		{
			if (const auto realm = info.realm(); realm != "default")
				ui->cbSources->addItem(info.deviceName() + " - " + realm, deviceInfoId(info));
		}
	}

	connect(ui->btnPlay, &QPushButton::clicked, this, [this] {
		const auto deviceInfo = selectedDeviceInfo();
		auto fmt = deviceInfo.preferredFormat();
		assert_r(fmt.codec() == "audio/pcm");
		//assert_and_return_r(deviceInfo.isFormatSupported(fmt), );
		_audio.playTone(ui->sbToneFrequency->value(), 10000, deviceInfo, fmt, Channel::L);
	});

	connect(ui->btnStopAudio, &QPushButton::clicked, this, [this] {
		_audio.stopPlayback();
	});
}

CMainWindow::~CMainWindow()
{
	delete ui;
}

void CMainWindow::setupInfoTab()
{

}

static QString sampleTypeName(QAudioFormat::SampleType st)
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

static QString byteOrderName(QAudioFormat::Endian endian)
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
