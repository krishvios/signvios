// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2021 Sorenson Communications, Inc. -- All rights reserved
#pragma once

//
// Includes
//

#include "IstiVideoOutput.h"			// for SstiImageSettings
#include "IstiPlatform.h"
#include "GStreamerSample.h"
#include "GStreamerAppSource.h"
#include "GStreamerAppSink.h"
#include "FrameCaptureBin.h"
#include "MediaPipeline.h"
#include "EncodePipeline.h"

// __STDC_CONSTANT_MACROS must be defined so that UINT64_C will be defined in stdint.h
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#endif

#include <cstdint>
#include <cstring>
#include <gst/gst.h>


#include "stiSVX.h"
#include "stiError.h"
#include "IstiVideoInput.h"
#include "stiOSMutex.h"
#include "CstiCameraControl.h"
#include "CstiGStreamerVideoGraphBase2.h"
#include "CaptureBinBase.h"
#include "SelfViewBinBase.h"
#include "EncodeBinBase.h"
#include "IVideoInputVP2.h"

//
// Constants
//

#define ENABLE_VIDEO_INPUT
//#define ENABLE_ENCODE

// Enable one of the following

//#define ENABLE_HARDWARE_JPEG_ENCODER
//#define ENABLE_FAKESINK


//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiGStreamerVideoGraph : public CstiGStreamerVideoGraphBase2
{
public:
	
	CstiGStreamerVideoGraph ();

	~CstiGStreamerVideoGraph () override = default;

	stiHResult DisplaySettingsSet (
		IstiVideoOutput::SstiImageSettings *pImageSettings) override;
	
protected:

	const EncodePipelineBase2 &encodeCapturePipelineBase2Get() const override
	{
		return m_encodeCapturePipeline;
	}

	EncodePipelineBase2 &encodeCapturePipelineBase2Get() override
	{
		return m_encodeCapturePipeline;
	}

protected:

	IstiVideoInput::SstiVideoRecordSettings defaultVideoRecordSettingsGet () override;

	std::unique_ptr<FrameCaptureBinBase> frameCaptureBinCreate () override;

private:
	
	stiHResult cpuSpeedSet (
		CstiBSPInterfaceBase::EstiCpuSpeed);
	
	EncodePipeline m_encodeCapturePipeline;
};
