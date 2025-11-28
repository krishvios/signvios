/*!
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioTest.h"
#include "CstiAudioHandler.h"


void AudioTest::initialize (CstiAudioHandler *audioHandler)
{
	m_audioHandler = audioHandler;
}


void AudioTest::testAudioInputStart (
	AudioInputType input,
	AudioPipeline::OutputDevice outputDevice,
	int numberOfBands)
{
	if (m_audioHandler)
	{
		m_audioHandler->testInputStart (input, outputDevice, numberOfBands);
	}
}


void AudioTest::testAudioInputStop ()
{
	if (m_audioHandler)
	{
		m_audioHandler->testInputStop ();
	}
}


void AudioTest::testAudioOutputStart (AudioInputType input, int volume, AudioPipeline::OutputDevice outputDevice)
{
	if (m_audioHandler)
	{
		m_audioHandler->testOutputStart (input, volume, outputDevice);
	}
}


void AudioTest::testAudioOutputStop ()
{
	if (m_audioHandler)
	{
		m_audioHandler->testOutputStop ();
	}
}
