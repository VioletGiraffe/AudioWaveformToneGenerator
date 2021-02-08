#include "caudiooutput.h"
#include "assert/advanced_assert.h"

#include <numbers>


void CAudioOutput::playTone(const uint32_t hz, const uint32_t ms, const QAudioDeviceInfo& device, QAudioFormat format, const Channel ch, const float amplitude)
{
	stopPlayback();

	const uint32_t nSamples = format.sampleRate() * ms / 1000;

	assert_and_return_r(amplitude >= 0.0f && amplitude <= 1.0f, );

	assert_r(format.sampleSize() == 32);
	const auto nChannels = format.channelCount();
	QByteArray& buffer = _data.buffer();
	buffer.resize(nSamples * (format.sampleSize() / 8) * nChannels);

	format.setSampleType(QAudioFormat::Float);

	{
		static_assert(sizeof(float) == 4);

		std::byte* data = reinterpret_cast<std::byte*>(buffer.data());
		const float sampleRateF = static_cast<float>(format.sampleRate());
		const float omega = 2.0f * std::numbers::pi_v<float> * static_cast<float>(hz) / sampleRateF;
		for (uint32_t i = 0; i < nSamples; ++i)
		{
			for (int c = 0; c < nChannels; ++c)
			{
				float sample = 0.0f;
				if (c == static_cast<int>(ch))
					sample = amplitude * std::sinf(omega * i);

				std::memcpy(data + (i * nChannels + c) * sizeof(float), &sample, sizeof(float));
			}
		}
	}

	assert_and_return_r(_data.open(QBuffer::ReadOnly), );
	assert_r(_data.pos() == 0);

	_output = std::make_unique<QAudioOutput>(device, format);
	_output->start(&_data);
}

void CAudioOutput::stopPlayback()
{
	if (_output)
	{
		_output->stop();
		_output.reset();
	}
}
