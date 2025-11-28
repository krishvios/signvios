/*!
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "IAudioTest.h"

class CstiAudioHandler;

class AudioTest : public IAudioTest
{
public:

	AudioTest () = default;
	~AudioTest () override = default;

	AudioTest (const AudioTest &other) = delete;
	AudioTest (AudioTest &&other) = delete;
	AudioTest &operator= (const AudioTest &other) = delete;
	AudioTest &operator= (AudioTest &&other) = delete;

	void initialize (CstiAudioHandler *audioHandler);

	void testAudioInputStart (AudioInputType input, AudioPipeline::OutputDevice outputDevice, int numberOfBands) override;
	void testAudioInputStop () override;

	void testAudioOutputStart (AudioInputType input, int volume, AudioPipeline::OutputDevice device) override;
	void testAudioOutputStop () override;

private:

	CstiAudioHandler *m_audioHandler {nullptr};
};
