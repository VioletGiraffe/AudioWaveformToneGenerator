#include "caudiooutputwasapi.h"

#include "math/math.hpp"
#include "system/win_utils.hpp"

#include <wil/com.h>
#include <wil/resource.h>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <Windows.h>
#include <Functiondiscoverykeys_devpkey.h>

#include <iostream>
#include <numbers>

using namespace wil;

static std::vector<ChannelInfo> channelsFromMask(const DWORD mask)
{
	static constexpr auto channelInfo = std::to_array<std::pair<const char* /* channel name */, uint32_t>>({
		{"Left", SPEAKER_FRONT_LEFT },
		{"Right", SPEAKER_FRONT_RIGHT },
		{"Center", SPEAKER_FRONT_CENTER },
		{"LFE", SPEAKER_LOW_FREQUENCY },
		{"Back Left", SPEAKER_BACK_LEFT },
		{"Back Right", SPEAKER_BACK_RIGHT },
		{"Wide Left", SPEAKER_FRONT_LEFT_OF_CENTER },
		{"Wide Right", SPEAKER_FRONT_RIGHT_OF_CENTER },
		{"Back Center", SPEAKER_BACK_CENTER },
		{"Side Left", SPEAKER_SIDE_LEFT },
		{"Side Right", SPEAKER_SIDE_RIGHT },
		{"Top Center", SPEAKER_TOP_CENTER },
		{"Top Front Left", SPEAKER_TOP_FRONT_LEFT },
		{"Top Front Right", SPEAKER_TOP_FRONT_RIGHT },
		{"Top Back Left", SPEAKER_TOP_BACK_LEFT },
		{"Top Back Center", SPEAKER_TOP_BACK_CENTER },
		{"Top Back Right", SPEAKER_TOP_BACK_RIGHT }
	});

	std::vector<ChannelInfo> channels;
	size_t index = 0;
	for (const auto& ch : channelInfo)
	{
		if ((mask & ch.second) != 0)
			channels.emplace_back(ch.first, index++);
	}

	return channels;
}

void generateTone(BYTE* pData, const UINT32 nBufferFrames, const int nChannelsTotal, const int sampleRate, float hz, const size_t channelIndex, const uint64_t samplesPlayedSoFar)
{
	const float sampleRateF = static_cast<float>(sampleRate);
	// s = A * sin(2 * Pi * f * t) = A * sin(Omega * t)
	// Omega = 2 * Pi * f
	// t = sampleIndex / samplesPerSecond
	const float omega = 2.0f * std::numbers::pi_v<float> * hz / sampleRateF;
	for (uint64_t i = 0; i < nBufferFrames; ++i)
	{
		for (size_t c = 0; c < nChannelsTotal; ++c)
		{
			float sample = 0;
			if (c == channelIndex)
				sample = 1.0f * std::sin(omega * (i + samplesPlayedSoFar));

			std::memcpy(pData + (i * nChannelsTotal + c) * sizeof(sample), &sample, sizeof(sample));
		}
	}
}

void CAudioOutputWasapi::setFrequency(float hz)
{
	_signal.setFrequency(hz);
}

void CAudioOutputWasapi::setChannelIndex(size_t channelIndex)
{
	_signal.setChannelIndex(channelIndex);
}

CAudioOutputWasapi::~CAudioOutputWasapi()
{
	stopPlayback();
}

bool CAudioOutputWasapi::playTone(std::wstring deviceId)
{
	if (_bPlaybackStarted)
		return true;

	_bPlaybackStarted = true;
	_bTerminateThread = false;

	_thread = std::thread(&CAudioOutputWasapi::playbackThread, this, std::move(deviceId));
	return true;
}

void CAudioOutputWasapi::stopPlayback()
{
	if (!_bPlaybackStarted)
		return;

	_bTerminateThread = true;
	_thread.join();
	_bPlaybackStarted = false;
}

AudioFormat CAudioOutputWasapi::mixFormat(const std::wstring& deviceId) const noexcept
{
	com_ptr_nothrow<IMMDeviceEnumerator> pDeviceEnumerator;
	HRESULT hr = ::CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pDeviceEnumerator);
	assert_and_return_message_r(SUCCEEDED(hr), "CoCreateInstance failed: " + ErrorStringFromHRESULT(hr), {});

	com_ptr_nothrow<IMMDeviceCollection> pDevices;
	hr = pDeviceEnumerator->EnumAudioEndpoints(
		eRender,
		DEVICE_STATE_ACTIVE,
		&pDevices);
	assert_and_return_message_r(SUCCEEDED(hr), "IMMDeviceEnumerator.EnumAudioEndpoints error: " + ErrorStringFromHRESULT(hr), {});

	UINT n = 0;
	pDevices->GetCount(&n);
	com_ptr_nothrow<IMMDevice> pDevice;
	for (UINT i = 0; i < n; ++i)
	{
		pDevices->Item(i, &pDevice);
		if (!pDevice)
			continue;

		unique_cotaskmem_string id;
		pDevice->GetId(&id);
		if (deviceId == id.get())
			break;
		else
			pDevice.reset();
	}	

	com_ptr_nothrow<IAudioClient> pAudioClient;
	hr = pDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)&pAudioClient);
	assert_and_return_message_r(SUCCEEDED(hr), "IMMDevice.Activate error: " + ErrorStringFromHRESULT(hr), {});

	unique_cotaskmem_ptr<WAVEFORMATEX> pMixFormat;
	hr = pAudioClient->GetMixFormat(out_param(pMixFormat));
	assert_and_return_message_r(SUCCEEDED(hr) && pMixFormat, "IAudioClient.GetMixFormat error: " + ErrorStringFromHRESULT(hr), {});

	AudioFormat fmt;

	assert_and_return_r(pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE, {});

	WAVEFORMATEXTENSIBLE* pFormatEx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMixFormat.get());
	fmt.channels = channelsFromMask(pFormatEx->dwChannelMask);
	assert_r(fmt.channels.size() == pMixFormat->nChannels);
	if (pFormatEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		fmt.sampleFormat = AudioFormat::Float;
	else if (pFormatEx->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
		fmt.sampleFormat = AudioFormat::PCM;
	else
		assert_unconditional_r("Unknown subformat!");

	fmt.sampleRate = pMixFormat->nSamplesPerSec;
	fmt.bitsPerSample = pMixFormat->wBitsPerSample;

	return fmt;
}

std::vector<CAudioOutputWasapi::DeviceInfo> CAudioOutputWasapi::devices() const
{
	com_ptr_nothrow<IMMDeviceEnumerator> pDeviceEnumerator;
	HRESULT hr = ::CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pDeviceEnumerator);
	assert_and_return_message_r(SUCCEEDED(hr), "CoCreateInstance failed: " + ErrorStringFromHRESULT(hr), {});

	com_ptr_nothrow<IMMDeviceCollection> pDevices;
	hr = pDeviceEnumerator->EnumAudioEndpoints(
		eRender,
		DEVICE_STATE_ACTIVE,
		&pDevices);
	assert_and_return_message_r(SUCCEEDED(hr), "IMMDeviceEnumerator.EnumAudioEndpoints error: " + ErrorStringFromHRESULT(hr), {});

	UINT n = 0;
	pDevices->GetCount(&n);
	std::vector<DeviceInfo> devices;
	devices.reserve(n);
	for (UINT i = 0; i < n; ++i)
	{
		com_ptr_nothrow<IMMDevice> pDevice;
		pDevices->Item(i, &pDevice);
		if (!pDevice)
			continue;

		unique_cotaskmem_string deviceID;
		pDevice->GetId(&deviceID);

		com_ptr_nothrow<IPropertyStore> props;
		pDevice->OpenPropertyStore(STGM_READ, &props);
		if (!props)
			continue;

		wil::unique_prop_variant pv;
		props->GetValue(PKEY_Device_FriendlyName, &pv);
		devices.emplace_back(deviceID.get(), pv.pwszVal);
	}

	return devices;
}

vector2D<float> CAudioOutputWasapi::currentSamplesBuffer() const
{
	return _currentSamplesBuffer.samples();
}

void CAudioOutputWasapi::playbackThread(std::wstring deviceId)
{
	_samplesPlayedSoFar = 0;

	CO_INIT_HELPER(COINIT_MULTITHREADED);

	com_ptr_nothrow<IMMDeviceEnumerator> pDeviceEnumerator;
	HRESULT hr = ::CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pDeviceEnumerator);
	assert_and_return_message_r(SUCCEEDED(hr), "CoCreateInstance failed: " + ErrorStringFromHRESULT(hr), );

	com_ptr_nothrow<IMMDeviceCollection> pDevices;
	hr = pDeviceEnumerator->EnumAudioEndpoints(
		eRender,
		DEVICE_STATE_ACTIVE,
		&pDevices);
	assert_and_return_message_r(SUCCEEDED(hr), "IMMDeviceEnumerator.EnumAudioEndpoints error: " + ErrorStringFromHRESULT(hr), );

	UINT n = 0;
	pDevices->GetCount(&n);
	com_ptr_nothrow<IMMDevice> pDevice;
	for (UINT i = 0; i < n; ++i)
	{
		pDevices->Item(i, &pDevice);
		if (!pDevice)
			continue;

		unique_cotaskmem_string id;
		pDevice->GetId(&id);
		if (deviceId == id.get())
			break;
		else
			pDevice.reset();
	}

	com_ptr_nothrow<IAudioClient> pAudioClient;
	hr = pDevice->Activate(
		__uuidof(IAudioClient),
		CLSCTX_ALL,
		nullptr,
		(void**)&pAudioClient);
	assert_and_return_message_r(SUCCEEDED(hr), "IMMDevice.Activate error: " + ErrorStringFromHRESULT(hr), );

	REFERENCE_TIME MinimumDevicePeriod = 0;
	hr = pAudioClient->GetDevicePeriod(nullptr, &MinimumDevicePeriod);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetDevicePeriod error: " + ErrorStringFromHRESULT(hr), );

	unique_cotaskmem_ptr<WAVEFORMATEX> pMixFormat;
	hr = pAudioClient->GetMixFormat(out_param(pMixFormat));
	assert_and_return_message_r(SUCCEEDED(hr) && pMixFormat, "IAudioClient.GetMixFormat error: " + ErrorStringFromHRESULT(hr), );

	WAVEFORMATEXTENSIBLE* pFormatEx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMixFormat.get());
	assert_r(pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
	assert_r(pFormatEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
		MinimumDevicePeriod,
		0,
		pMixFormat.get(),
		nullptr);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.Initialize error: " + ErrorStringFromHRESULT(hr), );

	// event
	unique_event_nothrow hEvent;
	hEvent.create();
	assert_and_return_message_r(hEvent, "CreateEvent error: " + ErrorStringFromLastError(), );

	hr = pAudioClient->SetEventHandle(hEvent.get());
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.SetEventHandle error: " + ErrorStringFromHRESULT(hr), );

	UINT32 numBufferFrames = 0;
	hr = pAudioClient->GetBufferSize(&numBufferFrames);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetBufferSize error: " + ErrorStringFromHRESULT(hr), );
	std::cout << "buffer frame size=" << numBufferFrames << "[frames]" << std::endl;

	com_ptr_nothrow<IAudioRenderClient> pAudioRenderClient;
	hr = pAudioClient->GetService(
		__uuidof(IAudioRenderClient),
		(void**)&pAudioRenderClient);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetService error: " + ErrorStringFromHRESULT(hr), );

	BYTE* pData = nullptr;
	hr = pAudioRenderClient->GetBuffer(numBufferFrames, &pData);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetBuffer error: " + ErrorStringFromHRESULT(hr), );

	{
		const auto [f, chIndex] = _signal.params();
		generateTone(pData, numBufferFrames, pMixFormat->nChannels, pMixFormat->nSamplesPerSec, f, chIndex, 0);
	}

	const auto bytesPerSample = pMixFormat->wBitsPerSample / 8;
	_currentSamplesBuffer.setData(pData, numBufferFrames, pMixFormat->nChannels);

	hr = pAudioRenderClient->ReleaseBuffer(numBufferFrames, 0);
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.ReleaseBuffer error: " + ErrorStringFromHRESULT(hr), );
	_samplesPlayedSoFar += numBufferFrames;

	hr = pAudioClient->Start();
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.Start error: " + ErrorStringFromHRESULT(hr), );

	UINT32 numPaddingFrames = 0;
	while (!_bTerminateThread)
	{
		::WaitForSingleObject(hEvent.get(), INFINITE);

		hr = pAudioClient->GetCurrentPadding(&numPaddingFrames);
		assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetCurrentPadding error: " + ErrorStringFromHRESULT(hr), );

		UINT32 numAvailableFrames = numBufferFrames - numPaddingFrames;
		if (numAvailableFrames == 0)
			continue;

		hr = pAudioRenderClient->GetBuffer(numAvailableFrames, &pData);
		assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetBuffer error: " + ErrorStringFromHRESULT(hr), );

		const auto [f, chIndex] = _signal.params();
		generateTone(pData, numAvailableFrames, pMixFormat->nChannels, pMixFormat->nSamplesPerSec, f, chIndex, _samplesPlayedSoFar);
		_currentSamplesBuffer.setData(pData, numAvailableFrames, pMixFormat->nChannels);

		hr = pAudioRenderClient->ReleaseBuffer(numAvailableFrames, 0);
		assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.ReleaseBuffer error: " + ErrorStringFromHRESULT(hr), );
		_samplesPlayedSoFar += numAvailableFrames;
	}

	//// Let the current buffer play to the end
	//do
	//{
	//	// wait for buffer to be empty
	//	::WaitForSingleObject(hEvent, INFINITE);

	//	NumPaddingFrames = 0;
	//	hr = pAudioClient->GetCurrentPadding(&NumPaddingFrames);
	//	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.GetCurrentPadding error: " + ErrorStringFromHRESULT(hr), );

	//} while (NumPaddingFrames > 0);

	hr = pAudioClient->Stop();
	assert_and_return_message_r(SUCCEEDED(hr), "IAudioClient.Stop error: " + ErrorStringFromHRESULT(hr), );
}
