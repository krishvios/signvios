/*!\brief Audio pipeline that takes its input from a file.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPipeline.h"
#include "IAudioTest.h"


class AudioFileSourcePipeline: virtual public AudioPipeline
{
public:

	AudioFileSourcePipeline () = default;
	~AudioFileSourcePipeline () override = default;

	AudioFileSourcePipeline (const AudioFileSourcePipeline &other) = delete;
	AudioFileSourcePipeline (AudioFileSourcePipeline &&other) = delete;
	AudioFileSourcePipeline &operator= (const AudioFileSourcePipeline &other) = delete;
	AudioFileSourcePipeline &operator= (AudioFileSourcePipeline &&other) = delete;

	void create (
		AudioInputType input,
		int volume,
		AudioPipeline::OutputDevice outputDevice);

	void volumeSet (int volume);

protected:

private:
};
