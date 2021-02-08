#pragma once

#include "compiler/compiler_warnings_control.h"

DISABLE_COMPILER_WARNINGS
#include <QAudioOutput>
#include <QBuffer>
RESTORE_COMPILER_WARNINGS

#include <memory>

enum class Channel {
	L,
	R,
	C,
	SurroundL,
	SurroundR,
	LFE
};

class CAudioOutput
{
public:
	void playTone(uint32_t hz, uint32_t ms, const QAudioDeviceInfo& device, QAudioFormat format, Channel ch, float amplitude = 1.0f);
	void stopPlayback();

private:
	QBuffer _data;
	std::unique_ptr<QAudioOutput> _output;
};
