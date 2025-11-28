/*!\brief Specializations for the VP2 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioPlaybackPipelineVP2.h"
#include <string>

AudioPlaybackPipelineVP2::AudioPlaybackPipelineVP2 ()
{
	aecOutputDeviceSet ("hw:0,5");
}

GStreamerBin AudioPlaybackPipelineVP2::aecBinGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bLinked = false;
	bool result  = false;
	const int dspBitWidth {32};
	const int dspBitDepth {32};
	const int dspChannelRate {48000};
	const int dspChannels {2};

	stiDEBUG_TOOL (g_stiAudioHandlerDebug,
		stiTrace("%s: Adding AEC to pipeline\n", __func__);
	);

	GStreamerElement gstElementAudioTee;
	GStreamerElement gstElementAudioAECConvert;
	GStreamerElement gstElementAudioAECResample;
	GStreamerElement gstElementAECFilter;
	GStreamerElement gstElementAECSink;
	GStreamerElement gstElementAudioQueue1;
	GStreamerElement gstElementAudioQueue2;

	GStreamerBin aecBin ("aec_bin");
	stiTESTCOND (aecBin.get (), stiRESULT_ERROR);

	gstElementAudioTee = GStreamerElement ("tee", "audio_output_tee_element");
	stiTESTCOND (gstElementAudioTee.get (), stiRESULT_ERROR);

	gstElementAudioAECConvert = GStreamerElement ("audioconvert", "audio_output_aec_convert");
	stiTESTCOND (gstElementAudioAECConvert.get (), stiRESULT_ERROR);

	gstElementAudioAECResample = GStreamerElement ("audioresample", "audio_output_aec_resample");
	stiTESTCOND (gstElementAudioAECResample.get (), stiRESULT_ERROR);

	gstElementAECFilter = GStreamerElement ("capsfilter", "audio_output_aec_capsfilter");
	stiTESTCOND (gstElementAECFilter.get (), stiRESULT_ERROR);

	gstElementAECSink = GStreamerElement ("alsasink", "audio_output_aec_element");
	stiTESTCOND (gstElementAECSink.get (), stiRESULT_ERROR);

	gstElementAECSink.propertySet ("buffer-time", G_GUINT64_CONSTANT(40 * 1000));
	gstElementAECSink.propertySet ("sync", FALSE);
	gstElementAECSink.propertySet ("device", aecOutputDeviceGet ());

	{
		GStreamerCaps gstCaps(filterTypeGet (),
				"rate", G_TYPE_INT, dspChannelRate,
				"channels", G_TYPE_INT, dspChannels,
				"width", G_TYPE_INT, dspBitWidth,
				"depth", G_TYPE_INT, dspBitDepth,
				NULL);
		gstElementAECFilter.propertySet ("caps", gstCaps);
	}

	gstElementAudioQueue1 = GStreamerElement ("queue", "audio_output_queue_one");
	stiTESTCOND (gstElementAudioQueue1.get (), stiRESULT_ERROR);

	gstElementAudioQueue2 = GStreamerElement ("queue", "audio_output_queue_two");
	stiTESTCOND (gstElementAudioQueue2.get (), stiRESULT_ERROR);

	aecBin.elementAdd (gstElementAudioTee);
	aecBin.elementAdd (gstElementAudioQueue1);
	aecBin.elementAdd (gstElementAudioQueue2);
	aecBin.elementAdd (gstElementAudioAECConvert);
	aecBin.elementAdd (gstElementAudioAECResample);
	aecBin.elementAdd (gstElementAECFilter);
	aecBin.elementAdd (gstElementAECSink);

	{
		GStreamerPad srcPad = gstElementAudioTee.requestPadGet ("src%d");
		GStreamerPad sinkPad = gstElementAudioQueue1.staticPadGet ("sink");

		stiTESTCOND (srcPad.get () != nullptr, stiRESULT_ERROR);
		stiTESTCOND (sinkPad.get () != nullptr, stiRESULT_ERROR);

		if (srcPad.link (sinkPad) != GST_PAD_LINK_OK)
		{
			stiTHROWMSG(stiRESULT_ERROR, "Can't link tee to queue1\n");
		}
	}

	{
		GStreamerPad srcPad  = gstElementAudioTee.requestPadGet ("src%d");
		GStreamerPad sinkPad = gstElementAudioQueue2.staticPadGet ("sink");

		stiTESTCOND (srcPad.get () != nullptr, stiRESULT_ERROR);
		stiTESTCOND (sinkPad.get () != nullptr, stiRESULT_ERROR);

		if (srcPad.link (sinkPad) != GST_PAD_LINK_OK)
		{
			stiTHROWMSG(stiRESULT_ERROR, "Can't link tee to queue2\n");
		}
	}

	// Create a sink pad for the bin
	{
		auto audioTeePad = gstElementAudioTee.staticPadGet ("sink");
		stiTESTCOND(audioTeePad.get(), stiRESULT_ERROR);

		result = aecBin.ghostPadAdd ("sink", audioTeePad);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

	// Create a src pad for the bin
	{
		auto audioTeePad = gstElementAudioQueue1.staticPadGet ("src");
		stiTESTCOND(audioTeePad.get(), stiRESULT_ERROR);

		result = aecBin.ghostPadAdd ("src", audioTeePad);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

	bLinked = gstElementAudioQueue2.link (gstElementAudioAECConvert)
			&& gstElementAudioAECConvert.link (gstElementAudioAECResample)
			&& gstElementAudioAECResample.link (gstElementAECFilter)
			&& gstElementAECFilter.link (gstElementAECSink);
	stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "AudioPlaybackPipelineVP2::aecBinGet: Failed to link queue2 through aecsink\n");

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		aecBin.clear ();
	}

	return aecBin;
}


