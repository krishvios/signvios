// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

// Includes
//

#include <CstiAudioHandlerBase.h>
#include <atomic>
#include "AudioRecordPipelineVP4.h"
#include "AudioPlaybackPipelineVP4.h"
#include "AudioPipelineOverridesVP4.h"

//
// Forward declarations
//
namespace vpe
{
class BluetoothAudio;
}

class CstiAudioHandler : public CstiAudioHandlerBase<
	AudioRecordPipelineVP4,
	AudioPlaybackPipelineVP4,
	AudioFileSourcePipelinePlatform<VP4Overrides>,
	TestAudioInputPipelinePlatform<VP4Overrides>>
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

