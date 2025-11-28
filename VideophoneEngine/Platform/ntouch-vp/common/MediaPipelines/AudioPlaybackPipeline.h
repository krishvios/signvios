/*!\brief Audio playback pipeline
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "stiSVX.h"
#include "AudioPipeline.h"
#include "GStreamerAppSource.h"
#include "BluetoothAudioBase.h"
#include "IstiAudioOutput.h"
#include <mutex>
#include <condition_variable>


class AudioPlaybackPipeline : virtual public AudioPipeline
{
public:

	AudioPlaybackPipeline () = default;
	~AudioPlaybackPipeline () override = default;

	AudioPlaybackPipeline (const AudioPlaybackPipeline &other) = delete;
	AudioPlaybackPipeline (AudioPlaybackPipeline &&other) = delete;
	AudioPlaybackPipeline &operator= (const AudioPlaybackPipeline &other) = delete;
	AudioPlaybackPipeline &operator= (AudioPlaybackPipeline &&other) = delete;

	void create (
		EstiAudioCodec codec,
		const SstiAudioDecodeSettings &settings,
		const vpe::BluetoothAudioDevice &bluetoothDevice,
		nonstd::observer_ptr<vpe::BluetoothAudioBase> bluetooth,
		AudioPipeline::OutputDevice outputDevice,
		bool disableHDMIOutput);

	stiHResult pushBuffer(GStreamerBuffer &buffer);

	bool aecEnabledGet ()
	{
		return m_aecEnabled;
	}

	CstiSignal<bool> readyStateChangedSignal;

protected:

	std::string aecOutputDeviceGet ()
	{
		return m_aecOutputDevice;
	}

	void aecOutputDeviceSet (const std::string& aecOutputDevice)
	{
		m_aecOutputDevice = aecOutputDevice;
	}

private:

	void NeedData (
		GStreamerAppSource &appSrc);

	void EnoughData (
		GStreamerAppSource &appSrc);

	virtual GStreamerBin aecBinGet () { return {}; };
	virtual void G722AppsrcPropertiesSet ();

	GStreamerAppSource m_gstElementAppSrc;

	std::mutex m_mutexNeedsData;
	bool m_needsData {false};

	bool m_aecEnabled {false};

	std::string m_aecOutputDevice {};
};
