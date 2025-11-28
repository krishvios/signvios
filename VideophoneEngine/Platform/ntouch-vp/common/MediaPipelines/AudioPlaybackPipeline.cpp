/*!\brief Audio playback pipeline
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioPlaybackPipeline.h"


GStreamerElement createAudioSink (const std::string &device)
{
	stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);

	GStreamerElement sink ("alsasink", "audio_alsasink_element");
	stiTESTCOND (sink.get (), stiRESULT_ERROR);

	sink.propertySet ("buffer-time", G_GUINT64_CONSTANT(40 * 1000));
	sink.propertySet ("sync", FALSE);
	sink.propertySet ("device", device);

STI_BAIL:

	return sink;
}


void AudioPlaybackPipeline::create (
	EstiAudioCodec codec,
	const SstiAudioDecodeSettings &settings,
	const vpe::BluetoothAudioDevice &bluetoothDevice,
	nonstd::observer_ptr<vpe::BluetoothAudioBase> bluetooth,
	AudioPipeline::OutputDevice outputDevice,
	bool disableHDMIOutput)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GStreamerElement sourceQueue;
	GStreamerElement audioParse;
	GStreamerElement decoder;
	GStreamerElement capsFilter;
	GStreamerElement gstElementAudioConvert;
	GStreamerElement gstElementAudioResample;
	GStreamerElement gstElementAudioSink;
	GStreamerElement gstElementCapsFilter;

	bool bLinked = false;

	m_outputDevice = outputDevice;

	GStreamerPipeline::create ("audio_decode_pipeline");

	m_needsData = false;
	m_aecEnabled = false;

	stiDEBUG_TOOL(g_stiAudioHandlerDebug,
		stiTrace("Enter DecodePipelineCreate\n");
	);

	m_gstElementAppSrc = GStreamerAppSource("audio_app_src_element");
	stiTESTCOND (m_gstElementAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_gstElementAppSrc.propertySet ("is-live", TRUE);
	m_gstElementAppSrc.propertySet ("max-bytes", G_GUINT64_CONSTANT(1));

	m_gstElementAppSrc.streamTypeSet (GST_APP_STREAM_TYPE_STREAM);
	m_gstElementAppSrc.sizeSet (-1);

	m_gstElementAppSrc.needDataCallbackSet ([this](GStreamerAppSource &appSrc) { NeedData (appSrc); });
	m_gstElementAppSrc.enoughDataCallbackSet ([this](GStreamerAppSource &appSrc) { EnoughData (appSrc); });

	elementAdd (m_gstElementAppSrc);

	sourceQueue = GStreamerElement ("queue", "app_source_queue");
	stiTESTCOND(sourceQueue.get (), stiRESULT_ERROR);

	sourceQueue.propertySet ("leaky", 2); // 2=downstream

	elementAdd (sourceQueue);

	bLinked = m_gstElementAppSrc.link (sourceQueue);
	stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "Can't link app source to queue\n");

	gstElementAudioConvert = GStreamerElement ("audioconvert", "audio_output_convert");
	stiTESTCOND (gstElementAudioConvert.get (), stiRESULT_ERROR);

	gstElementAudioResample = GStreamerElement ("audioresample", "audio_output_resample");
	stiTESTCOND (gstElementAudioResample.get (), stiRESULT_ERROR);

	{
		int sampleRate {48000};

		if (bluetoothDevice.m_connected)
		{
			if (bluetoothDevice.m_transportType == vpe::transportType::typeSco)
			{
				sampleRate = bluetoothDevice.m_sampleRate;
			}
			else
			{
				sampleRate = bluetoothSampleRateGet();
			}
		}
		else if (outputDevice == OutputDevice::Headset)
		{
			sampleRate = headphoneSampleRateGet();
		}

		GStreamerCaps gstCaps (filterTypeGet (),
				"rate", G_TYPE_INT, sampleRate,
				"channels", G_TYPE_INT, 2,
				"width", G_TYPE_INT, 16,
				"depth", G_TYPE_INT, 16,
				NULL);

		gstElementCapsFilter = GStreamerElement ("capsfilter", "audio_output_capsfilter");
		stiTESTCOND (gstElementCapsFilter.get (), stiRESULT_ERROR);

		gstElementCapsFilter.propertySet ("caps", gstCaps);
	}

	stiDEBUG_TOOL(g_stiAudioHandlerDebug,
		stiTrace("done with the capsfilter\n");
	);

	if (bluetoothDevice.m_connected)
	{
		std::string alsaDevice = bluetooth->BluetoothAudioDeviceGet(bluetoothDevice);

		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			vpe::trace("AudioPlaybackPipeline::", __FUNCTION__, ": output is bluetooth: ", bluetoothDevice.m_address, ", alsa device: ", alsaDevice, "\n");
		);

		gstElementAudioSink = createAudioSink (alsaDevice);
	}
	else if (outputDevice == OutputDevice::Headset)
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("AudioPlaybackPipeline::%s: output is headphone\n", __FUNCTION__);
		);

		gstElementAudioSink = createAudioSink (headsetOutputDeviceGet ());
	}
	else
	{
		stiDEBUG_TOOL(g_stiAudioHandlerDebug,
			stiTrace("AudioPlaybackPipeline::%s: output is hdmi\n", __FUNCTION__);
		);

		if  (disableHDMIOutput)
		{
			gstElementAudioSink = GStreamerElement("fakesink");
			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("AudioPlaybackPipeline::%s: hdmi is disabled\n", __FUNCTION__);
			);
		}
		else
		{
			gstElementAudioSink = createAudioSink (defaultOutputDeviceGet ());
			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("AudioPlaybackPipeline::%s: hdmi is enabled\n", __FUNCTION__);
			);
		}
	}

	elementAdd (gstElementAudioConvert);
	elementAdd (gstElementAudioResample);
	elementAdd (gstElementCapsFilter);
	elementAdd (gstElementAudioSink);

	bLinked = gstElementAudioConvert.link (gstElementAudioResample)
			&& gstElementAudioResample.link (gstElementCapsFilter);

	if (!bluetoothDevice.m_connected && outputDevice != OutputDevice::Headset && !disableHDMIOutput)
	{
		auto aecBin = aecBinGet ();

		if (aecBin.get ())
		{
			elementAdd (aecBin);

			bLinked = gstElementCapsFilter.link (aecBin)
					&& aecBin.link (gstElementAudioSink);
			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "Can't link resample through filter\n");
			stiDEBUG_TOOL (g_stiAudioHandlerDebug,
				stiTrace("%s: aecBin.get()=TRUE\n", __func__);
			);

			m_aecEnabled = true;
		}
		else
		{
			bLinked = gstElementCapsFilter.link (gstElementAudioSink);
			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "Can't link resample through filter\n");
			stiDEBUG_TOOL (g_stiAudioHandlerDebug,
				stiTrace("%s: aecBin.get()=FALSE\n", __func__);
			);
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("%s: Not adding AEC to pipeline\n", __func__);
		);

		bLinked = gstElementCapsFilter.link (gstElementAudioSink);
		stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "Can't link resample through filter\n");
	}

	switch (codec)
	{
		case estiAUDIO_G711_ALAW:
			audioParse = GStreamerElement ("audioparse", "audio_output_audioparse");
			stiTESTCOND (audioParse.get (), stiRESULT_ERROR);

			audioParse.propertySet ("format", alawFormatIdGet ());
			audioParse.propertySet ("channels", 1);
			audioParse.propertySet ("rate", 8000);

			decoder = GStreamerElement ("alawdec", "audio_output_alaw_decoder");
			stiTESTCOND (decoder.get (), stiRESULT_ERROR);

			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("G711_ALAW Decoder Selected\n");
			);

			elementAdd(audioParse);
			elementAdd(decoder);

			bLinked = sourceQueue.link (audioParse)
					&& audioParse.link (decoder)
					&& decoder.link (gstElementAudioConvert);
			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate: Failed to link decode pipeline\n");

			break;

		case estiAUDIO_NONE:
		case estiAUDIO_G711_MULAW:
		{
			audioParse = GStreamerElement ("audioparse", "audio_output_audioparse");
			stiTESTCOND (audioParse.get (), stiRESULT_ERROR);

			audioParse.propertySet ("format", mulawFormatIdGet ());
			audioParse.propertySet ("channels", 1);
			audioParse.propertySet ("rate", 8000);

			decoder = GStreamerElement ("mulawdec", "audio_output_mulaw_decoder");
			stiTESTCOND (decoder.get (), stiRESULT_ERROR);

			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("G711_ULAW Decoder Selected\n");
			);

			elementAdd (audioParse);
			elementAdd (decoder);

			bLinked = sourceQueue.link (audioParse)
					&& audioParse.link (decoder)
					&& decoder.link (gstElementAudioConvert);
			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate: Failed to link decode pipeline\n");

			break;
		}

		case estiAUDIO_G722:
		{
			G722AppsrcPropertiesSet();

			capsFilter = GStreamerElement ("capsfilter", "audio_output_G722_filter");
			stiTESTCOND (capsFilter.get (), stiRESULT_ERROR);

			GStreamerCaps gstCaps("audio/G722",
					"rate", G_TYPE_INT, 16000,
					"channels", G_TYPE_INT, 1,
					NULL);

			capsFilter.propertySet ("caps", gstCaps);

			decoder = GStreamerElement (g722DecoderNameGet (), "audio_output_G722_decoder");
			stiTESTCOND (decoder.get (), stiRESULT_ERROR);

			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("G722 Decoder Selected: %s\n", g722DecoderNameGet ().c_str ());
			);

			elementAdd (capsFilter);
			elementAdd (decoder);

			bLinked = sourceQueue.link (capsFilter)
					&& capsFilter.link (decoder)
					&& decoder.link (gstElementAudioConvert);

			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate: Failed to link decode pipeline\n");

			break;
		}

		case estiAUDIO_RAW:
			audioParse = GStreamerElement ("audioparse", "audio_output_audioparse");
			stiTESTCOND (audioParse.get (), stiRESULT_ERROR);

			audioParse.propertySet ("format", 0);
			audioParse.propertySet ("channels", 2);
			audioParse.propertySet ("rate", 48000);

			stiDEBUG_TOOL(g_stiAudioHandlerDebug,
				stiTrace("RAW Decoder\n");
			);

			elementAdd (audioParse);

			bLinked = sourceQueue.link (audioParse)
					&& audioParse.link (gstElementAudioConvert);

			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate: Failed to link decode pipeline\n");

			break;

		case estiAUDIO_AAC:
		{
			// Make sure the settings have been set for the decoder.
			stiTESTCOND(settings.channels != 0, stiRESULT_ERROR);

			decoder = GStreamerElement (aacDecoderNameGet (), "audio_output_aac_decoder");
			stiTESTCOND (decoder.get (), stiRESULT_ERROR);

			GStreamerCaps gstCaps ("audio/mpeg",
										"channels", G_TYPE_INT, settings.channels,
										"rate", G_TYPE_INT, settings.sampleRate,
										"mpegversion", G_TYPE_INT, 4,
										"stream-format", G_TYPE_STRING, "raw",
										NULL);
			stiTESTCONDMSG (gstCaps.get () != nullptr, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate:  can't create caps\n");

			capsFilter = GStreamerElement ("capsfilter", "audio_output_aac_decoder_filter");
			stiTESTCOND (capsFilter.get (), stiRESULT_ERROR);

			capsFilter.propertySet ("caps", gstCaps);

			elementAdd (capsFilter);
			elementAdd (decoder);

			bLinked = sourceQueue.link (capsFilter)
					&& capsFilter.link (decoder)
					&& decoder.link (gstElementAudioConvert);
			stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipeline::DecodePipelineCreate: Failed to link decode pipeline\n");

			break;
		}

		default:
			stiTHROW(stiRESULT_INVALID_CODEC);
	}

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		writeGraph ("AudioOutput_pipeline_create");
	);

	busWatchEnable ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


void AudioPlaybackPipeline::G722AppsrcPropertiesSet()
{
	m_gstElementAppSrc.propertySet ("do-timestamp", TRUE);
	m_gstElementAppSrc.propertySet ("format", GST_FORMAT_TIME);
}


/*!
 * \brief gstreamer calls this when it can receive more audio data
 */
void AudioPlaybackPipeline::NeedData (
	GStreamerAppSource &appSrc)
{
	stiDEBUG_TOOL(g_stiAudioHandlerDebug > 1,
		vpe::trace("NeedData: NeedsData = ", m_needsData, "\n");
	);

	std::unique_lock<std::mutex> lock (m_mutexNeedsData);

	if (!m_needsData)
	{
		//stiTrace ("CstiVideoOutput::need_data\n");

		m_needsData = true;

		readyStateChangedSignal.Emit (true);
	}
}


/*!
 * \brief gstreamer calls this when it's audio pipeline is full
 */
void AudioPlaybackPipeline::EnoughData (
	GStreamerAppSource &appSrc)
{
	stiDEBUG_TOOL(g_stiAudioHandlerDebug > 1,
			vpe::trace("EnoughData: NeedsData = ", m_needsData, "\n");
	);

	std::unique_lock<std::mutex> lock (m_mutexNeedsData);

	if (m_needsData)
	{
		m_needsData = false;

		readyStateChangedSignal.Emit (false);
	}
}


stiHResult AudioPlaybackPipeline::pushBuffer(GStreamerBuffer &buffer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (get () != nullptr)
	{
		auto nResult = m_gstElementAppSrc.pushBuffer (buffer);
		stiTESTCOND (nResult == GST_FLOW_OK, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}
