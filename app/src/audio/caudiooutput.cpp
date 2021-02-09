#include "caudiooutput.h"
#include "assert/advanced_assert.h"
#include "math/math.hpp"

#include <cmath> // sinf
#include <cstring> // memcpy
#include <limits>
#include <numbers>

std::vector<Channel> Channel::fromFormat(const QAudioFormat& fmt)
{
	static constexpr const char* channelName[] {
		"Left",
		"Right",
		"Center",
		"LFE",
		"SurroundLeft",
		"SurroundRight",
		"SideLeft",
		"SideRight"
	};

	assert_and_return_r(fmt.channelCount() < std::size(channelName), {});

	std::vector<Channel> channels;
	for (int i = 0; i < fmt.channelCount(); ++i)
		channels.emplace_back(channelName[i], i);

	return channels;
}


bool CAudioOutput::playTone(const uint32_t hz, const uint32_t ms, const QAudioDeviceInfo& device, QAudioFormat format, const int channelIndex, const float amplitude)
{
	stopPlayback();

	const size_t nSamples = format.sampleRate() * ms / 1000;

	assert_and_return_r(amplitude >= 0.0f && amplitude <= 1.0f, false);

	assert_r(format.sampleSize() == 32);

	format.setSampleType(QAudioFormat::Float);

	{
		static_assert(sizeof(float) == 4);

		const auto nChannels = static_cast<size_t>(format.channelCount());
		QByteArray& buffer = _data.buffer();

		const size_t bufferLength = nSamples * (format.sampleSize() / 8) * nChannels;
		assert_and_return_r(bufferLength < std::numeric_limits<int>::max() - 100, false);
		buffer.resize(static_cast<int>(bufferLength));

		std::byte* data = reinterpret_cast<std::byte*>(buffer.data());
		const float sampleRateF = static_cast<float>(format.sampleRate());
		const float omega = 2.0f * std::numbers::pi_v<float> * static_cast<float>(hz) / sampleRateF;
		for (size_t i = 0; i < nSamples; ++i)
		{
			for (size_t c = 0; c < nChannels; ++c)
			{
				int32_t sample = 0;
				if (c == channelIndex)
					sample = Math::floor<int32_t>(amplitude * (std::numeric_limits<int32_t>::max() - 1) * std::sin(omega * i));

				std::memcpy(data + (i * nChannels + c) * sizeof(sample), &sample, sizeof(sample));
			}
		}
	}

	//assert_and_return_r(_data.open(QBuffer::ReadOnly), false);
	//assert_r(_data.pos() == 0);

	_output = std::make_unique<QAudioOutput>(device, format);
	_output->start(&_data);
	return true;
}

void CAudioOutput::stopPlayback()
{
	if (_output)
	{
		_output->stop();
		_output.reset();
	}
}
