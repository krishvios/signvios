#pragma once

#include "CstiVideoOutputBase.h"
#include "PlaybackPipelineBase.h"
#include "GStreamerAppSource.h"
#include "GStreamerEventProbe.h"
#include "DisplaySinkBin.h"
#include "DecodePipeline.h"

#include <gst/gst.h>

//
// Constants
//
#define ENABLE_VIDEO_OUTPUT
//#define ENABLE_DEBUG_PAD_PROBES
//#define ENABLE_FRAME_CAPTURE

#define MAX_FRAMES 10			// about 1/3 of a second (29.9 fps)

#define MAX_DISPLAY_WIDTH 1920
#define MAX_DISPLAY_HEIGHT 1080

#define VIDEO_PLAYBACK_BUFFER_SIZE (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT)


namespace CEC
{
	class ICECAdapter;
}


class CstiVideoOutputBase2 : public CstiVideoOutputBase
{
public:

	CstiVideoOutputBase2 (
		int playbackBufferSize, 
		int numPaybackBuffers);

	CstiVideoOutputBase2 (const CstiVideoOutputBase2 &other) = delete;
	CstiVideoOutputBase2 (CstiVideoOutputBase2 &&other) = delete;
	CstiVideoOutputBase2 &operator= (const CstiVideoOutputBase2 &other) = delete;
	CstiVideoOutputBase2 &operator= (CstiVideoOutputBase2 &&other) = delete;

	~CstiVideoOutputBase2 () override;

    stiHResult Startup () override;

	stiHResult videoFileLoad(
		std::string server,
		std::string fileGUID,
		std::string clientToken,
		int maxDownloadSpeed,
		bool videoLoop) override;

	stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec) override;

	stiHResult VideoPlaybackReset ();

	stiHResult VideoPlaybackStart () override;

	stiHResult VideoPlaybackStop () override;

	stiHResult videoFilePlay(
		EPlayRate speed) override;

	stiHResult videoFileSeek(
		uint32_t seconds) override;

	stiHResult VideoFilePlay (
		const std::string &filePath) override;

	stiHResult VideoFileStop () override;

	stiHResult DisplayResolutionSet (
		unsigned int width,
		unsigned int height) override;

	void RemoteViewWidgetSet (
		void *qquickItem) override;

	void playbackViewWidgetSet (
		void *qquickItem) override;

	EstiH264FrameFormat H264FrameFormatGet () override
	{
		return eH264FrameFormatByteStream;
	}

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;

	void ScreenshotTake (
		EstiScreenshotType eType) override;

	bool CECSupportedGet() override;
	
	stiHResult DisplayPowerSet (
		bool bOn) override;

	bool DisplayPowerGet () override;

	void PreferredDisplaySizeGet (
		unsigned int *displayWidth,
		unsigned int *displayHeight) const override;

	std::string tvVendorGet();

	
protected:

	void panCropRectUpdate() override;

	void eventVideoCodecSet (
		EstiVideoCodec eVideoCodec);
	void eventVideoPlaybackReset ();
	void eventVideoPlaybackStart ();

#ifdef ENABLE_VIDEO_OUTPUT
	stiHResult videoDecodePipelineDestroy ();
	stiHResult videoDecodePipelinePlay ();
	stiHResult videoDecodePipelineReady ();

	//
	// callbacks required for Gstreamer appsrc
	//
	static void appSrcNeedData (
		GstAppSrc *src,
		guint length,
		gpointer user_data);

	static void appSrcEnoughData (
		GstAppSrc *src,
		gpointer user_data);

	void decodeEventProbe (
		GStreamerPad &gstPad,
		GstEvent *event);

#ifdef ENABLE_FRAME_CAPTURE
	
	stiHResult FrameCapturePipelineGet (
		GstElement **ppGstElementPipeline) const;
		
	stiHResult FrameCaptureSet (
		bool bFrameCapture);

	stiHResult FrameCaptureGet (
		bool * pbFrameCapture);

	stiHResult FrameCaptureBufferReceived (
		GstBuffer* pGstBuffer);
	
	// frame capture sink bin functions
	stiHResult FrameCapturePipelineStart ();
	stiHResult FrameCapturePipelineCreate ();
	stiHResult FrameCapturePipelineDestroy ();

	static GstFlowReturn FrameCaptureAppSinkGstBufferPut (
		GstElement* pGstElement,
		gpointer pUser);
#endif
	
#if defined(ENABLE_DEBUG_PAD_PROBES)
	static GstPadProbeReturn DebugProbeFunction (
		GstPad * pGstPad,
		GstPadProbeInfo *info,
		gpointer pUserData);
#endif

#endif

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

	enum EVideoFilePortalDirection
	{
		eMOVE_LEFT,
		eMOVE_RIGHT,
		eMOVE_UP,
		eMOVE_DOWN
	};

	// value is defined by width*height
	enum EDisplaySize
	{
		eDisplaySizeUnknown = 0,
		eDisplaySize640x360 = 230400,
		eDisplaySize640x480 = 307200,
		eDisplaySize720x480 = 345600,
		eDisplaySize800x600 = 480000,
		eDisplaySize1024x768 = 786432,
		eDisplaySize1280x720 = 921600,
		eDisplaySize1280x800 = 1024000,
		eDisplaySize1280x1024 = 1310720,
		eDisplaySize1360x768 = 1044480,
		eDisplaySize1366x768 = 1049088,
		eDisplaySize1440x900 = 1296000,
		eDisplaySize1536x864 = 1327104,
		eDisplaySize1600x900 = 1440000,
		eDisplaySize1680x1050 = 1764000,
		eDisplaySize1920x1080 = 2073600,
		eDisplaySize1920x1200 = 2304000,
		eDisplaySize2048x1152 = 2359296,
		eDisplaySize2560x1080 = 2764800,
		eDisplaySize2560x1440 = 3686400,
		eDisplaySize3440x1440 = 4953600,
		eDisplaySize3840x2160 = 8294400,
		eDisplaySize7680x4320 = 33177600
	};


	void *m_qquickItem {nullptr};

	CstiSignalConnection::Collection m_signalConnections;

	DecodePipeline m_videoDecodePipeline;
	GStreamerAppSource m_decoderAppSrc;

	bool m_videoPlaybackStarting {false};

	int m_requestedVideoBuffers {0};

	std::unique_ptr<DisplaySinkBin> m_displaySinkBin;

	GStreamerEventProbe m_decodeEventProbe;

	CstiSignalConnection::Collection m_decodePipelineConnections;
	std::unique_ptr<PlaybackPipelineBase> m_playbackPipeline;

	virtual std::string decodeElementName(EstiVideoCodec eCodec) = 0;

	stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy) override;
	
	stiHResult RemoteViewHoldSet (
		EstiBool bHold) override;

	// move decode display sink
	stiHResult DecodeDisplaySinkMove () override;

	virtual stiHResult VideoSinkVisibilityUpdate (
		bool bClearPreviousFrame) override;

	// check if the video frame size has changed;
	stiHResult VideoPlaybackSizeCheck ();

	//
	// Event Handlers
	//
	void EventBufferReturnErrorTimerTimeout ();

	static void GstreamerVideoFrameDiscardCallback (
		gpointer mem);
	
	stiHResult H264PipelineGet (
		GstElement **ppGstElementPipeline) const;
		
//Debug
	static void videoDecoderPipelinePadAddedCallback (
		GstElement * dbin,
		GstPad * pad,
		gpointer user_data);

	EstiPowerStatus CECDisplayPowerGet ();

protected: 
	static constexpr int NUM_FLUSH_VIDEO_BUFS = 10;


#ifdef ENABLE_FRAME_CAPTURE
	bool m_bFrameCapture = false;
	std::mutex m_frameCaptureMutex;
	
	// Frame capture sink bin
	GstElement *m_pGstElementFrameCapturePipeline = nullptr;
	GStreamerAppSource m_gstElementFrameCaptureAppSrc;
	GstElement *m_pGstElementFrameCaptureConvert = nullptr;
	GstElement *m_pGstElementFrameCaptureJpegEnc = nullptr;
	GstElement *m_pGstElementFrameCaptureAppSink = nullptr;
	
	unsigned int m_unFrameCapturePipelineWatch = 0;
	gulong m_unFrameCaptureNewBufferSignalHandler = 0;
#endif


	ERunningState m_RunningState = eNone;

	std::string m_destDir;

	bool m_bCECSupported = false;
	bool m_bTelevisionPowered = false;
	
	CEC::ICECAdapter *m_pCecAdapter = nullptr;

	unsigned int m_eDisplayHeight = 0;
	unsigned int m_eDisplayWidth = 0;
	
	// Video File variables
	std::string m_VideoFileUri;
	GstElement *m_pVideoFileSink = nullptr;
	CstiTimer m_videoFilePanTimer;
	int m_nVideoFilePortalX = 0;
	int m_nVideoFilePortalY = 0;
	EVideoFilePortalDirection m_eVideoFilePortalDirection = eMOVE_RIGHT;

	enum EPlaybinFlags
	{
		eGST_PLAY_FLAG_DOWNLOAD = 0x80 // Used to tell playbin to attempt progressive download buffering.
	};

	GstFlowReturn pushBuffer (
		GStreamerBuffer &outputBuffer) override;

	virtual void platformVideoDecodePipelineCreate (); 

	virtual stiHResult H264DecodePipelineCreate ();
	virtual stiHResult H264DecodePipelineDestroy ();
	virtual stiHResult H264DecodePipelinePlay ();
	virtual stiHResult H264DecodePipelinePrime ();

#ifdef ENABLE_VIDEO_OUTPUT
	virtual stiHResult videoDecodePipelineCreate (); 
	stiHResult videoDecodePipelinePause ();
	stiHResult videoDecodePipelineNull ();

	void eventNeedsData ();
	void eventEnoughData ();
#endif

	void eventVideoFileLoad( 
		std::string server,
		std::string fileGUID,
		std::string clientToken,
		int maxDownloadSpeed,
		bool videoLoop);
	void eventVideoFilePlay(
		EPlayRate speed);
	void eventVideoFileStop ();
	void eventVideoFileSeek(
		gint64 seconds);
	void eventRingBufferingTimerTimeout();
	void eventUpdatePlaybackTimerTimeout();
	void eventVideoFilePlay (
		const std::string &filePathName);
	void eventRestartVideo();
	void EventGstreamerVideoFrameDiscard (IstiVideoPlaybackFrame *) override;

	virtual void eventVideoPlaybackStop ();
	virtual void eventDisplayResolutionSet (uint32_t, uint32_t);
	void eventDecodeSizeSet (uint32_t, uint32_t);

	std::unique_ptr<PlaybackPipelineBase> playbackPipelineCreate (const std::string &name);

private:

	bool m_videoFileLoadStarting {false};
	std::string m_filePathName;
	std::string m_server;
	std::string m_fileGUID;
	std::string m_clientToken;
    int m_maxDownloadSpeed {0};
	bool m_videoLoop {false};

	bool m_waitingForShutdown {false};
	bool m_decodePipelineNeedsData {false};

	std::condition_variable m_conditionDecoderPipelineReady;
	std::mutex m_mutexDecoderPipelineReady;

	gulong m_videoLatencySignalHandler {0};

	unsigned int m_unNextRequestId = 0;

	guint notify_id = 0;
	
	guint current_audio{0};
	guint current_video{0};
	guint current_text{0};
};
