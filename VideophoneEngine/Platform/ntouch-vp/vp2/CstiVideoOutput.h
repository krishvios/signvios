/*!
 * \file CstiVideoOutput.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015-2016 Sorenson Communications, Inc. -- All rights reserved
 */

#ifndef CstiVideoOutput_H
#define CstiVideoOutput_H

#include "stiSVX.h"
#include "stiError.h"

#include "CstiVideoOutputBase.h"

#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiTimer.h"

#include "CstiMonitorTask.h"
#include "CstiVideoInput.h"
#include "CstiCECControl.h"

#include "GStreamerAppSource.h"
#include "GStreamerAppSink.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/gstbufferlist.h>

#include <iostream>
#include <memory>
#include <queue>
#include <condition_variable>
#include <mutex>

//
// Constants
//
#define MAX_FRAMES 10			// about 1/3 of a second (29.9 fps)
#define MAX_DISPLAY_WIDTH 1920
#define MAX_DISPLAY_HEIGHT 1080
#define VIDEO_PLAYBACK_BUFFER_SIZE (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT)
#define MIN_DISPLAY_WIDTH 128
#define MIN_DISPLAY_HEIGHT 96

#include "CstiVideoPlaybackFrame.h"

//#define ENABLE_H264_DEBUG_PROBES
//#define ENABLE_H263_DEBUG_PROBES


class CstiVideoOutput: public CstiVideoOutputBase
{
public:

	/*!
	 * \brief Get instance
	 *
	 * \return IstiVideoOutput*
	 */
	CstiVideoOutput ();

	~CstiVideoOutput () override;

	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask,
		CstiVideoInput *pVideoInput,
		std::shared_ptr<CstiCECControl> cecControl);

	stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec) override;

	virtual stiHResult VideoPlaybackReset ();

	stiHResult VideoPlaybackStart () override;

	stiHResult VideoPlaybackStop () override;

	EstiH264FrameFormat H264FrameFormatGet () override
	{
		return eH264FrameFormatByteStream;
	}

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;

	void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const;
		
	stiHResult DisplaySettingsSet (
		const SstiDisplaySettings *pDisplaySettings);
	
	stiHResult DisplayModeCapabilitiesGet (
		uint32_t *pun32DisplayModeCapabilitiesBitMask) override;

	stiHResult DisplayModeSet (
		EDisplayModes eDisplayMode) override;

	void ScreenshotTake (
		EstiScreenshotType eType) override;

	virtual stiHResult FrameCaptureSet (
		bool bFrameCapture);

	virtual stiHResult FrameCaptureGet (
		bool * pbFrameCapture);

	bool CECSupportedGet() override;

	stiHResult DisplayPowerSet (
		bool bOn) override;

	bool DisplayPowerGet () override;

	stiHResult videoFileLoad(
		std::string server,
		std::string fileGUID,
		std::string clientToken,
		int maxDownloadSpeed,
		bool loopVideo) override
	{
		return stiRESULT_SUCCESS;
	}

	stiHResult VideoFilePlay (
		const std::string &filePath) override;

	stiHResult VideoFileStop () override;

	void PreferredDisplaySizeGet (
		unsigned int *displayWidth,
		unsigned int *displayHeight) const override;

	std::string tvVendorGet();

	stiHResult DisplayResolutionSet (
		unsigned int width,
		unsigned int height) override;

private:
	enum EstiPowerStatus
	{
		ePowerStatusOff = 0,
		ePowerStatusOn,
		ePowerStatusUnknown,
	};

	enum ERunningState
	{
		eNone = 0,
		eRunning = 1,
		eStopping = 2,
		eStopped = 3
	};

private:

	stiHResult FrameCaptureBufferReceived (
		GStreamerBuffer &gstBuffer);

	stiHResult DecodePipelinesStart ();

	// pipeline functions
	stiHResult H264DecodePipelineStart ();
	stiHResult H264DecodePipelineCreate ();
	stiHResult H264DecodePipelineDestroy ();
	stiHResult H264DecodePipelinePrime ();

	stiHResult H263DecodePipelineStart ();
	stiHResult H263DecodePipelineCreate ();
	stiHResult H263DecodePipelineDestroy ();

	// frame capture sink bin functions
	stiHResult FrameCapturePipelineStart ();
	stiHResult FrameCapturePipelineCreate ();
	stiHResult FrameCapturePipelineDestroy ();

	static GstFlowReturn FrameCaptureAppSinkGstBufferPut (
		GstElement* pGstElement,
		gpointer pUser);

#ifdef ENABLE_DEBUG_PROBES
	static gboolean DebugProbeFunction (
		GstPad * pGstPad,
		GstMiniObject * pGstMiniObject,
		gpointer pUserData);
#endif

	// move decode display sink
	stiHResult DecodeDisplaySinkMove ();

	virtual stiHResult VideoSinkVisibilityUpdate (
		bool bClearReferenceFrame) override;

	// check if the video frame size has changed;
	stiHResult VideoPlaybackSizeCheck ();

	void videoFilePipelineCleanup();

	//
	// callbacks required for Gstreamer appsrc
	//
	void H264NeedData (
		GStreamerAppSource &appSrc);

	void H264EnoughData (
		GStreamerAppSource &appSrc);

	void H263NeedData (
		GStreamerAppSource &appSrc);

	void H263EnoughData (
		GStreamerAppSource &appSrc);

	//
	// callback for appsink
	//
	static GstFlowReturn OutputDisplayJpegAvailable (
		GstAppSink *pAppSink,
		gpointer pUserData);

	stiHResult DisplaySinkStart ();
	stiHResult DisplaySinkStop ();

	stiHResult DisplaySinkCreate (
		bool bMirrored);
	stiHResult DisplaySinkAddAndLink (
		GstElement *pGstElementPipeline,
		GstPad *pGstLinkPad);
	stiHResult DisplaySinkRemove (
		GstElement *pGstElementPipeline);

	stiHResult DisplaySinkPlay ();
	stiHResult DisplaySinkNull ();

	// video file functions
	static gboolean VideoFileBusCallback(
		GstBus *pPipelineBus,
		GstMessage *pMsg,
		gpointer data);
	
	stiHResult VideoFilePipelineStop ();

	stiHResult videoFrameSizeSet(
		GstElement *decoder);

	void panCropRectUpdate() override;

	//
	// Event Handlers
	//
	void EventVideoPlaybackReset ();
	void EventVideoPlaybackStart ();
	void EventVideoPlaybackStop ();
	void EventVideoCodecSet (EstiVideoCodec);
	void EventGstreamerVideoFrameDiscard (IstiVideoPlaybackFrame *) override;
	void EventDisplayModeSet (EDisplayModes);
	void EventHdmiOutStatusChanged ();
	void EventDisplaySettingsSet (std::shared_ptr<SstiDisplaySettings> displaySettings);
	void EventH264NeedsData ();
	void EventH264EnoughData ();
	void EventH263NeedsData ();
	void EventH263EnoughData ();
	void EventVideoFilePlay (std::string);
	void EventVideoFileStop ();
	void EventBufferReturnErrorTimerTimeout ();

	static gboolean EventDropperFunction (
		GstPad * pGstPad,
		GstMiniObject * pGstMiniObject,
		gpointer pUserData);
	
	stiHResult H264PipelineGet (
		GstElement **ppGstElementPipeline) const;
	
	stiHResult H263PipelineGet (
		GstElement **ppGstElementPipeline) const;
	
	stiHResult FrameCapturePipelineGet (
		GstElement **ppGstElementPipeline) const;
	
	static gboolean H264BusCallback (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		void *pUserData);
	
	static gboolean H263BusCallback (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		void *pUserData);

	static gboolean FrameCaptureBusCallback (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		void *pUserData);
	
	stiHResult BusMessageProcess (
		GstBus *pGstBus,
		GstMessage *pGstMessage,
		void *pUserData,
		GstElement *pGstElementPipeline);
	
	EstiPowerStatus CECDisplayPowerGet ();

	stiHResult BestDisplayModeSet ();

private:
	static constexpr int NUM_DEC_PRIME_BITSTREAM_BUFS = 15;
	static constexpr const char *SCREENSHOT_SCRIPT_OVERRIDE = "/ScreenshotOverride.sh";
	static constexpr const char *SCREENSHOT_SHELL_SCRIPT = "/usr/bin/Screenshot.sh";
	static constexpr const char *SCREENSHOT_FILE = "/tmp/Screenshot.jpg";
	static constexpr const int VIDEO_FILE_PAN_PORTAL_WIDTH = 1536;
	static constexpr const int VIDEO_FILE_PAN_PORTAL_HEIGHT = 864;
	static constexpr const int VIDEO_FILE_PAN_WIDTH = 1920;
	static constexpr const int VIDEO_FILE_PAN_HEIGHT = 1080;

private:

	bool ImageSettingsValidate (
		const IstiVideoOutput::SstiImageSettings *pImageSettings);
	
	unsigned int maxDisplayWidthGet ()
	{
		return MAX_DISPLAY_WIDTH;
	}

	unsigned int minDisplayWidthGet ()
	{
		return MIN_DISPLAY_WIDTH;
	}

	unsigned int maxDisplayHeightGet ()
	{
		return MAX_DISPLAY_HEIGHT;
	}

	unsigned int minDisplayHeightGet ()
	{
		return MIN_DISPLAY_HEIGHT;
	}

	unsigned int m_unNextRequestId = 0;

	CstiMonitorTask *m_pMonitorTask = nullptr;

	/// H264
	bool m_bH264DecodePipelineNeedsData = false;

	GstElement *m_pGstElementH264DecodePipeline = nullptr;

	unsigned int m_unH264DecodePipelineWatch = 0;
	gulong m_unH264DoLatencySignalHandler = 0;

	GStreamerAppSource m_gstElementH264DecodeAppSrc;
	std::condition_variable m_conditionH264PipelineReady;
	std::mutex m_mutexH264PipelineReady;

	GstElement *m_pGstElementH264DecodeParser = nullptr;
	GstElement *m_pGstElementH264Decoder = nullptr;

	GstElement *m_pGstElementH264DecodeOutputSelector = nullptr;
	GstPad *m_pGstPadH264DecodeOutputSelectorDisplaySrc = nullptr;
	GstPad *m_pGstPadH264DecodeOutputSelectorFakeSrc = nullptr;
	GstElement *m_pGstElementH264DecodeFakeSink = nullptr;

	// H263
	bool m_bH263DecodePipelineNeedsData = false;

	GstElement *m_pGstElementH263DecodePipeline = nullptr;

	unsigned int m_unH263DecodePipelineWatch = 0;
	gulong m_unH263DoLatencySignalHandler = 0;

	GStreamerAppSource m_gstElementH263DecodeAppSrc;
	std::condition_variable m_conditionH263PipelineReady;
	std::mutex m_mutexH263PipelineReady;

	GstElement *m_pGstElementH263DecodeParser = nullptr;
	GstElement *m_pGstElementH263Decoder = nullptr;
	GstElement *m_pGstElementH263DisplayVidconv = nullptr;

	GstElement *m_pGstElementH263DecodeOutputSelector = nullptr;
	GstPad *m_pGstPadH263DecodeOutputSelectorDisplaySrc = nullptr;
	GstPad *m_pGstPadH263DecodeOutputSelectorFakeSrc = nullptr;
	GstElement *m_pGstElementH263DecodeFakeSink = nullptr;

	// Single display sink
	GStreamerElement m_decodeDisplaySink;

	bool m_bFrameCapture = false;
	std::mutex m_frameCaptureMutex;

	// Frame capture sink bin
	GstElement *m_pGstElementFrameCapturePipeline = nullptr;
	GStreamerAppSource m_gstElementFrameCaptureAppSrc;
	GstElement *m_pGstElementFrameCaptureConvert = nullptr;
	GstElement *m_pGstElementFrameCaptureJpegEnc = nullptr;
	GStreamerAppSink m_gstElementFrameCaptureAppSink;

	unsigned int m_unFrameCapturePipelineWatch = 0;
	gulong m_unNewBufferSignalHandler = 0;


	ERunningState m_RunningState = eNone;

	std::string m_destDir;

	bool m_bCECSupported = false;
	bool m_bTelevisionPowered = false;

	guint64 m_un64JitterSum = 0;
	guint64 m_un64LastQoSTime = 0;
	guint64 m_un64Latency = 0;
	guint64 m_un64PrevAvgJitter = 0;
	guint64 m_un64AvgJitter = 0;
	int m_nNumDropped = 0;

	EDisplayModes m_eDisplayMode = eDISPLAY_MODE_UNKNOWN;
	float m_fDisplayRate = 0.0;

	// Video File variables
	GstElement *m_pVideoFileConv = nullptr;
	GstElement *m_pVideoFilePipeline = nullptr;
	bool m_bVideoFileWaitingForShutdown = false;

	GstFlowReturn pushBuffer(GStreamerBuffer &gstBuffer) override;
	void applySPSFixups (uint8_t *spsNalUnit) override;
};

#endif //#ifndef CstiVideoOutput_H
