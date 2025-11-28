/*!\brief Test audio input pipeline
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "AudioPipeline.h"
#include "IAudioTest.h"
#include <vector>


struct TestAudioInputSettings
{
	AudioInputType inputType {AudioInputType::Unknown};
	AudioPipeline::OutputDevice outputDevice {AudioPipeline::OutputDevice::None};
	int numberOfBands {2};
};


class TestAudioInputPipeline : virtual public AudioPipeline
{
public:

	TestAudioInputPipeline () = default;
	~TestAudioInputPipeline () override = default;

	TestAudioInputPipeline (const TestAudioInputPipeline &other) = delete;
	TestAudioInputPipeline (TestAudioInputPipeline &&other) = delete;
	TestAudioInputPipeline &operator= (const TestAudioInputPipeline &other) = delete;
	TestAudioInputPipeline &operator= (TestAudioInputPipeline &&other) = delete;

	void create (const TestAudioInputSettings &settings, bool useHeadsetInput);

	CstiSignal<std::vector<float>&, std::vector<float>&> micMagnitudeSignal;

protected:

private:

	void spectrumMessageHandle (const GstStructure &messageStruct);

	int m_numberOfBands {2};
};
