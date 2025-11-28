/*!\brief Specializations for the X86 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioRecordPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesX86.h"

class AudioRecordPipelineX86
	: public AudioRecordPipeline,
	  public AudioPipelineOverrides<X86Overrides>
{
public:

	AudioRecordPipelineX86 () = default;
	~AudioRecordPipelineX86 () override = default;

	AudioRecordPipelineX86 (const AudioRecordPipelineX86 &other) = delete;
	AudioRecordPipelineX86 (AudioRecordPipelineX86 &&other) = delete;
	AudioRecordPipelineX86 &operator= (const AudioRecordPipelineX86 &other) = delete;
	AudioRecordPipelineX86 &operator= (AudioRecordPipelineX86 &&other) = delete;

protected:

private:

};
