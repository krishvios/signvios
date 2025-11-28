#include "FrameCaptureBinBase.h"

#include "GStreamerCaps.h"
#include "GStreamerPad.h"
#include "stiError.h"
#include "stiTools.h"
#include <cmath>
		 
FrameCaptureBinBase::FrameCaptureBinBase()
:
	CstiEventQueue ("FrameCaptureBinBase"),
	GStreamerBin ("Frame_Capture_Bin")
{
}


FrameCaptureBinBase::~FrameCaptureBinBase()
{
	CstiEventQueue::StopEventLoop();

	stateSet(GST_STATE_NULL);

	if (m_unFrameCaptureNewBufferSignalHandler)
	{
		g_signal_handler_disconnect (m_gstElementFrameCaptureAppSink.get (), m_unFrameCaptureNewBufferSignalHandler);
		m_unFrameCaptureNewBufferSignalHandler = 0;
	}
}

stiHResult FrameCaptureBinBase::startup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto result = false;

	result = CstiEventQueue::StartEventLoop();
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}

void FrameCaptureBinBase::create()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);
	
	gboolean bLinked; 
	GStreamerElement convertCapsElement;
	GStreamerCaps gstCaps;
	GStreamerPad testPad;
	GStreamerPad sinkPad;
	GStreamerPad srcPad;

	m_gstElementFrameCaptureAppSrc = GStreamerAppSource ("frame_capture_app_src_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSrc.get () != nullptr, stiRESULT_ERROR);
	
	m_gstElementFrameCaptureAppSrc.propertySet("format", GST_FORMAT_TIME);
	m_gstElementFrameCaptureAppSrc.propertySet("is-live", TRUE);

	m_msdkFactory = GStreamerElementFactory ("msdkvpp");

	// We need num-buffers set to 1 so that appsrc will send an eos (when it gets a buffer)
	// which tells the jpeg encoder to push the buffer to the appsink.
	m_gstElementFrameCaptureAppSrc.propertySet("num-buffers", 1);

	stiTESTCOND (cropScaleBinBaseGet() != nullptr, stiRESULT_ERROR);
	cropScaleBinBaseGet()->create();
	stiTESTCOND(cropScaleBinBaseGet()->get (), stiRESULT_ERROR);

	m_gstElementFrameCaptureJpegEnc = jpegEncoderGet();

	// Create the appsink element
	m_gstElementFrameCaptureAppSink = GStreamerAppSink ("frame_capture_appsink_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSink.get () != nullptr, stiRESULT_ERROR);

	m_gstElementFrameCaptureAppSink.propertySet("sync", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet("async", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet("qos", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet("enable-last-sample", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet("emit-signals", TRUE);
	m_gstElementFrameCaptureAppSink.propertySet("max-buffers", 1);

	elementAdd(m_gstElementFrameCaptureAppSrc);
	elementAdd(*cropScaleBinBaseGet());

	elementAdd(m_gstElementFrameCaptureJpegEnc);
	elementAdd(m_gstElementFrameCaptureAppSink);

	bLinked = m_gstElementFrameCaptureAppSrc.link(*cropScaleBinBaseGet());
	stiTESTCOND(bLinked, stiRESULT_ERROR);

	bLinked = cropScaleBinBaseGet()->link(m_gstElementFrameCaptureJpegEnc);
	stiTESTCOND(bLinked, stiRESULT_ERROR);

	bLinked = m_gstElementFrameCaptureJpegEnc.link(m_gstElementFrameCaptureAppSink);
	stiTESTCOND(bLinked, stiRESULT_ERROR);

	// Connect the signal to notify us that we have a frame to process.
	m_unFrameCaptureNewBufferSignalHandler = g_signal_connect (m_gstElementFrameCaptureAppSink.get (),
															   "new-sample", G_CALLBACK (frameCaptureAppSinkGstBufferPut), this);

#if 0
	sinkPad = m_gstElementFrameCaptureAppSink.staticPadGet("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

 	gst_pad_add_probe (sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, 
					   (GstPadProbeCallback)debugProbeBufferFunction, this, nullptr);

 	gst_pad_add_probe (sinkPad.get(), (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH), 
					   (GstPadProbeCallback)debugProbeEventFunction, this, nullptr);
#endif

STI_BAIL:

	return;
}



GStreamerElement FrameCaptureBinBase::jpegEncoderGet()
{
	// Create the jpeg encoder.
	GStreamerElement jpegEncoder = GStreamerElement("jpegenc", "jpeg_encoder_element");
	jpegEncoder.propertySet("idct-method", 1);

	return jpegEncoder;
}

GstFlowReturn FrameCaptureBinBase::frameCaptureAppSinkGstBufferPut (
	GstElement* pGstElement,
	gpointer pUser)
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("FrameCaptureBinBase::FrameCaptureAppSinkGstBufferPut\n");
	);
   	auto frameCaptureBinBase = (FrameCaptureBinBase *)pUser;

	auto gstAppSink = GStreamerAppSink((GstAppSink*)pGstElement, true);

	if (gstAppSink.get () != nullptr)
	{
		auto sample = gstAppSink.pullSample ();
		
		if (sample.get () != nullptr)
		{
			auto buffer = sample.bufferGet ();

			if (buffer.get () != nullptr)
			{
				if (frameCaptureBinBase)
				{
					frameCaptureBinBase->frameCaptureReceivedSignal.Emit (buffer);
				}
				else
				{
					stiASSERTMSG (estiFALSE, "FrameCaptureBinBase::frameCaptureAppSinkGstBufferPut: WARNING no frameCaptureBinBase\n");
				}
			}
		}
	}

	if (frameCaptureBinBase->m_msdkFactory.get())
	{
		frameCaptureBinBase->PostEvent([frameCaptureBinBase]{
			frameCaptureBinBase->eventFrameCaptureBinReset();
			});
	}

	return GST_FLOW_OK;
}


stiHResult FrameCaptureBinBase::frameCaptureBinStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("FrameCaptureBinBase::frameCaptureBinStart\n");
	);

	GstStateChangeReturn stateChangeReturn;
	stateChangeReturn = stateSet (GST_STATE_PLAYING);

	if (stateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FrameCaptureBinBase::FrameCaptureBinStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (frameCaptureBin, GST_STATE_PLAYING)\n");
	}

STI_BAIL:

	return (hResult);
}


void FrameCaptureBinBase::eventFrameCaptureBinReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("FrameCaptureBinBase::eventFrameCaptureReset\n");
	);

	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = stateSet (GST_STATE_NULL);

	if (StateChangeReturn == GST_STATE_CHANGE_FAILURE)
	{
		stiTHROWMSG (stiRESULT_ERROR, "FrameCaptureBinBase::eventFrameCaptureReset: GST_STATE_CHANGE_FAILURE: gst_element_set_state (frameCaptureBin, GST_STATE_NULL)\n");
	}

	elementRemove(m_gstElementFrameCaptureAppSrc);
	m_gstElementFrameCaptureAppSrc.clear();
	if (cropScaleBinBaseGet())
	{
		elementRemove(*cropScaleBinBaseGet());
		cropScaleBinBaseGet()->clear();
	}
	elementRemove(m_gstElementFrameCaptureJpegEnc);
	m_gstElementFrameCaptureJpegEnc.clear();
	elementRemove(m_gstElementFrameCaptureAppSink);
	m_gstElementFrameCaptureAppSink.clear();
	m_msdkFactory.clear();

	PostEvent([this]{create();});

//	create();

STI_BAIL:

	return;
}

void FrameCaptureBinBase::frameCapture (
	const GStreamerPad &gstPad,
	GStreamerBuffer &gstBuffer,
	bool crop,
	SstiImageCaptureSize imageSize)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	
	GStreamerCaps scaleSizeCaps;
	int nWidth = 0;
	int nHeight = 0;
	int nImgXpos = 0;
	int nImgYpos = 0;
	int nImgWidth = 0;
	int nImgHeight = 0;

	CstiCameraControl::SstiPtzSettings ptzRectangle {};

	bool bResult = false;
	GstStructure *pStruct = nullptr;
	GStreamerCaps gstCaps = gstPad.currentCapsGet();

	stiASSERT (!gstBuffer.flagIsSet(GST_BUFFER_FLAG_GAP));

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("FrameCaptureBinBase::frameCapture: bFrameCapture = TRUE, crop = ", crop, "\n");
	);

	hResult = frameCaptureBinStart();
	stiTESTRESULT ();
	 
	pStruct = gst_caps_get_structure (gstCaps.get (), 0);
	bResult = gst_structure_get_int (pStruct, "width", &nWidth);
	stiTESTCOND (bResult, stiRESULT_ERROR);
	bResult = gst_structure_get_int (pStruct, "height", &nHeight);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	m_eDisplayMode = eDISPLAY_MODE_HDMI_1920_BY_1080;
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("FrameCaptureBinBase::frameCapture: frame image size = ", nWidth, "x", nHeight, " maxWidth: ", maxCaptureWidthGet(), ", mode: ", m_eDisplayMode, "\n");
	);

	pStruct = nullptr;		// don't need to unref this because the struct belongs to the caps

	if (crop)
	{
		float scale = 1.0;

		// If we are in 720 mode normalize the image corrdinates to 1080 mode
		if (m_eDisplayMode & eDISPLAY_MODE_HDMI_1280_BY_720)
		{
			scale *= 3;
		}
		else if (m_eDisplayMode & eDISPLAY_MODE_HDMI_1920_BY_1080)
		{
			scale *= 2;
		}

		// Check to see if we are zoomed in.  If so normalize the values to the zoom area.
		if (nWidth != maxCaptureWidthGet())
		{
			scale *= (float)nWidth / (float)maxCaptureWidthGet();
		}

		nImgXpos = static_cast<int>(imageSize.n32Xorigin * scale);
		nImgYpos = static_cast<int>(imageSize.n32Yorigin * scale);

		nImgWidth =  static_cast<int>(imageSize.n32Width * scale);
		nImgHeight = static_cast<int>(imageSize.n32Height * scale);

		if ((nImgXpos + nImgWidth > nWidth) ||
			(nImgYpos + nImgHeight > nHeight))
		{
			stiTHROWMSG(stiRESULT_ERROR, "Crop Dimensions %dx%d-%dx%d invalid with frame size %dx%d\n",
						nImgXpos, nImgYpos,
						nImgWidth, nImgHeight,
						nWidth, nHeight);
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace("FrameCaptureBinBase::frameCapture: crop params = ", nImgXpos, "x", nImgYpos, "-", nImgWidth, "x", nImgHeight, "\n");
		);

		ptzRectangle.vertPos = nImgYpos;
		ptzRectangle.vertZoom = nImgHeight + 1;
		ptzRectangle.horzPos = nImgXpos;
		ptzRectangle.horzZoom = nImgWidth + 1;
	}
	else
	{
		ptzRectangle.vertPos = 0;
		ptzRectangle.vertZoom = m_columns;
		ptzRectangle.horzPos = 0;
		ptzRectangle.horzZoom = m_rows;
	}

	stiTESTCOND (cropScaleBinBaseGet() != nullptr, stiRESULT_ERROR);
	cropScaleBinBaseGet()->cropRectangleSet(ptzRectangle, nWidth, nHeight);

	if (gstBuffer.get () != nullptr)
	{
		
		m_gstElementFrameCaptureAppSrc.capsSet(gstCaps);

		stiDEBUG_TOOL (g_stiVideoInputDebug,
				vpe::trace ("FrameCaptureBinBase::frameCapture: calling pushBuffer ()\n");
				);

		GstFlowReturn flowReturn;
		flowReturn = m_gstElementFrameCaptureAppSrc.pushBuffer (gstBuffer);

		if (flowReturn != GST_FLOW_OK)
		{
			vpe::trace ("FrameCaptureBinBase::frameCapture: FAIL: ", __FUNCTION__, " pushBuffer ()\n");
			stiASSERT (false);
		}
	}
	else
	{
		vpe::trace ("FrameCaptureBinBase::frameCapture: FAIL: ", __FUNCTION__, " gst_buffer_new_allocate()\n");
		stiASSERT (false);
	}

STI_BAIL:
	return;
}


#if 0
GstPadProbeReturn FrameCaptureBinBase::debugProbeBufferFunction (
	GstPad * gstPad,
	GstPadProbeInfo *info,
	gpointer userData)
{

	if ((info->type & GST_PAD_PROBE_TYPE_BUFFER) != 0)
	{
		vpe::trace(__PRETTY_FUNCTION__, " We have a buffer  for m_gstElementFrameCaptureAppSink !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#if 0
		GstBuffer *buffer = static_cast<GstBuffer *>(info->data);

		GstMapInfo gstMapInfo;
		gst_buffer_map (buffer, &gstMapInfo, (GstMapFlags) GST_MAP_READ);
		vpe::trace("debugProbeFunction buffer mapped for m_frameCaptureAppSink\n");
		gst_buffer_unmap (buffer, &gstMapInfo);
		vpe::trace("debugProbeFunction buffer unmapped\n");
#endif
	}
	return GST_PAD_PROBE_OK;
}

GstPadProbeReturn FrameCaptureBinBase::debugProbeEventFunction (
	GstPad * gstPad,
	GstPadProbeInfo *info,
	gpointer userData)
{

   	if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) == GST_EVENT_EOS)
   	{
   		vpe::trace(__PRETTY_FUNCTION__, " We have a EOS event\n");
	}
	else if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) == GST_EVENT_FLUSH_START)
	{
			vpe::trace(__PRETTY_FUNCTION__, " We have a FLUSH_START\n");
	}
	else if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) == GST_EVENT_FLUSH_STOP)
	{
			vpe::trace(__PRETTY_FUNCTION__, " We have a FLUSH_STOP\n");
	}
	else if (GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) == GST_EVENT_STREAM_START) 
	{
		vpe::trace(__PRETTY_FUNCTION__, " We have a STREAM_START\n");
	}
	else
	{
		vpe::trace(__PRETTY_FUNCTION__, " We have event: ",  GST_EVENT_TYPE_NAME(GST_PAD_PROBE_INFO_DATA (info)), " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
	return GST_PAD_PROBE_OK;

}
#endif
