// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

// Includes
//

#include <CstiAudioHandlerBase.h>
#include <atomic>
#include "AudioRecordPipelineVP3.h"
#include "AudioPlaybackPipelineVP3.h"
#include "AudioPipelineOverridesVP3.h"

//
// Forward declarations
//
namespace vpe
{
class BluetoothAudio;
}

class CstiAudioHandler : public CstiAudioHandlerBase<
	AudioRecordPipelineVP3,
	AudioPlaybackPipelineVP3,
	AudioFileSourcePipelinePlatform<VP3Overrides>,
	TestAudioInputPipelinePlatform<VP3Overrides>>
{
public:

	CstiAudioHandler ()  = default;

	stiHResult Initialize (
		EstiAudioCodec eAudioCodec,
		CstiMonitorTask *pMonitor,
		const nonstd::observer_ptr<vpe::BluetoothAudio> &bluetooth);

	~CstiAudioHandler() override = default;

	CstiAudioHandler (const CstiAudioHandler &other) = delete;
	CstiAudioHandler (CstiAudioHandler &&other) = delete;
	CstiAudioHandler &operator= (const CstiAudioHandler &other) = delete;
	CstiAudioHandler &operator= (CstiAudioHandler &&other) = delete;

	void dspReset();

protected:

private:

	bool doesEdidSupportHdmiAudio() override;

};

