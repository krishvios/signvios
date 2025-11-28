/*!\brief Base class for audio pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioPipeline.h"
#include "stiSVX.h"


void AudioPipeline::speakerEnable ()
{
	if (system (speakerEnableStringGet ().c_str()) != 0)
	{
		stiASSERTMSG (false, "error configuring speaker\n");
	}
}

void AudioPipeline::headsetEnable ()
{
	if (system (headphoneEnableStringGet ().c_str()) != 0)
	{
		stiASSERTMSG (false, "error configuring headset\n");
	}
}

GstStateChangeReturn AudioPipeline::stateSet (GstState state)
{
	auto result = GStreamerElement::stateSet (state);

	// If we went to a playing state then change the amixer settings
	// NOTE: this currently needs to happen *after* setting the state to playing, not before.
	if (state == GST_STATE_PLAYING)
	{
		switch (m_outputDevice)
		{
			case OutputDevice::Headset:

				headsetEnable ();

				break;

			case OutputDevice::Default:
			case OutputDevice::Speaker:

				speakerEnable ();

				break;

			case OutputDevice::None:

				break;
		}

		m_pipelineState = PipelineState::Playing;
	}
	else
	{
		m_pipelineState = PipelineState::Paused;
	}

	return result;
}

#define DEFAULT_RINGER_FILENAME "medium_tone_ringer.raw"
#define LOW_TONE_RINGER_FILENAME "low_tone_ringer.raw"
#define HI_TONE_RINGER_FILENAME "hi_tone_ringer.raw"
#define STEREO_TEST_FILENAME "stereo_test.raw"
#define TONE_TEST_FILENAME "1khz_tone.raw"

std::string audioInputTypeNameGet (AudioInputType input)
{
	std::string dataDir;
	std::string fileName;

	stiOSStaticDataFolderGet (&dataDir);

	switch (input)
	{
		case AudioInputType::LowPitchRinger:
		{
			fileName = dataDir + "/" + LOW_TONE_RINGER_FILENAME;
			break;
		}
		case AudioInputType::MediumPitchRinger:
		{
			fileName = dataDir + "/" + DEFAULT_RINGER_FILENAME;
			break;
		}
		case AudioInputType::HighPitchRinger:
		{
			fileName = dataDir + "/" + HI_TONE_RINGER_FILENAME;
			break;
		}
		case AudioInputType::StereoTest:
		{
			fileName = dataDir + "/" + STEREO_TEST_FILENAME;
			break;
		}
		case AudioInputType::ToneTest:
		{
			fileName = dataDir + "/" + TONE_TEST_FILENAME;
			break;
		}
		case AudioInputType::Microphone:
		case AudioInputType::Unknown:
		{
			break;
		}
	}

	return fileName;
}

