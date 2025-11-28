/*!\brief Specializations for the VP2 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPlaybackPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP2.h"


class AudioPlaybackPipelineVP2
	: public AudioPlaybackPipeline,
	  public AudioPipelineOverrides<VP2Overrides>
{
public:

	AudioPlaybackPipelineVP2 ();
	~AudioPlaybackPipelineVP2 () override = default;

	AudioPlaybackPipelineVP2 (const AudioPlaybackPipelineVP2 &other) = delete;
	AudioPlaybackPipelineVP2 (AudioPlaybackPipelineVP2 &&other) = delete;
	AudioPlaybackPipelineVP2 &operator= (const AudioPlaybackPipelineVP2 &other) = delete;
	AudioPlaybackPipelineVP2 &operator= (AudioPlaybackPipelineVP2 &&other) = delete;

protected:

	GStreamerBin aecBinGet () override;

	void G722AppsrcPropertiesSet () override {}

private:
};


