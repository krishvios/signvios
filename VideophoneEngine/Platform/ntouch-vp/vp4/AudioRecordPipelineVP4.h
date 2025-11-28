/*!\brief Specializations for the VP4 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioRecordPipeline.h"
#include "AudioPipelineOverrides.h"
#include "AudioPipelineOverridesVP4.h"

class AudioRecordPipelineVP4
	: public AudioRecordPipeline,
	  public AudioPipelineOverrides<VP4Overrides>
{
public:

	AudioRecordPipelineVP4 () = default;
	~AudioRecordPipelineVP4 () override = default;

	AudioRecordPipelineVP4 (const AudioRecordPipelineVP4 &other) = delete;
	AudioRecordPipelineVP4 (AudioRecordPipelineVP4 &&other) = delete;
	AudioRecordPipelineVP4 &operator= (const AudioRecordPipelineVP4 &other) = delete;
	AudioRecordPipelineVP4 &operator= (AudioRecordPipelineVP4 &&other) = delete;

	void aecLevelSet (AecSuppressionLevel aecLevel);

protected:

private:

	GStreamerElement aecElementCreate() override;
};
