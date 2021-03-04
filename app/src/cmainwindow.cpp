#include "cmainwindow.h"

#include "assert/advanced_assert.h"
#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include "ui_cmainwindow.h"

#include <QDebug>
#include <QLineSeries>
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

	_audio.setChannelIndex(ui->cbChannel->currentData().toUInt());
	_audio.setFrequency(static_cast<float>(ui->sbToneFrequency->value()));

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
	connect(ui->btnPlay, &QPushButton::clicked, this, &CMainWindow::play);

	// Stop
	ui->btnStopAudio->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
	ui->btnStopAudio->setText({});
	connect(ui->btnStopAudio, &QPushButton::clicked, this, &CMainWindow::stopPlayback);

	setupChart();
}

CMainWindow::~CMainWindow()
{
	delete ui;
}

void CMainWindow::setupChart()
{
	_chart.legend()->hide();
	//_chart.setTitle("Simple line chart example");

	ui->chartWidget->setChart(&_chart);
	ui->chartWidget->setRenderHint(QPainter::Antialiasing);

	connect(&_chartUpdateTimer, &QTimer::timeout, this, [this] {
		ui->chartWidget->setUpdatesEnabled(false);
		if (auto oldSeries = _chart.series(); !oldSeries.empty())
		{
			for (auto* s : oldSeries)
				s->deleteLater();

			_chart.removeAllSeries();
		}

		const auto samples = _audio.currentSamplesBuffer();

		for (size_t c = 0; c < samples.height(); ++c)
		{
			auto* series = new QLineSeries;
			//seriesForChannel.emplace_back(series);

			for (size_t i = 0, n = samples.width(); i < n; ++i)
			{
				series->append(static_cast<qreal>(i), samples[c][i]);
			}

			_chart.addSeries(series);
		}

		_chart.createDefaultAxes();
		_chart.axisY()->setRange(-1.0, +1.0);
		_chart.axisX()->setRange(0, samples.width());
		ui->chartWidget->setUpdatesEnabled(true);
	});
}

void CMainWindow::newDeviceSelected()
{
	_audio.stopPlayback();

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

void CMainWindow::play()
{
	const auto deviceInfo = selectedDeviceInfo();
	auto fmt = _audio.mixFormat(deviceInfo.id);
	assert_r(fmt.sampleFormat == AudioFormat::Float);
	assert_and_return_r(ui->cbChannel->currentIndex() >= 0, );
	_audio.playTone(ui->cbSources->currentData().toString().toStdWString());

	_chartUpdateTimer.start(100);
}

void CMainWindow::stopPlayback()
{
	_chartUpdateTimer.stop();
	_audio.stopPlayback();
}
