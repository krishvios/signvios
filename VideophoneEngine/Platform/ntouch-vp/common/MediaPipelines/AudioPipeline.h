/*!\brief Base class for audio pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "MediaPipeline.h"


class AudioPipeline: public MediaPipeline
{
public:

	AudioPipeline () = default;

	AudioPipeline (const std::string &name)
	:
		MediaPipeline (name)
	{
	}

	~AudioPipeline () override = default;

	AudioPipeline (const AudioPipeline &other) = delete;
	AudioPipeline (AudioPipeline &&other) = delete;
	AudioPipeline &operator= (const AudioPipeline &other) = delete;
	AudioPipeline &operator= (AudioPipeline &&other) = delete;

	GstStateChangeReturn stateSet (GstState state) override;

	bool isPlaying ()
	{
		return m_pipelineState == PipelineState::Playing;
	}

	enum class OutputDevice
	{
		None,
		Default,
		Speaker,
		Headset
	};

	enum class InputDevice
	{
		Default,
		Headset
	};

protected:

	virtual int audioInputRateGet () const = 0;

	virtual int headphoneSampleRateGet () const = 0;

	virtual int bluetoothSampleRateGet() const = 0;

	virtual std::string micInputDeviceGet () const = 0;

	virtual std::string headsetInputDeviceGet () const = 0;

	virtual std::string defaultOutputDeviceGet () const = 0;

	virtual std::string headsetOutputDeviceGet () const = 0;

	virtual std::string speakerOutputDeviceGet () const = 0;

	virtual std::string filterTypeGet () const = 0;

	virtual int alawFormatIdGet () const = 0;

	virtual int mulawFormatIdGet () const = 0;

	virtual std::string audioTypeGet () const = 0;

	virtual std::string g722EncoderNameGet () const = 0;

	virtual std::string g722DecoderNameGet () const = 0;

	virtual std::string aacDecoderNameGet () const = 0;

	virtual std::string aacEncoderNameGet () const = 0;

	void speakerEnable ();

	void headsetEnable ();

	virtual std::string headphoneEnableStringGet () const = 0;

	virtual std::string speakerEnableStringGet () const = 0;

	enum class PipelineState
	{
		Paused,
		Playing
	};

	PipelineState m_pipelineState {PipelineState::Paused};

	OutputDevice m_outputDevice {OutputDevice::None};

private:
};


enum class AudioInputType
{
	Unknown = -1,
	Microphone,
	LowPitchRinger,
	MediumPitchRinger,
	HighPitchRinger,
	StereoTest,
	ToneTest,
};

std::string audioInputTypeNameGet (AudioInputType input);

