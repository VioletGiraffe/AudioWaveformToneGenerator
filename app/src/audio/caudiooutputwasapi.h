#pragma once

#include <array>
#include <atomic>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <utility>

struct Signal {
	float hz;
	size_t channelIndex;
};

struct ChannelInfo {
	std::string name;
	size_t index;
};

struct AudioFormat {
	std::vector<ChannelInfo> channels;

	uint32_t sampleRate = 0;
	enum {PCM, Float} sampleFormat;
	uint16_t bitsPerSample = 0;
};

class CAudioOutputWasapi final
{
public:
	~CAudioOutputWasapi();

	bool playTone(Signal signal);
	void stopPlayback();

	[[nodiscard]] AudioFormat mixFormat(const std::wstring& deviceId) const noexcept;

	struct DeviceInfo {
		const std::wstring id;
		const std::wstring friendlyName;
	};
	[[nodiscard]] std::vector<DeviceInfo> devices() const noexcept;

private:
	void playbackThread(Signal signal);

private:
	enum ChannelMask : uint32_t {
		SPEAKER_FRONT_LEFT = 0x1,
		SPEAKER_FRONT_RIGHT = 0x2,
		SPEAKER_FRONT_CENTER = 0x4,
		SPEAKER_LOW_FREQUENCY = 0x8,
		SPEAKER_BACK_LEFT = 0x10,
		SPEAKER_BACK_RIGHT = 0x20,
		SPEAKER_FRONT_LEFT_OF_CENTER = 0x40,
		SPEAKER_FRONT_RIGHT_OF_CENTER = 0x80,
		SPEAKER_BACK_CENTER = 0x100,
		SPEAKER_SIDE_LEFT = 0x200,
		SPEAKER_SIDE_RIGHT = 0x400,
		SPEAKER_TOP_CENTER = 0x800,
		SPEAKER_TOP_FRONT_LEFT = 0x1000,
		SPEAKER_TOP_FRONT_CENTER = 0x2000,
		SPEAKER_TOP_FRONT_RIGHT = 0x4000,
		SPEAKER_TOP_BACK_LEFT = 0x8000,
		SPEAKER_TOP_BACK_CENTER = 0x10000,
		SPEAKER_TOP_BACK_RIGHT = 0x20000
	};

	std::thread _thread;
	std::atomic_bool _bPlaybackStarted = false;
	std::atomic_bool _bTerminateThread = false;

	uint32_t _samplesPlayedSoFar = 0;
};
