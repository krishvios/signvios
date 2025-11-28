/*!\brief Specializations for the VP3 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPlaybackPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP3.h"

class AudioPlaybackPipelineVP3
	: public AudioPlaybackPipeline,
	  public AudioPipelineOverrides<VP3Overrides>
{
public:

	AudioPlaybackPipelineVP3 ();
	~AudioPlaybackPipelineVP3 () = default;

	AudioPlaybackPipelineVP3 (const AudioPlaybackPipelineVP3 &other) = delete;
	AudioPlaybackPipelineVP3 (AudioPlaybackPipelineVP3 &&other) = delete;
	AudioPlaybackPipelineVP3 &operator= (const AudioPlaybackPipelineVP3 &other) = delete;
	AudioPlaybackPipelineVP3 &operator= (AudioPlaybackPipelineVP3 &&other) = delete;

protected:

	GStreamerBin aecBinGet () override;

private:
};


