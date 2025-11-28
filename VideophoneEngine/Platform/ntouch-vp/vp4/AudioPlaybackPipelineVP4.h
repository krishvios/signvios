/*!\brief Specializations for the VP4 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPlaybackPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP4.h"

class AudioPlaybackPipelineVP4
	: public AudioPlaybackPipeline,
	  public AudioPipelineOverrides<VP4Overrides>
{
public:

	AudioPlaybackPipelineVP4 ();
	~AudioPlaybackPipelineVP4 () override = default;

	AudioPlaybackPipelineVP4 (const AudioPlaybackPipelineVP4 &other) = delete;
	AudioPlaybackPipelineVP4 (AudioPlaybackPipelineVP4 &&other) = delete;
	AudioPlaybackPipelineVP4 &operator= (const AudioPlaybackPipelineVP4 &other) = delete;
	AudioPlaybackPipelineVP4 &operator= (AudioPlaybackPipelineVP4 &&other) = delete;

protected:

	GStreamerBin aecBinGet () override;

private:
};


