#include "cmainwindow.h"
#include "ui_cmainwindow.h"

#include "assert/advanced_assert.h"

#include <QAudioDeviceInfo>
#include <QAudioOutput>
#include <QDebug>

CMainWindow::CMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::CMainWindow)
{
	ui->setupUi(this);

	for (const auto& info: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
	{
		ui->cbSources->addItem(info.deviceName());
	}

	connect(ui->cbSources, (void (QComboBox::*)(int))&QComboBox::activated, this, &CMainWindow::outputDeviceSelected);
	outputDeviceSelected(ui->cbSources->currentIndex());

//	QAudioFormat format;
//	// Set up the format, eg.
//	format.setSampleRate(8000);
//	format.setChannelCount(1);
//	format.setSampleSize(8);
//	format.setCodec("audio/pcm");
//	format.setByteOrder(QAudioFormat::LittleEndian);
//	format.setSampleType(QAudioFormat::UnSignedInt);

//	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
//	if (!info.isFormatSupported(format)) {
//		qWarning() << "Raw audio format not supported by backend, cannot play audio.";
//		return;
//	}

//	audio = new QAudioOutput(format, this);
//	connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
//	audio->start(&sourceFile);
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

void CMainWindow::outputDeviceSelected(int deviceIndex)
{
	const auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
	ui->infoText->clear();
	assert_and_return_r(deviceIndex < devices.size(), );

	const auto& info = devices[deviceIndex];
	QString infoText = "Byte orders:\n";
	for (const auto value: info.supportedByteOrders())
		infoText += '\t' + byteOrderName(value) + '\n';

	infoText += "Channel counts:\n";
	for (const auto value: info.supportedChannelCounts())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample rates:\n";
	for (const auto value: info.supportedSampleRates())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample sizes:\n";
	for (const auto value: info.supportedSampleSizes())
		infoText += '\t' + QString::number(value) + '\n';

	infoText += "Sample types:\n";
	for (const auto value: info.supportedSampleTypes())
		infoText += '\t' + sampleTypeName(value) + '\n';

	infoText += "Codecs:\n";
	for (const auto& value: info.supportedCodecs())
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
