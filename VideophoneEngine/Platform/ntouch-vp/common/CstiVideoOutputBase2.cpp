#include "CstiVideoOutputBase2.h"
#include "stiDebugTools.h"
#include "stiTools.h"
#include "CstiVideoInput.h"
#include "stiTools.h"
#include <cmath>
#include "CstiCECControl.h"

#define MAX_LATENESS 50 * GST_MSECOND
#define TIME_TO_DELAY_QOS 1500 * GST_MSECOND
#define DEFAULT_LATENCY 100 * GST_MSECOND
#define MAX_LATENCY 250 * GST_MSECOND

#define DECODE_MAX_LATENESS       (20 * GST_MSECOND)
#define DECODE_LATENCY_IN_BUFFERS 3
#define DECODE_LATENCY_IN_NS      (70 * GST_MSECOND)

//#define ENABLE_VIDEO_OUTPUT

CstiVideoOutputBase2::CstiVideoOutputBase2 (
	int playbackBufferSize, int numPaybackBuffers)
:
	CstiVideoOutputBase(playbackBufferSize, numPaybackBuffers)
{
	m_signalConnections.push_back(m_bufferReturnErrorTimer.timeoutEvent.Connect (
	[this]() {
		PostEvent(
			std::bind(&CstiVideoOutputBase2::EventBufferReturnErrorTimerTimeout, this));
	}));
}

CstiVideoOutputBase2::~CstiVideoOutputBase2 ()
{
	// Will wait until thread is joined
	CstiEventQueue::StopEventLoop();

#ifdef ENABLE_VIDEO_OUTPUT
	videoDecodePipelineDestroy ();
#endif

#ifdef ENABLE_FRAME_CAPTURE
	FrameCapturePipelineDestroy ();
#endif

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::~CstiVideoOutput: Frames in Application: ", m_FramesInApplication.size (), "\n");
		vpe::trace ("CstiVideoOutputBase2::~CstiVideoOutput: Frames in GStreamer: ", m_FramesInGstreamer.size (), "\n");
	);

	if (m_Signal)
	{
		stiOSSignalClose (&m_Signal);
		m_Signal = nullptr;
	}
}



stiHResult CstiVideoOutputBase2::Startup ()
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::Startup\n");
	);
	
	m_playbackPipeline = playbackPipelineCreate("PlaybackPipelineBasePlayback");

	m_signalConnections.push_back(videoFileClosedSignalGet().Connect(
		[this](const std::string)
		{
			if (m_videoFileLoadStarting)
			{
				PostEvent([this]{eventRestartVideo ();});
			}
			else if (m_waitingForShutdown)
			{
				PostEvent ([this] 
				{
					m_waitingForShutdown = false;
					eventVideoFileLoad (m_server, 
										m_fileGUID, 
										m_clientToken, 
										m_maxDownloadSpeed, 
										m_videoLoop);
				});

			}

		}));
		
	return CstiVideoOutputBase::Startup();
}


stiHResult CstiVideoOutputBase2::H264DecodePipelineCreate ()
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutputBase2::H264DecodePipelineDestroy ()
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutputBase2::H264DecodePipelinePlay ()
{
	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutputBase2::H264DecodePipelinePrime ()
{
	return stiRESULT_SUCCESS;
}


/*!
 * \brief VideoFileLoad: Copies server, fileGUID, clientToke, maxDownloadSpeed
 * 		  and sends it with an event to start the video playing
 *
 * \param server Server address
 * \param fileGUID Path to the file on the server.
 * \param cleintToken 
 * \param maxDownloadSpeed needed when downloading the default Greeting
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutputBase2::videoFileLoad (
	const std::string server,
	const std::string fileGUID,
	const std::string clientToken,
	int maxDownloadSpeed,
	bool videoLoop) 
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::videoFileLoad\n");
	);
	
	auto hResult = stiRESULT_SUCCESS;

	stiTESTCOND(!fileGUID.empty(), stiRESULT_ERROR);

	PostEvent (
		[this, server, fileGUID, clientToken, maxDownloadSpeed, videoLoop] 
		{
			eventVideoFileLoad (server, 
							   fileGUID, 
							   clientToken, 
							   maxDownloadSpeed, 
							   videoLoop);
		});

STI_BAIL:

	return (hResult);
}


/*!
 * \brief EventScreenSaverVideoFileLoad: Creates the G-Streamer pipeline used to play the video file.
 *
 * \param server Server address
 * \param fileGUID Path to the file on the server.
 * \param cleintToken 
 * \param maxDownloadSpeed needed when downloading the default Greeting
 *
 * \return Success or failure result
 */
void CstiVideoOutputBase2::eventVideoFileLoad (
	const std::string server,
	const std::string fileGUID,
	const std::string clientToken,
	int maxDownloadSpeed,
	bool videoLoop)
{
	auto hResult = stiRESULT_SUCCESS;
	auto createPipeline = false;

	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::EventVideoFileLoad\n");
	);

	stiTESTCOND(!m_bPlaying, stiRESULT_ERROR);

	stiTESTCOND(!m_playbackPipeline->get (), stiRESULT_ERROR);

	createPipeline = true;
	hResult = m_playbackPipeline->videoFileLoad(server, 
												fileGUID,
												clientToken,
												maxDownloadSpeed,
												videoLoop);
	stiTESTRESULT();

	m_waitingForShutdown = false;
	m_server.clear();     
	m_fileGUID.clear();   
	m_clientToken.clear();
	m_maxDownloadSpeed = 0;
	m_videoLoop = false;

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("CstiVideoOutputBase2::EventVideoFileLoad: stiIS_ERROR(hResult)\n");
		);
		
		// If we have a pipeline that we didn't create we are probalby waiting for it to shutdown 
		// (i.e. screensaver didn't cleanup fast enough before we tried to play a video). So when it
		// finishes the shutdown try again to load the video.
		if (m_playbackPipeline->get () &&
			!createPipeline)
		{
			m_waitingForShutdown = true;
			m_server = server;     
			m_fileGUID = fileGUID;   
			m_clientToken = clientToken;
			m_maxDownloadSpeed = maxDownloadSpeed;
			m_videoLoop = videoLoop;
		}
		else
		{
			// Noitfy caller that the load of the file failed.
			videoFileStartFailedSignal.Emit ();

			if (m_playbackPipeline->get ())
			{
				m_playbackPipeline->playbackPipelineStop ();
			}
		}
	}
}


stiHResult CstiVideoOutputBase2::videoFilePlay (
	const EPlayRate speed)
{
	PostEvent ([this, speed]{eventVideoFilePlay (speed);});

	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase2::eventVideoFilePlay (
	const EPlayRate speed)
{
	auto hResult = stiRESULT_SUCCESS;

	stiTESTCOND(m_playbackPipeline->get (), stiRESULT_ERROR);

	m_playbackPipeline->videoFilePlay (speed);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("CstiVideoOutputBase2::EventVideoFilePlay: stiIS_ERROR(hResult)\n");
		);
	}
}


/*!
 * \brief Sets the QQuickItem that defines the video output window for the display sink
 */
void CstiVideoOutputBase2::RemoteViewWidgetSet (
	void *qquickItem)
{
	PostEvent (
		[this, qquickItem]
		{
			stiDEBUG_TOOL (g_stiVideoOutputDebug,
				stiTrace ("CstiVideoOutputBase2::RemoteViewWidgetSet setting qquickItem = %p\n", qquickItem);
			);

			if (m_qquickItem != qquickItem)
			{
				m_qquickItem = qquickItem;

				platformVideoDecodePipelineCreate ();
			}
		});
}


/*!
 * \brief Sets the QQuickItem that defines the video output window for the display sink
 */
void CstiVideoOutputBase2::playbackViewWidgetSet (
	void *qquickItem)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		stiTrace ("CstiVideoOutputBase2::playbackViewWidgetSet setting qquickItem = %p\n", qquickItem);
	);
	
	m_playbackPipeline->playbackViewWidgetSet (qquickItem);
}


void CstiVideoOutputBase2::eventRestartVideo ()
{
	m_videoFileLoadStarting = false;

	eventVideoFilePlay(m_filePathName);
}


/*!
 * \brief videoFileStop: Sends an event to stop playing the video.
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutputBase2::VideoFileStop ()
{
	PostEvent ([this]{eventVideoFileStop ();});

	return stiRESULT_SUCCESS;
}

/*!
 * \brief eventVideoFileStop: Causes the video to stop playing and the G-Streamer pipeline to be cleaned up.
 *
 * \param poEvent
 *
 * \return Success or failure result
 */
void CstiVideoOutputBase2::eventVideoFileStop ()
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::eventVideoFileStop\n");
	);
	
	m_videoFilePanTimer.Stop ();

	m_playbackPipeline->playbackPipelineStop ();
}


stiHResult CstiVideoOutputBase2::videoFileSeek (
		const uint32_t seconds)
{
	PostEvent ([this, seconds]{eventVideoFileSeek(seconds);});

	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase2::eventVideoFileSeek (
		const gint64 seconds)
{
	auto hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);

	stiTESTCOND(m_playbackPipeline->get (), stiRESULT_ERROR);

	m_playbackPipeline->videoFileSeek (seconds);

STI_BAIL:

	return;
}


/*!
 * \brief VideoFilePlay: Copies pFilePathName and sends it with an event to start the video playing
 *
 * \param filePathName Name and Path to the video file to be played.
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutputBase2::VideoFilePlay (
	const std::string &filePathName)
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::VideoFilePlay %s\n", filePathName.c_str());
	);
	
	auto hResult = stiRESULT_SUCCESS;

	stiTESTCOND(!filePathName.empty(), stiRESULT_ERROR);

	PostEvent (
		[this, filePathName] 
		{
			eventVideoFilePlay (filePathName);
		});

STI_BAIL:

	return (hResult);
}


/*!
 * \brief EventVideoFilePlay: Creates the G-Streamer pipeline used to play the video file and starts it playing.
 *
 * \param filePathName
 *
 * \return Success or failure result
 */
void CstiVideoOutputBase2::eventVideoFilePlay (
	const std::string &filePathName)
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("CstiVideoOutputBase2::eventVideoFilePlay %s\n", filePathName.c_str());
	);
	
	auto hResult = stiRESULT_SUCCESS;

	// If we don't have a qquickItem we need to wait for the item.  If we are already
	// playing then stop the player and restart it with the file we want to play.
	if (m_playbackPipeline->get ()) 
	{
		
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("CstiVideoOutputBase2::eventVideoFilePlay: m_qquickItem = %p\n", m_qquickItem);
			stiTrace("CstiVideoOutputBase2::eventVideoFilePlay: m_playbackPipeline->get() = %p\n", m_playbackPipeline->get());
		);
		
		m_videoFileLoadStarting = true;
		m_filePathName = filePathName;

		if (m_playbackPipeline->get ())
		{
			PostEvent ([this]{eventVideoFileStop ();});
		}
	}
	else 
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("CstiVideoOutputBase2::eventVideoFilePlay: m_qquickItem is not NULL\n");
			stiTrace("CstiVideoOutputBase2::eventVideoFilePlay: m_playbackPipeline->get() = %p\n", m_playbackPipeline->get());
		);
		
		// Initialize the pan variables.
		m_nVideoFilePortalX = 0;
		m_nVideoFilePortalY = 0;
		m_eVideoFilePortalDirection = eMOVE_RIGHT;

		hResult = m_playbackPipeline->screensaverLoad (filePathName,
													  VIDEO_FILE_PAN_WIDTH - VIDEO_FILE_PAN_PORTAL_WIDTH,
													  VIDEO_FILE_PAN_HEIGHT - VIDEO_FILE_PAN_PORTAL_HEIGHT);
		stiTESTRESULT();

		m_videoFileLoadStarting = false;
		m_filePathName.clear ();

		m_videoFilePanTimer.Start ();
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("CstiVideoOutputBase2::eventVideoFilePlay: stiIS_ERROR(hResult)\n");
		);

		// If we created a pipeline then clean it up.
		if (m_playbackPipeline->get ())
		{
			m_playbackPipeline->playbackPipelineStop ();
		}
		
		videoScreensaverStartFailedSignal.Emit ();
	}
}


void CstiVideoOutputBase2::panCropRectUpdate ()
{
	// Tell the playback pipeline to update the crop rect position.
	m_playbackPipeline->panCropRectUpdate (m_nVideoFilePortalX,
										  m_nVideoFilePortalY,
										  VIDEO_FILE_PAN_WIDTH - (VIDEO_FILE_PAN_PORTAL_WIDTH + m_nVideoFilePortalX),   
										  VIDEO_FILE_PAN_HEIGHT - (VIDEO_FILE_PAN_PORTAL_HEIGHT + m_nVideoFilePortalY));
}


std::unique_ptr<PlaybackPipelineBase> CstiVideoOutputBase2::playbackPipelineCreate (
	const std::string &name)
{
	auto pipeline = sci::make_unique<PlaybackPipelineBase>(name);

	m_signalConnections.push_back(pipeline->videoFileStartFailedSignal.Connect([this]
		{
			videoFileStartFailedSignal.Emit ();
		}));

	m_signalConnections.push_back(pipeline->videoFileCreatedSignal.Connect([this]
		{
			videoFileCreatedSignal.Emit ();
		}));

	m_signalConnections.push_back(pipeline->videoFileReadyToPlaySignal.Connect([this]
		{
			videoFileReadyToPlaySignal.Emit ();
		}));

	m_signalConnections.push_back(pipeline->videoFileEndSignal.Connect([this]
		{
			videoFileEndSignal.Emit ();
		}));

	m_signalConnections.push_back(pipeline->videoFileClosedSignal.Connect(
		[this](const std::string fileGUID)
		{
			videoFileClosedSignal.Emit (fileGUID);
		}));

	m_signalConnections.push_back(pipeline->videoFilePlayProgressSignal.Connect(
		[this](uint64_t position, uint64_t duration, uint64_t scale)
		{
			videoFilePlayProgressSignal.Emit (position, duration, scale);
		}));

	m_signalConnections.push_back(pipeline->videoFileSeekReadySignal.Connect([this]
		{
			videoFileSeekReadySignal.Emit ();
		}));

	m_signalConnections.push_back(pipeline->videoScreensaverStartFailedSignal.Connect([this]
		{
			videoScreensaverStartFailedSignalGet().Emit ();
		}));

	m_signalConnections.push_back(pipeline->decodeFrameSizeUpdateSignal.Connect (
		[this](uint32_t width, uint32_t height)
		{
    		if (m_un32DecodeWidth != width ||
    			m_un32DecodeHeight != height)
			{
				m_un32DecodeWidth = width;
				m_un32DecodeHeight = height;

				decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);
			}
		}));
	
	pipeline->create ();

	return pipeline;
}


/*!
 * \brief Sets codec for video playback
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase2::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::VideoPlaybackCodecSet: eVideoCodec = ", VideoCodecToString (eVideoCodec), "\n");
	);

	PostEvent ([this, eVideoCodec]
	{
		eventVideoCodecSet(eVideoCodec);
	});

	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase2::eventVideoCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::eventVideoCodecSet: set to ", VideoCodecToString (eVideoCodec), ", current video codec is ", VideoCodecToString (m_eCodec), "\n");
	);
	
	if (m_bPlaying)
	{
		stiASSERTMSG (estiFALSE, "WARNING can't change codec while playing\n");
	}
	else
	{
#ifdef ENABLE_VIDEO_OUTPUT
		m_eCodec = eVideoCodec;
		
		eventVideoPlaybackReset ();
				
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutputBase2::eventVideoCodecSet: m_eCodec = ", VideoCodecToString (m_eCodec), "\n");
		);
#endif
	}
}


stiHResult CstiVideoOutputBase2::VideoPlaybackReset ()
{
	PostEvent ([this]
	{
		eventVideoPlaybackReset();
	});

	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase2::eventVideoPlaybackReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace("CstiVideoOutputBase2::EventVideoPlaybackReset: Restarting decode pipelines\n");
	);

	if (m_qquickItem)
	{
		hResult = videoDecodePipelineCreate ();
		stiTESTRESULT ();
	
		hResult = videoDecodePipelinePlay ();
		stiTESTRESULT ();
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace("CstiVideoOutputBase2::EventVideoPlaybackReset: No m_qquickItem\n");
		);
	}

STI_BAIL:;
}


stiHResult CstiVideoOutputBase2::VideoPlaybackStart ()
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		 vpe::trace ("CstiVideoOutput::VideoPlaybackStart\n");
	);

	PostEvent (
		[this]
		{
			if (!m_qquickItem)
			{
				stiASSERTMSG(estiFALSE, "CstiVideoOutput:Base2:VideoPlaybackStart: Warning: can't start playback because quick widget isn't ready\n");
				m_videoPlaybackStarting = true;
			}
			else
			{
				eventVideoPlaybackStart ();
			}
		});

	return stiRESULT_SUCCESS;
}

void CstiVideoOutputBase2::eventVideoPlaybackStart ()
{
	EstiRemoteVideoMode eMode {eRVM_VIDEO};
	bool bModeChanged = false;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::eventVideoPlaybackStart\n");
	);

	m_maxFramesInGstreamer = 0;
	m_maxFramesInApplication = 0;
	
	m_bFirstKeyframeRecvd = false;
	m_bSPSAndPPSSentToDecoder = false;
	
	if (m_bPlaying)
	{
		keyframeNeededSignal.Emit ();
	}

	m_bPlaying = true;

	if (m_oBitstreamFifo.CountGet () > 0)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}

	if (m_bPrivacy)
	{
		bModeChanged = true;
		eMode = eRVM_PRIVACY;
	}
	else if (m_bHolding)
	{
		bModeChanged = true;
		eMode = eRVM_HOLD;
	}

#ifdef ENABLE_VIDEO_OUTPUT
	// Wait for playback to have started, otherwise we'll get flooded by frames,
	// specially for the non live case.
	ChangeStateWait (m_videoDecodePipeline, 1);
#endif

	if (bModeChanged)
	{
		remoteVideoModeChangedSignal.Emit (eMode);
	}
}


/*!
 * \brief Sends the event that will stop audio playback
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutputBase2::VideoPlaybackStop ()
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::VideoPlaybackStop\n");
	);

	PostEvent ([this]
	{
		eventVideoPlaybackStop();
	});

	return stiRESULT_SUCCESS;
}


GstFlowReturn CstiVideoOutputBase2::pushBuffer (
	GStreamerBuffer &outputBuffer)
{
	if (m_decoderAppSrc.get ())
	{
		return m_decoderAppSrc.pushBuffer (outputBuffer);
	}

	return GST_FLOW_ERROR;
}


void CstiVideoOutputBase2::EventGstreamerVideoFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//stiDEBUG_TOOL (g_stiVideoOutputDebug,
	//	vpe::trace ("CstiVideoOutputBase2::EventGstreamerVideoFrameDiscard\n");
	//);
	
	auto pVideoPlaybackFrame = static_cast<CstiVideoPlaybackFrame *>(pVideoFrame);
	
	// This call means the pipeline is still running
	// so stop the buffer return error timer
	m_bufferReturnErrorTimer.Stop ();
	
	m_FramesInGstreamer.remove (pVideoPlaybackFrame);
	
	hResult = VideoFrameDiscard (pVideoFrame);
	stiTESTRESULT ();
	
	pVideoFrame = nullptr;
	
	//TODO: Move this functionality into gstreamer with a bus call back
	// or something similar
	//hResult = VideoPlaybackSizeCheck ();
	//stiTESTRESULT ();

STI_BAIL:

	return;
}


stiHResult CstiVideoOutputBase2::DisplayResolutionSet (
		unsigned int width,
		unsigned int height)
{
	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayResolutionSet: width = ", width, " height = ", height, "\n");
	);

	PostEvent ([this, width, height]
	{
		eventDisplayResolutionSet(width, height);
	});
	
	return stiRESULT_SUCCESS;
}


void CstiVideoOutputBase2::eventDecodeSizeSet (
	uint32_t inputWidth,
	uint32_t inputHeight)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::EventDecodeSizeSet, inputWidth = ", inputWidth, ", inputHeight = ", inputHeight, "\n");
	);
	
	if (m_un32DecodeWidth != inputWidth || m_un32DecodeHeight != inputHeight)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutputBase2::EventDecodeSizeSet: input-size-changed from ", m_un32DecodeWidth, "x", m_un32DecodeHeight, " to ", inputWidth, "x", inputHeight, "\n");
		);
		
		m_un32DecodeWidth = inputWidth;
		m_un32DecodeHeight = inputHeight;

		decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);
	}
}


void CstiVideoOutputBase2::ScreenshotTake (
	EstiScreenshotType eType)
{
	
#ifdef ENABLE_FRAME_CAPTURE
	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = FrameCaptureSet (true);
	stiTESTRESULT ();
STI_BAIL:
#else
	stiASSERTMSG (estiFALSE, "CstiVideoOutputBase2::ScreenshotTake: Warning, screenshot not implemented\n");
#endif
	
	return;
}

#ifdef ENABLE_VIDEO_OUTPUT

#if defined(ENABLE_DEBUG_PAD_PROBES)
GstPadProbeReturn CstiVideoOutputBase2::DebugProbeFunction (
	GstPad * pGstPad,
	GstPadProbeInfo *info,
	gpointer pUserData)
{
	GstBuffer * pGstBuffer;
	
	pGstBuffer = GST_PAD_PROBE_INFO_BUFFER (info);

	stiDEBUG_TOOL (g_stiVideoOutputDebug > 2,
		GstMapInfo gstMapInfo;
		gst_buffer_map (pGstBuffer, &gstMapInfo, (GstMapFlags) GST_MAP_READ);
		stiTrace ("DebugProbeFunction: buffer %p, pushing, timestamp %"
				GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT
				", offset %" G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT
				", size %d, flags 0x%x\n",
				pGstBuffer,
				GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (pGstBuffer)),
				GST_TIME_ARGS (GST_BUFFER_DURATION (pGstBuffer)),
				gstMapInfo.offset, GST_BUFFER_OFFSET_END (pGstBuffer),
				gstMapInfo.size, GST_BUFFER_FLAGS (pGstBuffer));
		gst_buffer_unmap (pGstBuffer, &gstMapInfo);
	);

	return TRUE;
}
#endif

//
// this entry point is needed for GStreamer, but we don't use it for
// anything.
//
static void notify (
	gpointer data)
{
	vpe::trace ("notify was called from CstiVideoOutput.cpp code", "\n");
}

stiHResult CstiVideoOutputBase2::RemoteViewPrivacySet (
	EstiBool bPrivacy)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bModeChanged = false;

	Lock ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::RemoteViewPrivacySet: in privacy = ", m_bPrivacy, ", privacy = ", bPrivacy, ", holding = ", m_bHolding, "\n");
	);

	//
	// If we are going on privacy we will need a new SPS/PPS pair sent to the decoder
	// before decoding frames when we come off of privacy.
	//
	if (!m_bPrivacy && bPrivacy)
	{
		m_bSPSAndPPSSentToDecoder = false;
	}

	PostEvent ([this]
	{
		VideoSinkVisibilityUpdate (true);
	});

	EstiRemoteVideoMode eMode;

	if (bPrivacy)
	{
		if (!m_bPrivacy)
		{
			// If we are turning on privacy and it was not already on.
			bModeChanged = true;
			eMode = eRVM_PRIVACY;
		}

		// If we are holding then show holding.
		if (m_bHolding)
		{
			bModeChanged = true;
			eMode = eRVM_HOLD;
		}
	}
	else
	{
		if (m_bHolding)
		{
			// If we are turning off privacy and hold is on.
			bModeChanged = true;
			eMode = eRVM_HOLD;
		}
		else
		{
			// If we are turning off privacy and hold is off.
			// We want to resume vidoe after NUM_FLUSH_VIDEO_BUFS
			m_requestedVideoBuffers = NUM_FLUSH_VIDEO_BUFS;
		}
	}

	m_bPrivacy = bPrivacy;
	
	if (bModeChanged)
	{
		remoteVideoModeChangedSignal.Emit (eMode);
	}

	Unlock ();

	return (hResult);
}

stiHResult CstiVideoOutputBase2::RemoteViewHoldSet (
	EstiBool bHold)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bModeChanged = false;
	
	Lock ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::RemoteViewHoldSet: in privacy = ", m_bPrivacy, ", hold = ", bHold, ", holding = ", m_bHolding, "\n");
	);

	//
	// If we are going on hold we will need a new SPS/PPS pair sent to the decoder
	// before decoding frames when we come off of hold.
	//
	if (!m_bHolding && bHold)
	{
		m_bSPSAndPPSSentToDecoder = false;
	}

	PostEvent ([this]
	{
		VideoSinkVisibilityUpdate(true);
	});

	EstiRemoteVideoMode eMode;

	if (bHold)
	{
		if (!m_bHolding)
		{
			// If we are turning on hold and it was not already on.
			bModeChanged = true;
			eMode = eRVM_HOLD;
		}
	}
	else
	{
		if (m_bPrivacy)
		{
			// If we are turning off hold and privacy is on.
			bModeChanged = true;
			eMode = eRVM_PRIVACY;
		}
		else
		{
			// If we are turning off hold and privacy is off.
			// We want to resume vidoe after NUM_FLUSH_VIDEO_BUFS
			m_requestedVideoBuffers = NUM_FLUSH_VIDEO_BUFS;
		}
	}
	
	m_bHolding = bHold;

	if (bModeChanged)
	{
		remoteVideoModeChangedSignal.Emit (eMode);
	}

	Unlock ();

	return (hResult);
}

stiHResult CstiVideoOutputBase2::videoDecodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstAppSrcCallbacks callbacks;

	bool bResult = true;
	GStreamerPad gstPad;
	gboolean bLinked;
	bool bMirrored = false;
	GStreamerElement elementParser;
	GStreamerElement elementDecoder;
	GStreamerPad decoderSrcPad;
	GStreamerPad displaySinkPad;
	GStreamerCaps appSrcCaps;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineCreate: creating ", VideoCodecToString (m_eCodec), "pipeline\n");
	);

	hResult = videoDecodePipelineDestroy ();
	stiTESTRESULT ();

	stiASSERT (m_videoDecodePipeline.get() == NULL);
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		if (m_FramesInGstreamer.size () > 0)
		{
			vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineCreate: After destruction, frames in GStreamer: ", m_FramesInGstreamer.size (), "\n");
		}
		
		if (m_FramesInApplication.size () > 0)
		{
			vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineCreate: After destruction, frames in Application: ", m_FramesInApplication.size (), "\n");
		}
	);
	
	stiASSERTMSG (m_FramesInGstreamer.size () == 0, "%d frames stuck in gstreamer. They will be leaked\n", m_FramesInGstreamer.size ());
	
	// Reset the number of frames in gstreamer
	m_FramesInGstreamer.clear ();

	m_videoDecodePipeline = DecodePipeline ("video_decode_pipeline_element");
	stiTESTCOND (m_videoDecodePipeline.get(), stiRESULT_ERROR);

	// Register the bus callbacks so the connected signals are called
	m_videoDecodePipeline.registerBusCallbacks();

	m_decodePipelineConnections.push_back (
		m_videoDecodePipeline.decodeErrorSignal.Connect (
			[this]{
				stiDEBUG_TOOL (g_stiVideoPlaybackKeyframeDebug,
					vpe::trace ("CstiVideoOutput -- Requesting keyframe due to decoder reported corruption\n");
				);

				PostEvent ([this]{keyframeNeededSignal.Emit ();});
			}
		)
	);
	
	m_decodePipelineConnections.push_back (
		m_videoDecodePipeline.decodeStreamingErrorSignal.Connect (
			[this]{
				PostEvent ([this]{eventVideoPlaybackReset ();});
			}
		)
	);

	//m_videoLatencySignalHandler = g_signal_connect(m_pGstElementH264DecodePipeline, "do-latency", G_CALLBACK (do_latency_cb), NULL);
	
	m_decoderAppSrc = GStreamerAppSource ("video_decode_app_src_element");
	stiTESTCOND (m_decoderAppSrc.get () != nullptr, stiRESULT_ERROR);

	callbacks.need_data = appSrcNeedData;
	callbacks.enough_data = appSrcEnoughData;
	callbacks.seek_data = NULL;                 // we won't ever be seeking on this
	gst_app_src_set_callbacks (GST_APP_SRC(m_decoderAppSrc.get ()), &callbacks, this, notify);
	g_object_set (m_decoderAppSrc.get (), "do-timestamp", TRUE, NULL);
	g_object_set (m_decoderAppSrc.get (), "is-live", TRUE, NULL);
	g_object_set (m_decoderAppSrc.get (), "format", GST_FORMAT_TIME, NULL);
	g_object_set (m_decoderAppSrc.get (), "min-latency", DECODE_LATENCY_IN_NS, NULL);

	m_videoDecodePipeline.elementAdd(m_decoderAppSrc);
	stiTESTCOND (bResult, stiRESULT_ERROR);

#ifdef ENABLE_FRAME_CAPTURE
	// add app src
	m_videoDecodePipeline.elementAdd(m_gstElementFrameCaptureAppSrc);
	stiTESTCOND (bResult, stiRESULT_ERROR);
#endif	

	if (estiVIDEO_H264 == m_eCodec)
	{
		appSrcCaps = GStreamerCaps ("video/x-h264,stream-format=byte-stream,alignment=au");
		
		elementParser = GStreamerElement("h264parse", "h264parse_element");
		stiTESTCOND (elementParser.get (), stiRESULT_ERROR);

		elementDecoder = GStreamerElement(decodeElementName(estiVIDEO_H264), "264dec_element");
		stiTESTCOND (elementDecoder.get (), stiRESULT_ERROR);
	}
	else if (estiVIDEO_H265 == m_eCodec)
	{
		appSrcCaps = GStreamerCaps ("video/x-h265,stream-format=byte-stream,alignment=au");
		
		elementParser = GStreamerElement("h265parse", "h265parse_element");
		stiTESTCOND (elementParser.get (), stiRESULT_ERROR);

		elementDecoder = GStreamerElement(decodeElementName(estiVIDEO_H265), "265dec_element");
		stiTESTCOND (elementDecoder.get (), stiRESULT_ERROR);
	}
	else
	{
		stiASSERTMSG (estiFALSE, "CstiVideoOutputBase2::videoDecodePipelineCreate: WARNING m_eCodec is unknown\n");
	}
	
	if (appSrcCaps.get () != nullptr)
	{
		m_decoderAppSrc.capsSet (appSrcCaps);
	}
	
	m_videoDecodePipeline.elementAdd(elementParser);
	m_videoDecodePipeline.elementAdd(elementDecoder);
	
	bLinked = m_decoderAppSrc.link(elementParser);
	stiTESTCOND (bLinked, stiRESULT_ERROR);
	
	bLinked = elementParser.link(elementDecoder);
	stiTESTCOND (bLinked, stiRESULT_ERROR);
	
	m_displaySinkBin = sci::make_unique<DisplaySinkBin>(m_qquickItem, bMirrored);
	stiTESTCOND (m_displaySinkBin != nullptr, stiRESULT_ERROR);
	stiTESTCOND (m_displaySinkBin->get (), stiRESULT_ERROR);

	// Add display sink
	m_videoDecodePipeline.elementAdd(*m_displaySinkBin);

	decoderSrcPad = elementDecoder.staticPadGet ("src");

	displaySinkPad = m_displaySinkBin->staticPadGet ("sink");
	
	if (displaySinkPad.get () && decoderSrcPad.get ())
	{
		decoderSrcPad.link (displaySinkPad);
		m_videoDecodePipeline.writeGraph ("decode_pipeline");
	}
	
	m_decodeEventProbe = GStreamerEventProbe (decoderSrcPad, &CstiVideoOutputBase2::decodeEventProbe, this);
	
	// Setup the bus to report warnings to our callback
	m_videoDecodePipeline.busWatchEnable();

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		m_videoDecodePipeline.writeGraph("videoDecodePipelineCreate");
	);
	
	gstPad = m_decoderAppSrc.staticPadGet("src");
	stiTESTCOND (gstPad.get(), stiRESULT_ERROR);

	gstPad.addBufferProbe(
		[this](
			GStreamerPad gstPad,
			GStreamerBuffer gstBuffer)
		{
			if (m_requestedVideoBuffers != 0)
			{
				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					vpe::trace ("CstiVideoOutput video buffer probe entered. ", m_requestedVideoBuffers, " buffers requested.", "\n");
				);
				
				m_requestedVideoBuffers--;
				
				if (m_requestedVideoBuffers == 0)
				{
					remoteVideoModeChangedSignal.Emit (eRVM_VIDEO);
				}
			}

			return GST_PAD_PROBE_OK;
		});
	
#if defined(ENABLE_DEBUG_PAD_PROBES)
	TODO: Add probes at proper pads
	gstPad = m_decoderAppSrc.staticPadGet("src");
	stiTESTCOND (gstPad.get(), stiRESULT_ERROR);

	gst_pad_add_probe (gstPad.get(), GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)DebugProbeFunction, this);
#endif

STI_BAIL:

	return (hResult);
}


//Debug
#if 0
static void stream_notify_cb (
	GstStreamCollection * collection,
	GstStream * stream,
    GParamSpec * pspec,
	guint * val)
{
	g_print ("Got stream-notify from stream %s for %s (collection %p)\n",
		stream->stream_id, pspec->name, collection);
	
	if (g_str_equal (pspec->name, "caps"))
	{
		GStreamerCaps gstCaps (gst_stream_get_caps (stream), false);
		gchar *caps_str = gst_caps_to_string (gstCaps.get ());
		g_print (" New caps: %s\n", caps_str);
		g_free (caps_str);
	}
}


static void print_tag_foreach (
	const GstTagList * tags,
	const gchar * tag,
    gpointer user_data)
{
	GValue val = { 0, };
	gchar *str;
	gint depth = GPOINTER_TO_INT (user_data);

	if (!gst_tag_list_copy_value (&val, tags, tag))
	{
		return;
	}

	if (G_VALUE_HOLDS_STRING (&val))
	{
		str = g_value_dup_string (&val);
	}
	else
	{
		str = gst_value_serialize (&val);
	}

	g_print ("%*s%s: %s\n", 2 * depth, " ", gst_tag_get_nick (tag), str);
	
	g_free (str);

	g_value_unset (&val);
}

static void dump_collection (
	GstStreamCollection * collection)
{
	guint i;
	GstTagList *tags;

	for (i = 0; i < gst_stream_collection_get_size (collection); i++)
	{
		GstStream *stream = gst_stream_collection_get_stream (collection, i);
		
		g_print (" Stream %u type %s flags 0x%x\n", i,
			gst_stream_type_get_name (gst_stream_get_stream_type (stream)),
			gst_stream_get_stream_flags (stream));
		
		g_print ("  ID: %s\n", gst_stream_get_stream_id (stream));

		GStreamerCaps gstCaps(gst_stream_get_caps (stream), false);
		
		if (gstCaps.get () != nullptr)
		{
			gchar *caps_str = gst_caps_to_string (gstCaps.get ());
			g_print ("  caps: %s\n", caps_str);
			g_free (caps_str);
		}

		tags = gst_stream_get_tags (stream);
		
		if (tags)
		{
			g_print ("  tags:\n");
			gst_tag_list_foreach (tags, print_tag_foreach, GUINT_TO_POINTER (3));
			gst_tag_list_unref (tags);
		}
	}
}

gboolean CstiVideoOutputBase2::switch_streams (
	CstiVideoOutput *pVideoOutput)
{
	guint i, nb_streams;
	gint nb_video = 0, nb_audio = 0, nb_text = 0;
	GstStream *videos[256];
	GstStream *audios[256];
	GstStream *texts[256];
	GList *streams = NULL;
	GstEvent *ev;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("Switching Streams...\n");
	);

	/* Calculate the number of streams of each type */
	nb_streams = gst_stream_collection_get_size (pVideoOutput->gstStreamCollection);
	
	for (i = 0; i < nb_streams; i++)
	{
		GstStream *stream = gst_stream_collection_get_stream (pVideoOutput->gstStreamCollection, i);
		
		GstStreamType stype = gst_stream_get_stream_type (stream);
		
		if (stype == GST_STREAM_TYPE_VIDEO)
		{
			videos[nb_video] = stream;
			nb_video += 1;
		}
		else if (stype == GST_STREAM_TYPE_AUDIO)
		{
			audios[nb_audio] = stream;
			nb_audio += 1;
		} 
		else if (stype == GST_STREAM_TYPE_TEXT)
		{
			texts[nb_text] = stream;
			nb_text += 1;
		}
	}

	if (nb_video)
	{
		pVideoOutput->current_video = (pVideoOutput->current_video + 1) % nb_video;
		streams =
			g_list_append (streams,
			(gchar *) gst_stream_get_stream_id (videos[pVideoOutput->current_video]));
			
		g_print ("  Selecting video channel #%d : %s\n", pVideoOutput->current_video,
			gst_stream_get_stream_id (videos[pVideoOutput->current_video]));
	}
	
	if (nb_audio)
	{
		pVideoOutput->current_audio = (pVideoOutput->current_audio + 1) % nb_audio;
		streams =
			g_list_append (streams,
			(gchar *) gst_stream_get_stream_id (audios[pVideoOutput->current_audio]));
			
		g_print ("  Selecting audio channel #%d : %s\n", pVideoOutput->current_audio,
			gst_stream_get_stream_id (audios[pVideoOutput->current_audio]));
	}
	
	if (nb_text)
	{
		pVideoOutput->current_text = (pVideoOutput->current_text + 1) % nb_text;
		streams =
			g_list_append (streams,
			(gchar *) gst_stream_get_stream_id (texts[pVideoOutput->current_text]));
			
		g_print ("  Selecting text channel #%d : %s\n", pVideoOutput->current_text,
			gst_stream_get_stream_id (texts[pVideoOutput->current_text]));
	}

	ev = gst_event_new_select_streams (streams);
	gst_element_send_event (pVideoOutput->m_videoDecodePipeline.get(), ev);

	return G_SOURCE_CONTINUE;
}
#endif //#if 0 (DEBUG)

void CstiVideoOutputBase2::decodeEventProbe (
	GStreamerPad &gstPad,
	GstEvent *event)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug > 2,
		vpe::trace ("CstiVideoOutputBase2::decodeEventProbe", "\n");
	);
	
	if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutputBase2::decodeEventProbe: GST_EVENT_CAPS", "\n");
		);
		
		stiHResult hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG(hResult);
		int width {0};
		int height {0};
		GStreamerCaps gstCaps = gstPad.currentCapsGet();

		auto structure = gstCaps.structureGet (0);

		bool bResult = structure.intGet ("width", &width);
		stiTESTCOND (bResult, stiRESULT_ERROR);

		bResult = structure.intGet ("height", &height);
		stiTESTCOND (bResult, stiRESULT_ERROR);

		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutputBase2::decodeEventProbe: width = ", width, ", height = ", height, "\n");
		);
		
		PostEvent (
			[this, width, height]
			{
				eventDecodeSizeSet (width, height);
			});
	}

STI_BAIL:;
}


 stiHResult CstiVideoOutputBase2::videoDecodePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineDestroy\n");
	);

	m_decodePipelineConnections.clear();
	
	remoteVideoModeChangedSignal.Emit (eRVM_BLANK);

	if (m_videoDecodePipeline.get())
	{
		//
		// Stop pipeline
		//
		StateChangeReturn = m_videoDecodePipeline.stateSet(GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_videoDecodePipeline, GST_STATE_NULL)\n");
		}

		if (m_videoLatencySignalHandler)
		{
			g_signal_handler_disconnect (m_videoDecodePipeline.get(), m_videoLatencySignalHandler);
			m_videoLatencySignalHandler = 0;
		}

		m_videoDecodePipeline.clear();

		m_decoderAppSrc.clear ();
		m_displaySinkBin = nullptr;
	}

STI_BAIL:

	return (hResult);
}


void CstiVideoOutputBase2::appSrcNeedData (
	GstAppSrc *src,
	guint length,
	gpointer user_data)
{
	CstiVideoOutputBase2 *pThis = (CstiVideoOutputBase2 *)user_data;

	std::lock_guard<std::mutex> lock (pThis->m_mutexDecoderPipelineReady);

	if (!pThis->m_decodePipelineNeedsData)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
			vpe::trace ("CstiVideoOutputBase2:::appSrcNeedData\n");
		);

		pThis->m_decodePipelineNeedsData = true;
		pThis->m_conditionDecoderPipelineReady.notify_all ();

		pThis->PostEvent ([pThis]
			{
				pThis->eventNeedsData ();
			});
	}
}


void CstiVideoOutputBase2::eventNeedsData ()
{
	//
	// If there are buffers in the fifo then signal to the engine that we
	// are ready.
	//
	stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
		vpe::trace ("CstiVideoOutputBase2:::EventNeedsData\n");
	);
	
	if (m_oBitstreamFifo.CountGet () > 0 && m_bPlaying)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}
}


void CstiVideoOutputBase2::appSrcEnoughData (
	GstAppSrc *src,
	gpointer user_data)
{
	CstiVideoOutputBase2 *pThis = (CstiVideoOutputBase2 *)user_data;

	std::lock_guard<std::mutex> lock (pThis->m_mutexDecoderPipelineReady);

	if (pThis->m_decodePipelineNeedsData)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
			vpe::trace ("CstiVideoOutputBase2:::appSrcEnoughData\n");
		);
		
		pThis->m_decodePipelineNeedsData = false;

		pThis->PostEvent ([pThis]
			{
				pThis->eventEnoughData ();
			});
	}
}


void CstiVideoOutputBase2::eventEnoughData ()
{
	//
	// Signal to the engine that we are not ready
	// to receive frames.
	//
	stiDEBUG_TOOL (g_stiVideoOutputDebug > 3,
		vpe::trace ("CstiVideoOutputBase2:::EventEnoughData\n");
	);
	
	stiOSSignalSet2 (m_Signal, 0);
}


stiHResult CstiVideoOutputBase2::videoDecodePipelinePlay ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelinePlay\n");
	);
#ifdef ENABLE_VIDEO_OUTPUT
	
	GstStateChangeReturn StateChangeReturn;

	
	//StateChangeReturn = gst_element_get_state (m_pGstElementH264DecodePipeline, &State, &PendingState, 0);
	
	//if (State != GST_STATE_PLAYING)
	//{
		StateChangeReturn = m_videoDecodePipeline.stateSet(GST_STATE_PLAYING);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_videoDecodePipeline, GST_STATE_PLAYING)\n");
		}
	//}
	
	// Don't wait
	//ChangeStateWait (m_pGstElementH264DecodePipeline);

	
STI_BAIL:

#endif

	return (hResult);
}

stiHResult CstiVideoOutputBase2::videoDecodePipelineReady ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineReady\n");
	);
#ifdef ENABLE_VIDEO_OUTPUT
	
	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = m_videoDecodePipeline.stateSet(GST_STATE_READY);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_videoDecodePipeline, GST_STATE_READY)\n");
	}

	ChangeStateWait (m_videoDecodePipeline, 1);

	
STI_BAIL:

#endif

	return (hResult);
}

stiHResult CstiVideoOutputBase2::videoDecodePipelinePause ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelinePause\n");
	);
	
#ifdef ENABLE_VIDEO_OUTPUT
	
	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = m_videoDecodePipeline.stateSet(GST_STATE_PAUSED);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_videoDecodePipeline, GST_STATE_PAUSED)\n");
	}
	
	ChangeStateWait (m_videoDecodePipeline, 1);

STI_BAIL:

#endif

	return (hResult);
}

stiHResult CstiVideoOutputBase2::videoDecodePipelineNull ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::videoDecodePipelineNull\n");
	);
	
#ifdef ENABLE_VIDEO_OUTPUT 
	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = m_videoDecodePipeline.stateSet(GST_STATE_NULL);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_videoDecodePipeline, GST_STATE_NULL)\n");
	}
	
	ChangeStateWait (m_videoDecodePipeline, 1);

	
STI_BAIL:

#endif

	return (hResult);
}

#ifdef ENABLE_FRAME_CAPTURE
stiHResult CstiVideoOutputBase2::FrameCapturePipelineGet (
	GstElement **ppGstElementPipeline) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (ppGstElementPipeline)
	{
		*ppGstElementPipeline = m_pGstElementFrameCapturePipeline;
	}
	
	return (hResult);
}

GstFlowReturn CstiVideoOutputBase2::FrameCaptureAppSinkGstBufferPut (
	GstElement* pGstElement,
	gpointer pUser)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::FrameCaptureAppSinkGstBufferPut\n");
	);

	auto gstAppSink = GStreamerAppSink(pGstElement);

	if (gstAppSink.get != nullptr)
	{
		auto gstSample = gstAppSink.pullSample ();

		if (gstSample.get () != nullptr)
		{
			auto gstBuffer = sample.bufferGet ();

			if (gstBuffer.get () != nullptr)
			{
				CstiVideoOutput *pCstiVideoOutput = (CstiVideoOutput*)pUser;

				if (pCstiVideoOutput)
				{
					pCstiVideoOutput->FrameCaptureBufferReceived (gstBuffer);
				}
				else
				{
					stiASSERTMSG (estiFALSE, "WARNING no pCstiVideoOutput\n");
				}
			}
			else
			{
				stiASSERTMSG (estiFALSE, "WARNING can't get buffer from appsink\n");
			}
		}
	}

	return GST_FLOW_OK;
}


stiHResult CstiVideoOutputBase2::FrameCapturePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bLinked;
	GstBus *pGstBus;

	stiASSERT (m_pGstElementFrameCapturePipeline == NULL);

	m_pGstElementFrameCapturePipeline = gst_pipeline_new ("decode_frame_capture_pipeline_element");
	stiTESTCOND (m_pGstElementFrameCapturePipeline, stiRESULT_ERROR);

	m_gstElementFrameCaptureAppSrc = GStreamerAppSource ("decode_frame_capture_app_src_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_pGstElementFrameCaptureConvert = gst_element_factory_make("nvvideoconvert", "decode_frame_capture_video_convert");
	stiTESTCOND(m_pGstElementFrameCaptureConvert, stiRESULT_ERROR);
	//g_object_set (G_OBJECT (m_pGstElementFrameCaptureConvert), "output-buffers", 2, NULL);

	char croprect[64];
	sprintf (croprect, "%dx%d-%dx%d",
				0, 0, stiMAX_CAPTURE_WIDTH, stiMAX_CAPTURE_HEIGHT);

	g_object_set (G_OBJECT (m_pGstElementFrameCaptureConvert), "croprect", croprect, NULL);

	/*
	char output_size[64];
	sprintf (output_size, "%dx%d", stiMAX_CAPTURE_WIDTH, stiMAX_CAPTURE_HEIGHT);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureConvert), "output-size", output_size, NULL);
	*/

	m_pGstElementFrameCaptureJpegEnc = gst_element_factory_make("nv_omx_jpegenc", "decode_jpeg_encoder_element");
	stiTESTCOND (m_pGstElementFrameCaptureJpegEnc, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureJpegEnc), "use-timestamps", FALSE, NULL);
	//g_object_set (G_OBJECT (m_pGstElementFrameCaptureJpegEnc), "input-buffers", 1, NULL);
	//g_object_set (G_OBJECT (m_pGstElementFrameCaptureJpegEnc), "output-buffers", 1, NULL);

	// Create the appsink element
	m_pGstElementFrameCaptureAppSink = gst_element_factory_make ("appsink", "decode_frame_capture_appsink_element");
	stiTESTCOND (m_pGstElementFrameCaptureAppSink, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "sync", FALSE, NULL);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "async", FALSE, NULL);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "qos", FALSE, NULL);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "enable-last-sample", FALSE, NULL);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "emit-signals", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureAppSink), "max-buffers", 1, NULL);

	gst_bin_add_many (GST_BIN(m_pGstElementFrameCapturePipeline),
		m_gstElementFrameCaptureAppSrc.elementGet (),
		m_pGstElementFrameCaptureConvert,
		m_pGstElementFrameCaptureJpegEnc,
		m_pGstElementFrameCaptureAppSink,
		NULL);

	bLinked = gst_element_link_many (
		m_gstElementFrameCaptureAppSrc.elementGet (),
		m_pGstElementFrameCaptureConvert,
		m_pGstElementFrameCaptureJpegEnc,
		m_pGstElementFrameCaptureAppSink,
		NULL);

	if (!bLinked)
	{
		stiTHROWMSG (stiRESULT_ERROR, "failed linking m_gstElementFrameCaptureAppSrc, m_pGstElementFrameCaptureJpegEnc, m_pGstElementFrameCaptureAppSink\n");
	}

	pGstBus = gst_pipeline_get_bus (GST_PIPELINE (m_pGstElementFrameCapturePipeline));
	stiTESTCOND (pGstBus, stiRESULT_ERROR);

	m_unFrameCapturePipelineWatch = gst_bus_add_watch (pGstBus, FrameCaptureBusCallback, this);

	g_object_unref (pGstBus);

	m_unFrameCaptureNewBufferSignalHandler = g_signal_connect (m_pGstElementFrameCaptureAppSink, "new-sample",
															   G_CALLBACK (FrameCaptureAppSinkGstBufferPut), this);

STI_BAIL:

	return hResult;
}

stiHResult CstiVideoOutputBase2::FrameCapturePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstStateChangeReturn StateChangeReturn;

	if (m_pGstElementFrameCapturePipeline)
	{
		// Set to NULL state before removing
		StateChangeReturn = gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutputBase2::FrameCapturePipelineDestroy: Could not gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_NULL)\n");
		}

		if (m_unFrameCapturePipelineWatch)
		{
			g_source_remove (m_unFrameCapturePipelineWatch);
			m_unFrameCapturePipelineWatch = 0;
		}

		if (m_unFrameCaptureNewBufferSignalHandler)
		{
			g_signal_handler_disconnect (m_pGstElementFrameCaptureAppSink, m_unFrameCaptureNewBufferSignalHandler);
			m_unFrameCaptureNewBufferSignalHandler = 0;
		}

		gst_object_unref (m_pGstElementFrameCapturePipeline);
		m_pGstElementFrameCapturePipeline = NULL;
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoOutputBase2::FrameCapturePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::FrameCapturePipelineStart\n");
	);

	hResult = FrameCapturePipelineDestroy ();
	stiTESTRESULT ();

	hResult = FrameCapturePipelineCreate ();
	stiTESTRESULT ();

	GstStateChangeReturn StateChangeReturn;

	stiTESTCOND (m_pGstElementFrameCapturePipeline, stiRESULT_ERROR);

	StateChangeReturn = gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutputBase2::FrameCapturePipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_PLAYING)\n");
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoOutputBase2::FrameCaptureBufferReceived (
	GStreamerBuffer &gstBuffer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (gstBuffer.get () != nullptr)
	{
		const char *pszShellScript = nullptr;
		struct stat FileStat;

		std::string staticFolder;
		stiOSStaticDataFolderGet (&StaticFolder);
		std::string screenshotScriptOverride = staticFolder + SCREENSHOT_SCRIPT_OVERRIDE;
		
		if (stat (screenshotScriptOverride.c_str(), &FileStat) == 0)
		{
			vpe::trace ("CstiVideoOutputBase2::FrameCaptureBufferReceived: Found Independant script.\n");
			pszShellScript = screenshotScriptOverride.c_str();
		}
		else if (stat (SCREENSHOT_SHELL_SCRIPT, &FileStat) == 0)
		{
			vpe::trace ("CstiVideoOutputBase2::FrameCaptureBufferReceived: Found screenshot script.\n");
			pszShellScript = SCREENSHOT_SHELL_SCRIPT;
		}

		if (pszShellScript)
		{
			//save the buffer and let the upper layer know it's available.
			FILE *pFile = fopen (SCREENSHOT_FILE, "wb");

			if (pFile)
			{
				fwrite (gstBuffer.dataGet (), gstBuffer.sizeGet (), sizeof(char), pFile);

				fclose (pFile);

				//
				// Call the shell script to process the file.
				//
				char szCommand[256];
				sprintf (szCommand, "%s &", pszShellScript);

				if (system (szCommand))
				{
					stiASSERTMSG (false, "error: %s", szCommand);
				}
			}
		}
		else
		{
			vpe::trace ("Did not find screenshot script.\n");
		}
	}

	// Notify app if needed
	//CstiVideoInput *pGStreamerVideoInput = (CstiVideoInput*)IstiVideoInput::InstanceGet ();
	//CstiEvent oEvent (CstiVideoInput::estiVIDEO_INPUT_FRAME_CAPTURE_RECEIVED, (stiEVENTPARAM)pGstBuffer);
	//pGStreamerVideoInput->MsgSend (&oEvent, sizeof (oEvent));

	return hResult;
}

stiHResult CstiVideoOutputBase2::FrameCaptureGet (
	bool * pbFrameCapture)
{
	std::lock_guard<std::mutex> lock (m_frameCaptureMutex);
	*pbFrameCapture = m_bFrameCapture;

	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutputBase2::FrameCaptureSet (
	bool bFrameCapture)
{
	std::lock_guard<std::mutex> lock (m_frameCaptureMutex);
	m_bFrameCapture = bFrameCapture;

	if (m_bFrameCapture)
	{
		g_object_set (G_OBJECT(m_pGstElementFrameCaptureJpegEnc), "drop", FALSE, NULL);
	}

	return stiRESULT_SUCCESS;
}
#endif // #ifdef ENABLE_FRAME_CAPTURE


#endif



/*!
 * \brief the event handler that actually stops audio playback
 *
 * \return stiHResult
 */
void CstiVideoOutputBase2::eventVideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::EventVideoPlaybackStop\n");
	);

	m_bPlaying = false;
	m_bSPSAndPPSSentToDecoder = false;

	stiOSSignalSet2 (m_Signal, 0);

#ifdef ENABLE_VIDEO_OUTPUT
	hResult = videoDecodePipelineReady ();
	stiTESTRESULT ();

	hResult = videoDecodePipelinePlay ();
	stiTESTRESULT ();
	ChangeStateWait (m_videoDecodePipeline, 1);
	
#endif

	m_un32DecodeWidth = DEFAULT_DECODE_WIDTH;
	m_un32DecodeHeight = DEFAULT_DECODE_HEIGHT;

	decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);
	videoOutputPlaybackStoppedSignal.Emit();

STI_BAIL:;
}


/*!\brief Enables or disables the display sink's video based on privacy, hold and remote view visiblity.
 *
 */
stiHResult CstiVideoOutputBase2::VideoSinkVisibilityUpdate (
	bool bClearPreviousFrame)
{
	return stiRESULT_SUCCESS;
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiVideoOutputBase2::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
#ifndef stiINTEROP_EVENT
	pCodecs->push_back (estiVIDEO_H265);
	pCodecs->push_back (estiVIDEO_H264);
#else
	if (g_stiEnableVideoCodecs & 4)
	{
		pCodecs->push_back (estiVIDEO_H265);
	}

	if (g_stiEnableVideoCodecs & 2)
	{
		pCodecs->push_back (estiVIDEO_H264);
	}
#endif

	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiVideoOutputBase2::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)	// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutputBase2::CodecCapabilitiesGet: m_eDisplayHeight = ", m_eDisplayHeight, " m_eDisplayWidth = ", m_eDisplayWidth, "\n");
	);

	switch (eCodec)
	{
		case estiVIDEO_H265:
		{
			auto pstH265Caps = (SstiH265Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			pstCaps->Profiles.push_back (stProfileAndConstraint);

			// This is so the remote encoder does not send us 
			// a bigger size than we are currently sending 
			// to the display
			if (m_eDisplayHeight < 1080)
			{
				stiDEBUG_TOOL (g_stiDisplayDebug,
					vpe::trace ("CstiVideoOutputBase2::CodecCapabilitiesGet: Current display height is 1080 or less. Restrict H.265 receive level to 3.1\n");
				);
				
				pstH265Caps->eLevel = estiH265_LEVEL_3_1;
			}
			else
			{
				pstH265Caps->eLevel = estiH265_LEVEL_4;
			}

			break;
		}
		case estiVIDEO_H264:
		{
			auto pstH264Caps = (SstiH264Capabilities*)pstCaps;

#ifndef stiINTEROP_EVENT
			stProfileAndConstraint.eProfile = estiH264_HIGH;
			stProfileAndConstraint.un8Constraints = 0x0c;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			// This is so the remote encoder does not send us 
			// a bigger size than we are currently sending 
			// to the display
			if (m_eDisplayHeight < 1080)
			{
				stiDEBUG_TOOL (g_stiDisplayDebug,
					vpe::trace ("CstiVideoOutputBase2::CodecCapabilitiesGet: Current display height is 1080 or less. Restrict H.264 receive level to 3.1\n");
				);
				
				pstH264Caps->eLevel = estiH264_LEVEL_3_1;
			}
			else
			{
				pstH264Caps->eLevel = estiH264_LEVEL_4_1;
			}

#else
			if (g_stiEnableVideoProfiles & 4)
			{
				stProfileAndConstraint.eProfile = estiH264_HIGH;
				stProfileAndConstraint.un8Constraints = 0x0c;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			}

			if (g_stiEnableVideoProfiles & 2)
			{
				stProfileAndConstraint.eProfile = estiH264_MAIN;
				stProfileAndConstraint.un8Constraints = 0x00;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			}

			if (g_stiEnableVideoProfiles & 1)
			{
				stProfileAndConstraint.eProfile = estiH264_BASELINE;
				stProfileAndConstraint.un8Constraints = 0xe0;
				pstH264Caps->Profiles.push_back (stProfileAndConstraint);
			}

			// This is so the remote encoder does not send us 
			// a bigger size than we are currently sending 
			// to the display
			if (m_eDisplayHeight < 1080)
			{
				stiDEBUG_TOOL (g_stiDisplayDebug,
					vpe::trace ("CstiVideoOutputBase2::CodecCapabilitiesGet: Current display height is 1080 or less. Restrict H.264 receive level to 3.1\n");
				);
				
				pstH264Caps->eLevel = estiH264_LEVEL_3_1;
			}
			else
			{
				pstH264Caps->eLevel = estiH264_LEVEL_4_1;
			}
#endif

			break;
		}

		default:
			stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiVideoOutputBase2::DecodeDisplaySinkMove ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	return (hResult);
}


void CstiVideoOutputBase2::eventDisplayResolutionSet (
	uint32_t width,
	uint32_t height)
{
	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutputBase2::EventDisplayResolutionSet: width = ", width, " height = ", height, "\n");
	);

	std::string mode {"Unknown"};
	uint32_t displaySize = width * height;

		switch (displaySize)
		{
		case eDisplaySizeUnknown:
			mode = "Unknown";
			break;
		case eDisplaySize640x360:
			mode = "640x360";
			break;
		case eDisplaySize640x480:
			mode = "640x480";
			break;
		case eDisplaySize720x480:
			mode = "720x480";
			break;
		case eDisplaySize800x600:
			mode = "800x600";
			break;
		case eDisplaySize1024x768:
			mode = "1024x768";
			break;
		case eDisplaySize1280x720:
			mode = "1280x720";
			break;
		case eDisplaySize1280x800:
			mode = "1280x800";
			break;
		case eDisplaySize1280x1024:
			mode = "1280x1024";
			break;
		case eDisplaySize1360x768:
			mode = "1360x768";
			break;
		case eDisplaySize1366x768:
			mode = "1366x768";
			break;
		case eDisplaySize1440x900:
			mode = "1440x900";
			break;
		case eDisplaySize1536x864:
			mode = "1536x864";
			break;
		case eDisplaySize1600x900:
			mode = "1600x900";
			break;
		case eDisplaySize1680x1050:
			mode = "1680x1050";
			break;
		case eDisplaySize1920x1080:
			mode = "1920x1080";
			break;
		case eDisplaySize1920x1200:
			mode = "1920x1200";
			break;
		case eDisplaySize2048x1152:
			mode = "2048x1152";
			break;
		case eDisplaySize2560x1080:
			mode = "2560x1080";
			break;
		case eDisplaySize2560x1440:
			mode = "2560x1440";
			break;
		case eDisplaySize3440x1440:
			mode = "3440x1440";
			break;
		case eDisplaySize3840x2160:
			mode = "3840x2160";
			break;
		case eDisplaySize7680x4320:
			mode = "7680x4320";
			break;
		default:
			mode = "Unknown";
			break;
		}

		IstiLogging *pLogging = IstiLogging::InstanceGet();
		pLogging->onBootUpLog("EventType=HDMIDisplayMode DisplayMode= " + mode + "\n");

	if (m_eDisplayHeight != height
		|| m_eDisplayWidth != width)
	{
		m_eDisplayHeight = height;
		m_eDisplayWidth = width;
		
		keyframeNeededSignal.Emit ();
	}
	
}

stiHResult CstiVideoOutputBase2::DisplayPowerSet (
	bool bOn)
{
	stiHResult hResult = stiRESULT_SUCCESS;

#ifdef CEC_ENABLED
	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutputBase2::DisplayPowerSet: current = ", torf(m_cecControl->tvPowered()), " setting to ", torf(bOn), "\n");
	);
	
	if (m_cecControl->cecSupported())
	{
		stiDEBUG_TOOL(g_stiDisplayDebug,
			vpe::trace ("CstiVideoOutputBase2::DisplayPowerSet: CEC -is- supported\n");
		);

		if (bOn && !m_cecControl->tvPowered())
		{
			stiDEBUG_TOOL(g_stiDisplayDebug,
				vpe::trace ("CstiVideoOutputBase2::DisplayPowerSet: Powering on television\n");
			);

			m_cecControl->displayPowerSet(bOn);		// turn on the tv
			m_cecControl->activeSet();				// set us to the active source
		}
		else if (!bOn && m_cecControl->tvPowered())
		{
			stiDEBUG_TOOL(g_stiDisplayDebug,
				vpe::trace ("CstiVideoOutputBase2::DisplayPowerSet: Powering off television\n");
			);
			// this assumes that turning it off just works......
			m_cecControl->displayPowerSet(bOn);
		}
	}
#endif
//STI_BAIL:
	return (hResult);
}

bool CstiVideoOutputBase2::DisplayPowerGet ()
{
#ifdef CEC_ENABLED
	bool bResult = false;

	bResult = m_cecControl->tvPowered();

	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutputBase2::DisplayPowerGet returns ", torf(bResult), "\n");
	);

	return bResult;
#else
	return true;
#endif
}

bool CstiVideoOutputBase2::CECSupportedGet ()
{
#ifdef CEC_ENABLED
	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CECSupportedGet: m_bCECSupported = ", torf(m_cecControl->cecSupported()), "\n");
	);

	return (m_cecControl->cecSupported());
#else
	vpe::trace ("CstiVideoOutputBase2::CECSupportedGet(): WARNING - CEC Disabled\n");

	return false;
#endif
}

std::string CstiVideoOutputBase2::tvVendorGet()
{
#ifdef CEC_ENABLED
	return(m_cecControl->tvVendorGet());
#else
	return std::string("Unknown");
#endif
}


/*!
 * \brief EventBufferReturnErrorTimerTimeout: Fires when a buffer has been
 * handed downstream and not returned after BUFFER_RETURN_TIMEOUT
 */
void CstiVideoOutputBase2::EventBufferReturnErrorTimerTimeout ()
{
	stiLOG_ENTRY_NAME (CstiVideoOutputBase2::EventBufferReturnErrorTimerTimeout);
	
	stiASSERTMSG (estiFALSE, "All buffers stuck in gstreamer for %d ms. m_FramesInGstreamer = %d, m_FramesInApplication = %d\n",
				  BUFFER_RETURN_ERROR_TIMEOUT, m_FramesInGstreamer.size (), m_FramesInApplication.size ());

	stiHResult hResult = stiRESULT_SUCCESS;
	hResult = VideoPlaybackReset ();
	
	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "VideoPlaybackReset failed\n");
	}
}


/*!
 * \brief Sets the preferred display size of this device.
 *  Set 0 to not signal a preferred display size.
 */
void CstiVideoOutputBase2::PreferredDisplaySizeGet (
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	*displayWidth = 0;
	*displayHeight = 0;
}


void CstiVideoOutputBase2::platformVideoDecodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiLOG_ENTRY_NAME (CstiVideoOutputBase2::platformVideoDecodePipelineCreate);

	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase2::platformVideoDecodePipelineCreate\n");
	);

	hResult = videoDecodePipelineCreate ();
	stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutputBase2::platformVideoDecodePipelineCreate: can't create output pipeline\n");

	hResult = videoDecodePipelineReady ();
	stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutputBase2::videoDecodePipelineReady: can't ready output pipeline\n");
		
	hResult = videoDecodePipelinePlay ();
	stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutputBase2::videoDecodePipelineReady: can't play output pipeline\n");
	ChangeStateWait (m_videoDecodePipeline, 1);

	if (m_videoPlaybackStarting)
	{
		m_videoPlaybackStarting = false;

		eventVideoPlaybackStart ();
	}
}

