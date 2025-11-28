/*!\brief Audio record pipeline.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioRecordPipeline.h"
#include "stiTrace.h"
#include <cmath>


#undef ADD_AUDIO_DELAY
#if APPLICATION == APP_NTOUCH_VP3
	#define ADD_AUDIO_DELAY
#endif


void AudioRecordPipeline::create (
	EstiAudioCodec codec,
	const IstiAudioInput::SstiAudioRecordSettings &settings,
	bool useHeadsetInput,
	const vpe::BluetoothAudioDevice &bluetoothDevice,
	nonstd::observer_ptr<vpe::BluetoothAudioBase> bluetooth)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GStreamerElement gstSource;
	GStreamerElement preFilter;
	GStreamerElement audioConvert;
	GStreamerElement audioResample;
	GStreamerElement postFilter;
	GStreamerElement level;
	GStreamerElement encoder;

#ifdef ADD_AUDIO_DELAY
	GStreamerElement queue;
	int audioDelay = 300000000;
#endif

	std::string encoderName;

	int nRate = 0;
	int nChans = 0;

	bool bLinked = false;

	stiDEBUG_TOOL(g_stiAudioEncodeDebug,
		vpe::trace (__func__, ": Enter\n");
	);

	GStreamerPipeline::create ("audio_input_pipeline");

	m_aecElement.clear ();

	gstSource = GStreamerElement ("alsasrc", "default_audio_input_source");
	stiTESTCOND (gstSource.get () != nullptr, stiRESULT_ERROR);
	gstSource.propertySet ("buffer-time", G_GUINT64_CONSTANT(40 * 1000));

	// Check for connected headset
	// TODO: some speakers advertise the headset profiles... how to handle that?
	if (bluetoothDevice.m_connected && bluetoothDevice.m_transportType == vpe::transportType::typeSco)
	{
		std::string alsaDevice = bluetooth->BluetoothAudioDeviceGet(bluetoothDevice);
		stiDEBUG_TOOL(g_stiAudioEncodeDebug,
			vpe::trace ("AudioRecordPipeline::", __FUNCTION__, ": input is bluetooth mic: ", bluetoothDevice.m_address, ", alsa device: ", alsaDevice, "\n");
			);

		gstSource.propertySet ("device", alsaDevice);
		m_inputType = InputType::bluetooth;
	}
	else if (useHeadsetInput)
	{
		stiDEBUG_TOOL(g_stiAudioEncodeDebug,
			vpe::trace ("AudioRecordPipeline::", __FUNCTION__, ": input is wired mic\n");
			);

		gstSource.propertySet ("device", headsetInputDeviceGet ());
		m_inputType = InputType::headphone;
	}
	else
	{
		stiDEBUG_TOOL(g_stiAudioEncodeDebug,
			vpe::trace ("AudioRecordPipeline::", __FUNCTION__, ": input is RCU mic\n");
			);

		gstSource.propertySet ("device", micInputDeviceGet ());
		m_inputType = InputType::internal;
	}

	{
		const int audioInputChans {2};
		int audioRate {0};

		if (bluetoothDevice.m_connected && bluetoothDevice.m_transportType == vpe::transportType::typeSco)
		{
			audioRate = bluetoothDevice.m_sampleRate;
		}
		else
		{
			audioRate = audioInputRateGet();
		}
		// filter the audio input
		GStreamerCaps gstCaps(audioTypeGet (),
				"rate", G_TYPE_INT, audioRate,
				"channels", G_TYPE_INT, audioInputChans,
				"width", G_TYPE_INT, 16,
				"depth", G_TYPE_INT, 16,
				"endianness", G_TYPE_INT, 1234,
				NULL);

		stiDEBUG_TOOL(g_stiAudioEncodeDebug,
			vpe::trace("audio_input_prefilter = type ", audioTypeGet(),
				"\nrate = ", audioRate, "\nChannels = ", audioInputChans, "\n");
		);
		
		preFilter = GStreamerElement ("capsfilter", "audio_input_prefilter");
		preFilter.propertySet ("caps", gstCaps);
	}

	switch (codec)
	{
		case estiAUDIO_G711_ALAW:
			encoderName = "alawenc";
			nRate = 8000;
			nChans = 1;
			break;
		case estiAUDIO_NONE:
		case estiAUDIO_G711_MULAW:
			encoderName = "mulawenc";
			nRate = 8000;
			nChans = 1;
			break;
		case estiAUDIO_G722:
			encoderName = g722EncoderNameGet ();
			nRate = 16000;
			nChans = 1;
			break;
		case estiAUDIO_AAC:
			encoderName = aacEncoderNameGet ();
			nRate = static_cast<int>(settings.sampleRate);
			nChans = 2;
			break;
		case estiAUDIO_RAW:
		default:
			vpe::trace("AudioInputCodecSet: invalid codec ", m_codec, "\n");
			stiTHROW(stiRESULT_INVALID_CODEC);
	}

	m_codec = codec;

	//
	// create the resample
	audioConvert = GStreamerElement ("audioconvert", "audio_input_convert");
	audioResample = GStreamerElement ("audioresample", "audio_input_resample");

	{
		//
		// this filter resamples to what the encoder likes
		GStreamerCaps gstCaps(audioTypeGet (),
				"rate", G_TYPE_INT, nRate,
				"channels", G_TYPE_INT, nChans,
				"width", G_TYPE_INT, 16,
				"depth", G_TYPE_INT, 16,
				"endianness", G_TYPE_INT, 1234,
				NULL);
		postFilter = GStreamerElement ("capsfilter", "audio_input_postfilter");
		postFilter.propertySet ("caps", gstCaps);
	}

	//
	// Create the input level element
	level = GStreamerElement ("level", "audio_input_level");
	stiTESTCOND (level.get () != nullptr, stiRESULT_ERROR);
	level.propertySet ("message", TRUE);

	//
	// now the encoder
	encoder = GStreamerElement (encoderName, "audio_input_encoder");
	stiTESTCOND(encoder.get () != nullptr, stiRESULT_ERROR);

	if ((m_codec == estiAUDIO_AAC || m_codec == estiAUDIO_G722) &&
			settings.encodingBitrate > 0)
	{
		// Set the encode bitrate.
		encoder.propertySet ("bitrate", settings.encodingBitrate);
	}

#ifdef ADD_AUDIO_DELAY
	//
	// Create the queue element to add audio delay
	queue = GStreamerElement ("queue", "audio_input_queue");
	queue.propertySet ("min-threshold-time", audioDelay);
#endif

	//
	// Create the appsink element
	//
	m_appSink = GStreamerAppSink("audio_input_appsink");
	stiTESTCOND (m_appSink.get () != nullptr, stiRESULT_ERROR);

	m_appSink.newPrerollCallbackSet ([this](GStreamerAppSink appSink) { eventNewPreroll (appSink); });
	m_appSink.newBufferCallbackSet ([this](GStreamerAppSink appSink) { eventNewBuffer (appSink); });
	m_appSink.propertySet ("qos", TRUE);
	m_appSink.propertySet ("sync", FALSE);
	m_appSink.propertySet ("max-buffers", 1);

	elementAdd (gstSource);
	elementAdd (preFilter);
	elementAdd (audioConvert);
	elementAdd (audioResample);
	elementAdd (level);
	elementAdd (postFilter);
	elementAdd (encoder);
#ifdef ADD_AUDIO_DELAY
	elementAdd (queue);
#endif
	elementAdd (m_appSink);

	bLinked = gstSource.link (preFilter);

	if (m_aecElement.get())
	{
		elementAdd(m_aecElement);

		bLinked = bLinked
			&& preFilter.link(m_aecElement)
			&& m_aecElement.link(audioConvert);
	}
	else
	{
		bLinked = bLinked && preFilter.link (audioConvert);
	}

	bLinked = bLinked
		&& audioConvert.link (audioResample)
		&& audioResample.link (level)
		&& level.link (postFilter)
		&& postFilter.link (encoder)
#ifdef ADD_AUDIO_DELAY
		&& encoder.link (queue)
		&& queue.link (m_appSink);
#else
		&& encoder.link (m_appSink);
#endif

	stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioRecordPipeline: Can't link pipeline\n");

	registerElementMessageCallback ("level", [this](const GstStructure &messageStruct)
		{
			micLevelProcess(messageStruct);
		});

	busWatchEnable ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


void AudioRecordPipeline::eventNewPreroll (
	GStreamerAppSink gstAppSink)
{
	//
	// Make sure our app sink pointer is not null and that the appsink pointer
	// sent in the event is the same one we are currently using.
	//
	if (m_appSink == gstAppSink)
	{
		auto sample = gstAppSink.pullPreroll ();

		m_newSampleSignal.Emit (m_codec, sample);
	}
}


void AudioRecordPipeline::eventNewBuffer (
	GStreamerAppSink gstAppSink)
{
	//
	// Make sure our app sink pointer is not null and that the appsink pointer
	// sent in the event is the same one we are currently using.
	//
	if (m_appSink == gstAppSink)
	{
		auto sample = gstAppSink.pullSample ();

		m_newSampleSignal.Emit (m_codec, sample);
	}
}

void AudioRecordPipeline::micLevelProcess (
	const GstStructure &messageStruct)
{
	gint channels;
	const GValue *value;
	GValueArray *array;

	value = gst_structure_get_value(&messageStruct, "rms");
	array = (GValueArray *)g_value_get_boxed(value);

	channels = array->n_values;

	for (int i = 0; i < channels; i++)
	{
		value = array->values + i;
		gdouble rms_DB = g_value_get_double(value);

		stiDEBUG_TOOL(g_stiAudioEncodeDebug > 1,
			stiTrace ("CstiAudioInput::micLevelProcess channel: %d, rms_DB: %f\n", i, rms_DB);
		);

		const int volFactor {20}; // Normalize the level for the audio meter
		int audioLevel {static_cast<int>(pow(10, (rms_DB + volFactor) / 20) * 100)};

		m_audioLevelSignal.Emit (audioLevel);
	}
}
