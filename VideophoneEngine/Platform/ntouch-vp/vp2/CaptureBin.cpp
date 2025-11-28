// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include "CaptureBin.h"
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"
#include "GStreamerCaps.h"
#include "GStreamerPad.h"
#include "GStreamerElementFactory.h"
#include "stiOVUtils.h"
#include <media/ov640_v4l2.h>

#define DEFAULT_ENCODE_FRAME_RATE_NUM 30000
#define DEFAULT_ENCODE_FRAME_RATE_DEN 1001

stiHResult SinkGhostPadCreate (
	GstElement *pGstElementBin,
	GstElement *pGstElementLinkSink);

stiHResult SrcGhostPadCreate (
	GstElement *pGstElementBin,
	GstElement *pGstElement);

CaptureBin::CaptureBin ()
:
	CaptureBinBase (PropertyRange (CaptureBin::MIN_BRIGHTNESS_VALUE,
									CaptureBin::MAX_BRIGHTNESS_VALUE,
									CaptureBin::DEFAULT_BRIGHTNESS_VALUE,
									CaptureBin::DEFAULT_BRIGHTNESS_STEP_SIZE),
					 PropertyRange (CaptureBin::MIN_SATURATION_VALUE,
									CaptureBin::MAX_SATURATION_VALUE,
									CaptureBin::DEFAULT_SATURATION_VALUE,
									CaptureBin::DEFAULT_SATURATION_STEP_SIZE))
{
}


CaptureBin::~CaptureBin ()
{
	PTZBinRemove ();

	//
	// Release and unref the request pads
	//
	if (m_pGstElementEncodeTee)
	{
		if (m_pGstPadTeeSrc0)
		{
			gst_element_release_request_pad (m_pGstElementEncodeTee, m_pGstPadTeeSrc0);
			gst_object_unref (m_pGstPadTeeSrc0);
			m_pGstPadTeeSrc0 = NULL;
		}

		if (m_pGstPadTeeSrc1)
		{
			gst_element_release_request_pad (m_pGstElementEncodeTee, m_pGstPadTeeSrc1);
			gst_object_unref (m_pGstPadTeeSrc1);
			m_pGstPadTeeSrc1 = NULL;
		}

		m_pGstElementEncodeTee = NULL;
	}
}


void CaptureBin::create ()
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;
	GStreamerElement queue0;
	GStreamerElement queue1;
	GStreamerPad selfViewSrcPad;
	GStreamerPad encodeSrcPad;

	clear ();
	GStreamerBin::create ("capture_bin");

#ifdef ENABLE_USE_TEST_SRC
	hResult = TestSrcCaptureCreate ();
	stiTESTRESULT ();
#else
	hResult = CameraCaptureCreate ();
	stiTESTRESULT ();
#endif

	elementAdd(m_captureSource);

	hResult = PTZBinCreate (rcuConnectedGet());
	stiTESTRESULT ();

	hResult = PTZBinAddAndLink ();
	stiTESTRESULT ();

	hResult = TeeWithQueuesCreate ();
	stiTESTRESULT ();

	hResult = TeeWithQueuesAddAndLink ();
	stiTESTRESULT ();

	// Create source pads for the bin
	{
		queue0 = GStreamerElement(m_pGstElementEncodeQueue0, true);
		selfViewSrcPad = queue0.staticPadGet ("src");
		stiTESTCOND(selfViewSrcPad.get(), stiRESULT_ERROR);

		result = ghostPadAdd ("selfviewsrc", selfViewSrcPad);
		stiTESTCOND(result, stiRESULT_ERROR);

		queue1 = GStreamerElement(m_pGstElementEncodeQueue1, true);
		encodeSrcPad = queue1.staticPadGet ("src");
		stiTESTCOND(encodeSrcPad.get(), stiRESULT_ERROR);

		result = ghostPadAdd ("encodesrc", encodeSrcPad);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


stiHResult CaptureBin::cropRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	//
	// Set the ptz rectangle.
	// Note: since we are mirrored we need to flip the X component of the origin to the other side
	//
	if ((maxCaptureWidthGet() - ptzRectangle.horzPos - ptzRectangle.horzZoom) >= 0
		&& (maxCaptureWidthGet() - ptzRectangle.horzPos - ptzRectangle.horzZoom) <= maxCaptureWidthGet()
		&& ptzRectangle.vertPos >= 0
		&& ptzRectangle.vertPos <= maxCaptureWidthGet()
		&& ptzRectangle.horzZoom >= 0
		&& ptzRectangle.horzZoom <= maxCaptureWidthGet()
		&& ptzRectangle.vertZoom >= 0
		&& ptzRectangle.vertZoom <= maxCaptureWidthGet())
	{
		CaptureBinBase::cropRectangleSet(ptzRectangle);

		if (!empty())
		{
			char croprect[64];
			sprintf (croprect, "%dx%d-%dx%d",
					maxCaptureWidthGet() - ptzRectangle.horzPos - ptzRectangle.horzZoom,
					ptzRectangle.vertPos, ptzRectangle.horzZoom, ptzRectangle.vertZoom);

			stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiCapturePTZDebug || g_stiVideoInputDebug,
				stiTrace("CaptureBin::cropRectangleSet: setting croprect = %s\n", croprect);
			);

			if (m_pGstElementPTZVideoConvert)
			{
				g_object_set (G_OBJECT (m_pGstElementPTZVideoConvert), "croprect", croprect, nullptr);
			}
		}
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "CaptureBin::cropRectangleSet() - invalid input parameters");
	}
	
STI_BAIL:
	
	return hResult;
}


stiHResult CaptureBin::brightnessSet (int brightness)
{
	auto hResult = CaptureBinBase::brightnessSet(brightness);
	stiTESTRESULT();

	// The purpose of this extra code is to force the
	// ISP into making AE changes when the target Y is set
	// We turn on manual mode which locks the registers at thier current value
	// then we set the brightness very high and wait  0.1 second
	// which seems to kick the ISP into making an adjustement that we dont see.
	// Then we set the correct brightness then set it back to auto mode.
	ValueSet (0x80140174, 8, 1);

	ValueSet (0x80140146, 8, 128);

	usleep (100000);

	ValueSet (0x80140146, 8, brightness);

	ValueSet (0x80140174, 8, 0);

STI_BAIL:

	return hResult;
}


void CaptureBin::rcuConnectedSet(bool rcuConnected)
{
	m_rcuConnected = rcuConnected;
}


bool CaptureBin::rcuConnectedGet() const
{
	return m_rcuConnected;
}


void CaptureBin::cameraControlFdSet(int cameraControlFd)
{
	m_cameraControlFd = cameraControlFd;
}

stiHResult CaptureBin::saturationSet (int saturation)
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CaptureBin::%s Saturation: %d\n", __func__, saturation);
	);

	auto hResult = CaptureBinBase::saturationSet(saturation);
	stiTESTRESULT();

	if (rcuConnectedGet())
	{
		struct v4l2_control ctrl;

		ctrl.id = V4L2_CID_SATURATION;
		ctrl.value = saturation;

		stiTESTCOND (m_cameraControlFd > -1, stiRESULT_ERROR);

		if (xioctl(m_cameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::saturationSet: Error xoict failed\n");
		}
	}

STI_BAIL:

	return hResult;
}


stiHResult CaptureBin::CameraCaptureCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CaptureBin::CameraCaptureCreate\n");
	);

	// Create the capture source
	m_captureSource = GStreamerElement ("nv4l2src", "capture_source_element");
	stiTESTCOND (m_captureSource.get (), stiRESULT_ERROR);

	m_captureSource.propertySet ("device", g_szCameraDeviceName);
	m_captureSource.propertySet ("queue-size", 5);

STI_BAIL:

	return hResult;
}


stiHResult CaptureBin::PTZBinCreate (
	bool cameraConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GStreamerCaps gstCaps;

	// PTZ bin
	stiASSERT (m_pGstElementPTZBin == nullptr);

	m_pGstElementPTZBin = gst_bin_new ("PTZ_bin");
	stiTESTCOND (m_pGstElementPTZBin, stiRESULT_ERROR);

	// Create the filter before nvvidconv
	if (cameraConnected)
	{
		gstCaps = GStreamerCaps ("video/x-nv-yuv",
					"framerate", GST_TYPE_FRACTION, DEFAULT_ENCODE_FRAME_RATE_NUM, DEFAULT_ENCODE_FRAME_RATE_DEN,
					"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
					"width", G_TYPE_INT, maxCaptureWidthGet(),
					"height", G_TYPE_INT, maxCaptureHeightGet(),
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I', '4', '2', '0'),
					"interlaced", G_TYPE_BOOLEAN, FALSE,
					NULL);
		stiTESTCOND (gstCaps.get () != nullptr, stiRESULT_ERROR);
	}
	else
	{
		gstCaps = GStreamerCaps ("video/x-raw-yuv",
						"framerate", GST_TYPE_FRACTION, DEFAULT_ENCODE_FRAME_RATE_NUM, DEFAULT_ENCODE_FRAME_RATE_DEN,
						"pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
						"width", G_TYPE_INT, maxCaptureWidthGet(),
						"height", G_TYPE_INT, maxCaptureHeightGet(),
						"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I', '4', '2', '0'),
						NULL);
		stiTESTCOND (gstCaps.get () != nullptr, stiRESULT_ERROR);
	}

	m_pGstElementPTZPreVideoConvertFilter = gst_element_factory_make("capsfilter", "PTZ_pre_video_convert_filter_element");
	stiTESTCOND (m_pGstElementPTZPreVideoConvertFilter, stiRESULT_ERROR);

	g_object_set (G_OBJECT(m_pGstElementPTZPreVideoConvertFilter), "caps", gstCaps.get (), NULL);

	// Create the video converter
	m_pGstElementPTZVideoConvert = gst_element_factory_make ("nvvidconv", "PTZ_video_convert_element");
	stiTESTCOND (m_pGstElementPTZVideoConvert, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementPTZVideoConvert), "output-buffers", 4, NULL);

STI_BAIL:

	return hResult;
}


gboolean EventDropperFunction (
	GstPad * pGstPad,
	GstMiniObject * pGstMiniObject,
	gpointer pUserData)
{
	gboolean bResult = TRUE;
	GstEvent * pGstEvent = GST_EVENT (pGstMiniObject);

	int nEventType = GST_EVENT_TYPE (pGstEvent);

	switch (nEventType)
	{
		case GST_EVENT_EOS:
		case GST_EVENT_NEWSEGMENT:

			stiTrace ("****** dropping event: %d\n", nEventType);

			//bResult = FALSE;
			break;
	}

	return bResult;
}


stiHResult CaptureBin::PTZBinAddAndLink ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstPadSrc = nullptr;
	gboolean bResult = false;
	gboolean bLinked;

	stiTESTCOND (m_pGstElementPTZBin, stiRESULT_ERROR);
	stiTESTCOND (m_pGstElementPTZPreVideoConvertFilter, stiRESULT_ERROR);
	stiTESTCOND (m_pGstElementPTZVideoConvert, stiRESULT_ERROR);

		// add elements
	gst_bin_add_many (GST_BIN (m_pGstElementPTZBin),
		m_pGstElementPTZPreVideoConvertFilter,
		m_pGstElementPTZVideoConvert,
		NULL);

	// link all the elements
	bLinked = gst_element_link_many(
				m_pGstElementPTZPreVideoConvertFilter,
				m_pGstElementPTZVideoConvert,
				NULL);

	stiTESTCOND (bLinked, stiRESULT_ERROR);


	// add bin to pipeline
	bResult = gst_bin_add (GST_BIN (get ()), m_pGstElementPTZBin);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	{
		// Add sink ghost pad and link to src passed in
		hResult = SinkGhostPadCreate (
			m_pGstElementPTZBin,
			m_pGstElementPTZPreVideoConvertFilter);
		stiTESTRESULT ();

		GStreamerElement ptzBin (m_pGstElementPTZBin, true);
		auto result = m_captureSource.linkPads ("src", ptzBin, "sink");
		stiTESTCOND (result, stiRESULT_ERROR);
	}

	// Add a probe that drops unwanted events
	pGstPadSrc = gst_element_get_static_pad (m_captureSource.get (), "src");
	stiTESTCOND (pGstPadSrc, stiRESULT_ERROR);

	gst_pad_add_event_probe (pGstPadSrc, (GCallback)EventDropperFunction, this);

	gst_object_unref (pGstPadSrc);
	pGstPadSrc = nullptr;

	hResult = SrcGhostPadCreate (
		m_pGstElementPTZBin,
		m_pGstElementPTZVideoConvert);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CaptureBin::TeeWithQueuesCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pGstElementEncodeTee = gst_element_factory_make ("tee", "encode_tee_element");
	stiTESTCOND (m_pGstElementEncodeTee, stiRESULT_ERROR);

	m_pGstElementEncodeQueue0 = gst_element_factory_make ("queue", nullptr);
	stiTESTCOND (m_pGstElementEncodeQueue0, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementEncodeQueue0), "max-size-buffers", 2, NULL);

	m_pGstElementEncodeQueue1 = gst_element_factory_make ("queue", nullptr);
	stiTESTCOND (m_pGstElementEncodeQueue1, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementEncodeQueue1), "max-size-buffers", 2, NULL);

STI_BAIL:

	return hResult;
}


stiHResult CaptureBin::TeeWithQueuesAddAndLink ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPadLinkReturn eGstPadLinkReturn;
	gboolean bResult = false;
	GstPadTemplate *pGstPadTemplate;
	GstPad *pGstPadQueueSink0 = nullptr;
	GstPad *pGstPadQueueSink1 = nullptr;

	GstPad *pGstPadSrc = nullptr;
	GstPad *pGstPadSink = nullptr;

	bResult = gst_bin_add (GST_BIN (get ()), m_pGstElementEncodeTee);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bResult = gst_bin_add (GST_BIN (get ()), m_pGstElementEncodeQueue0);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bResult = gst_bin_add (GST_BIN (get ()), m_pGstElementEncodeQueue1);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	pGstPadTemplate = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (m_pGstElementEncodeTee), "src%d");
	stiTESTCOND (pGstPadTemplate, stiRESULT_ERROR);

	//
	// Link tee pad with m_pGstElementEncodeQueue0
	//
	m_pGstPadTeeSrc0 = gst_element_request_pad (m_pGstElementEncodeTee, pGstPadTemplate, nullptr, nullptr);
	stiTESTCOND (m_pGstPadTeeSrc0, stiRESULT_ERROR);

	pGstPadQueueSink0 = gst_element_get_static_pad (m_pGstElementEncodeQueue0, "sink");
	stiTESTCOND (pGstPadQueueSink0, stiRESULT_ERROR);

	eGstPadLinkReturn = gst_pad_link (m_pGstPadTeeSrc0, pGstPadQueueSink0);
	stiTESTCOND (eGstPadLinkReturn == GST_PAD_LINK_OK, stiRESULT_ERROR);

	gst_object_unref (pGstPadQueueSink0);
	pGstPadQueueSink0 = nullptr;

	//
	// Link tee pad with m_pGstElementEncodeQueue1
	//
	m_pGstPadTeeSrc1 = gst_element_request_pad (m_pGstElementEncodeTee, pGstPadTemplate, nullptr, nullptr);
	stiTESTCOND (m_pGstPadTeeSrc1, stiRESULT_ERROR);

	pGstPadQueueSink1 = gst_element_get_static_pad (m_pGstElementEncodeQueue1, "sink");
	stiTESTCOND (pGstPadQueueSink1, stiRESULT_ERROR);

	eGstPadLinkReturn = gst_pad_link (m_pGstPadTeeSrc1, pGstPadQueueSink1);
	stiTESTCOND (eGstPadLinkReturn == GST_PAD_LINK_OK, stiRESULT_ERROR);

	gst_object_unref (pGstPadQueueSink1);
	pGstPadQueueSink1 = nullptr;

	// link upstream
	pGstPadSrc = gst_element_get_static_pad (m_pGstElementPTZBin, "src");
	stiTESTCOND (pGstPadSrc, stiRESULT_ERROR);

	pGstPadSink = gst_element_get_static_pad (m_pGstElementEncodeTee, "sink");
	stiTESTCOND (pGstPadSrc, stiRESULT_ERROR);

	eGstPadLinkReturn = gst_pad_link (pGstPadSrc, pGstPadSink);
	stiTESTCONDMSG (eGstPadLinkReturn == GST_PAD_LINK_OK, stiRESULT_ERROR, "didn't link upstream (reason %d)\n", eGstPadLinkReturn);

	gst_object_unref (pGstPadSrc);
	pGstPadSrc = nullptr;

	gst_object_unref (pGstPadSink);
	pGstPadSink = nullptr;

STI_BAIL:

	if (pGstPadQueueSink0)
	{
		gst_object_unref (pGstPadQueueSink0);
		pGstPadQueueSink0 = nullptr;
	}

	if (pGstPadQueueSink1)
	{
		gst_object_unref (pGstPadQueueSink1);
		pGstPadQueueSink1 = nullptr;
	}

	if (pGstPadSrc)
	{
		gst_object_unref (pGstPadSrc);
		pGstPadSrc = nullptr;
	}

	if (pGstPadSink)
	{
		gst_object_unref (pGstPadSink);
		pGstPadSink = nullptr;
	}

	return hResult;
}


stiHResult CaptureBin::PTZBinRemove ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (get () && m_pGstElementPTZBin)
	{
		gst_bin_remove (GST_BIN (get ()), m_pGstElementPTZBin);
		m_pGstElementPTZBin = nullptr;
		m_pGstElementPTZPreVideoConvertFilter = nullptr;
		m_pGstElementPTZVideoConvert = nullptr;
	}
//STI_BAIL:

	return hResult;
}


