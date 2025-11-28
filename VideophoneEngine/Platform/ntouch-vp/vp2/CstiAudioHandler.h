// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#pragma once

// Includes
//
#include "CstiAudioHandlerBase.h"
#include "AudioRecordPipelineVP2.h"
#include "AudioPlaybackPipelineVP2.h"
#include "AudioPipelineOverridesVP2.h"

//
// Forward declarations
//
namespace vpe
{
class BluetoothAudio;
}


class CstiAudioHandler : public CstiAudioHandlerBase<
	AudioRecordPipelineVP2,
	AudioPlaybackPipelineVP2,
	AudioFileSourcePipelinePlatform<VP2Overrides>,
	TestAudioInputPipelinePlatform<VP2Overrides>>
{
public:

	CstiAudioHandler () = default;

	stiHResult Initialize (
		EstiAudioCodec eAudioCodec,
		CstiMonitorTask *pMonitor,
		const nonstd::observer_ptr<vpe::BluetoothAudio> &bluetooth);

	~CstiAudioHandler() override = default;

	CstiAudioHandler (const CstiAudioHandler &other) = delete;
	CstiAudioHandler (CstiAudioHandler &&other) = delete;
	CstiAudioHandler &operator= (const CstiAudioHandler &other) = delete;
	CstiAudioHandler &operator= (CstiAudioHandler &&other) = delete;

protected:

private:

	CstiSignalConnection m_deviceUpdatedSignalConnection;
};

