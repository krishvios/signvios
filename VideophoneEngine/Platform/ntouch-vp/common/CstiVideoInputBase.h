// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiEventQueue.h"
#include "IVideoInputVP2.h"
#include "CstiTimer.h"
#include "GstreamerBufferElement.h"
#include "CstiCameraControl.h"
#include "IstiVideoOutput.h"

//#define ENABLE_ENCODER_SETTINGS_TEST

//
// Forward Declarations
//
class MonitorTaskBase;
class CstiCameraControl;
class CstiGStreamerVideoGraphBase;

class CstiVideoInputBase : public CstiEventQueue, public IVideoInputVP2
{
public:

	CstiVideoInputBase ();

	CstiVideoInputBase (const CstiVideoInputBase &other) = delete;
	CstiVideoInputBase (CstiVideoInputBase &&other) = delete;
	CstiVideoInputBase &operator= (const CstiVideoInputBase &other) = delete;
	CstiVideoInputBase &operator= (CstiVideoInputBase &&other) = delete;

	~CstiVideoInputBase () override;

	stiHResult VideoRecordStart () override;
	stiHResult VideoRecordStop () override;

	stiHResult Startup ();

	stiHResult BitstreamFifoPut (
		GstreamerBufferElement *poGstreamerBufferElement);

	GstreamerBufferElement *BitstreamFifoGet ();

	stiHResult BitstreamFifoFlush ();

	stiHResult KeyFrameRequest () override;

	stiHResult PrivacyGet (
		EstiBool *pbEnabled) const override;

	stiHResult PrivacySet (
		EstiBool bEnable) override;

	stiHResult BrightnessRangeGet (
		PropertyRange *range) const override;

	int BrightnessGet () const override;

	stiHResult SaturationRangeGet (
		PropertyRange *range) const override;

	int SaturationGet () const override;

	stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings) override = 0;

	stiHResult VideoRecordFrameGet (
		SstiRecordVideoFrame* pRecordVideoFrame) override;

	void DisplaySettingsGet (
		IstiVideoOutput::SstiDisplaySettings *pDisplaySettings) const;

	stiHResult DisplaySettingsSet (
		IstiVideoOutput::SstiImageSettings *pImageSettings);

	stiHResult VideoRecordSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height);

	stiHResult AdvancedStatusGet (
		SstiAdvancedVideoInputStatus &advancedVideoInputStatus) const override;

	void NotifyUISizePosChanged()
	{
		videoInputSizePosChangedSignal.Emit();
	}

	stiHResult OutputModeSet (
		EDisplayModes eOutputMode);  // async

	stiHResult FrameCaptureRequest (
		EstiVideoCaptureDelay eDelay,
		SstiImageCaptureSize *pImageSize) override;  // async

	stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback) override
	{
		stiHResult hResult {stiRESULT_SUCCESS};
		
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("contrastLoopStart is not implemented\n");
		);
		return hResult;
	}

	stiHResult contrastLoopStop () override
	{
		stiHResult hResult {stiRESULT_SUCCESS};
		
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("contrastLoopStop is not implemented\n");
		);
		return hResult;
	}

	void selfViewEnabledSet (
		bool enabled) override
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("selfViewEnabledSet is not implemented\n");
		);
	}

	void systemSleepSet (
		bool enabled) override
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("systemSleepSet is not implemented\n");
		);
	}
	
	// TODO: Intel bug. This is needed because gstreamer/icamerasrc/libcamhal
	// have a problem when we set system time. (libcamhal timeout)
	void currentTimeSet (
		const time_t currentTime) override
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("currentTimeSet is not implemented\n");
		);
	}

	//
	// Camera methods
	//
	EstiResult cameraMove (
		uint8_t un8ActionBitMask) override;

	EstiResult cameraPTZGet (
		uint32_t *pun32HorzPercent,
		uint32_t *pun32VertPercent,
		uint32_t *pun32ZoomPercent,
		uint32_t *pun32ZoomWidthPercent,
		uint32_t *pun32ZoomHeightPercent) override;

	float aspectRatioGet () const override;

protected:

	void eventEncodeBitstreamReceived (GStreamerSample &gstSample);
	void eventFrameCaptureReceived (GStreamerBuffer &gstBuffer);
	void EventEncodeKeyFrameRequest ();
	void EventEncodePrivacySet (EstiBool bEnable);
	void EventEncodeCodecSet (EstiVideoCodec eVideoCodec);
	void EventDisplaySettingsSet (std::shared_ptr<IstiVideoOutput::SstiImageSettings> imageSettings);
	void EventFrameTimerTimeout ();
	void EventOutputModeSet (EDisplayModes eMode);
	void EventPTZSettingsSet (const CstiCameraControl::SstiPtzSettings &ptzSettings);
	void EventEncodeStart ();
	void EventEncodeStop ();
	void EventFrameCaptureRequest (SstiImageCaptureSize *pImageSize);
	void eventPipelineReset ();
	void eventEncodeProfileSet (EstiVideoProfile videoProfile);
	void EventUsbRcuConnectionStatusChanged ();
	void EventUsbRcuReset ();

protected:

	virtual bool RcuAvailableGet () const = 0;

	stiHResult monitorTaskInitialize ();
	
	stiHResult rcuStatusInitialize ();

	stiHResult gstreamerVideoGraphInitialize ();

	stiHResult bufferElementsInitialize ();

	stiHResult cameraControlInitialize (
		int captureWidth,
		int captureHeight,
		int minZoomLevel,
		int defaultSelfViewWidth,
		int defaultSelfViewHeight);
	
	void cameraErrorRestartPipeline ();
	void selfViewRestart ();

	EstiBool RcuConnectedGet () const override;

	void RcuConnectedSet (
		bool bRcuConnected);

	virtual stiHResult NoLockAutoExposureWindowSet () = 0;
	virtual stiHResult NoLockCameraOpen () { return stiRESULT_SUCCESS; }
	virtual stiHResult NoLockCameraClose () { return stiRESULT_SUCCESS; }
	virtual stiHResult NoLockRcuStatusChanged () { return stiRESULT_SUCCESS; }
	virtual stiHResult NoLockCameraSettingsSet () { return stiRESULT_SUCCESS; }
	stiHResult EncodePipelineReset () override { return stiRESULT_SUCCESS; } 
	stiHResult EncodePipelineDestroy () override { return stiRESULT_SUCCESS; } 

	bool m_bRequestedKeyFrameForEmptyFifo = false;

	bool m_bPrivacy {false};

	std::mutex m_BitstreamFifoMutex;

	CFifo<GstreamerBufferElement> m_oGstreamerBitstreamBufferFifo;
	CFifo<GstreamerBufferElement> m_oBitstreamFifo;

	bool m_portraitVideo {false};

	CstiCameraControl *m_pCameraControl {nullptr};

	CstiSignalConnection::Collection m_signalConnections;

	CstiCameraControl::SstiPtzSettings m_PtzSettings {};

	const int FRAME_TIMER_WAIT_TIME_MS = 1000;
	CstiTimer m_frameTimer {FRAME_TIMER_WAIT_TIME_MS};

	uint64_t m_un64LastFrameCount {0};
	std::chrono::steady_clock::time_point m_lastFrameTime {};
	float m_fps {0};

	bool m_bRecording {false};
	bool m_recordingToFile {false};
	bool m_bGotKeyframeAfterRecordStart {false};

	SstiImageCaptureSize m_ImageSize{};
	
	SstiVideoRecordSettings m_videoRecordSettings {};

	mutable std::mutex m_RcuConnectedMutex;

	virtual MonitorTaskBase *monitorTaskBaseGet() = 0;
	virtual CstiGStreamerVideoGraphBase &videoGraphBaseGet() = 0;
	virtual const CstiGStreamerVideoGraphBase &videoGraphBaseGet() const = 0;
	
#ifdef ENABLE_ENCODER_SETTINGS_TEST
	
	guint m_encoderSettingsTestTimerID {0};
	
	bool m_encoderSettingsTestSwitch {false};

	static gboolean encoderSettingsTest (
		void * userData);
	
	void eventEncoderSettingsTest ();
#endif

};
