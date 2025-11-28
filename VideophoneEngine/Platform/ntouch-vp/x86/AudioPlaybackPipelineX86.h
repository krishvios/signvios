/*!\brief Specializations for the X86 audio playback pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPlaybackPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesX86.h"

class AudioPlaybackPipelineX86
	: public AudioPlaybackPipeline,
	  public AudioPipelineOverrides<X86Overrides>
{
public:

	AudioPlaybackPipelineX86 () = default;
	~AudioPlaybackPipelineX86 () override = default;

	AudioPlaybackPipelineX86 (const AudioPlaybackPipelineX86 &other) = delete;
	AudioPlaybackPipelineX86 (AudioPlaybackPipelineX86 &&other) = delete;
	AudioPlaybackPipelineX86 &operator= (const AudioPlaybackPipelineX86 &other) = delete;
	AudioPlaybackPipelineX86 &operator= (AudioPlaybackPipelineX86 &&other) = delete;

protected:

private:
};


