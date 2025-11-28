/*!\brief Audio pipeline that takes its input from a file.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */
#include "AudioFileSourcePipeline.h"
#include "stiSVX.h"


static const std::string volumeElementName {"volume_element"};


void AudioFileSourcePipeline::create (
	AudioInputType input,
	int volume,
	AudioPipeline::OutputDevice outputDevice)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	GStreamerElement multiFileSrc;
	GStreamerElement audioParse;
	GStreamerElement audioConvert;
	GStreamerElement audioResample;
	GStreamerElement volumeElement;
	GStreamerElement audioSink;
	bool linked = false;

	auto fileName = audioInputTypeNameGet (input);
	stiTESTCOND (!fileName.empty (), stiRESULT_ERROR);

	vpe::trace ("filename: ", fileName, ", volume: ", volume, ", outputDevice: ", static_cast<int>(outputDevice), "\n");

	m_outputDevice = outputDevice;

	GStreamerPipeline::create ("file_source_pipeline");

	multiFileSrc = GStreamerElement ("multifilesrc", "multifilesrc_element");
	stiTESTCOND (multiFileSrc.get(), stiRESULT_ERROR);
	multiFileSrc.propertySet ("location", fileName);
	multiFileSrc.propertySet ("loop", 1);

	audioParse = GStreamerElement ("audioparse", "audioparse_element");
	stiTESTCOND (audioParse.get(), stiRESULT_ERROR);
	audioParse.propertySet ("channels", 2);
	audioParse.propertySet ("rate", 48000);

	audioConvert = GStreamerElement ("audioconvert", "audioconvert_element");
	stiTESTCOND (audioConvert.get(), stiRESULT_ERROR);

	audioResample = GStreamerElement ("audioresample", "audioresample_element");
	stiTESTCOND (audioResample.get(), stiRESULT_ERROR);

	volumeElement = GStreamerElement ("volume", volumeElementName);
	stiTESTCOND (volumeElement.get(), stiRESULT_ERROR);
	volumeElement.propertySet ("volume", (gfloat)volume / 10.0);

	audioSink = GStreamerElement ("alsasink", "alsasink_element");
	stiTESTCOND (audioSink.get(), stiRESULT_ERROR);

	switch (outputDevice)
	{
		case OutputDevice::None:
		case OutputDevice::Speaker:
		{
			audioSink.propertySet ("device", speakerOutputDeviceGet ());
			break;
		}
		case OutputDevice::Headset:
		{
			audioSink.propertySet ("device", headsetOutputDeviceGet ());
			break;
		}
		case OutputDevice::Default:
		{
			audioSink.propertySet ("device", defaultOutputDeviceGet ());
			break;
		}
	}

	elementAdd (multiFileSrc);
	elementAdd (audioParse);
	elementAdd (audioConvert);
	elementAdd (audioResample);
	elementAdd (volumeElement);
	elementAdd (audioSink);

	linked = multiFileSrc.link (audioParse)
			&& audioParse.link (audioConvert)
			&& audioConvert.link (audioResample)
			&& audioResample.link (volumeElement)
			&& volumeElement.link (audioSink);

	if (!linked)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiAudioHandlerBase::rawFileSourcePipelineCreate: Failed to link elements\n");
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear  ();
	}
}


void AudioFileSourcePipeline::volumeSet (int volume)
{
	auto fVolume = static_cast<gfloat>(volume / 10.0);

	stiDEBUG_TOOL(g_stiAudioHandlerDebug,
		stiTrace("AudioFileSourcePipeline::volumeSet - setting to %f\n", fVolume);
	);

	auto volumeElement = elementGetByName (volumeElementName);
	if (volumeElement.get ())
	{
		volumeElement.propertySet ("volume", fVolume);
	}
}
