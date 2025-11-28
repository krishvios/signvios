/*!\brief Specializations for the VP3 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioPlaybackPipelineVP3.h"
#include <string>

AudioPlaybackPipelineVP3::AudioPlaybackPipelineVP3 ()
{
	aecOutputDeviceSet ("hw:0,8");
}

GStreamerBin AudioPlaybackPipelineVP3::aecBinGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bLinked = false;
	bool result  = false;
	const int dspChannelRate {48000};
	const int dspChannels {2};

	stiDEBUG_TOOL (g_stiAudioHandlerDebug,
			stiTrace("%s: Adding AEC to pipeline\n", __func__);
	);

	GStreamerElement aecTee;
	GStreamerElement aecAudioConvert;
	GStreamerElement aecAudioResample;
	GStreamerElement aecCapsFilter;
	GStreamerElement aecAlsaSink;
	GStreamerElement aecQueue1;
	GStreamerElement aecQueue2;

	GStreamerBin aecBin ("aec_bin");
	stiTESTCOND (aecBin.get (), stiRESULT_ERROR);

	aecTee = GStreamerElement ("tee", "audio_output_aec_tee_element");
	stiTESTCOND (aecTee.get (), stiRESULT_ERROR);

	aecAudioConvert = GStreamerElement ("audioconvert", "audio_output_aec_convert");
	stiTESTCOND (aecAudioConvert.get (), stiRESULT_ERROR);

	aecAudioResample = GStreamerElement ("audioresample", "audio_output_aec_resample");
	stiTESTCOND (aecAudioResample.get (), stiRESULT_ERROR);

	aecCapsFilter = GStreamerElement ("capsfilter", "audio_output_aec_capsfilter");
	stiTESTCOND (aecCapsFilter.get (), stiRESULT_ERROR);

	aecAlsaSink = GStreamerElement ("alsasink", "audio_output_aec_element");
	stiTESTCOND (aecAlsaSink.get (), stiRESULT_ERROR);

	aecAlsaSink.propertySet ("buffer-time", G_GUINT64_CONSTANT(40 * 1000));
	aecAlsaSink.propertySet ("sync", FALSE);
	aecAlsaSink.propertySet ("device", aecOutputDeviceGet ());

	{
		GStreamerCaps gstCaps(filterTypeGet (),
				"format", G_TYPE_STRING, "S32LE",
				"rate", G_TYPE_INT, dspChannelRate,
				"channels", G_TYPE_INT, dspChannels,
				NULL);
		aecCapsFilter.propertySet ("caps", gstCaps);
	}

	aecQueue1 = GStreamerElement ("queue", "audio_output_queue_one");
	stiTESTCOND (aecQueue1.get (), stiRESULT_ERROR);

	aecQueue2 = GStreamerElement ("queue", "audio_output_queue_two");
	stiTESTCOND (aecQueue2.get (), stiRESULT_ERROR);

	aecBin.elementAdd (aecTee);
	aecBin.elementAdd (aecQueue1);
	aecBin.elementAdd (aecQueue2);
	aecBin.elementAdd (aecAudioConvert);
	aecBin.elementAdd (aecAudioResample);
	aecBin.elementAdd (aecCapsFilter);
	aecBin.elementAdd (aecAlsaSink);

	{
		auto srcPad = aecTee.requestPadGet ("src_%u");
		auto sinkPad = aecQueue1.staticPadGet ("sink");

		stiTESTCOND (srcPad.get () != nullptr, stiRESULT_ERROR);
		stiTESTCOND (sinkPad.get () != nullptr, stiRESULT_ERROR);

		if (srcPad.link (sinkPad) != GST_PAD_LINK_OK)
		{
			stiTHROWMSG(stiRESULT_ERROR, "Can't link tee to queue1\n");
		}
	}

	{
		GStreamerPad srcPad = aecTee.requestPadGet ("src_%u");
		GStreamerPad sinkPad = aecQueue2.staticPadGet ("sink");

		stiTESTCOND (srcPad.get () != nullptr, stiRESULT_ERROR);
		stiTESTCOND (sinkPad.get () != nullptr, stiRESULT_ERROR);

		if (srcPad.link (sinkPad) != GST_PAD_LINK_OK)
		{
			stiTHROWMSG(stiRESULT_ERROR, "Can't link tee to queue2\n");
		}
	}

	// Create a sink pad for the bin
	{
		auto audioTeePad = aecTee.staticPadGet ("sink");
		stiTESTCOND(audioTeePad.get(), stiRESULT_ERROR);

		result = aecBin.ghostPadAdd ("sink", audioTeePad);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

	// Create a src pad for the bin
	{
		auto audioTeePad = aecQueue1.staticPadGet ("src");
		stiTESTCOND(audioTeePad.get(), stiRESULT_ERROR);

		result = aecBin.ghostPadAdd ("src", audioTeePad);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

	bLinked = aecQueue2.link (aecAudioConvert)
		&& aecAudioConvert.link (aecAudioResample)
		&& aecAudioResample.link (aecCapsFilter)
		&& aecCapsFilter.link (aecAlsaSink);
	stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipelineVP3::aecBinGet: Failed to link queue2 through aecsink\n");


STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		aecBin.clear ();
	}

	return aecBin;
}
