/*!
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "AudioPipeline.h"

class IAudioTest
{
public:

	IAudioTest (const IAudioTest &other) = delete;
	IAudioTest (IAudioTest &&other) = delete;
	IAudioTest &operator= (const IAudioTest &other) = delete;
	IAudioTest &operator= (IAudioTest &&other) = delete;

	static IAudioTest* InstanceGet ();

	virtual void testAudioInputStart (AudioInputType input, AudioPipeline::OutputDevice outputDevice, int numberOfBands) = 0;
	virtual void testAudioInputStop () = 0;
	
	virtual void testAudioOutputStart (AudioInputType input, int volume, AudioPipeline::OutputDevice outputDevice) = 0;
	virtual void testAudioOutputStop () = 0;

protected:

	IAudioTest () = default;
	virtual ~IAudioTest () = default;
};
