#pragma once

#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include <QAudioOutput>
#include <QBuffer>
#include <QString>
RESTORE_COMPILER_WARNINGS

#include <memory>
#include <vector>

struct Channel final {
	static std::vector<Channel> fromFormat(const QAudioFormat& fmt);

	const QString name;
	const int index;
};

class CAudioOutput
{
public:
	bool playTone(uint32_t hz, uint32_t ms, const QAudioDeviceInfo& device, QAudioFormat format, int channelIndex, float amplitude = 1.0f);
	void stopPlayback();

private:
	QBuffer _data;
	std::unique_ptr<QAudioOutput> _output;
};
