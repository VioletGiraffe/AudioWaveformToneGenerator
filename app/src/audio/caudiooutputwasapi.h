#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <utility>

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

	void setFrequency(float hz);
	void setChannelIndex(size_t channelIndex);

	bool playTone(std::wstring deviceId);
	void stopPlayback();

	[[nodiscard]] AudioFormat mixFormat(const std::wstring& deviceId) const noexcept;

	struct DeviceInfo {
		const std::wstring id;
		const std::wstring friendlyName;
	};
	[[nodiscard]] std::vector<DeviceInfo> devices() const noexcept;

private:
	void playbackThread(std::wstring deviceId);

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

	struct Signal {
		inline std::pair<float, size_t> params() const noexcept {
			std::lock_guard lock{ _signalParamsMutex };
			return { _hz, _channelIndex };
		}

		inline void setFrequency(float hz) noexcept {
			std::lock_guard lock{ _signalParamsMutex };
			_hz = hz;
		}

		inline void setChannelIndex(size_t channelIndex) noexcept {
			std::lock_guard lock{ _signalParamsMutex };
			_channelIndex = channelIndex;
		}

	private:
		mutable std::mutex _signalParamsMutex;

		size_t _channelIndex = 0;
		float _hz = 1000.0f;
	};

	std::thread _thread;
	Signal _signal;

	std::atomic_bool _bPlaybackStarted = false;
	std::atomic_bool _bTerminateThread = false;

	uint64_t _samplesPlayedSoFar = 0;
};
