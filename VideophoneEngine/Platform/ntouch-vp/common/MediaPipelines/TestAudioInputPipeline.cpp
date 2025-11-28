/*!\brief Test audio input pipeline
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "stiSVX.h"
#include "TestAudioInputPipeline.h"

#define MIC_CHANNELS 2

void TestAudioInputPipeline::create (
	const TestAudioInputSettings &settings,
	bool useHeadsetInput)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	GStreamerElement audioSource;
	GStreamerElement audioParse;
	GStreamerElement audioConvert;
	GStreamerElement audioResample;
	GStreamerElement capsFilter;
	GStreamerElement spectrum;
	GStreamerElement audioSink;

	bool linked = false;

	m_outputDevice = settings.outputDevice;
	m_numberOfBands = settings.numberOfBands;

	GStreamerPipeline::create ("test_input_pipeline");

	spectrum = GStreamerElement ("spectrum", "spectrum_element");
	stiTESTCOND (spectrum.get(), stiRESULT_ERROR);
	spectrum.propertySet ("bands", m_numberOfBands);
	spectrum.propertySet ("multi-channel", true);

	elementAdd (spectrum);

	if (settings.outputDevice == OutputDevice::None)
	{
		audioSink = GStreamerElement ("fakesink", "fakesink_element");
		stiTESTCOND (audioSink.get(), stiRESULT_ERROR);
	}
	else
	{
		audioSink = GStreamerElement ("alsasink", "alsasink_element");
		stiTESTCOND (audioSink.get(), stiRESULT_ERROR);

		switch (settings.outputDevice)
		{
			case OutputDevice::None:
				break;

			case OutputDevice::Speaker:
				audioSink.propertySet ("device", speakerOutputDeviceGet ());
				break;
			case OutputDevice::Headset:
				audioSink.propertySet ("device", headsetOutputDeviceGet ());
				break;

			case OutputDevice::Default:
				audioSink.propertySet ("device", defaultOutputDeviceGet ());
				break;
		}
	}

	elementAdd (audioSink);

	if (settings.inputType == AudioInputType::Microphone)
	{
		audioSource = GStreamerElement ("alsasrc", "alsasrc_element");
		stiTESTCOND (audioSource.get(), stiRESULT_ERROR);
		audioSource.propertySet ("device", useHeadsetInput ? headsetInputDeviceGet () : micInputDeviceGet ());

		{
			GStreamerCaps gstCaps ("audio/x-raw",
					"rate", G_TYPE_INT, audioInputRateGet (),
					"channels", G_TYPE_INT, MIC_CHANNELS,
					"width", G_TYPE_INT, 16,
					"depth", G_TYPE_INT, 16,
					nullptr);

			capsFilter = GStreamerElement ("capsfilter", "source_capsfilter");
			stiTESTCOND (capsFilter.get (), stiRESULT_ERROR);

			capsFilter.propertySet ("caps", gstCaps);
		}

		audioConvert = GStreamerElement ("audioconvert", "audioconvert_element");
		stiTESTCOND (audioConvert.get(), stiRESULT_ERROR);

		audioResample = GStreamerElement ("audioresample", "audioresample_element");
		stiTESTCOND (audioResample.get(), stiRESULT_ERROR);

		elementAdd (audioSource);
		elementAdd (audioConvert);
		elementAdd (audioResample);
		elementAdd (capsFilter);

		linked = audioSource.link (audioConvert) && audioConvert.link (audioResample) &&
				 audioResample.link (capsFilter) && capsFilter.link (spectrum) && spectrum.link (audioSink);
	}
	else
	{
		auto fileName = audioInputTypeNameGet (settings.inputType);
		stiTESTCOND (!fileName.empty (), stiRESULT_ERROR);

		audioSource = GStreamerElement ("multifilesrc", "multifilesrc_element");
		stiTESTCOND (audioSource.get(), stiRESULT_ERROR);
		audioSource.propertySet ("location", fileName);
		audioSource.propertySet ("loop", 1);

		audioParse = GStreamerElement ("audioparse", "audioparse_element");
		stiTESTCOND (audioParse.get(), stiRESULT_ERROR);
		audioParse.propertySet ("channels", 2);
		audioParse.propertySet ("rate", 48000);

		elementAdd (audioSource);
		elementAdd (audioParse);

		linked = audioSource.link (audioParse) && audioParse.link (spectrum) && spectrum.link (audioSink);
	}

	stiTESTCONDMSG (linked, stiRESULT_ERROR, "CstiAudioHandlerBase::inputTestPipelineCreate: Failed to link elements\n");

	// Magnitude data from spectrum comes by bus messages
	busWatchEnable ();

	registerElementMessageCallback ("spectrum", [this](const GstStructure &messageStruct)
		{
			spectrumMessageHandle(messageStruct);
		});

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


void TestAudioInputPipeline::spectrumMessageHandle (const GstStructure &messageStruct)
{
	std::vector<float> channelMagnitudes[MIC_CHANNELS];

	const auto magnitudes = gst_structure_get_value (&messageStruct, "magnitude");

	if (GST_VALUE_HOLDS_LIST (magnitudes)) // multi-channel is set to off
	{
		for (auto i = 0; i < m_numberOfBands; i++)
		{
			const auto mag = gst_value_list_get_value (magnitudes, i);

			if (mag != nullptr)
			{
				channelMagnitudes[0].push_back (g_value_get_float (mag));
			}
		}
	}
	else // multi-channel is set to on
	{
		for (auto i = 0U; i < MIC_CHANNELS; i++)
		{
			const auto channel = gst_value_array_get_value (magnitudes, i);

			for (auto j = 0; j < m_numberOfBands; j++)
			{
				const auto mag = gst_value_array_get_value (channel, j);

				if (mag != nullptr)
				{
					channelMagnitudes[i].push_back (g_value_get_float (mag));
				}
			}
		}
	}

	micMagnitudeSignal.Emit (channelMagnitudes[0], channelMagnitudes[1]);
}
