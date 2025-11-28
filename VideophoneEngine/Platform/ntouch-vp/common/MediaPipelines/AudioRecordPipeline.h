/*!\brief Audio record pipeline.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "stiSVX.h"
#include "AudioPipeline.h"
#include "GStreamerAppSink.h"
#include "GStreamerElement.h"
#include "BluetoothAudioBase.h"
#include "IstiAudioInput.h"


enum class AecSuppressionLevel
{
	Unknown = -1,
	Low = 0,
	Moderate = 1,
	High = 2,
};

enum class InputType {
	unknown = 0,
	bluetooth = 1,
	headphone = 2,
	internal = 3,
};


class AudioRecordPipeline : virtual public AudioPipeline
{
public:

	AudioRecordPipeline () = default;
	~AudioRecordPipeline () override = default;

	AudioRecordPipeline (const AudioRecordPipeline &other) = delete;
	AudioRecordPipeline (AudioRecordPipeline &&other) = delete;
	AudioRecordPipeline &operator= (const AudioRecordPipeline &other) = delete;
	AudioRecordPipeline &operator= (AudioRecordPipeline &&other) = delete;

	void create (
		EstiAudioCodec codec,
		const IstiAudioInput::SstiAudioRecordSettings &settings,
		bool useHeadsetInput,
		const vpe::BluetoothAudioDevice &bluetoothDevice,
		nonstd::observer_ptr<vpe::BluetoothAudioBase> bluetooth);

	CstiSignal<EstiAudioCodec, const GStreamerSample &> m_newSampleSignal;
	CstiSignal<int> m_audioLevelSignal;

	bool aecEnabledGet ()
	{
		return m_aecElement.get () != nullptr;
	}

	InputType m_inputType {InputType::unknown};

protected:

	GStreamerElement m_aecElement;

private:

	virtual void micLevelProcess (const GstStructure &messageStruct);

	virtual GStreamerElement aecElementCreate() { return {}; }

	void eventNewBuffer (GStreamerAppSink gstAppSink);
	void eventNewPreroll (GStreamerAppSink gstAppSink);

	GStreamerAppSink m_appSink;

	EstiAudioCodec m_codec {estiAUDIO_NONE};

	IstiAudioInput::SstiAudioRecordSettings m_AudioRecordSettings {};
};
