/*!
*  \file CstiGStreamerVideoGraphBase.h
*  \brief The GStremer Interface
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2021 Sorenson Communications, Inc. -- All rights reserved
*
*/
#pragma once

#include "stiError.h"
#include "stiDefs.h"
#include "CstiSignal.h"
#include "GStreamerElement.h"
#include "GStreamerAppSink.h"
#include "GStreamerPipeline.h"
#include "IstiVideoInput.h"
#include "IstiVideoOutput.h"
#include "CstiCameraControl.h"
#include "IstiPlatform.h"
#include "FrameCaptureBinBase.h"
#include "EncodePipelineBase.h"
#include "CstiBSPInterfaceBase.h"

#include <gst/gst.h>
#include <chrono>

class GStreamerSample;
class GStreamerPad;

class CstiGStreamerVideoGraphBase
{
public:

	CstiGStreamerVideoGraphBase (
		bool rcuConnected);
	virtual ~CstiGStreamerVideoGraphBase ();

	CstiGStreamerVideoGraphBase (const CstiGStreamerVideoGraphBase &other) = delete;
	CstiGStreamerVideoGraphBase (CstiGStreamerVideoGraphBase &&other) = delete;
	CstiGStreamerVideoGraphBase &operator = (const CstiGStreamerVideoGraphBase &other) = delete;
	CstiGStreamerVideoGraphBase &operator = (CstiGStreamerVideoGraphBase &&other) = delete;

	stiHResult Initialize ();

	CstiSignal<> pipelineResetSignal;
	CstiSignal<GStreamerBuffer &> frameCaptureReceivedSignal;
	CstiSignal<GStreamerSample &> encodeBitstreamReceivedSignal;

	void FrameCountGet (
		uint64_t *frameCount,
		std::chrono::steady_clock::time_point *lastFrameTime);

	stiHResult PrivacySet (
		bool bEnable);

	int maxCaptureWidthGet () const
	{
		return encodeCapturePipelineBaseGet().maxCaptureWidthGet();
	}

	int maxCaptureHeightGet () const
	{
		return encodeCapturePipelineBaseGet().maxCaptureHeightGet();
	}

	void DisplaySettingsGet (
		IstiVideoOutput::SstiImageSettings *pImageSettings) const;

	stiHResult VideoRecordSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const;

	stiHResult OutputModeSet (
		EDisplayModes eMode);

	stiHResult PTZSettingsSet (
		const CstiCameraControl::SstiPtzSettings &ptzSettings);

	float aspectRatioGet () const;

	CstiSignal<> aspectRatioChanged;

	virtual int32_t MaxPacketSizeGet () = 0;

	stiHResult KeyFrameRequest ();

	virtual stiHResult EncodeSettingsSet (
		const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings,
		bool *pbEncodeSizeChanged) = 0;
		
	virtual IstiVideoInput::SstiVideoRecordSettings EncodeSettingsGet () const = 0;

	virtual stiHResult DisplaySettingsSet (
		IstiVideoOutput::SstiImageSettings *pImageSettings) = 0;

	virtual stiHResult PipelineReset () = 0;

	virtual stiHResult EncodeStart () = 0;

	virtual stiHResult EncodeStop () = 0;

	virtual stiHResult FrameCaptureSet (
		bool bFrameCapture,
		SstiImageCaptureSize *pImageSize);

	virtual stiHResult brightnessRangeGet (
		PropertyRange *range) const;

	virtual stiHResult brightnessGet (
		int *brightness) const;

	virtual stiHResult brightnessSet (
		int brightness);

	virtual stiHResult saturationRangeGet (
		PropertyRange *range) const;

	virtual stiHResult saturationGet (
		int *saturation) const;

	virtual stiHResult saturationSet (
		int saturation);

	bool rcuConnectedGet () const
	{
		return m_bRcuConnected;
	}

	void rcuConnectedSet (
		bool rcuConnected)
	{
		m_bRcuConnected = rcuConnected;
	}

protected:

	stiHResult cpuSpeedSet (
		CstiBSPInterfaceBase::EstiCpuSpeed cpuSpeed);
	
	IstiVideoInput::SstiVideoRecordSettings m_VideoRecordSettings {};
	IstiVideoOutput::SstiImageSettings m_DisplayImageSettings {};

	CstiSignalConnection m_encodeCameraErrorConnection;
	CstiSignalConnection m_aspectRationChangedConnection;
	CstiSignalConnection m_encodeBitstreamReceivedConnection;

	bool m_bPrivacy {false};
	bool m_bEncoding {false};

	bool m_bVideoCodecSwitched {false};
	EstiVideoProfile m_videoProfile {estiPROFILE_NONE};

	EDisplayModes m_eDisplayMode {eDISPLAY_MODE_UNKNOWN};

	std::unique_ptr<FrameCaptureBinBase> m_frameCaptureBin;

	stiHResult PTZVideoConvertUpdate ();
	virtual void ptzRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle);
	CstiCameraControl::SstiPtzSettings ptzRectangleGet () const;

	stiMUTEX_ID m_pFrameCaptureMutex {nullptr};

	virtual std::unique_ptr<FrameCaptureBinBase> frameCaptureBinCreate () = 0;

	virtual const EncodePipelineBase &encodeCapturePipelineBaseGet () const = 0;
	virtual EncodePipelineBase &encodeCapturePipelineBaseGet () = 0;

	CstiCameraControl::SstiPtzSettings m_PtzSettings {};

private:

	
	void frameCountIncrement ();
	
	std::function <void(const GStreamerPad &gstPad, GStreamerBuffer &gstBuffer)> m_frameCaptureCallback;
	
	bool m_bRcuConnected {false};

	uint64_t m_frameCount {0};
	
	std::chrono::steady_clock::time_point m_lastFrameTime {};
	
	GStreamerSample m_currentGstSample;

	CstiSignalConnection::Collection m_signalConnections;
};
