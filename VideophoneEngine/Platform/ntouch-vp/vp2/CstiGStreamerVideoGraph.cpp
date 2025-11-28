// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

//
// Includes
//
#include "CstiGStreamerVideoGraph.h"
#include "GStreamerAppSink.h"
#include "stiTrace.h"
#include "IstiVideoInput.h"
#include "CstiEvent.h"
#include "CstiVideoInput.h"
#include "CstiBSPInterface.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/interfaces/propertyprobe.h>
#include <gst/gstdebugutils.h>
#include <gst/audio/gstbaseaudiosink.h>
#include <math.h>

//
// Constant
//

//
// Typedefs
//
#define ENABLE_VIDEO_INPUT
#define ENABLE_SELF_VIEW_SETTINGS_SET
//#define ENABLE_USE_TEST_SRC
#define USE_RTP_PAY_FOR_H263
#define ENABLE_HARDWARE_263_ENCODER
//#define ENABLE_NEW_DISPLAY_SINK
//#define ENABLE_FAKESINK

#define CROPRECT_BUFFER_SIZE 64
#define OUTPUT_SIZE_BUFFER_SIZE 64
#define DEFAULT_ENCODE_SLICE_SIZE 1300
#define DEFAULT_ENCODE_BIT_RATE 10000000
#define DEFAULT_ENCODE_FRAME_RATE 30
#define DEFAULT_ENCODE_WIDTH 1920
#define DEFAULT_ENCODE_HEIGHT 1072

#define CAPTURE_WIDTH_1080P 1920
#define CAPTURE_HEIGHT_1080P 1072

#define CAPTURE_WIDTH_720P 1280
#define CAPTURE_HEIGHT_720P 720

#define HDMI_AUDIO_INPUT_DEVICE "hw:0,2"
#define HDMI_AUDIO_OUTPUT_DEVICE "hw:0,1"

//
// these two are defined in the kernel in drivers/media/video/tc358743_regs.h
// as MASK_S_V_FORMAT_1080P and MASK_S_V_FORMAT_1080P
// other resolutions are not being supported.
#define HDMI_RESOLUTION_720 12
#define HDMI_RESOLUTION_1080 15

static IstiVideoInput::SstiVideoRecordSettings defaultVideoRecordSettings ()
{
	IstiVideoInput::SstiVideoRecordSettings settings;

	settings.unMaxPacketSize = DEFAULT_ENCODE_SLICE_SIZE;
	settings.unTargetBitRate = DEFAULT_ENCODE_BIT_RATE;
	settings.unTargetFrameRate = DEFAULT_ENCODE_FRAME_RATE;
	settings.unRows = DEFAULT_ENCODE_HEIGHT;
	settings.unColumns = DEFAULT_ENCODE_WIDTH;
	settings.eProfile = estiH264_BASELINE;
	settings.unConstraints = 0xe0;
	settings.unLevel = estiH264_LEVEL_4_1;
	settings.ePacketization = estiH264_SINGLE_NAL;

	return settings;
};


CstiGStreamerVideoGraph::CstiGStreamerVideoGraph (
	bool rcuConnected)
:
	CstiGStreamerVideoGraphBase (rcuConnected),
	m_unFrameCaptureNewBufferSignalHandler (0),

	m_pGstElementHdmiCapturePipeline (nullptr),

	m_pGstElementHdmiVideoSource (nullptr),
	m_pGstElementHdmiInputFilter (nullptr),
	m_pGstElementHdmiVideoQueue (nullptr),
	m_pGstElementHdmiVideoSink (nullptr),

	m_pGstElementHdmiAudioSource (nullptr),
	m_pGstElementHdmiAudioFilter (nullptr),
	m_pGstElementHdmiAudioQueue (nullptr),
	m_pGstElementHdmiAudioSink (nullptr),

	m_nHdmiInResolution (0)
{
	m_VideoRecordSettings = defaultVideoRecordSettings ();

	m_bEncodePipelinePlaying = false;
	m_bHdmiPassthrough = false;
}


CstiGStreamerVideoGraph::~CstiGStreamerVideoGraph ()
{
	EncodePipelineDestroy ();
	
	HdmiPipelineDestroy ();
}

stiHResult CstiGStreamerVideoGraph::PipelineReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bHdmiPassthrough)
	{
		hResult = HdmiPipelineStart ();
		stiTESTRESULT ();
	}
	else
	{
		hResult = EncodePipelineStart ();
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::PipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = EncodePipelineDestroy ();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::EncodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodePipelineCreate begin\n");
	);

	hResult = EncodePipelineDestroy ();
	stiTESTRESULT ();

	hResult = HdmiPipelineDestroy ();
	stiTESTRESULT ();

	//
	// Only create the capture pipeline if the camera is connected.
	//
	if (rcuConnectedGet())
	{
		// Setup the capture pipeline

		stiASSERT (m_encodeCapturePipeline.get () == nullptr);

		m_encodeCapturePipeline.create(
			m_DisplayImageSettings,
			m_bPrivacy,
			m_VideoRecordSettings);
		stiTESTCOND (m_encodeCapturePipeline.get (), stiRESULT_ERROR);

		m_aspectRationChangedConnection = m_encodeCapturePipeline.aspectRatioChanged.Connect (
			[this]
			{
				aspectRatioChanged.Emit ();
			});

		m_encodeBitstreamReceivedConnection = m_encodeCapturePipeline.encodeBitstreamReceivedSignal.Connect (
			[this](GStreamerSample &gstSample)
			{
				encodeBitstreamReceivedSignal.Emit (gstSample);
			});

		hResult = PTZVideoConvertUpdate ();
		stiTESTRESULT ();

		if (m_frameCaptureBin.get())
		{
			m_encodeCapturePipeline.elementAdd(*m_frameCaptureBin);
		}
	}

STI_BAIL:

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodePipelineCreate end\n");
	);

	return (hResult);
}


stiHResult CstiGStreamerVideoGraph::EncodePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodePipelineDestroy: begin\n");
	);

	if (m_encodeCapturePipeline.get())
	{
		GstStateChangeReturn StateChangeReturn;

		StateChangeReturn = m_encodeCapturePipeline.stateSet(GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiGStreamerVideoGraph::EncodePipelineDestroy: Could not gst_element_set_state (m_encodeCapturePipeline, GST_STATE_NULL)\n");
		}
	}

	m_bEncodePipelinePlaying = false;
	
	m_encodeCapturePipeline.clear ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodePipelineDestroy end\n");
	);

STI_BAIL:
	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::EncodePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodePipelineStart\n");
	);
	
#ifdef ENABLE_VIDEO_INPUT
	if (rcuConnectedGet())
	{
		// Reset all record settings to create a reproducible
		// pipeline everytime
		m_VideoRecordSettings = defaultVideoRecordSettings ();
		
		hResult = EncodePipelineCreate ();
		stiTESTRESULT ();

		if (m_encodeCapturePipeline.get ())
		{
			auto stateChangeReturn = m_encodeCapturePipeline.stateSet (GST_STATE_PLAYING);
			stiTESTCONDMSG (stateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR, "CstiGStreamerVideoGraph::EncodePipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementEncodeCapturePipeline, GST_STATE_PLAYING)\n");
		}
	}
	
	m_bEncodePipelinePlaying = true;
	
STI_BAIL:
#else
	m_bEncodePipelinePlaying = true;
#endif
	
	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::HdmiPipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nInWidth = 0;
	int nInHeight = 0;
	
	gboolean bLinked;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::HdmiPipelineCreate: Start\n");
	);
	//
	// start by destroying the self-view pipeline
	//
	hResult = EncodePipelineDestroy ();
	stiTESTRESULT ();
	
	hResult = HdmiPipelineDestroy ();
	stiTESTRESULT ();
	
	
	stiASSERT (m_pGstElementHdmiCapturePipeline == nullptr);
	m_pGstElementHdmiCapturePipeline = gst_pipeline_new ("hdmi_capture_pipeline");
	stiTESTCOND (m_pGstElementHdmiCapturePipeline, stiRESULT_ERROR);

	m_pGstElementHdmiVideoSource = gst_element_factory_make("nv4l2src", "hdmi_srouce_element");
	stiTESTCOND (m_pGstElementHdmiVideoSource, stiRESULT_ERROR);
	g_object_set(G_OBJECT(m_pGstElementHdmiVideoSource),
			"device", "/dev/video1",
			"do-timestamp", TRUE,
			NULL);
 
	if (m_nHdmiInResolution == HDMI_RESOLUTION_1080)
	{
		nInWidth = CAPTURE_WIDTH_1080P;
		nInHeight = CAPTURE_HEIGHT_1080P;
	}
	else if (m_nHdmiInResolution == HDMI_RESOLUTION_720)
	{
		nInWidth = CAPTURE_WIDTH_720P;
		nInHeight = CAPTURE_HEIGHT_720P;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "Unsupported resolution: m_nHdmiInResolution = %d\n", m_nHdmiInResolution);
	}
	
	{
		GStreamerCaps gstCaps ("video/x-nv-yuv",
				"width", G_TYPE_INT, nInWidth,
				"height",G_TYPE_INT, nInHeight,
				"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I', '4', '2', '0'),
				/*"framerate", GST_TYPE_FRACTION, 60, 1,*/
				NULL);

		m_pGstElementHdmiInputFilter = gst_element_factory_make ("capsfilter", "hdmi_passthrough_input_filter_element");
		stiTESTCOND (m_pGstElementHdmiInputFilter, stiRESULT_ERROR);
	
		g_object_set(G_OBJECT(m_pGstElementHdmiInputFilter), "caps", gstCaps.get (), NULL);
	}
	
	//m_pGstElementHdmiVideoConvert = gst_element_factory_make("nvvidconv", "hdmi_passthrough_video_convert_element");
	//stiTESTCOND (m_pGstElementHdmiVideoConvert, stiRESULT_ERROR);
	//g_object_set (G_OBJECT (m_pGstElementHdmiVideoConvert), "output-buffers", 4, NULL);
	
	m_pGstElementHdmiVideoQueue = gst_element_factory_make ("queue", "hdmi_passthrough_video_queue_element");
	stiTESTCOND (m_pGstElementHdmiVideoQueue, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementHdmiVideoQueue), "max-size-buffers", 2, NULL);

	m_pGstElementHdmiVideoSink = gst_element_factory_make ("nv_omx_videosink", "hdmi_passthough_video_sink");
	stiTESTCOND (m_pGstElementHdmiVideoSink, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "overlay", 1, NULL);
	g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "hdmi", TRUE, NULL);
	//g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "sync", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "sync", FALSE, NULL);
	
#if 1
	m_pGstElementHdmiAudioSource = gst_element_factory_make("alsasrc", "hdmi_passthrough_audio_source");
	g_object_set(GST_OBJECT(m_pGstElementHdmiAudioSource), "device", HDMI_AUDIO_INPUT_DEVICE, NULL);
	g_object_set(GST_OBJECT(m_pGstElementHdmiAudioSource), "do-timestamp", TRUE, NULL);
//	Nicholas Dufresne from Collabora said this setting would decrease cpu usage.  if it becomes
//  an issue, look at doing this
//	g_object_set(GST_OBJECT(m_pGstElementHdmiAudioSource), "latency-time", 20000ll, NULL);

	m_pGstElementHdmiAudioQueue = gst_element_factory_make("queue", "hdmi_audio_passthrough_queue");
	stiTESTCONDMSG (m_pGstElementHdmiAudioQueue, stiRESULT_ERROR, "HdmiPipelineCreate: can't make the audio queue\n");

	{
		GStreamerCaps gstCaps ("audio/x-raw-int",
				"rate", G_TYPE_INT, 48000,
				"channels", G_TYPE_INT, 2,
				"width", G_TYPE_INT, 16,
				"depth", G_TYPE_INT, 16,
				"endianness", G_TYPE_INT, 1234,
				NULL);
		stiTESTCONDMSG (gstCaps.get () != nullptr, stiRESULT_ERROR, "HdmiPipelineCreate: can't creat caps\n");
		
		m_pGstElementHdmiAudioFilter = gst_element_factory_make("capsfilter", "hdmi_audio_passthrough_filter");
		stiTESTCONDMSG (m_pGstElementHdmiAudioFilter, stiRESULT_ERROR, "HdmiPipelineCreate: can't creat filter\n");

		g_object_set(G_OBJECT(m_pGstElementHdmiAudioFilter), "caps", gstCaps.get (), NULL);
	}
	
	m_pGstElementHdmiAudioSink = gst_element_factory_make("alsasink", "hdmi_audio_passthrough_sink");
	stiTESTCONDMSG (m_pGstElementHdmiAudioSink, stiRESULT_ERROR, "HdmiPipelineCreate: can't make the audio sink\n");
		
	g_object_set(G_OBJECT(m_pGstElementHdmiAudioSink),"device", HDMI_AUDIO_OUTPUT_DEVICE, NULL);
	g_object_set(G_OBJECT(m_pGstElementHdmiAudioSink), "sync", TRUE, NULL);
	g_object_set(G_OBJECT(m_pGstElementHdmiAudioSink), "slave-method", 0, NULL);
	//g_object_set(G_OBJECT(m_pGstElementHdmiAudioSink), "qos", TRUE, NULL);
		
	gst_bin_add_many (GST_BIN(m_pGstElementHdmiCapturePipeline),
			m_pGstElementHdmiVideoSource,
			m_pGstElementHdmiInputFilter,
			//m_pGstElementHdmiVideoConvert,
			m_pGstElementHdmiVideoQueue,
			m_pGstElementHdmiVideoSink,
			
			m_pGstElementHdmiAudioSource,
			m_pGstElementHdmiAudioQueue,
			m_pGstElementHdmiAudioFilter,
			m_pGstElementHdmiAudioSink,
			NULL);
	
	bLinked = gst_element_link_many (
				m_pGstElementHdmiVideoSource,
				m_pGstElementHdmiInputFilter,
				//m_pGstElementHdmiVideoConvert,
				m_pGstElementHdmiVideoQueue,
				m_pGstElementHdmiVideoSink,
				NULL);
	
	if (!bLinked)
	{
		stiTHROWMSG(stiRESULT_ERROR, "Can't link hdmi video passthrough pipeline");
	}
	
	bLinked = gst_element_link_many(
				m_pGstElementHdmiAudioSource,
				m_pGstElementHdmiAudioQueue,
				m_pGstElementHdmiAudioFilter,
				m_pGstElementHdmiAudioSink,
				NULL);
	
	if (!bLinked)
	{
		stiTHROWMSG(stiRESULT_ERROR, "Can't link hdmi audio passthrough pipeline");
	}
#else
	gst_bin_add_many (GST_BIN(m_pGstElementHdmiCapturePipeline),
			m_pGstElementHdmiVideoSource,
			m_pGstElementHdmiInputFilter,
			m_pGstElementHdmiVideoSink,
			NULL);
	
	bLinked = gst_element_link_many (
			m_pGstElementHdmiVideoSource,
			m_pGstElementHdmiInputFilter,
			m_pGstElementHdmiVideoSink,
			NULL);
	
	if (!bLinked)
	{
		stiTHROWMSG(stiRESULT_ERROR, "Can't link hdmi video passthrough pipeline");
	}
#endif

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(m_pGstElementHdmiCapturePipeline), GST_DEBUG_GRAPH_SHOW_ALL, "HdmiPipelineCreate");
	);
	
STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::HdmiPipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::HdmiPipelineDestroy\n");
	);

	if (m_pGstElementHdmiCapturePipeline)
	{
		GstStateChangeReturn StateChangeReturn;

		StateChangeReturn = gst_element_set_state (m_pGstElementHdmiCapturePipeline, GST_STATE_NULL);

		if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
		{
			stiTHROWMSG (stiRESULT_ERROR, "CstiGStreamerVideoGraph::HdmiPipelineDestroy: Could not gst_element_set_state (m_pGstElementHdmiCapturePipeline, GST_STATE_NULL)\n");
		}
		
		gst_object_unref (m_pGstElementHdmiCapturePipeline);
		
		m_pGstElementHdmiCapturePipeline = nullptr;
		m_pGstElementHdmiVideoSource = nullptr;
		m_pGstElementHdmiInputFilter = nullptr;
		//m_pGstElementHdmiVideoConvert = NULL;
		m_pGstElementHdmiVideoQueue = nullptr;
		m_pGstElementHdmiVideoSink = nullptr;
		
		m_pGstElementHdmiAudioSource = nullptr;
		m_pGstElementHdmiAudioFilter = nullptr;
		m_pGstElementHdmiAudioQueue = nullptr;
		m_pGstElementHdmiAudioSink = nullptr;
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::HdmiPipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::HdmiPipelineStart\n");
	);

	hResult = HdmiPipelineCreate ();
	stiTESTRESULT ();

	GstStateChangeReturn StateChangeReturn;

	stiTESTCOND (m_pGstElementHdmiCapturePipeline, stiRESULT_ERROR);

	StateChangeReturn = gst_element_set_state (m_pGstElementHdmiCapturePipeline, GST_STATE_PLAYING);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiGStreamerVideoGraph::HdmiPipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (m_pGstElementHdmiCapturePipeline, GST_STATE_PLAYING)\n");
	}

STI_BAIL:

	stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
		GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(m_pGstElementHdmiCapturePipeline), GST_DEBUG_GRAPH_SHOW_ALL, "HdmiPipelineStart");
	);

	return (hResult);
}


int32_t CstiGStreamerVideoGraph::MaxPacketSizeGet ()
{
	return m_VideoRecordSettings.unMaxPacketSize;
}


stiHResult CstiGStreamerVideoGraph::EncodeStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::EncodeStart);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraph::EncodeStart\n");
	);

	// Set the cpu speed to high for calls
	hResult = cpuSpeedSet (CstiBSPInterfaceBase::estiCPU_SPEED_HIGH);

#ifdef ENABLE_VIDEO_INPUT

	// TODO: May have to flush the encode side or
	// somehow mark the first frame to be actually
	// processed by the app sink
	if (!m_bVideoCodecSwitched)
	{
		m_encodeCapturePipeline.keyframeEventSend ();
	}
	
	m_encodeCapturePipeline.encodingStart ();
	m_bEncoding = true;

	m_bVideoCodecSwitched = FALSE;
	
#endif

	{
		auto ptzRectangle = ptzRectangleGet();

		// Adjust the PTZ setting for the current encode settings.
		if (m_VideoRecordSettings.unColumns != static_cast<unsigned int>(ptzRectangle.horzZoom) ||
			m_VideoRecordSettings.unRows != static_cast<unsigned int>(ptzRectangle.vertZoom))
		{
			hResult = PTZVideoConvertUpdate ();
			stiTESTRESULT();
		}
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraph::EncodeStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::EncodeStop);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraph::EncodeStop\n");
	);
	
	m_encodeCapturePipeline.encodingStop ();
	m_bEncoding = false;

	hResult = PTZVideoConvertUpdate();

	m_VideoRecordSettings = defaultVideoRecordSettings ();
	
	// Set the cpu speed to medium when not in a call
	hResult = cpuSpeedSet (CstiBSPInterfaceBase::estiCPU_SPEED_MED);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraph::VideoRecordSizeSet (
	uint32_t un32Width,
	uint32_t un32Height)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::VideoRecordSizeSet);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraph::VideoRecordSizeSet: un32Width = %d\n", un32Width);
		stiTrace ("CstiGStreamerVideoGraph::VideoRecordSizeSet: un32Height = %d\n", un32Height);
	);
	
	m_VideoRecordSettings.unColumns = un32Width;
	m_VideoRecordSettings.unRows = un32Height;

	return hResult;
}


stiHResult CstiGStreamerVideoGraph::EncodeSettingsSet (
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings,
	bool *pbEncodeSizeChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::EncodeSettingsSet);
	
#if 0
	// Testing
	//Forcing frame size and bitrate
	videoRecordSettings.unTargetBitRate = 4000000;
	videoRecordSettings.unColumns = 1920;
	videoRecordSettings.unRows = 1088;
#endif
	
	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug || g_stiVideoEncodeTaskDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet encoding = %d\n", m_encodeCapturePipeline.isEncoding ());
		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet Current width is %d\n", m_VideoRecordSettings.unColumns);
		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet Current height is %d\n", m_VideoRecordSettings.unRows);

		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet new width to %d\n", videoRecordSettings.unColumns);
		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet new height to %d\n", videoRecordSettings.unRows);
	);
	
	if (m_VideoRecordSettings.unColumns != videoRecordSettings.unColumns
		|| m_VideoRecordSettings.unRows != videoRecordSettings.unRows)
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug || g_stiVideoEncodeTaskDebug,
			stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet Size did change\n");
			stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet encoding = %d\n", m_encodeCapturePipeline.isEncoding ());
		);
		
		m_VideoRecordSettings = videoRecordSettings;
		
		if (m_encodeCapturePipeline.isEncoding ())
		{
			hResult = PTZVideoConvertUpdate ();
		}

		hResult = m_encodeCapturePipeline.EncodeConvertResize (m_VideoRecordSettings.unColumns, m_VideoRecordSettings.unRows);
		stiTESTRESULT ();

		if (pbEncodeSizeChanged)
		{
			*pbEncodeSizeChanged = true;
		}
	}
	else
	{
		m_VideoRecordSettings = videoRecordSettings;
		
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug || g_stiVideoEncodeTaskDebug,
			stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet Size did not change\n");
			stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet encoding = %d\n", m_encodeCapturePipeline.isEncoding ());
		);
	}
	
#ifdef ENABLE_VIDEO_INPUT
	
	hResult = m_encodeCapturePipeline.H264EncodeSettingsSet (m_VideoRecordSettings);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug || g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::EncodeSettingsSet H264 Setting bitrate to %d\n", m_VideoRecordSettings.unTargetBitRate);
	);

	m_encodeCapturePipeline.bitrateSet (m_VideoRecordSettings.unTargetBitRate);
#endif

STI_BAIL:

	return (hResult);

}

IstiVideoInput::SstiVideoRecordSettings CstiGStreamerVideoGraph::EncodeSettingsGet () const
{
	return m_VideoRecordSettings;
}


#define DISPLAY_MULTIPLE 2

stiHResult CstiGStreamerVideoGraph::DisplaySettingsSet (
	IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Fixup settings to be even
//	pImageSettings->unPosX = pImageSettings->unPosX - (pImageSettings->unPosX % DISPLAY_MULTIPLE);
//	pImageSettings->unPosY = pImageSettings->unPosY - (pImageSettings->unPosY % DISPLAY_MULTIPLE);
	pImageSettings->unWidth = pImageSettings->unWidth - (pImageSettings->unWidth % DISPLAY_MULTIPLE);
	pImageSettings->unHeight = pImageSettings->unHeight - (pImageSettings->unHeight % DISPLAY_MULTIPLE);

	m_DisplayImageSettings = *pImageSettings;

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiVideoSizeDebug,
		stiTrace("CstiGStreamerVideoGraph::DisplaySettingsSet: encoding = %d, m_DisplayImageSettings.bVisible = %d (%d, %d, %d, %d)\n",
				m_encodeCapturePipeline.isEncoding (),
			m_DisplayImageSettings.bVisible,
			m_DisplayImageSettings.unPosX, m_DisplayImageSettings.unPosY, m_DisplayImageSettings.unWidth, m_DisplayImageSettings.unHeight);
	);
	
#ifdef ENABLE_VIDEO_INPUT
#ifdef ENABLE_SELF_VIEW_SETTINGS_SET
	if (m_bHdmiPassthrough)
	{
		stiTESTCOND (m_pGstElementHdmiCapturePipeline, stiRESULT_ERROR);
		stiTESTCOND (m_pGstElementHdmiVideoSink, stiRESULT_ERROR);
		
		char win[50];
		sprintf (win, "%dx%d-%dx%d",
					m_DisplayImageSettings.unPosX, m_DisplayImageSettings.unPosY,
					m_DisplayImageSettings.unWidth, m_DisplayImageSettings.unHeight);
		
		// I dont know why but these settings must be set before the disablevideo setting will take effect
		g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "window", win, NULL);
			
		if (m_DisplayImageSettings.bVisible)
		{
			g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "disablevideo", FALSE, NULL);
		}
		else
		{
			g_object_set (G_OBJECT (m_pGstElementHdmiVideoSink), "disablevideo", TRUE, NULL);
		}
	}
	else
	{
#ifndef ENABLE_FAKESINK
		m_encodeCapturePipeline.displaySettingsSet (m_DisplayImageSettings);
#endif
	}

STI_BAIL:

#endif
#endif

	return (hResult);
}


stiHResult CstiGStreamerVideoGraph::RcuConnectedSet (
	bool bRcuConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::RcuConnectedSet (%s)\n", bRcuConnected == true ? "true" : "false");
	);

	CstiGStreamerVideoGraphBase::rcuConnectedSet(bRcuConnected);
	m_encodeCapturePipeline.rcuConnectedSet(bRcuConnected);

	if (rcuConnectedGet())
	{
		if (!m_bHdmiPassthrough)
		{
			hResult = EncodePipelineStart ();
			stiTESTRESULT ();
		}
	}
	else
	{
		hResult = EncodePipelineDestroy ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
}


bool CstiGStreamerVideoGraph::HdmiPassthroughGet ()
{
	return (m_bHdmiPassthrough);
};


stiHResult CstiGStreamerVideoGraph::HdmiPassthroughSet (
	bool bHdmiPassthrough)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraph::HdmiPassthroughSet(%s)\n", bHdmiPassthrough ? "true" : "false");
	);
	
	if (bHdmiPassthrough != m_bHdmiPassthrough)
	{
		m_bHdmiPassthrough = bHdmiPassthrough;
		
		if (m_bHdmiPassthrough)
		{
			if (m_encodeCapturePipeline.isEncoding ())
			{
				stiTHROWMSG (stiRESULT_ERROR, "CstiGStreamerVideoGraph::HdmiPassthroughSet: Can't go to passthrough when encoding\n");
			}
			
			hResult = HdmiPipelineStart ();
			stiTESTRESULT ();
		}
		else
		{
			hResult = EncodePipelineStart ();
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return (hResult);
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
stiHResult CstiGStreamerVideoGraph::BusMessageProcess (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	gpointer pUserData,
	GstElement *pGstElementPipeline)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiTESTCONDMSG (pGstMessage, stiRESULT_ERROR, "CstiGStreamerVideoGraph::BusMessageProcess: pGstMessage is NULL\n");
	
	switch (GST_MESSAGE_TYPE(pGstMessage))
	{
		case GST_MESSAGE_ELEMENT:
		{
			if (gst_structure_has_name (pGstMessage->structure, "dec_error"))
			{
				const GstStructure *pGstStructure = gst_message_get_structure (pGstMessage);

				gboolean bErrorFrame;
				guint nDecodedMBs;
				guint nConcealedMBs;
				guint nFrameDecodeTime;

				gst_structure_get_boolean (pGstStructure, "bErrorFrame", &bErrorFrame);
				gst_structure_get_uint (pGstStructure, "DecodedMBs", &nDecodedMBs);
				gst_structure_get_uint (pGstStructure, "ConcealedMBs", &nConcealedMBs);
				gst_structure_get_uint (pGstStructure, "FrameDecodeTime", &nFrameDecodeTime);

				stiDEBUG_TOOL (g_stiVideoInputDebug,
					stiTrace("CstiGStreamerVideoGraph::BusMessageProcess: bErrorFrame = %d\n", bErrorFrame);
					stiTrace("CstiGStreamerVideoGraph::BusMessageProcess: nDecodedMBs = %d\n", nDecodedMBs);
					stiTrace("CstiGStreamerVideoGraph::BusMessageProcess: nConcealedMBs = %d\n", nConcealedMBs);
					stiTrace("CstiGStreamerVideoGraph::BusMessageProcess: nFrameDecodeTime = %d\n", nFrameDecodeTime);
				);

			}
		}
		break;

		case GST_MESSAGE_ERROR:
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_error (pGstMessage, &pError, &pDebugInfo);

			std::string errorString ("CstiGStreamerVideoGraph::BusMessageProcess: GST_MESSAGE_ERROR\n");
			
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
			
			stiDEBUG_TOOL (g_stiVideoInputDebug,
						   
				std::string warningString ("CstiGStreamerVideoGraph::BusMessageProcess: GST_MESSAGE_WARNING\n");
			
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
					
				stiTrace (warningString.c_str ());
				
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
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace ("CstiGStreamerVideoGraph::BusMessageProcess: Element %s changed state from %s to %s. Pending: %s\n",
					GST_OBJECT_NAME (pGstMessage->src),
					gst_element_state_get_name (old_state),
					gst_element_state_get_name (new_state),
					gst_element_state_get_name (pending_state));
			);

			if (pGstElementPipeline && pGstMessage->src == GST_OBJECT(pGstElementPipeline))
			{
				char szFileName[256];

				sprintf (szFileName, "ntouchvp2_encode_%s_%s_%d", gst_element_state_get_name (old_state), gst_element_state_get_name (new_state), s_nGraphNumber);

				s_nGraphNumber++;

				stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
					GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN(pGstElementPipeline), GST_DEBUG_GRAPH_SHOW_ALL, szFileName);
				);
			}
			
		}
		break;

		default:
			break;
	}
	
STI_BAIL:
  
	return hResult;
}

stiHResult CstiGStreamerVideoGraph::HdmiInResolutionSet (
	int nResolution)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CStiGstreamerVideoGraph::HdmiInResolutionSet - resolution = %d\n", nResolution);
	);
	
	if (nResolution != m_nHdmiInResolution)
	{
		m_nHdmiInResolution = nResolution;
		
		if (m_bHdmiPassthrough)
		{
			pipelineResetSignal.Emit ();
		}
	}

	return (hResult);
}

stiHResult CstiGStreamerVideoGraph::PipelinePlayingGet (
	bool * pbPipelinePlaying)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiTESTCOND (pbPipelinePlaying, stiRESULT_ERROR);
	
	*pbPipelinePlaying = m_bEncodePipelinePlaying;
	
STI_BAIL:

	return (hResult);
}


std::unique_ptr<FrameCaptureBinBase> CstiGStreamerVideoGraph::frameCaptureBinCreate ()
{
	std::unique_ptr<FrameCaptureBinBase> pipeline = sci::make_unique<FrameCaptureBin>();

	pipeline->create ();

	return pipeline;
}

stiHResult CstiGStreamerVideoGraph::FrameCaptureSet (
	bool frameCapture,
	SstiImageCaptureSize *imageSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	FrameCaptureBin *pipeline = static_cast<FrameCaptureBin *>(m_frameCaptureBin.get());
	pipeline->maxCaptureWidthSet(maxCaptureWidthGet());

	pipeline->frameCaptureJpegEncDropSet(frameCapture);

	hResult = CstiGStreamerVideoGraphBase::FrameCaptureSet (frameCapture,
															imageSize);

	return hResult;
}


void CstiGStreamerVideoGraph::cameraControlFdSet(int cameraControlFd)
{
	m_encodeCapturePipeline.cameraControlFdSet(cameraControlFd);
}
