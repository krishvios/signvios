// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#pragma once

//
// Includes
//

#include "IstiVideoOutput.h"			// for SstiImageSettings
#include "IstiPlatform.h"
#include "GStreamerSample.h"
#include "GStreamerAppSource.h"
#include "GStreamerAppSink.h"
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
#include "CstiGStreamerVideoGraphBase.h"
#include "FrameCaptureBin.h"

//
// Constants
//

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

class CstiGStreamerVideoGraph : public CstiGStreamerVideoGraphBase
{
public:
	
	CstiGStreamerVideoGraph (
		bool rcuConnected);
	
	~CstiGStreamerVideoGraph () override;

	stiHResult PipelineReset ();

	stiHResult PipelineDestroy ();

	stiHResult EncodeStart ();

	stiHResult EncodeStop ();
	
	stiHResult PipelinePlayingGet (
		bool * pbPipelinePlaying);
	
	stiHResult EncodeSettingsSet (
		const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings,
		bool *pbEncodeSizeChanged);
	
	IstiVideoInput::SstiVideoRecordSettings EncodeSettingsGet () const;
	
	stiHResult DisplaySettingsSet (
		IstiVideoOutput::SstiImageSettings *pImageSettings);
	
	int32_t MaxPacketSizeGet ();
	
	stiHResult FrameCaptureGet (
		bool * pbFrameCapture);

	bool HdmiPassthroughGet ();
	
	stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough);

	stiHResult RcuConnectedSet (
		bool bRcuConnected);

	stiHResult HdmiInResolutionSet (
		int nHdmiInResolution);
	
	stiHResult FrameCaptureSet (
		bool frameCapture,
		SstiImageCaptureSize *imageSize) override;

	void cameraControlFdSet (
		int cameraControlFd);

protected:

	const EncodePipelineBase &encodeCapturePipelineBaseGet () const override
	{
		return m_encodeCapturePipeline;
	}

	EncodePipelineBase &encodeCapturePipelineBaseGet () override
	{
		return m_encodeCapturePipeline;
	}

	std::unique_ptr<FrameCaptureBinBase> frameCaptureBinCreate () override;

private:

	stiHResult BusMessageProcess (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		gpointer pUserData,
		GstElement *pGstElementPipeline);
	
	stiHResult VideoRecordSizeSet (
		uint32_t un32Width,
		uint32_t un32Height);
	
	// pipelines
	// only one of these two pipelines
	// can be running or even created at one time
	
	// Encode pipeline functions
	stiHResult EncodePipelineCreate ();
	stiHResult EncodePipelineDestroy ();
	stiHResult EncodePipelineStart ();
	
	stiHResult HdmiPipelineCreate ();
	stiHResult HdmiPipelineDestroy ();
	stiHResult HdmiPipelineStart ();

	// test src capture functions
	stiHResult TestSrcCaptureCreate ();

	stiHResult CaptureAdd (
		GstElement *pGstElementPipeline);

	stiHResult PrivacySourceBinCreate ();
	stiHResult PrivacySourceBinAdd (
		GstElement *pGstElementPipeline);
	stiHResult PrivacySourceBinRemove (
		GstElement *pGstElementPipeline);

	bool m_bVideoCodecSwitched {false};

	// Encode pipeline
	EncodePipeline m_encodeCapturePipeline;

	gulong m_unFrameCaptureNewBufferSignalHandler;

	// Hdmi pipeline
	GstElement *m_pGstElementHdmiCapturePipeline;
	
	// hdmi passthrough video elements
	GstElement *m_pGstElementHdmiVideoSource;
	GstElement *m_pGstElementHdmiInputFilter;
	//GstElement *m_pGstElementHdmiVideoConvert;
	GstElement *m_pGstElementHdmiVideoQueue;
	GstElement *m_pGstElementHdmiVideoSink;

	// hdmi passthrough audio elements
	GstElement *m_pGstElementHdmiAudioSource;
	GstElement *m_pGstElementHdmiAudioFilter;
	GstElement *m_pGstElementHdmiAudioQueue;
	GstElement *m_pGstElementHdmiAudioSink;

	bool m_bEncodePipelinePlaying;
	bool m_bHdmiPassthrough;

	int m_nHdmiInResolution;
};
