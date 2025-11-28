/*!\brief Specializations for the VP2 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioRecordPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP2.h"


class AudioRecordPipelineVP2
	: public AudioRecordPipeline,
	  public AudioPipelineOverrides<VP2Overrides>
{
public:

	AudioRecordPipelineVP2 () = default;
	~AudioRecordPipelineVP2 () override = default;

	AudioRecordPipelineVP2 (const AudioRecordPipelineVP2 &other) = delete;
	AudioRecordPipelineVP2 (AudioRecordPipelineVP2 &&other) = delete;
	AudioRecordPipelineVP2 &operator= (const AudioRecordPipelineVP2 &other) = delete;
	AudioRecordPipelineVP2 &operator= (AudioRecordPipelineVP2 &&other) = delete;

protected:

private:

	void micLevelProcess (const GstStructure &messageStruct) override;
};
