/*!\brief Specializations for the VP3 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioRecordPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP3.h"

class AudioRecordPipelineVP3
	: public AudioRecordPipeline,
	  public AudioPipelineOverrides<VP3Overrides>
{
public:

	AudioRecordPipelineVP3 () = default;
	~AudioRecordPipelineVP3 () override = default;

	AudioRecordPipelineVP3 (const AudioRecordPipelineVP3 &other) = delete;
	AudioRecordPipelineVP3 (AudioRecordPipelineVP3 &&other) = delete;
	AudioRecordPipelineVP3 &operator= (const AudioRecordPipelineVP3 &other) = delete;
	AudioRecordPipelineVP3 &operator= (AudioRecordPipelineVP3 &&other) = delete;

	void aecLevelSet (AecSuppressionLevel aecLevel);

protected:

private:

	GStreamerElement aecElementCreate() override;
};
