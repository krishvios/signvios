// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2016 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoOutput.h"
#include "GStreamerAppSource.h"
#include "GStreamerAppSink.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "VideoControl.h"
#include "CaptureBin.h"
#include "stiRemoteLogEvent.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <boost/filesystem.hpp>


#define ENABLE_VIDEO_OUTPUT
#define ENABLE_REMOTE_VIEW_SETTINGS_SET
#define ENABLE_SELF_VIEW_SETTINGS_SET
#define ENABLE_HARDWARE_263_DECODER
//#define ENABLE_263_DECODE_PARSER
//#define ENABLE_264_DECODE_PARSER
//#define ENABLE_NEW_DISPLAY_SINK

#define MAX_LATENESS 50 * GST_MSECOND
#define TIME_TO_DELAY_QOS 1500 * GST_MSECOND
#define DEFAULT_LATENCY 100 * GST_MSECOND
#define MAX_LATENCY 250 * GST_MSECOND

static const IstiVideoOutput::SstiDisplaySettings g_DefaultDisplaySettings =
{
	IstiVideoOutput::eDS_SELFVIEW_VISIBILITY |
	IstiVideoOutput::eDS_SELFVIEW_SIZE |
	IstiVideoOutput::eDS_SELFVIEW_POSITION |
	IstiVideoOutput::eDS_REMOTEVIEW_VISIBILITY |
	IstiVideoOutput::eDS_REMOTEVIEW_SIZE |
	IstiVideoOutput::eDS_REMOTEVIEW_POSITION |
	IstiVideoOutput::eDS_TOPVIEW,
	{
		0,  // self view visibility
		1920,  // self view width
		1072,  // self view height
		0,  // self view position X
		0  // self view position Y
	},
	{
		0,   // remote view visibility
		CstiVideoOutputBase::DEFAULT_DECODE_WIDTH, // remote view width
		CstiVideoOutputBase::DEFAULT_DECODE_HEIGHT, // remote view height
		0,   // remote view position X
		0    // remote view position Y
	},
	IstiVideoOutput::eSelfView
};

/*!
 * \brief default constructor
 */
CstiVideoOutput::CstiVideoOutput ()
:
	CstiVideoOutputBase (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT, NUM_DEC_BITSTREAM_BUFS)
{
	m_DisplaySettings = g_DefaultDisplaySettings;
	m_un64Latency = DEFAULT_LATENCY;
	
	m_signalConnections.push_back(m_bufferReturnErrorTimer.timeoutEvent.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoOutput::EventBufferReturnErrorTimerTimeout, this));
		}));
}

/*!
 * \brief Initialize
 * Only called once for object
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutput::Initialize (
	CstiMonitorTask *pMonitorTask,
	CstiVideoInput *pVideoInput,
	std::shared_ptr<CstiCECControl> cecControl)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pMonitorTask, stiRESULT_ERROR);
	stiTESTCOND (pVideoInput, stiRESULT_ERROR);
	stiTESTCOND (cecControl != nullptr, stiRESULT_ERROR);

	m_pMonitorTask = pMonitorTask;

	m_pVideoInput = pVideoInput;

	m_cecControl = cecControl;

	m_signalConnections.push_back (m_pMonitorTask->hdmiOutStatusChangedSignal.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoOutput::EventHdmiOutStatusChanged, this));
		}));

	// Setup HDMI out
	hResult = BestDisplayModeSet ();
	stiTESTRESULT ();

	if (nullptr == m_Signal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_Signal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);
		stiOSSignalSet2 (m_Signal, 0);
	}

	hResult = DecodePipelinesStart ();
	stiTESTRESULT ();

	hResult = FrameCapturePipelineStart ();
	stiTESTRESULT ();

	//g_timeout_add (500, (GSourceFunc) do_switch, this);

STI_BAIL:
	return (hResult);
}


CstiVideoOutput::~CstiVideoOutput ()
{
	// Will wait until thread is joined
	CstiEventQueue::StopEventLoop();

	DisplaySinkStop ();

	H264DecodePipelineDestroy ();

	H263DecodePipelineDestroy ();

	FrameCapturePipelineDestroy ();

	for (;;)
	{
		CstiVideoPlaybackFrame *pVideoPlaybackFrame = m_oBitstreamFifo.Get ();

		if (pVideoPlaybackFrame == nullptr)
		{
			break;
		}

		for (std::list<IstiVideoPlaybackFrame *>::iterator i = m_FramesInApplication.begin (); i != m_FramesInApplication.end (); ++i)
		{
			if ((*i) == pVideoPlaybackFrame)
			{
				vpe::trace ("CstiVideoOutput::~CstiVideoOutput: Frame is also in application.\n");
				break;
			}
		}

		for (std::list<IstiVideoPlaybackFrame *>::iterator i = m_FramesInGstreamer.begin (); i != m_FramesInGstreamer.end (); ++i)
		{
			if ((*i) == pVideoPlaybackFrame)
			{
				vpe::trace ("CstiVideoOutput::~CstiVideoOutput: Frame is also in GStreamer.\n");
				break;
			}
		}

		delete pVideoPlaybackFrame;
	}

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::~CstiVideoOutput: Frames in Application: ", m_FramesInApplication.size (), "\n");
		vpe::trace ("CstiVideoOutput::~CstiVideoOutput: Frames in GStreamer: ", m_FramesInGstreamer.size (), "\n");
	);

	if (m_Signal)
	{
		stiOSSignalClose (&m_Signal);
		m_Signal = nullptr;
	}
}

void CstiVideoOutput::EventHdmiOutStatusChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::EventHdmiOutStatusChanged\n");
	);

	hResult = BestDisplayModeSet ();
	stiTESTRESULT ();

	hdmiOutStatusChangedSignal.Emit ();

STI_BAIL:

	return;
}

void CstiVideoOutput::H264NeedData (
	GStreamerAppSource &appSrc)
{
	std::lock_guard<std::mutex> lock (m_mutexH264PipelineReady);

	if (!m_bH264DecodePipelineNeedsData)
	{
		vpe::trace ("CstiVideoOutput::H264NeedData\n");

		m_bH264DecodePipelineNeedsData = true;
		m_conditionH264PipelineReady.notify_all ();

		PostEvent (
			std::bind (&CstiVideoOutput::EventH264NeedsData, this));
	}
}


void CstiVideoOutput::EventH264NeedsData ()
{
	//
	// If there are buffers in the fifo then signal to the engine that we
	// are ready.
	//
	if (m_oBitstreamFifo.CountGet () > 0 && m_bPlaying)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}
}


void CstiVideoOutput::H264EnoughData (
	GStreamerAppSource &appSrc)
{
	std::lock_guard<std::mutex> lock (m_mutexH264PipelineReady);

	if (m_bH264DecodePipelineNeedsData)
	{
		//vpe::trace ("CstiVideoOutput::enough_data\n");

		m_bH264DecodePipelineNeedsData = false;

		PostEvent (
			std::bind (&CstiVideoOutput::EventH264EnoughData, this));
	}
}


void CstiVideoOutput::EventH264EnoughData ()
{
	//
	// Signal to the engine that we are not ready
	// to receive frames.
	//
	stiOSSignalSet2 (m_Signal, 0);
}

void CstiVideoOutput::H263NeedData (
	GStreamerAppSource &appSrc)
{
	std::lock_guard<std::mutex> lock (m_mutexH263PipelineReady);

	if (!m_bH263DecodePipelineNeedsData)
	{
		vpe::trace ("CstiVideoOutput::H263NeedData\n");

		m_bH263DecodePipelineNeedsData = true;
		m_conditionH263PipelineReady.notify_all ();

		PostEvent (
			std::bind (&CstiVideoOutput::EventH263NeedsData, this));
	}
}


void CstiVideoOutput::EventH263NeedsData ()
{
	//
	// If there are buffers in the fifo then signal to the engine that we
	// are ready.
	//
	if (m_oBitstreamFifo.CountGet () > 0 && m_bPlaying)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}
}


void CstiVideoOutput::H263EnoughData (
	GStreamerAppSource &appSrc)
{
	std::lock_guard<std::mutex> lock (m_mutexH263PipelineReady);

	if (m_bH263DecodePipelineNeedsData)
	{
		//vpe::trace ("CstiVideoOutput::enough_data\n");

		m_bH263DecodePipelineNeedsData = false;

		PostEvent (
			std::bind (&CstiVideoOutput::EventH263EnoughData, this));
	}
}


void CstiVideoOutput::EventH263EnoughData ()
{
	//
	// Signal to the engine that we are not ready
	// to receive frames.
	//
	stiOSSignalSet2 (m_Signal, 0);
}

stiHResult CstiVideoOutput::FrameCaptureBufferReceived (
	GStreamerBuffer &gstBuffer)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (gstBuffer.get () != nullptr)
	{
		const char *pszShellScript = nullptr;
		struct stat FileStat;

		std::string staticFolder;
		stiOSStaticDataFolderGet (&staticFolder);
		std::string screenshotScriptOverride = staticFolder + SCREENSHOT_SCRIPT_OVERRIDE;
		
		if (stat (screenshotScriptOverride.c_str(), &FileStat) == 0)
		{
			vpe::trace ("CstiVideoOutput::FrameCaptureBufferReceived: Found Independant script.\n");
			pszShellScript = screenshotScriptOverride.c_str();
		}
		else if (stat (SCREENSHOT_SHELL_SCRIPT, &FileStat) == 0)
		{
			vpe::trace ("CstiVideoOutput::FrameCaptureBufferReceived: Found screenshot script.\n");
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

	return hResult;
}


stiHResult CstiVideoOutput::FrameCaptureGet (
	bool * pbFrameCapture)
{
	std::lock_guard<std::mutex> lock (m_frameCaptureMutex);
	*pbFrameCapture = m_bFrameCapture;

	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoOutput::FrameCaptureSet (
	bool bFrameCapture)
{
	std::lock_guard<std::mutex> lock (m_frameCaptureMutex);
	m_bFrameCapture = bFrameCapture;

	if (m_bFrameCapture)
	{
		g_object_set(G_OBJECT(m_pGstElementFrameCaptureJpegEnc), "drop", FALSE, NULL);
	}

	return stiRESULT_SUCCESS;
}


void CstiVideoOutput::ScreenshotTake (
	EstiScreenshotType eType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = FrameCaptureSet (true);
	stiTESTRESULT ();

STI_BAIL:

	return;
}

#if defined(ENABLE_H264_DEBUG_PROBES) || defined(ENABLE_H263_DEBUG_PROBES)
gboolean CstiVideoOutput::DebugProbeFunction (
	GstPad * pGstPad,
	GstMiniObject * pGstMiniObject,
	gpointer pUserData)
{
	GStreamerSample gstSample(pGstMiniObject);
	auto gstBuffer = gstSample.bufferGet ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug > 2,
		stiTrace ("DebugProbeFunction: buffer %p, pushing, timestamp %"
				GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT
				", offset %" G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT
				", size %d, flags 0x%x\n",
				gstBuffer.get (),
				GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (gstBuffer.timestampGet ())),
				GST_TIME_ARGS (GST_BUFFER_DURATION (gstBuffer.durationGet ())),
				GST_BUFFER_OFFSET (gstBuffer.offsetGet ()), GST_BUFFER_OFFSET_END (gstBuffer.offsetEndGet ()),
				GST_BUFFER_SIZE (gstBuffer.sizeGet ()), GST_BUFFER_FLAGS (gstBuffer.flagsGet ()));
	);

	return TRUE;
}
#endif

gboolean CstiVideoOutput::EventDropperFunction (
	GstPad * pGstPad,
	GstMiniObject * pGstMiniObject,
	gpointer pUserData)
{
	GstEvent * pGstEvent = GST_EVENT (pGstMiniObject);

	int nEventType = GST_EVENT_TYPE (pGstEvent);

	switch (nEventType)
	{
		case GST_EVENT_EOS:
		case GST_EVENT_NEWSEGMENT:
		// drop
		return FALSE;
	}

	return TRUE;
}

#ifdef ENABLE_VIDEO_OUTPUT
static gboolean do_latency_cb (
	GstElement *pPipeline,
	gpointer user_data)
{
	GstQuery *q;
	gboolean live = FALSE;
	GstClockTime min = 0, max = G_MAXUINT64;

	q = gst_query_new_latency();

	if (gst_element_query(pPipeline, q))
	{
		gst_query_parse_latency(q, &live, &min, &max);
	}

	gst_query_unref(q);

	if (DEFAULT_LATENCY + min > max)
	{
		g_critical("The maximum latency of the pipeline %"
			   G_GUINT64_FORMAT " is less than %u ms away from"
			   " the minimum %" G_GUINT64_FORMAT ", add queues\n",
			   max, DEFAULT_LATENCY, min);
	}

	gst_element_send_event (pPipeline,
				gst_event_new_latency(DEFAULT_LATENCY +
						      min));

	return TRUE;
}
#endif


stiHResult CstiVideoOutput::DecodePipelinesStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DecodePipelinesStart\n");
	);

	hResult = H264DecodePipelineStart ();
	stiTESTRESULT ();

	hResult = H263DecodePipelineStart ();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

}

stiHResult CstiVideoOutput::H264DecodePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::unique_lock<std::mutex> lock (m_mutexH264PipelineReady);
	lock.unlock ();

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H264DecodePipelineStart\n");
	);
#ifdef ENABLE_VIDEO_OUTPUT
	bool bLocked = false;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		if (m_FramesInGstreamer.size () > 0)
		{
			vpe::trace ("CstiVideoOutput::H264DecodePipelineStart: Before starting, frames in GStreamer: ", m_FramesInGstreamer.size (), "\n");
		}
		
		if (m_FramesInApplication.size () > 0)
		{
			vpe::trace ("CstiVideoOutput::H264DecodePipelineStart: Before starting, frames in Application: ", m_FramesInApplication.size (), "\n");
		}
	);

	hResult = H264DecodePipelineCreate ();
	stiTESTRESULT ();

	lock.lock ();
	m_bH264DecodePipelineNeedsData = false;
	lock.unlock ();

	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = gst_element_set_state (m_pGstElementH264DecodePipeline, GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_pGstElementH264DecodePipeline, GST_STATE_PLAYING)\n");
	}

	lock.lock ();
	bLocked = true;

	if (m_bH264DecodePipelineNeedsData)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::H264DecodePipelineStart: Pipeline ready\n");
		);
	}
	else
	{
		for (;;)
		{
			auto status = m_conditionH264PipelineReady.wait_for (lock, std::chrono::seconds(20));

			if (status == std::cv_status::timeout)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::H264DecodePipelineStart: waiting for pipeline to be ready\n");
			}

			if (m_bH264DecodePipelineNeedsData)
			{
				break;
			}
		}
	}

	lock.unlock ();
	bLocked = false;

	//
	// Prime the decoder to get passed
	// the large delay that the decoder causes every time
	// it is instantiated and first used
	//
	hResult = H264DecodePipelinePrime ();
	stiTESTRESULT ();

STI_BAIL:

	if (bLocked)
	{
		bLocked = false;
	}
#endif

	return (hResult);
}


stiHResult CstiVideoOutput::H264DecodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPadLinkReturn eGstPadLinkReturn;
	bool bResult = true;
	GstPad *pGstPad = nullptr;
	gboolean bLinked;

	//GstElement * pGstElementPTZVideoConvert;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H264DecodePipelineCreate\n");
	);

	hResult = H264DecodePipelineDestroy ();
	stiTESTRESULT ();

	stiASSERT (m_pGstElementH264DecodePipeline == nullptr);
	
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		if (m_FramesInGstreamer.size () > 0)
		{
			vpe::trace ("CstiVideoOutput::H264DecodePipelineCreate: After destruction, frames in GStreamer: ", m_FramesInGstreamer.size (), "\n");
		}
		
		if (m_FramesInApplication.size () > 0)
		{
			vpe::trace ("CstiVideoOutput::H264DecodePipelineCreate: After destruction, frames in Application: ", m_FramesInApplication.size (), "\n");
		}
	);
	
	stiASSERTMSG (m_FramesInGstreamer.size () == 0, "%d frames stuck in gstreamer. They will be leaked\n", m_FramesInGstreamer.size ());
	
	// Reset the number of frames in gstreamer
	m_FramesInGstreamer.clear ();

	m_pGstElementH264DecodePipeline = gst_pipeline_new ("h264_decode_pipeline_element");

	m_unH264DoLatencySignalHandler = g_signal_connect(m_pGstElementH264DecodePipeline, "do-latency",
			 G_CALLBACK (do_latency_cb), NULL);

	stiTESTCOND (m_pGstElementH264DecodePipeline, stiRESULT_ERROR);

	m_gstElementH264DecodeAppSrc = GStreamerAppSource ("h264_decode_app_src_element");
	stiTESTCOND (m_gstElementH264DecodeAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_gstElementH264DecodeAppSrc.needDataCallbackSet([this](GStreamerAppSource &appSrc) { H264NeedData (appSrc); });
	m_gstElementH264DecodeAppSrc.enoughDataCallbackSet([this](GStreamerAppSource &appSrc) { H264EnoughData (appSrc); });

	m_gstElementH264DecodeAppSrc.propertySet ("do-timestamp", TRUE);
	m_gstElementH264DecodeAppSrc.propertySet ("is-live", TRUE);

	// add app src
	bResult = gst_bin_add (GST_BIN (m_pGstElementH264DecodePipeline), m_gstElementH264DecodeAppSrc.get ());
	stiTESTCOND (bResult, stiRESULT_ERROR);

	// Create decoder
	m_pGstElementH264Decoder = gst_element_factory_make ("nv_omx_h264dec", "h264_decode_element");
	stiTESTCOND (m_pGstElementH264Decoder, stiRESULT_ERROR);

	//g_object_set (G_OBJECT (m_pGstElementH264Decoder), "input-buffers", 4, NULL);
	//g_object_set (G_OBJECT (m_pGstElementH264Decoder), "output-buffers", 4, NULL);
	g_object_set (G_OBJECT (m_pGstElementH264Decoder), "low-latency", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementH264Decoder), "full-frame", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementH264Decoder), "enable-error-check", TRUE, NULL);


	gst_bin_add (GST_BIN(m_pGstElementH264DecodePipeline),m_pGstElementH264Decoder);

#ifdef ENABLE_264_DECODE_PARSER
	m_pGstElementH264DecodeParser = gst_element_factory_make ("h264parse", "h264_decode_parse_element");
	stiTESTCOND (m_pGstElementH264DecodeParser, stiRESULT_ERROR);

	gst_bin_add (GST_BIN(m_pGstElementH264DecodePipeline), m_pGstElementH264DecodeParser);

		bLinked = gst_element_link (
			m_pGstElementH264DecodeParser,
			m_pGstElementH264Decoder);

	if (!bLinked)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::m_pGstElementH264DecodeParser and m_pGstElementH264Decoder didn't link \n");
	}

	bLinked = gst_element_link (m_gstElementH264DecodeAppSrc.get (), m_pGstElementH264DecodeParser);
	stiTESTCOND (bLinked, stiRESULT_ERROR);
#else

	bLinked = gst_element_link (m_gstElementH264DecodeAppSrc.get (), m_pGstElementH264Decoder);
	stiTESTCOND (bLinked, stiRESULT_ERROR);
#endif


	// Create add and link output selector
	m_pGstElementH264DecodeOutputSelector = gst_element_factory_make ("output-selector", "h264_decode_output_selector_element");
	stiTESTCOND (m_pGstElementH264DecodeOutputSelector, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementH264DecodeOutputSelector), "pad-negotiation-mode", 2, NULL);

	// add output selector
	bResult = gst_bin_add (GST_BIN (m_pGstElementH264DecodePipeline), m_pGstElementH264DecodeOutputSelector);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bLinked = gst_element_link (m_pGstElementH264Decoder, m_pGstElementH264DecodeOutputSelector);
	stiTESTCOND (bLinked, stiRESULT_ERROR);

	// Create a fake sink
	m_pGstElementH264DecodeFakeSink = gst_element_factory_make ("fakesink", "h264_decode_fakesink0_element");
	stiTESTCOND (m_pGstElementH264DecodeFakeSink, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementH264DecodeFakeSink), "async", FALSE, NULL);

	bResult = gst_bin_add (GST_BIN (m_pGstElementH264DecodePipeline), m_pGstElementH264DecodeFakeSink);
	stiTESTCOND (bResult, stiRESULT_ERROR);


	// link output selector to fake sink
	m_pGstPadH264DecodeOutputSelectorFakeSrc = gst_element_get_request_pad (m_pGstElementH264DecodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadH264DecodeOutputSelectorFakeSrc);

	pGstPad = gst_element_get_static_pad (m_pGstElementH264DecodeFakeSink, "sink");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	eGstPadLinkReturn = gst_pad_link (m_pGstPadH264DecodeOutputSelectorFakeSrc, pGstPad);
	stiTESTCONDMSG (eGstPadLinkReturn == GST_PAD_LINK_OK, stiRESULT_ERROR, "m_pGstPadH264DecodeOutputSelectorFakeSrc and m_pGstElementH264DecodeFakeSink didn't link\n");

	gst_object_unref (pGstPad);
	pGstPad = nullptr;


	// link output selector to display sink
	m_pGstPadH264DecodeOutputSelectorDisplaySrc = gst_element_get_request_pad (m_pGstElementH264DecodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadH264DecodeOutputSelectorDisplaySrc);


	g_object_set (G_OBJECT (m_pGstElementH264DecodeOutputSelector), "active-pad", m_pGstPadH264DecodeOutputSelectorFakeSrc, NULL);

	// Setup the bus to report warnings to our callback
	GstBus *pGstBus;
	pGstBus = gst_pipeline_get_bus (GST_PIPELINE (m_pGstElementH264DecodePipeline));
	stiTESTCOND (pGstBus, stiRESULT_ERROR);
	
	m_unH264DecodePipelineWatch = gst_bus_add_watch (pGstBus, H264BusCallback, this);
	
	g_object_unref (pGstBus);
	pGstBus = nullptr;

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(m_pGstElementH264DecodePipeline), GST_DEBUG_GRAPH_SHOW_ALL, "H264DecodePipelineCreate");
	);

#ifdef ENABLE_H264_DEBUG_PROBES
	pGstPad = gst_element_get_static_pad (m_gstElementH264DecodeAppSrc.get (), "src");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

	if (m_pGstElementH264DecodeParser)
	{
		pGstPad = gst_element_get_static_pad (m_pGstElementH264DecodeParser, "sink");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;

		pGstPad = gst_element_get_static_pad (m_pGstElementH264DecodeParser, "src");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;
	}

	pGstPad = gst_element_get_static_pad (m_pGstElementH264Decoder, "sink");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

	pGstPad = gst_element_get_static_pad (m_pGstElementH264Decoder, "src");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

#endif

STI_BAIL:

	if (pGstPad)
	{
		gst_object_unref (pGstPad);
		pGstPad = nullptr;
	}

	return (hResult);
}

stiHResult CstiVideoOutput::H264DecodePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H264DecodePipelineDestroy\n");
	);

	remoteVideoModeChangedSignal.Emit (eRVM_BLANK);

	if (m_pGstElementH264DecodePipeline)
	{
		//
		// Stop pipeline
		//
		StateChangeReturn = gst_element_set_state (m_pGstElementH264DecodePipeline, GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_pGstElementH264DecodePipeline, GST_STATE_NULL)\n");
		}

		gst_element_release_request_pad (m_pGstElementH264DecodeOutputSelector, m_pGstPadH264DecodeOutputSelectorDisplaySrc);
		gst_object_unref (m_pGstPadH264DecodeOutputSelectorDisplaySrc);
		m_pGstPadH264DecodeOutputSelectorDisplaySrc = nullptr;
		gst_element_release_request_pad (m_pGstElementH264DecodeOutputSelector, m_pGstPadH264DecodeOutputSelectorFakeSrc);
		gst_object_unref (m_pGstPadH264DecodeOutputSelectorFakeSrc);
		m_pGstPadH264DecodeOutputSelectorFakeSrc = nullptr;

		if (m_unH264DecodePipelineWatch)
		{
			g_source_remove (m_unH264DecodePipelineWatch);
			m_unH264DecodePipelineWatch = 0;
		}

		if (m_unH264DoLatencySignalHandler)
		{
			g_signal_handler_disconnect (m_pGstElementH264DecodePipeline, m_unH264DoLatencySignalHandler);
			m_unH264DoLatencySignalHandler = 0;
		}

		gst_object_unref (GST_OBJECT (m_pGstElementH264DecodePipeline));
		m_pGstElementH264DecodePipeline = nullptr;

		m_gstElementH264DecodeAppSrc.clear ();
		m_pGstElementH264DecodeParser = nullptr;
		m_pGstElementH264Decoder = nullptr;
		m_pGstElementH264DecodeOutputSelector = nullptr;
		m_pGstElementH264DecodeFakeSink = nullptr;
	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoOutput::H264DecodePipelinePrime ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int i = 0;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H264DecodePipelinePrime\n");
	);

	for (i = 0; i < NUM_DEC_PRIME_BITSTREAM_BUFS; i++)
	{
		hResult = H264DecodePipelineBlackFramePush ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoOutput::H263DecodePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H263DecodePipelineStart\n");
	);
#ifdef ENABLE_VIDEO_OUTPUT
	bool bLocked = false;
	std::unique_lock<std::mutex> lock (m_mutexH263PipelineReady);
	lock.unlock ();

	hResult = H263DecodePipelineCreate ();
	stiTESTRESULT ();

	lock.lock ();
	m_bH263DecodePipelineNeedsData = false;
	lock.unlock ();

	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = gst_element_set_state (m_pGstElementH263DecodePipeline, GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_pGstElementH263DecodePipeline, GST_STATE_PLAYING)\n");
	}

	lock.lock ();

	bLocked = true;

	if (m_bH263DecodePipelineNeedsData)
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::H263DecodePipelineStart: Pipeline ready\n");
		);
	}
	else
	{
		for (;;)
		{
			auto status = m_conditionH263PipelineReady.wait_for (lock, std::chrono::seconds(20));

			if (status == std::cv_status::timeout)
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::H263DecodePipelineStart: waiting for pipeline to be ready\n");
			}

			if (m_bH263DecodePipelineNeedsData)
			{
				break;
			}
		}
	}

	lock.unlock ();
	bLocked = false;

	//
	// Prime the decoder to get passed
	// the large delay that the decoder causes every time
	// it is instantiated and first used
	//
	//hResult = H263DecodePipelinePrime ();
	//stiTESTRESULT ();

STI_BAIL:

	if (bLocked)
	{
		bLocked = false;
	}
#endif

	return (hResult);
}


stiHResult CstiVideoOutput::H263DecodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPadLinkReturn eGstPadLinkReturn;
	bool bResult = true;
	GstPad *pGstPad = nullptr;
	gboolean bLinked;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H263DecodePipelineCreate\n");
	);

	hResult = H263DecodePipelineDestroy ();
	stiTESTRESULT ();

	stiASSERT (m_pGstElementH263DecodePipeline == nullptr);

	m_pGstElementH263DecodePipeline = gst_pipeline_new ("H263_decode_pipeline_element");
	stiTESTCOND (m_pGstElementH263DecodePipeline, stiRESULT_ERROR);

	m_unH263DoLatencySignalHandler = g_signal_connect(m_pGstElementH263DecodePipeline, "do-latency",
			 G_CALLBACK (do_latency_cb), NULL);

	m_gstElementH263DecodeAppSrc = GStreamerAppSource("H263_decode_app_src_element");
	stiTESTCOND (m_gstElementH263DecodeAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_gstElementH263DecodeAppSrc.needDataCallbackSet([this](GStreamerAppSource &appSrc) { H263NeedData (appSrc); });
	m_gstElementH263DecodeAppSrc.enoughDataCallbackSet([this](GStreamerAppSource &appSrc) { H263EnoughData (appSrc); });

	m_gstElementH263DecodeAppSrc.propertySet ("do-timestamp", TRUE);
	m_gstElementH263DecodeAppSrc.propertySet ("is-live", TRUE);

	// add app src
	bResult = gst_bin_add (GST_BIN (m_pGstElementH263DecodePipeline), m_gstElementH263DecodeAppSrc.get ());
	stiTESTCOND (bResult, stiRESULT_ERROR);

#ifdef ENABLE_263_DECODE_PARSER
	m_pGstElementH263DecodeParser = gst_element_factory_make ("h263parse", "h263_decode_parse_element");
	stiTESTCOND (m_pGstElementH263DecodeParser, stiRESULT_ERROR);

	gst_bin_add (GST_BIN(m_pGstElementH263DecodePipeline), m_pGstElementH263DecodeParser);
#endif

#ifdef ENABLE_HARDWARE_263_DECODER
	m_pGstElementH263Decoder = gst_element_factory_make ("nv_omx_h263dec", "h263_decode_element");
	stiTESTCOND (m_pGstElementH263Decoder, stiRESULT_ERROR);

	gst_bin_add (GST_BIN(m_pGstElementH263DecodePipeline), m_pGstElementH263Decoder);
#else
	m_pGstElementH263Decoder = gst_element_factory_make ("ffdec_h263", "h263_decode_element");
	stiTESTCOND (m_pGstElementH263Decoder, stiRESULT_ERROR);

	gst_bin_add (GST_BIN(m_pGstElementH263DecodePipeline), m_pGstElementH263Decoder);

	m_pGstElementH263DisplayVidconv = gst_element_factory_make("nvvidconv", "h263_decode_display_nvvidconv_element");
	stiTESTCOND (m_pGstElementH263DisplayVidconv, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementH263DisplayVidconv), "qos", FALSE, NULL);
	g_object_set (G_OBJECT (m_pGstElementH263DisplayVidconv), "output-buffers", 2, NULL);

	gst_bin_add (GST_BIN(m_pGstElementH263DecodePipeline), m_pGstElementH263DisplayVidconv);
#endif

	if (m_pGstElementH263DisplayVidconv)
	{
		if (m_pGstElementH263DecodeParser)
		{
			bLinked = gst_element_link_many (
				m_gstElementH263DecodeAppSrc.get (),
				m_pGstElementH263DecodeParser,
				m_pGstElementH263Decoder,
				m_pGstElementH263DisplayVidconv,
				NULL);
		}
		else
		{
			bLinked = gst_element_link_many (
				m_gstElementH263DecodeAppSrc.get (),
				m_pGstElementH263Decoder,
				m_pGstElementH263DisplayVidconv,
				NULL);
		}
	}
	else
	{
		if (m_pGstElementH263DecodeParser)
		{
			bLinked = gst_element_link_many (
				m_gstElementH263DecodeAppSrc.get (),
				m_pGstElementH263DecodeParser,
				m_pGstElementH263Decoder,
				NULL);
		}
		else
		{
			bLinked = gst_element_link (m_gstElementH263DecodeAppSrc.get (), m_pGstElementH263Decoder);
		}
	}

	if (!bLinked)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::H263DecodeBinAddAndLink didn't link \n");
	}

	// Create add and link output selector
	m_pGstElementH263DecodeOutputSelector = gst_element_factory_make ("output-selector", "h263_decode_output_selector_element");
	stiTESTCOND (m_pGstElementH263DecodeOutputSelector, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementH263DecodeOutputSelector), "pad-negotiation-mode", 2, NULL);

	// add output selector
	bResult = gst_bin_add (GST_BIN (m_pGstElementH263DecodePipeline), m_pGstElementH263DecodeOutputSelector);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	if (m_pGstElementH263DisplayVidconv)
	{
		bLinked = gst_element_link (m_pGstElementH263DisplayVidconv, m_pGstElementH263DecodeOutputSelector);
		stiTESTCOND (bLinked, stiRESULT_ERROR);
	}
	else
	{
		bLinked = gst_element_link (m_pGstElementH263Decoder, m_pGstElementH263DecodeOutputSelector);
		stiTESTCOND (bLinked, stiRESULT_ERROR);
	}

	// Create a fake sink
	m_pGstElementH263DecodeFakeSink = gst_element_factory_make ("fakesink", "h263_decode_fakesink0_element");
	stiTESTCOND (m_pGstElementH263DecodeFakeSink, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementH263DecodeFakeSink), "async", FALSE, NULL);

	bResult = gst_bin_add (GST_BIN (m_pGstElementH263DecodePipeline), m_pGstElementH263DecodeFakeSink);
	stiTESTCOND (bResult, stiRESULT_ERROR);


	// link output selector to fake sink
	m_pGstPadH263DecodeOutputSelectorFakeSrc = gst_element_get_request_pad (m_pGstElementH263DecodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadH263DecodeOutputSelectorFakeSrc);

	pGstPad = gst_element_get_static_pad (m_pGstElementH263DecodeFakeSink, "sink");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	eGstPadLinkReturn = gst_pad_link (m_pGstPadH263DecodeOutputSelectorFakeSrc, pGstPad);
	stiTESTCONDMSG (eGstPadLinkReturn == GST_PAD_LINK_OK, stiRESULT_ERROR, "m_pGstPadH263DecodeOutputSelectorFakeSrc and m_pGstElementH263DecodeFakeSink didn't link\n");

	gst_object_unref (pGstPad);
	pGstPad = nullptr;


	// link output selector to display sink
	m_pGstPadH263DecodeOutputSelectorDisplaySrc = gst_element_get_request_pad (m_pGstElementH263DecodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadH263DecodeOutputSelectorDisplaySrc);

	g_object_set (G_OBJECT (m_pGstElementH263DecodeOutputSelector), "active-pad", m_pGstPadH263DecodeOutputSelectorFakeSrc, NULL);

	// Setup the bus to report warnings to our callback
	GstBus *pGstBus;
	pGstBus = gst_pipeline_get_bus (GST_PIPELINE (m_pGstElementH263DecodePipeline));
	stiTESTCOND (pGstBus, stiRESULT_ERROR);
	
	m_unH263DecodePipelineWatch = gst_bus_add_watch (pGstBus, H263BusCallback, this);
	
	g_object_unref (pGstBus);
	pGstBus = nullptr;

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(m_pGstElementH263DecodePipeline), GST_DEBUG_GRAPH_SHOW_ALL, "H263DecodePipelineCreate");
	);

#ifdef ENABLE_H263_DEBUG_PROBES
	pGstPad = gst_element_get_static_pad (m_gstElementH263DecodeAppSrc.get (), "src");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

	if (m_pGstElementH263DecodeParser)
	{
		pGstPad = gst_element_get_static_pad (m_pGstElementH263DecodeParser, "sink");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;

		pGstPad = gst_element_get_static_pad (m_pGstElementH263DecodeParser, "src");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;
	}

	pGstPad = gst_element_get_static_pad (m_pGstElementH263Decoder, "sink");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

	pGstPad = gst_element_get_static_pad (m_pGstElementH263Decoder, "src");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

	gst_object_unref(pGstPad);
	pGstPad = NULL;

	if (m_pGstElementH263DisplayVidconv)
	{
		pGstPad = gst_element_get_static_pad (m_pGstElementH263DisplayVidconv, "sink");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;

		pGstPad = gst_element_get_static_pad (m_pGstElementH263DisplayVidconv, "src");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		gst_pad_add_buffer_probe (pGstPad, (GCallback)DebugProbeFunction, this);

		gst_object_unref(pGstPad);
		pGstPad = NULL;
	}

#endif

STI_BAIL:

	if (pGstPad)
	{
		gst_object_unref (pGstPad);
		pGstPad = nullptr;
	}

	return (hResult);
}

stiHResult CstiVideoOutput::H263DecodePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::H263DecodePipelineDestroy\n");
	);

	remoteVideoModeChangedSignal.Emit (eRVM_BLANK);

	if (m_pGstElementH263DecodePipeline)
	{
		//
		// Stop pipeline
		//
		StateChangeReturn = gst_element_set_state (m_pGstElementH263DecodePipeline, GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "FAIL: gst_element_set_state (m_pGstElementH263DecodePipeline, GST_STATE_NULL)\n");
		}

		gst_element_release_request_pad (m_pGstElementH263DecodeOutputSelector, m_pGstPadH263DecodeOutputSelectorDisplaySrc);
		gst_object_unref (m_pGstPadH263DecodeOutputSelectorDisplaySrc);
		m_pGstPadH263DecodeOutputSelectorDisplaySrc = nullptr;
		gst_element_release_request_pad (m_pGstElementH263DecodeOutputSelector, m_pGstPadH263DecodeOutputSelectorFakeSrc);
		gst_object_unref (m_pGstPadH263DecodeOutputSelectorFakeSrc);
		m_pGstPadH263DecodeOutputSelectorFakeSrc = nullptr;


		if (m_unH263DecodePipelineWatch)
		{
			g_source_remove (m_unH263DecodePipelineWatch);
			m_unH263DecodePipelineWatch = 0;
		}

		if (m_unH263DoLatencySignalHandler)
		{
			g_signal_handler_disconnect (m_pGstElementH263DecodePipeline, m_unH263DoLatencySignalHandler);
			m_unH263DoLatencySignalHandler = 0;
		}

		gst_object_unref (GST_OBJECT (m_pGstElementH263DecodePipeline));
		m_pGstElementH263DecodePipeline = nullptr;

		m_gstElementH263DecodeAppSrc.clear ();
		m_pGstElementH263DecodeParser = nullptr;
		m_pGstElementH263Decoder = nullptr;
		m_pGstElementH263DisplayVidconv = nullptr;
		m_pGstElementH263DecodeOutputSelector = nullptr;
		m_pGstElementH263DecodeFakeSink = nullptr;

	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoOutput::DisplaySinkStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bMirrored = false;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkStart\n");
	);

	// Create display sink
	hResult = DisplaySinkCreate (bMirrored);
	stiTESTRESULT ();

	if (estiVIDEO_H264 == m_eCodec)
	{
		// Link display sink
		hResult = DisplaySinkAddAndLink (m_pGstElementH264DecodePipeline, m_pGstPadH264DecodeOutputSelectorDisplaySrc);
		stiTESTRESULT ();

		g_object_set (G_OBJECT (m_pGstElementH264DecodeOutputSelector), "active-pad", m_pGstPadH264DecodeOutputSelectorDisplaySrc, NULL);
	}
	else if (estiVIDEO_H263 == m_eCodec)
	{
		// Link display sink
		hResult = DisplaySinkAddAndLink (m_pGstElementH263DecodePipeline, m_pGstPadH263DecodeOutputSelectorDisplaySrc);
		stiTESTRESULT ();

		g_object_set (G_OBJECT (m_pGstElementH263DecodeOutputSelector), "active-pad", m_pGstPadH263DecodeOutputSelectorDisplaySrc, NULL);
	}
	else
	{
		stiASSERTMSG (estiFALSE, "WARNING m_eCodec is unknown\n");
	}
	
	hResult = DecodeDisplaySinkMove ();
	stiTESTRESULT ();

	// Set sink to playing
	hResult = DisplaySinkPlay ();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoOutput::DisplaySinkStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkStop\n");
	);

	if (estiVIDEO_H264 == m_eCodec)
	{
		//TODO: See if we still need this
		hResult = H264DecodePipelineBlackFramePush ();
		stiTESTRESULT ();

		// Move output selectors to fake sinks
		g_object_set (G_OBJECT (m_pGstElementH264DecodeOutputSelector), "active-pad", m_pGstPadH264DecodeOutputSelectorFakeSrc, NULL);

		hResult = DisplaySinkRemove (m_pGstElementH264DecodePipeline);
		stiTESTRESULT ();
	}
	else if (estiVIDEO_H263 == m_eCodec)
	{
		//TODO: See if we still need this
		hResult = H263DecodePipelineBlackFramePush ();
		stiTESTRESULT ();

		// Move output selectors to fake sinks
		g_object_set (G_OBJECT (m_pGstElementH263DecodeOutputSelector), "active-pad", m_pGstPadH263DecodeOutputSelectorFakeSrc, NULL);

		hResult = DisplaySinkRemove (m_pGstElementH263DecodePipeline);
		stiTESTRESULT ();
	}
	else
	{
		stiASSERTMSG (estiFALSE, "WARNING m_eCodec is unknown\n");
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoOutput::DisplaySinkCreate (
	bool bMirrored)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkCreate m_DisplaySettings.stRemoteView.unPosX = ", m_DisplaySettings.stRemoteView.unPosX, "\n");
		vpe::trace ("CstiVideoOutput::DisplaySinkCreate m_DisplaySettings.stRemoteView.unPosY = ", m_DisplaySettings.stRemoteView.unPosY, "\n");
		vpe::trace ("CstiVideoOutput::DisplaySinkCreate m_DisplaySettings.stRemoteView.unWidth = ", m_DisplaySettings.stRemoteView.unWidth, "\n");
		vpe::trace ("CstiVideoOutput::DisplaySinkCreate m_DisplaySettings.stRemoteView.unHeight = ", m_DisplaySettings.stRemoteView.unHeight, "\n");
		vpe::trace ("CstiVideoOutput::DisplaySinkCreate m_DisplaySettings.stRemoteView.bVisible = ", m_DisplaySettings.stRemoteView.bVisible, "\n");
	);
	
	stiASSERT (m_decodeDisplaySink.get () == nullptr);

	m_decodeDisplaySink = GStreamerElement ("nv_omx_videosink", "decode_display_sink_element");
	stiTESTCOND (m_decodeDisplaySink.get (), stiRESULT_ERROR);

	if (bMirrored)
	{
		m_decodeDisplaySink.propertySet ("mirror", true);
	}

	m_decodeDisplaySink.propertySet ("hdmi", true);
	m_decodeDisplaySink.propertySet ("overlay", 2);
	m_decodeDisplaySink.propertySet ("force-aspect-ratio", true);

	m_decodeDisplaySink.propertySet ("sync", false);

	m_decodeDisplaySink.propertySet ("async", false);
	m_decodeDisplaySink.propertySet ("enable-last-buffer", false);
	m_decodeDisplaySink.propertySet ("qos", true);
	m_decodeDisplaySink.propertySet ("force-fullscreen", false);

	m_decodeDisplaySink.propertySet ("max-lateness", MAX_LATENESS);

	char win[50];
	sprintf(win,"%dx%d-%dx%d", m_DisplaySettings.stRemoteView.unPosX,
	                           m_DisplaySettings.stRemoteView.unPosY,
	                           m_DisplaySettings.stRemoteView.unWidth,
	                           m_DisplaySettings.stRemoteView.unHeight);

	// I dont know why but these settings must be set before the disablevideo setting will take effect
	m_decodeDisplaySink.propertySet ("window", win);

	VideoSinkVisibilityUpdate (false);

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoOutput::DisplaySinkRemove (
	GstElement *pGstElementPipeline)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkRemove\n");
	);
	
	stiTESTCOND (pGstElementPipeline, stiRESULT_ERROR);
	stiTESTCOND (m_decodeDisplaySink.get (), stiRESULT_ERROR);

	hResult = DisplaySinkNull ();
	stiTESTRESULT ();

	gst_bin_remove (GST_BIN (pGstElementPipeline), m_decodeDisplaySink.get ());
	m_decodeDisplaySink.clear ();
	
STI_BAIL:

	return hResult;
}

stiHResult CstiVideoOutput::DisplaySinkAddAndLink (
	GstElement *pGstElementPipeline,
	GstPad *pGstLinkPad)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstPad = nullptr;
	gboolean bResult = false;
	gboolean bLinked;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkAddAndLink\n");
	);

	bResult = gst_bin_add (GST_BIN(pGstElementPipeline), m_decodeDisplaySink.get ());

	stiASSERTMSG (bResult, "DisplaySinkAddAndLink failed to add pipeline to decode display sink");

	// Fix bug#27559 attempt to remove the element and then add it on failure.
	// It could be that it already exists.
	if (!bResult)
	{
		GstElement *oldElement = gst_bin_get_by_name (GST_BIN(pGstElementPipeline), "decode_display_sink_element");
		gst_bin_remove (GST_BIN (pGstElementPipeline), oldElement);
		bResult = gst_bin_add (GST_BIN(pGstElementPipeline), m_decodeDisplaySink.get ());
		stiTESTCONDMSG (bResult, stiRESULT_ERROR, "DisplaySinkAddAndLink remove and add pipeline");
	}

	if (pGstLinkPad)
	{
		// link output selector to display sink
		pGstPad = gst_element_get_static_pad (m_decodeDisplaySink.get (), "sink");
		stiTESTCOND (pGstPad, stiRESULT_ERROR);

		bLinked = gst_pad_link (pGstLinkPad, pGstPad);
		stiTESTCONDMSG (bLinked == GST_PAD_LINK_OK, stiRESULT_ERROR, "pGstLinkPad and m_decodeDisplaySink didn't link\n");

		gst_object_unref(pGstPad);
		pGstPad = nullptr;
	}

STI_BAIL:

	if (pGstPad)
	{
		gst_object_unref(pGstPad);
		pGstPad = nullptr;
	}

	return (hResult);
}

stiHResult CstiVideoOutput::DisplaySinkPlay ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstStateChangeReturn StateChangeReturn;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkPlay\n");
	);

	stiTESTCOND (m_decodeDisplaySink.get (), stiRESULT_ERROR);

	StateChangeReturn = m_decodeDisplaySink.stateSet(GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::DisplaySinkPlay: GST_STATE_CHANGE_FAILURE: m_decodeDisplaySink.stateSet(GST_STATE_PLAYING)\n");
	}
	
	ChangeStateWait (m_decodeDisplaySink);

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoOutput::DisplaySinkNull ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstStateChangeReturn StateChangeReturn;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySinkNull\n");
	);

	stiTESTCOND (m_decodeDisplaySink.get (), stiRESULT_ERROR);

	StateChangeReturn = m_decodeDisplaySink.stateSet(GST_STATE_NULL);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::DisplaySinkNull: GST_STATE_CHANGE_FAILURE: m_decodeDisplaySink.stateSet (GST_STATE_NULL)\n");
	}

	ChangeStateWait (m_decodeDisplaySink);

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoOutput::H264PipelineGet (
	GstElement **ppGstElementPipeline) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (ppGstElementPipeline)
	{
		*ppGstElementPipeline = m_pGstElementH264DecodePipeline;
	}
	
	return (hResult);
}

stiHResult CstiVideoOutput::H263PipelineGet (
	GstElement **ppGstElementPipeline) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (ppGstElementPipeline)
	{
		*ppGstElementPipeline = m_pGstElementH263DecodePipeline;
	}
	
	return (hResult);
}

stiHResult CstiVideoOutput::FrameCapturePipelineGet (
	GstElement **ppGstElementPipeline) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (ppGstElementPipeline)
	{
		*ppGstElementPipeline = m_pGstElementFrameCapturePipeline;
	}
	
	return (hResult);
}

/*!
 * \brief H264BusCallback: Call back for H264 bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  gpointer - The this pointer
 * \return gboolean - Always returns true
 */
gboolean CstiVideoOutput::H264BusCallback (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	void *pUserData)
{
	auto pThis = (CstiVideoOutput *)pUserData;
	
	GstElement *pGstElementPipeline = nullptr;
	
	pThis->H264PipelineGet (&pGstElementPipeline);
	
	pThis->BusMessageProcess (pGstBus, pGstMessage, pUserData, pGstElementPipeline);
	
	return true;
}

/*!
 * \brief H263BusCallback: Call back for H263 bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  gpointer - The this pointer
 * \return gboolean - Always returns true
 */
gboolean CstiVideoOutput::H263BusCallback (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	void *pUserData)
{
	auto pThis = (CstiVideoOutput *)pUserData;
	
	GstElement *pGstElementPipeline = nullptr;
	
	pThis->H263PipelineGet (&pGstElementPipeline);
	
	pThis->BusMessageProcess (pGstBus, pGstMessage, pUserData, pGstElementPipeline);
	
	return true;
}

/*!
 * \brief FrameCaptureBusCallback: Call back for FrameCapture bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  gpointer - The this pointer
 * \return gboolean - Always returns true
 */
gboolean CstiVideoOutput::FrameCaptureBusCallback (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	void *pUserData)
{
	auto pThis = (CstiVideoOutput *)pUserData;
	
	GstElement *pGstElementPipeline = nullptr;
	
	pThis->FrameCapturePipelineGet (&pGstElementPipeline);
	
	pThis->BusMessageProcess (pGstBus, pGstMessage, pUserData, pGstElementPipeline);
	
	return true;
}

static int s_nGraphNumber = 0;

/*!
 * \brief BusMessageProcess: Process the messages from any bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  pUserData - The this pointer
 * \param  GstElement - The pipeline
 * \return stiHResult
 */
stiHResult CstiVideoOutput::BusMessageProcess (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	void *pUserData,
	GstElement *pGstElementPipeline)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto pThis = (CstiVideoOutput *)pUserData;
	
	stiTESTCONDMSG (pGstMessage, stiRESULT_ERROR, "CstiVideoOutput::BusMessageProcess: pGstMessage is NULL\n");
	
	switch (GST_MESSAGE_TYPE(pGstMessage))
	{
		case GST_MESSAGE_ELEMENT:
		{
			if (gst_structure_has_name (pGstMessage->structure, "decoder-status"))
			{
				const GstStructure *pGstStructure = gst_message_get_structure (pGstMessage);

				gboolean bErrorFrame = false;
				guint nDecodedMBs = 0;
				guint nConcealedMBs = 0;
				guint nFrameDecodeTime = 0;

				gst_structure_get_boolean (pGstStructure, "bErrorFrame", &bErrorFrame);
				gst_structure_get_uint (pGstStructure, "DecodedMBs", &nDecodedMBs);
				gst_structure_get_uint (pGstStructure, "ConcealedMBs", &nConcealedMBs);
				gst_structure_get_uint (pGstStructure, "FrameDecodeTime", &nFrameDecodeTime);

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					vpe::trace ("CstiVideoOutput::BusMessageProcess: bErrorFrame = ", bErrorFrame, "\n");
					vpe::trace ("CstiVideoOutput::BusMessageProcess: nDecodedMBs = ",nDecodedMBs, "\n");
					vpe::trace ("CstiVideoOutput::BusMessageProcess: nConcealedMBs = ",nConcealedMBs, "\n");
					vpe::trace ("CstiVideoOutput::BusMessageProcess: nFrameDecodeTime = ",nFrameDecodeTime, "\n");
				);

				if (bErrorFrame)
				{
					pThis->keyframeNeededSignal.Emit ();
				}

			}

			break;
		}

		case GST_MESSAGE_ERROR:
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_error (pGstMessage, &pError, &pDebugInfo);

			std::string errorString ("CstiVideoOutput::BusMessageProcess: GST_MESSAGE_ERROR");
			
			if (pGstMessage)
			{
				errorString.append (": GST_OBJECT_NAME = ");
				errorString.append (GST_OBJECT_NAME(GST_MESSAGE_SRC (pGstMessage)));
			}
			
			if (pError && pError->message)
			{
				errorString.append (": Error message = ");
				errorString.append (pError->message);
			}
			
			if (pDebugInfo)
			{
				errorString.append (": Debug message = ");
				errorString.append (pDebugInfo);
			}
			
			stiASSERTMSG (estiFALSE, errorString.c_str ());

			if (pError)
			{
				g_error_free (pError);
				pError = nullptr;
			}

			if (pDebugInfo)
			{
				g_free (pDebugInfo);
				pDebugInfo = nullptr;
			}

			break;
		}

		case GST_MESSAGE_WARNING:
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_warning (pGstMessage, &pError, &pDebugInfo);

			stiDEBUG_TOOL (g_stiVideoOutputDebug,
						   
				std::string warningString ("CstiVideoOutput::BusMessageProcess: GST_MESSAGE_WARNING");
			
				if (pGstMessage)
				{
					warningString.append (": GST_OBJECT_NAME = ");
					warningString.append (GST_OBJECT_NAME(GST_MESSAGE_SRC (pGstMessage)));
				}
				
				if (pError && pError->message)
				{
					warningString.append (": Warning message = ");
					warningString.append (pError->message);
				}
				
				if (pDebugInfo)
				{
					warningString.append (": Debug message = ");
					warningString.append (pDebugInfo);
				}
					
				vpe::trace (warningString.c_str ());
			);

			if (pError)
			{
				g_error_free (pError);
				pError = nullptr;
			}

			if (pDebugInfo)
			{
				g_free (pDebugInfo);
				pDebugInfo = nullptr;
			}

			break;
		}

		case GST_MESSAGE_STATE_CHANGED:
		{
			GstState old_state;
			GstState new_state;
			GstState pending_state;

			gst_message_parse_state_changed (pGstMessage, &old_state, &new_state, &pending_state);

			//
			// Dump the graph when the pipeline changes state.
			//
			stiDEBUG_TOOL (g_stiVideoOutputDebug,
				stiTrace ("CstiVideoOutput::BusMessageProcess: Element %s changed state from %s to %s. Pending: %s\n",
					GST_OBJECT_NAME (pGstMessage->src),
					gst_element_state_get_name (old_state),
					gst_element_state_get_name (new_state),
					gst_element_state_get_name (pending_state));
			);

			if (pGstElementPipeline && pGstMessage->src == GST_OBJECT(pGstElementPipeline))
			{
				char szFileName[256];

				sprintf (szFileName, "ntouchvp2_decode_%s_%s_%d", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state), s_nGraphNumber);

				s_nGraphNumber++;

				stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
					GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(pGstElementPipeline), GST_DEBUG_GRAPH_SHOW_ALL, szFileName);
				);
			}

			break;
		}

		default:
			break;
	}
	
STI_BAIL:
  
  return hResult;
}

GstFlowReturn CstiVideoOutput::FrameCaptureAppSinkGstBufferPut (
	GstElement* pGstElement,
	gpointer pUser)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::FrameCaptureAppSinkGstBufferPut\n");
	);

	auto gstAppSink = GStreamerAppSink((GstAppSink*)pGstElement, true);

	if (gstAppSink.get () != nullptr)
	{
		auto sample = gstAppSink.pullSample ();

		if (sample.get () != nullptr)
		{
			auto gstBuffer = sample.bufferGet ();

			if (gstBuffer.get () != nullptr)
			{
				auto pCstiVideoOutput = (CstiVideoOutput*)pUser;

				if (pCstiVideoOutput)
				{
					pCstiVideoOutput->FrameCaptureBufferReceived (gstBuffer);
				}
				else
				{
					stiASSERTMSG (estiFALSE, "WARNING no pCstiVideoOutput\n");
				}
			}
		}
	}

	return GST_FLOW_OK;
}


stiHResult CstiVideoOutput::FrameCapturePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bLinked;
	GstBus *pGstBus;

	stiASSERT (m_pGstElementFrameCapturePipeline == nullptr);

	m_pGstElementFrameCapturePipeline = gst_pipeline_new ("decode_frame_capture_pipeline_element");
	stiTESTCOND (m_pGstElementFrameCapturePipeline, stiRESULT_ERROR);

	m_gstElementFrameCaptureAppSrc = GStreamerAppSource ("decode_frame_capture_app_src_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_pGstElementFrameCaptureConvert = gst_element_factory_make("nvvidconv", "decode_frame_capture_video_convert");
	stiTESTCOND(m_pGstElementFrameCaptureConvert, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureConvert), "output-buffers", 2, NULL);

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
	m_gstElementFrameCaptureAppSink = GStreamerAppSink ("decode_frame_capture_appsink_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSink.get () != nullptr, stiRESULT_ERROR);
	g_object_set (m_gstElementFrameCaptureAppSink.get (), "sync", FALSE, NULL);
	g_object_set (m_gstElementFrameCaptureAppSink.get (), "async", FALSE, NULL);
	g_object_set (m_gstElementFrameCaptureAppSink.get (), "qos", FALSE, NULL);
	g_object_set (m_gstElementFrameCaptureAppSink.get (), "enable-last-buffer", FALSE, NULL);
	g_object_set (m_gstElementFrameCaptureAppSink.get (), "emit-signals", TRUE, NULL);
	//g_object_set (m_gstElementFrameCaptureAppSink.get (), "max-buffers", 1, NULL);

	gst_bin_add_many (GST_BIN(m_pGstElementFrameCapturePipeline),
		m_gstElementFrameCaptureAppSrc.get (),
		m_pGstElementFrameCaptureConvert,
		m_pGstElementFrameCaptureJpegEnc,
		m_gstElementFrameCaptureAppSink.get (),
		NULL);

	bLinked = gst_element_link_many (
		m_gstElementFrameCaptureAppSrc.get (),
		m_pGstElementFrameCaptureConvert,
		m_pGstElementFrameCaptureJpegEnc,
		m_gstElementFrameCaptureAppSink.get (),
		NULL);

	if (!bLinked)
	{
		stiTHROWMSG (stiRESULT_ERROR, "failed linking m_gstElementFrameCaptureAppSrc, m_pGstElementFrameCaptureJpegEnc, m_gstElementFrameCaptureAppSink\n");
	}

	pGstBus = gst_pipeline_get_bus (GST_PIPELINE (m_pGstElementFrameCapturePipeline));
	stiTESTCOND (pGstBus, stiRESULT_ERROR);

	m_unFrameCapturePipelineWatch = gst_bus_add_watch (pGstBus, FrameCaptureBusCallback, this);

	g_object_unref (pGstBus);

	m_unNewBufferSignalHandler = g_signal_connect (m_gstElementFrameCaptureAppSink.get (), "new-buffer", G_CALLBACK (FrameCaptureAppSinkGstBufferPut), this);

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoOutput::FrameCapturePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstStateChangeReturn StateChangeReturn;

	if (m_pGstElementFrameCapturePipeline)
	{
		// Set to NULL state before removing
		StateChangeReturn = gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::FrameCapturePipelineDestroy: Could not gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_NULL)\n");
		}

		if (m_unFrameCapturePipelineWatch)
		{
			g_source_remove (m_unFrameCapturePipelineWatch);
			m_unFrameCapturePipelineWatch = 0;
		}

		if (m_unNewBufferSignalHandler)
		{
			g_signal_handler_disconnect (m_gstElementFrameCaptureAppSink.get (), m_unNewBufferSignalHandler);
			m_unNewBufferSignalHandler = 0;
		}

		gst_object_unref (m_pGstElementFrameCapturePipeline);
		m_pGstElementFrameCapturePipeline = nullptr;
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoOutput::FrameCapturePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::FrameCapturePipelineStart\n");
	);

#ifdef ENABLE_VIDEO_OUTPUT
	hResult = FrameCapturePipelineDestroy ();
	stiTESTRESULT ();

	hResult = FrameCapturePipelineCreate ();
	stiTESTRESULT ();

	GstStateChangeReturn StateChangeReturn;

	stiTESTCOND (m_pGstElementFrameCapturePipeline, stiRESULT_ERROR);

	StateChangeReturn = gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiVideoOutput::FrameCapturePipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementFrameCapturePipeline, GST_STATE_PLAYING)\n");
	}

STI_BAIL:
#endif

	return (hResult);
}

stiHResult CstiVideoOutput::VideoPlaybackReset ()
{
	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoPlaybackReset, this));

	return stiRESULT_SUCCESS;
}


void CstiVideoOutput::EventVideoPlaybackReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::EventVideoPlaybackReset: Restarting decode pipelines\n");
	);
	
	hResult = DisplaySinkStop ();
	stiTESTRESULT ();
	
	hResult = DecodePipelinesStart ();
	stiTESTRESULT ();
	
	// If we were playing then re-start
	if (m_bPlaying)
	{
		hResult = DisplaySinkStart ();
		stiTESTRESULT ();
		
		if (m_oBitstreamFifo.CountGet () > 0)
		{
			stiOSSignalSet2 (m_Signal, 1);
		}
	}

STI_BAIL:

	return;
}

/*!
 * \brief Sends the event that will start audio playback
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutput::VideoPlaybackStart ()
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::VideoPlaybackStart\n");
	);

	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoPlaybackStart, this));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief the Event handler that actually starts VideoPlayback
 *
 * \return stiHResult
 */
void CstiVideoOutput::EventVideoPlaybackStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	EstiRemoteVideoMode eMode;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::EventVideoPlaybackStart\n");
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

	hResult = DisplaySinkStart ();
	stiTESTRESULT ();

	if (m_oBitstreamFifo.CountGet () > 0)
	{
		stiOSSignalSet2 (m_Signal, 1);
	}

	if (m_bPrivacy)
	{
		eMode = eRVM_PRIVACY;
	}
	else if (m_bHolding)
	{
		eMode = eRVM_HOLD;
	}
	else
	{
		eMode = eRVM_VIDEO;
	}

	remoteVideoModeChangedSignal.Emit (eMode);

STI_BAIL:

	return;
}


/*!
 * \brief Sends the event that will stop audio playback
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutput::VideoPlaybackStop ()
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::VideoPlaybackStop\n");
	);

	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoPlaybackStop, this));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief the event handler that actually stops audio playback
 *
 * \return stiHResult
 */
void CstiVideoOutput::EventVideoPlaybackStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::EventVideoPlaybackStop\n");
	);

	m_bPlaying = false;
	m_bSPSAndPPSSentToDecoder = false;

	stiOSSignalSet2 (m_Signal, 0);

	hResult = DisplaySinkStop ();
	stiTESTRESULT ();

	m_un32DecodeWidth = DEFAULT_DECODE_WIDTH;
	m_un32DecodeHeight = DEFAULT_DECODE_HEIGHT;

	decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);
	videoOutputPlaybackStoppedSignal.Emit();

STI_BAIL:

	return;
}


/*!
 * \brief Sets codec for video playback
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutput::VideoPlaybackCodecSet (
	EstiVideoCodec eVideoCodec)
{
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::VideoPlaybackCodecSet: eVideoCodec = ", eVideoCodec, "\n");
	);

	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoCodecSet, this, eVideoCodec));

	return stiRESULT_SUCCESS;
}


void CstiVideoOutput::EventVideoCodecSet (
	EstiVideoCodec eVideoCodec)
{
	if (m_bPlaying)
	{
		stiASSERTMSG (estiFALSE, "WARNING cant change codec while playing\n");
	}
	else
	{
		m_eCodec = eVideoCodec;
		
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::EventVideoPlaybackCodecSet: m_eCodec = ", m_eCodec, "\n");
		);
	}
}


GstFlowReturn CstiVideoOutput::pushBuffer(GStreamerBuffer &gstBuffer)
{
	if (estiVIDEO_H264 == m_eCodec)
	{
		if (m_gstElementH264DecodeAppSrc.get () != nullptr)
		{
			return m_gstElementH264DecodeAppSrc.pushBuffer (gstBuffer);
		}
	}
	else if (estiVIDEO_H263 == m_eCodec)
	{
		if (m_gstElementH263DecodeAppSrc.get () != nullptr)
		{
			return m_gstElementH263DecodeAppSrc.pushBuffer (gstBuffer);
		}
	}

	return GST_FLOW_ERROR;
}


void CstiVideoOutput::EventGstreamerVideoFrameDiscard (
	IstiVideoPlaybackFrame *pVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//stiDEBUG_TOOL (g_stiVideoOutputDebug,
	//	vpe::trace ("CstiVideoOutput::EventGstreamerVideoFrameDiscard\n");
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
	hResult = VideoPlaybackSizeCheck ();
	stiTESTRESULT ();

STI_BAIL:

	return;
}


/*!\brief Enables or disables the display sink's video based on privacy, hold and remote view visiblity.
 *
 */
stiHResult CstiVideoOutput::VideoSinkVisibilityUpdate (
	bool bClearReferenceFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_decodeDisplaySink.get ())
	{
		gboolean bVideoDisabled = FALSE;

		g_object_get (G_OBJECT (m_decodeDisplaySink.get ()), "disablevideo", &bVideoDisabled, NULL);

		if (m_bPrivacy || m_bHolding || !m_DisplaySettings.stRemoteView.bVisible)
		{
			if (!bVideoDisabled)
			{
				m_decodeDisplaySink.propertySet ("disablevideo", true);

				//
				// Clear the reference frame in the decoder by pushing a black key frame
				// into the pipeline.
				//
				if (bClearReferenceFrame)
				{
					hResult = DecodePipelineBlackFramePush ();
				}
			}
		}
		else
		{
			if (bVideoDisabled)
			{

				m_decodeDisplaySink.propertySet ("disablevideo", false);
			}
		}
	}

	return hResult;
}


stiHResult CstiVideoOutput::videoFrameSizeSet(
	GstElement *decoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GStreamerPad srcPad;
	gint nDecodeWidth = 0;
	gint nDecodeHeight = 0;

	stiTESTCOND(decoder != nullptr, stiRESULT_ERROR);

	srcPad = GStreamerPad(gst_element_get_static_pad (decoder, "src"), false);
	stiTESTCOND(srcPad.get() != nullptr, stiRESULT_ERROR);

	{
		GStreamerCaps gstCaps(gst_pad_get_negotiated_caps (srcPad.get()), false);
		if (gstCaps.get () != nullptr)
		{
			/*
			* according to the documentation:
			* "You do not need to free or unref the structure returned, it belongs to the GstCaps."
			*/
			auto gstStruct = gst_caps_get_structure (gstCaps.get (), 0);
			stiTESTCOND(gstStruct != nullptr, stiRESULT_ERROR);

			gst_structure_get_int (gstStruct, "width", &nDecodeWidth);
			gst_structure_get_int (gstStruct, "height", &nDecodeHeight);

			if (nDecodeWidth != (gint)m_un32DecodeWidth || nDecodeHeight != (gint)m_un32DecodeHeight)
			{
				m_un32DecodeWidth = nDecodeWidth;
				m_un32DecodeHeight = nDecodeHeight;
				decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);
			}
		}
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiVideoOutput::VideoPlaybackSizeCheck ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pGstElementH264DecodePipeline)
	{
		hResult = videoFrameSizeSet(m_pGstElementH264Decoder);
		stiTESTRESULT();
	}

STI_BAIL:

	return (hResult);
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiVideoOutput::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
#ifndef stiINTEROP_EVENT
	pCodecs->push_back (estiVIDEO_H264);
	pCodecs->push_back (estiVIDEO_H263);
#else
	if (g_stiEnableVideoCodecs & 2)
	{
		pCodecs->push_back (estiVIDEO_H264);
	}

	if (g_stiEnableVideoCodecs & 1)
	{
		pCodecs->push_back (estiVIDEO_H263);
	}
#endif

	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiVideoOutput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)	// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H264:
		{
			auto pstH264Caps = (SstiH264Capabilities*)pstCaps;

#ifndef stiINTEROP_EVENT
#if 0 // Disable High and Main for now. See bug 22551.
			stProfileAndConstraint.eProfile = estiH264_HIGH;
			stProfileAndConstraint.un8Constraints = 0x0c;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
#endif

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			if (m_eDisplayMode == eDISPLAY_MODE_HDMI_1280_BY_720)
			{
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

			if (m_eDisplayMode == eDISPLAY_MODE_HDMI_1280_BY_720)
			{
				pstH264Caps->eLevel = estiH264_LEVEL_3_1;
			}
			else
			{
				pstH264Caps->eLevel = estiH264_LEVEL_4_1;
			}
#endif

			break;
		}
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		default:
			stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


#define DISPLAY_MULTIPLE 2

stiHResult CstiVideoOutput::DecodeDisplaySinkMove ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DecodeDisplaySinkMove\n");
	);

	m_DisplaySettings.stRemoteView.unWidth -= (m_DisplaySettings.stRemoteView.unWidth % DISPLAY_MULTIPLE);
	m_DisplaySettings.stRemoteView.unHeight -= (m_DisplaySettings.stRemoteView.unHeight % DISPLAY_MULTIPLE);

	if (m_decodeDisplaySink.get ())
	{
		if (m_DisplaySettings.stRemoteView.bVisible)
		{
			char win[50];
			sprintf(win,"%dx%d-%dx%d", m_DisplaySettings.stRemoteView.unPosX,
					m_DisplaySettings.stRemoteView.unPosY,
					m_DisplaySettings.stRemoteView.unWidth,
					m_DisplaySettings.stRemoteView.unHeight);

			// I dont know why but these settings must be set before the disablevideo setting will take effect

			m_decodeDisplaySink.propertySet("window", win);
		}

		VideoSinkVisibilityUpdate (false);
	}

//STI_BAIL:

	return (hResult);
}


/*
 * Brief - return the bitmask of the capapabilites of the last connected hdmi device
 */
stiHResult CstiVideoOutput::DisplayModeCapabilitiesGet (
	uint32_t *pun32CapBitMask)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayModeCapabilitiesGet\n");
	);

	stiTESTCOND (m_pMonitorTask, stiRESULT_ERROR);

	hResult = m_pMonitorTask->HdmiOutCapabilitiesGet (nullptr, pun32CapBitMask, nullptr, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}


/*
 * Brief - return the bitmask of the capapabilites of the last connected hdmi device
 */
stiHResult CstiVideoOutput::DisplayModeSet (
	EDisplayModes eDisplayMode)
{
	stiDEBUG_TOOL (g_stiDisplayDebug,
		stiTrace ("CstiVideoOutput::DisplayModeSet: eDisplayMode = %x\n", eDisplayMode);
	);

	PostEvent (
		std::bind (&CstiVideoOutput::EventDisplayModeSet, this, eDisplayMode));

	return stiRESULT_SUCCESS;
}


void CstiVideoOutput::EventDisplayModeSet (
	EDisplayModes eNewDisplayMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::EventDisplayModeSet\n");
	);

	stiTESTCOND (m_pMonitorTask, stiRESULT_ERROR);
	stiTESTCOND (m_pVideoInput, stiRESULT_ERROR);

	if (m_eDisplayMode != eNewDisplayMode)
	{
		stiDEBUG_TOOL(g_stiDisplayDebug,
			vpe::trace ("CstiVideoOutput::EventDisplayModeSet: m_eDisplayMode = ", m_eDisplayMode, " eNewDisplayMode = ", eNewDisplayMode, "\n");
		);

		hResult = m_pMonitorTask->HdmiOutDisplayRateGet (eNewDisplayMode, &m_fDisplayRate);
		stiTESTRESULT ();

		hResult = m_pMonitorTask->HdmiOutDisplayModeSet (eNewDisplayMode, m_fDisplayRate);
		stiTESTRESULT ();

		m_eDisplayMode = eNewDisplayMode;

		hResult = VideoPlaybackReset ();
		stiTESTRESULT ();

		hResult = m_pVideoInput->OutputModeSet (eNewDisplayMode);
		stiTESTRESULT ();

		// We need a keyframe because we have reset the decoder
		keyframeNeededSignal.Emit ();
	}

	{
		std::string mode {"Unknown"};

		switch (m_eDisplayMode)
		{
		case eDISPLAY_MODE_UNKNOWN:
			mode = "Unknown";
			break;
		case eDISPLAY_MODE_COMPOSITE_720_BY_480:
			mode = "720x480";
			break;
		case eDISPLAY_MODE_HDMI_640_BY_480:
			mode = "640x480";
			break;
		case eDISPLAY_MODE_HDMI_720_BY_480:
			mode = "720x480";
			break;
		case eDISPLAY_MODE_HDMI_1280_BY_720:
			mode = "1280x720";
			break;
		case eDISPLAY_MODE_HDMI_1920_BY_1080:
			mode = "1920x1080";
			break;
		}

		stiRemoteLogEventSend ("EventType=HDMIDisplayMode DisplayMode=%s", mode.c_str());
	}

STI_BAIL:

	return;
}


stiHResult CstiVideoOutput::BestDisplayModeSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	EDisplayModes eNewDisplayMode = eDISPLAY_MODE_UNKNOWN;
	float fNewRate = 0.0;

	uint32_t un32HdmiOutCapabilitiesBitMask = 0;
	float f1080pRate = 0.0;
	float f720pRate = 0.0;

	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::BestDisplayModeSet\n");
	);

	stiTESTCOND (m_pMonitorTask, stiRESULT_ERROR);
	stiTESTCOND (m_pVideoInput, stiRESULT_ERROR);

	hResult = m_pMonitorTask->HdmiOutCapabilitiesGet (nullptr, &un32HdmiOutCapabilitiesBitMask, &f1080pRate, &f720pRate);
	stiTESTRESULT ();

	// Use 1080p if available
	// if not use 720p
	if (un32HdmiOutCapabilitiesBitMask & eDISPLAY_MODE_HDMI_1920_BY_1080)
	{
		eNewDisplayMode = eDISPLAY_MODE_HDMI_1920_BY_1080;
		fNewRate = f1080pRate;
	}
	else if (un32HdmiOutCapabilitiesBitMask & eDISPLAY_MODE_HDMI_1280_BY_720)
	{
		eNewDisplayMode = eDISPLAY_MODE_HDMI_1280_BY_720;
		fNewRate = f720pRate;
	}
	else
	{
		stiDEBUG_TOOL(g_stiDisplayDebug,
			vpe::trace ("CstiVideoOutput::BestDisplayModeSet: un32HdmiOutCapabilitiesBitMask does not support 1080p or 720p, probably off or disconnected\n");
		);
	}

	stiDEBUG_TOOL(g_stiDisplayDebug,
		stiTrace ("CstiVideoOutput::BestDisplayModeSet: un32HdmiOutCapabilitiesBitMask = %x \n", un32HdmiOutCapabilitiesBitMask);
		stiTrace ("CstiVideoOutput::BestDisplayModeSet: m_eDisplayMode = %d \n", m_eDisplayMode);
		stiTrace ("CstiVideoOutput::BestDisplayModeSet: m_fDisplayRate = %0.1f \n", m_fDisplayRate);
		stiTrace ("CstiVideoOutput::BestDisplayModeSet: eNewDisplayMode = %d \n", eNewDisplayMode);
		stiTrace ("CstiVideoOutput::BestDisplayModeSet: fNewRate = %0.1f \n", fNewRate);
	);

	if (eNewDisplayMode != eDISPLAY_MODE_UNKNOWN
		&&  (m_eDisplayMode != eNewDisplayMode
			|| m_fDisplayRate != fNewRate))
	{
		stiDEBUG_TOOL(g_stiDisplayDebug,
			stiTrace ("CstiVideoOutput::BestDisplayModeSet: calling HdmiOutDisplayModeSet with mode = %d and rate = %0.1f\n", eNewDisplayMode, fNewRate);
		);

		hResult = m_pMonitorTask->HdmiOutDisplayModeSet (eNewDisplayMode, fNewRate);
		stiTESTRESULT ();

		hResult = m_pVideoInput->OutputModeSet (eNewDisplayMode);
		stiTESTRESULT ();

		m_eDisplayMode = eNewDisplayMode;
		m_fDisplayRate = fNewRate;
	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoOutput::DisplayPowerSet (
	bool bOn)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayPowerSet, current = ", torf(m_cecControl->tvPowered()), "setting to ", torf(bOn), "\n");
	);
	if (m_cecControl->cecSupported())
	{
		stiDEBUG_TOOL(g_stiDisplayDebug,
			vpe::trace ("CstiVideoOutput::DisplayPowerSet: CEC -is- supported\n");
		);

		if (bOn && !m_cecControl->tvPowered())
		{
			stiDEBUG_TOOL(g_stiDisplayDebug,
				vpe::trace ("CstiVideoOutput::DisplayPowerSet: powering on television\n");
			);

			m_cecControl->displayPowerSet(bOn);		// turn on the tv
			m_cecControl->activeSet();				// set us to the active source
		}
		else if (!bOn && m_cecControl->tvPowered())
		{
			stiDEBUG_TOOL(g_stiDisplayDebug,
				vpe::trace ("CstiVideoOutput::DisplayPowerSet: powering off television\n");
			);
			// this assumes that turning it off just works......
			m_cecControl->displayPowerSet(bOn);
		}
	}

//STI_BAIL:
	return (hResult);
}


bool CstiVideoOutput::DisplayPowerGet ()
{
	bool bResult = false;

	bResult = m_cecControl->tvPowered();

	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayPowerGet returns ", torf(bResult), "\n");
	);

	return bResult;
}

bool CstiVideoOutput::CECSupportedGet ()
{
	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::CECSupportedGet: m_bCECSupported = ", torf(m_cecControl->cecSupported()), "\n");
	);

	return (m_cecControl->cecSupported());
}

std::string CstiVideoOutput::tvVendorGet()
{
	return(m_cecControl->tvVendorGet());
}

/*!
 * \brief VideoFilePlay: Copies pFilePathName and sends it with an event to start the video playing
 *
 * \param filePathName Name and Path to the video file to be played.
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutput::VideoFilePlay(
	const std::string &filePathName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	boost::system::error_code errorCode;

	stiTESTCOND(!filePathName.empty() &&
				boost::filesystem::is_regular_file(filePathName.c_str(), errorCode), stiRESULT_ERROR);

	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoFilePlay, this, filePathName));

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
void CstiVideoOutput::EventVideoFilePlay (
	const std::string filePathName)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;
	GstState pipeLineState;
	GstElement *decoder = nullptr;
	gboolean gstReturn = true;
	GError *pError = nullptr;

	if (!m_bPlaying &&
		!m_pVideoFilePipeline)
	{
		std::string videoFileCropRect = "0x0-" + std::to_string(VIDEO_FILE_PAN_PORTAL_WIDTH) + "x" + std::to_string(VIDEO_FILE_PAN_PORTAL_HEIGHT);

		std::string PipelineCommand;
		PipelineCommand = "filesrc location=" + filePathName + " ! qtdemux name=demux demux.video_00 ! h264parse name=h264parse ! nv_omx_h264dec name=h264_decode_element ! nvvidconv name=vidconv croprect=" + videoFileCropRect + " output-buffers=2 ! nv_omx_videosink name=videoSink overlay=2 hdmi=TRUE sync=TRUE";

		m_pVideoFilePipeline = gst_parse_launch(PipelineCommand.c_str (), &pError);
		stiTESTCOND(m_pVideoFilePipeline, stiRESULT_ERROR);

		GstBus *pVideoFilePipelineBus;
		pVideoFilePipelineBus = gst_pipeline_get_bus(GST_PIPELINE(m_pVideoFilePipeline));
		gst_bus_add_watch(pVideoFilePipelineBus, CstiVideoOutput::VideoFileBusCallback, this);
		gst_object_unref(pVideoFilePipelineBus);
		pVideoFilePipelineBus = nullptr;

		m_pVideoFileConv = gst_bin_get_by_name((GstBin *)m_pVideoFilePipeline, "vidconv");
		stiTESTCOND(m_pVideoFileConv, stiRESULT_ERROR);

		GstState state;
		StateChangeReturn = gst_element_get_state (m_pVideoFilePipeline, &state, nullptr, GST_CLOCK_TIME_NONE);
//		stiTrace ("state: %s ret: %s\n", gst_element_state_get_name(state), gst_element_state_change_return_get_name(StateChangeReturn));

		StateChangeReturn = gst_element_set_state (m_pVideoFilePipeline, GST_STATE_PAUSED);
		stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

		StateChangeReturn = gst_element_get_state (m_pVideoFilePipeline, &pipeLineState, nullptr, GST_CLOCK_TIME_NONE);
		stiTESTCOND(pipeLineState == GST_STATE_PAUSED, stiRESULT_ERROR);

		gstReturn = gst_element_seek_simple(m_pVideoFilePipeline, GST_FORMAT_TIME,
											(GstSeekFlags)(GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH), (gint64)0);
		stiTESTCOND(gstReturn != FALSE, stiRESULT_ERROR);

		// Update the frame size.
		decoder = gst_bin_get_by_name((GstBin *)m_pVideoFilePipeline, "h264_decode_element");
		stiTESTCOND(decoder != nullptr, stiRESULT_ERROR);

		hResult = videoFrameSizeSet(decoder);
		stiTESTRESULT();

		gst_object_unref(decoder);
		decoder = nullptr;

		StateChangeReturn = gst_element_set_state (m_pVideoFilePipeline, GST_STATE_PLAYING);
		stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

		// Initialize the pan variables.
		m_nVideoFilePortalX = 0;
		m_nVideoFilePortalY = 0;
		m_eVideoFilePortalDirection = eMOVE_RIGHT;

		m_videoFilePanTimer.Start ();
	}

STI_BAIL:
	if (stiIS_ERROR(hResult))
	{
		// If we created a pipeline then clean it up.
		if (m_pVideoFilePipeline)
		{
			VideoFilePipelineStop ();
		}
		videoScreensaverStartFailedSignal.Emit ();
	}
}

/*!
 * \brief VideoFileStop: Sends an event to stop playing the video.
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutput::VideoFileStop ()
{
	PostEvent (
		std::bind (&CstiVideoOutput::EventVideoFileStop, this));

	return stiRESULT_SUCCESS;
}

/*!
 * \brief EventVideoFileStop: Causes the video to stop playing and the G-Streamer pipeline to be cleaned up.
 *
 * \param poEvent
 *
 * \return Success or failure result
 */
void CstiVideoOutput::EventVideoFileStop ()
{
	VideoFilePipelineStop ();
}

/*!
 * \brief VideoFilePipelineStop: Stops the video from playing and cleans up the G-Streamer pipeline.
 *
 * \return Success or failure result
 */
stiHResult CstiVideoOutput::VideoFilePipelineStop()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;
	GstState state = GST_STATE_VOID_PENDING;
	GstState pending = GST_STATE_VOID_PENDING;

	if (m_pVideoFilePipeline)
	{
		gst_element_get_state(m_pVideoFilePipeline, &state, &pending, 0);
		if (state == GST_STATE_PLAYING ||
			state == GST_STATE_PAUSED ||
			state == GST_STATE_READY)
		{
			// Set to NULL state before removing
			StateChangeReturn = gst_element_set_state (m_pVideoFilePipeline, GST_STATE_NULL);
			stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

			videoFilePipelineCleanup();
		}
		else if (state == GST_STATE_NULL)
		{
			videoFilePipelineCleanup();
		}
		else
		{
			m_bVideoFileWaitingForShutdown = true;
		}
	}

STI_BAIL:

	return hResult;
}


void CstiVideoOutput::videoFilePipelineCleanup()
{
	// Cleanup the pipeline.
	gst_object_unref (m_pVideoFilePipeline);
	m_pVideoFilePipeline = nullptr;
	gst_object_unref (m_pVideoFileConv);
	m_pVideoFileConv = nullptr;
	m_bVideoFileWaitingForShutdown = false;

	m_un32DecodeWidth = DEFAULT_DECODE_WIDTH;
	m_un32DecodeHeight = DEFAULT_DECODE_HEIGHT;

	decodeSizeChangedSignal.Emit (m_un32DecodeWidth, m_un32DecodeHeight);

	// Signal that the file is closed.
	std::string emptyStr;
	videoFileClosedSignalGet().Emit(emptyStr);
}

/*!
 * \brief VideoFileBusCallback: Call back for VideoFile bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  gpointer - The this pointer
 * \return gboolean - Always returns true
 */
gboolean CstiVideoOutput::VideoFileBusCallback (
	GstBus *pPipelineBus,
	GstMessage *pMsg,
	gpointer data)
{
	auto pThis = (CstiVideoOutput *)data;

	// Lock to protect multiple threads accessing the same variables.
	pThis->Lock ();

	if (GST_MESSAGE_TYPE(pMsg) == GST_MESSAGE_EOS)
	{
		stiASSERTMSG (estiFALSE, "GST_MESSAGE_EOS\n");
	}
	else if (GST_MESSAGE_TYPE(pMsg) == GST_MESSAGE_SEGMENT_DONE)
	{
		if (!gst_element_seek(pThis->m_pVideoFilePipeline,
							  1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_SEGMENT,
							  GST_SEEK_TYPE_SET, 0,
							  GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
		{
			stiASSERTMSG (estiFALSE, "GST_MESSAGE_SEGMENT_DONE seek to frame 0 ERROR ON SEEK!\n");
		}
	}
	else if (GST_MESSAGE_TYPE(pMsg) == GST_MESSAGE_STATE_CHANGED &&
			 pThis->m_bVideoFileWaitingForShutdown)
	{
		pThis->PostEvent (
			std::bind (&CstiVideoOutput::EventVideoFileStop, pThis));
	}

	// Make sure that the mutex is unlocked.
	pThis->Unlock ();

	return true;
}


/*!
 * \brief panCropRectUpdte update the cropRect as a result of the panTimerTimeout.
 */
void CstiVideoOutput::panCropRectUpdate()
{
	// If we have the conversion filter and are not shutting down the update
	// corprect and apply it to the conversion filter.
	if (m_pVideoFileConv &&
		!m_bVideoFileWaitingForShutdown)
	{
		// Set the new croprect.
		std::string videoFileCropRect = std::to_string(m_nVideoFilePortalX) + "x" + std::to_string(m_nVideoFilePortalY) + "-" + std::to_string(VIDEO_FILE_PAN_PORTAL_WIDTH) + "x" + std::to_string(VIDEO_FILE_PAN_PORTAL_HEIGHT);
		g_object_set (G_OBJECT (m_pVideoFileConv), "croprect", videoFileCropRect.c_str(), NULL);
	}
}


/*!
 * \brief EventBufferReturnErrorTimerTimeout: Fires when a buffer has been
 * handed downstream and not returned after BUFFER_RETURN_TIMEOUT
 */
void CstiVideoOutput::EventBufferReturnErrorTimerTimeout ()
{
	stiLOG_ENTRY_NAME (CstiVideoOutput::EventBufferReturnErrorTimerTimeout);
	
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
void CstiVideoOutput::PreferredDisplaySizeGet (
	unsigned int *displayWidth,
	unsigned int *displayHeight) const
{
	*displayWidth = 0;
	*displayHeight = 0;
}


void CstiVideoOutput::applySPSFixups (uint8_t *spsNalUnit)
{
	// As we loop through all NAL units change the level to level 1.0
	// This is a hack to fix the video playback of iOs streams because the
	// decoder will store frames based (add latency) on dpb size based on level
	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		stiTrace ("CstiVideoOutputBase::eventVideoPlaybackFramePut: For H.264 in SPS NAL we are changing level from %0x to %0x\n", spsNalUnit[3], 10);
	);

	spsNalUnit[3] = 10; //  Changes level to 1.0.
}

void CstiVideoOutput::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	if (pDisplaySettings)
	{
		Lock ();

		if (pDisplaySettings->un32Mask & (IstiVideoOutput::eDS_SELFVIEW_SIZE | IstiVideoOutput::eDS_SELFVIEW_POSITION))
		{
			if (m_pVideoInput)
			{
				m_pVideoInput->DisplaySettingsGet(pDisplaySettings);
			}
		}
		else if (pDisplaySettings->un32Mask & (IstiVideoOutput::eDS_REMOTEVIEW_SIZE | IstiVideoOutput::eDS_REMOTEVIEW_POSITION))
		{
			auto adjustedX = static_cast<int>(m_DisplaySettings.stRemoteView.unPosX);
			auto adjustedY = static_cast<int>(m_DisplaySettings.stRemoteView.unPosY);
			auto adjustedWidth = static_cast<int>(m_DisplaySettings.stRemoteView.unWidth);
			auto adjustedHeight = static_cast<int>(m_DisplaySettings.stRemoteView.unHeight);

			auto videoAspectRatio = static_cast<float>(m_un32DecodeWidth) / m_un32DecodeHeight;
			auto maxAspectRatio = static_cast<float>(adjustedWidth) / adjustedHeight;

			if (maxAspectRatio > videoAspectRatio)
			{
				adjustedWidth = lroundf(videoAspectRatio * adjustedHeight);
			}
			else if (maxAspectRatio < videoAspectRatio)
			{
				adjustedHeight = lroundf(adjustedWidth / videoAspectRatio);
			}

			if (adjustedWidth == static_cast<int>(m_DisplaySettings.stRemoteView.unWidth))
			{
				adjustedY += (m_DisplaySettings.stRemoteView.unHeight - adjustedHeight) / 2;
			}
			else
			{
				adjustedX += (m_DisplaySettings.stRemoteView.unWidth - adjustedWidth) / 2;
			}

			pDisplaySettings->stRemoteView.unPosX = adjustedX;
			pDisplaySettings->stRemoteView.unPosY = adjustedY;
			pDisplaySettings->stRemoteView.unWidth = adjustedWidth;
			pDisplaySettings->stRemoteView.unHeight = adjustedHeight;
		}

		Unlock ();
	}
}

stiHResult CstiVideoOutput::DisplaySettingsSet (
	const SstiDisplaySettings *pDisplaySettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutputBase::DisplaySettingsSet\n");
	);
	
	auto spNewDisplaySettings = std::make_shared<SstiDisplaySettings>(*pDisplaySettings);

	stiTESTCOND (pDisplaySettings != nullptr, stiRESULT_ERROR);
	
	PostEvent (
		std::bind (&CstiVideoOutput::EventDisplaySettingsSet, this, spNewDisplaySettings));

STI_BAIL:

	return hResult;
}

void CstiVideoOutput::EventDisplaySettingsSet (
	std::shared_ptr<SstiDisplaySettings> displaySettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet\n");
	);

	bool bSelfViewImageSettingsValid = false;
	bool bRemoteViewImageSettingsValid = false;
	SstiImageSettings PreValidateSelfViewImageSettings{};
	SstiImageSettings PreValidateRemoteViewImageSettings{};

	stiTESTCOND (displaySettings != nullptr, stiRESULT_ERROR);

	PreValidateSelfViewImageSettings = displaySettings->stSelfView;

	if (displaySettings->un32Mask & (eDS_SELFVIEW_VISIBILITY | eDS_SELFVIEW_SIZE | eDS_SELFVIEW_POSITION))
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: SelfView:\n");
			OutputImageSettingsPrint (&displaySettings->stSelfView);
		);

		bSelfViewImageSettingsValid = ImageSettingsValidate (&displaySettings->stSelfView);

		if (bSelfViewImageSettingsValid)
		{
			if (!ImageSettingsEqual (&m_DisplaySettings.stSelfView, &displaySettings->stSelfView))
			{
				m_DisplaySettings.stSelfView = displaySettings->stSelfView;

				if (m_pVideoInput)
				{
					stiDEBUG_TOOL (g_stiVideoOutputDebug,
						vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Calling m_pVideoInput->DisplaySettingsSet\n");
						OutputImageSettingsPrint (&m_DisplaySettings.stSelfView);
					);

					hResult = m_pVideoInput->DisplaySettingsSet (&m_DisplaySettings.stSelfView);
					stiTESTRESULT ();
				}

				if (m_DisplaySettings.stSelfView.bVisible
					&& m_DisplaySettings.stSelfView.unWidth >= maxDisplayWidthGet ()
					&& m_DisplaySettings.stSelfView.unHeight >= 1072)
				{
					stiDEBUG_TOOL (g_stiVideoOutputDebug,
						vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: FULLSCREEN self view\n");
					);

					if (!m_bHidingRemoteView)
					{
						//The self view is full screen in 1080p mode
						//We can make the remote view not visible
						//because it is always behind the self view
						if (m_bPlaying)
						{
							if (m_DisplaySettings.stRemoteView.bVisible)
							{
								stiDEBUG_TOOL (g_stiVideoOutputDebug,
									vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Hiding remote view and calling DecodeDisplaySinkMove\n");
								);

								m_DisplaySettings.stRemoteView.bVisible = false;

								hResult = DecodeDisplaySinkMove ();
								stiTESTRESULT ();

								m_bHidingRemoteView = true;
							}
						}
					}
				}
				else
				{
					if (m_bHidingRemoteView)
					{
						if (m_bPlaying)
						{
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Un-hiding remote view and calling DecodeDisplaySinkMove\n");
								OutputImageSettingsPrint (&m_DisplaySettings.stRemoteView);
							);

							m_DisplaySettings.stRemoteView.bVisible = true;

							hResult = DecodeDisplaySinkMove ();
							stiTESTRESULT ();
						}

						m_bHidingRemoteView = false;
					}
				}
			}
			else
			{
				// Notify the UI that a size/pos change request was made.
				m_pVideoInput->NotifyUISizePosChanged();
			}
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Self View Settings: "
					"Mask = %x, bVisible = %d (%d, %d, %d, %d)\n",
					displaySettings->un32Mask,
					displaySettings->stSelfView.bVisible,
					displaySettings->stSelfView.unPosX,
					displaySettings->stSelfView.unPosY,
					displaySettings->stSelfView.unWidth,
					displaySettings->stSelfView.unHeight);
		}
	}

	PreValidateRemoteViewImageSettings = displaySettings->stRemoteView;

	if (displaySettings->un32Mask & (eDS_REMOTEVIEW_VISIBILITY | eDS_REMOTEVIEW_SIZE | eDS_REMOTEVIEW_POSITION))
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: RemoteView:\n");
			OutputImageSettingsPrint (&displaySettings->stRemoteView);
		);

		bRemoteViewImageSettingsValid = ImageSettingsValidate (&displaySettings->stRemoteView);

		if (bRemoteViewImageSettingsValid)
		{
			if (!ImageSettingsEqual (&m_DisplaySettings.stRemoteView, &displaySettings->stRemoteView))
			{
				m_DisplaySettings.stRemoteView = displaySettings->stRemoteView;

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Calling DecodeDisplaySinkMove\n");
				);

				hResult = DecodeDisplaySinkMove ();
				stiTESTRESULT ();
			}

			// Notify the UI that a size/pos change request was made.
			videoRemoteSizePosChangedSignal.Emit ();
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Remote View Settings: "
					"Mask = %x, bVisible = %d (%d, %d, %d, %d)\n",
					displaySettings->un32Mask,
					displaySettings->stRemoteView.bVisible,
					displaySettings->stRemoteView.unPosX,
					displaySettings->stRemoteView.unPosY,
					displaySettings->stRemoteView.unWidth,
					displaySettings->stRemoteView.unHeight);
		}
	}

STI_BAIL:;
}

bool CstiVideoOutput::ImageSettingsValidate (
	const IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	bool bValid = true;

	if (pImageSettings)
	{
		if (pImageSettings->bVisible)
		{
			if ((int)pImageSettings->unPosX < 0)
			{
				bValid = false;
			}

			if (pImageSettings->unPosX > maxDisplayWidthGet ())
			{
				bValid = false;
			}

			if (pImageSettings->unPosY < 0)
			{
				bValid = false;
			}

			if (pImageSettings->unPosY > maxDisplayHeightGet ())
			{
				bValid = false;
			}

			if (pImageSettings->unWidth < minDisplayWidthGet ())
			{
				bValid = false;
			}

			if (pImageSettings->unWidth > maxDisplayWidthGet ())
			{
				bValid = false;
			}

			if (pImageSettings->unHeight < minDisplayHeightGet ())
			{
				bValid = false;
			}

			if (pImageSettings->unHeight > maxDisplayHeightGet ())
			{
				bValid = false;
			}
		}
	}
	else
	{
		bValid = false;
	}

	return (bValid);
}

/*
 * CstiVideoOutput::DisplayResolutionSet method is not used by VP2 application,
 * this method is simply being overridden for compilation.
 */
stiHResult CstiVideoOutput::DisplayResolutionSet (
		unsigned int width,
		unsigned int height)
{	
	return stiRESULT_SUCCESS;
}
