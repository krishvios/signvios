#include "FrameCaptureBin.h"
#include "CstiGStreamerVideoGraph.h"
						   
#define CROPRECT_BUFFER_SIZE 64
#define OUTPUT_SIZE_BUFFER_SIZE 64
   

void FrameCaptureBin::create ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult);

	gboolean linked;

	m_gstElementFrameCaptureAppSrc = GStreamerAppSource ("frame_capture_app_src_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSrc.get () != nullptr, stiRESULT_ERROR);

	m_gstElementFrameCaptureConvert = GStreamerElement("nvvidconv", "frame_capture_video_convert");
	stiTESTCOND(m_gstElementFrameCaptureConvert.get(), stiRESULT_ERROR);

	m_gstElementFrameCaptureConvert.propertySet("output-buffers", 2);

	char croprect[64];
	sprintf (croprect, "%dx%d-%dx%d",
				0, 0, stiMAX_CAPTURE_WIDTH, stiMAX_CAPTURE_HEIGHT);

	m_gstElementFrameCaptureConvert.propertySet("croprect", croprect);

	/*
	char szOutputSize[64];
	sprintf (szOutputSize, "%dx%d", stiMAX_CAPTURE_WIDTH, stiMAX_CAPTURE_HEIGHT);
	g_object_set (G_OBJECT (m_pGstElementFrameCaptureConvert), "output-size", szOutputSize, NULL);
	*/
 
	m_gstElementFrameCaptureJpegEnc = GStreamerElement("nv_omx_jpegenc", "jpeg_encoder_element");
	stiTESTCOND (m_gstElementFrameCaptureJpegEnc.get(), stiRESULT_ERROR);
	m_gstElementFrameCaptureJpegEnc.propertySet("use-timestamps", FALSE);
	//g_object_set (G_OBJECT (m_pGstElementFrameCaptureJpegEnc), "input-buffers", 1, NULL);
	//g_object_set (G_OBJECT (m_pGstElementFrameCaptureJpegEnc), "output-buffers", 1, NULL);

	// Create the appsink element
	m_gstElementFrameCaptureAppSink = GStreamerAppSink ("frame_capture_appsink_element");
	stiTESTCOND (m_gstElementFrameCaptureAppSink.get () != nullptr, stiRESULT_ERROR);

	m_gstElementFrameCaptureAppSink.propertySet ("sync", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet ("async", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet ("qos", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet ("enable-last-buffer", FALSE);
	m_gstElementFrameCaptureAppSink.propertySet ("emit-signals", TRUE);

	elementAdd(m_gstElementFrameCaptureAppSrc);
	elementAdd(m_gstElementFrameCaptureConvert);
	elementAdd(m_gstElementFrameCaptureJpegEnc);
	elementAdd(m_gstElementFrameCaptureAppSink);

	linked = m_gstElementFrameCaptureAppSrc.link(m_gstElementFrameCaptureConvert);
	stiTESTCOND(linked, stiRESULT_ERROR);

	linked = m_gstElementFrameCaptureConvert.link(m_gstElementFrameCaptureJpegEnc);
	stiTESTCOND(linked, stiRESULT_ERROR);

	linked = m_gstElementFrameCaptureJpegEnc.link(m_gstElementFrameCaptureAppSink);
	stiTESTCOND(linked, stiRESULT_ERROR);

	m_unFrameCaptureNewBufferSignalHandler = g_signal_connect (m_gstElementFrameCaptureAppSink.get (), "new-buffer", G_CALLBACK (frameCaptureAppSinkGstBufferPut), this);

STI_BAIL:

	return;
}


void FrameCaptureBin::frameCapture (
	const GStreamerPad &gstPad,
	GStreamerBuffer &gstBuffer,
	bool crop,
	SstiImageCaptureSize imageSize)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);

	int nWidth = 0;
	int nHeight = 0;
	int nImgXpos = 0;
	int nImgYpos = 0;
	int nImgWidth = 0;
	int nImgHeight = 0;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("FrameCaptureBin::frameCapture: bFrameCapture = TRUE, crop = %d\n", crop);
	);

	auto gstCaps = gstPad.currentCapsGet ();
	auto pStruct = gst_caps_get_structure (gstCaps.get (), 0);
	bool bResult = gst_structure_get_int (pStruct, "width", &nWidth);
	stiTESTCOND (bResult, stiRESULT_ERROR);
	bResult = gst_structure_get_int(pStruct, "height", &nHeight);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("FrameCaptureBin::frameCapture: frame image size = %dx%d\n", nWidth, nHeight);
	);

	pStruct = nullptr;		// don't need to unref this because the struct belongs to the caps

	if (crop)
	{
		nImgXpos = imageSize.n32Xorigin;
		nImgYpos = imageSize.n32Yorigin;
		nImgWidth = imageSize.n32Width;
		nImgHeight = imageSize.n32Height;

		// If we are in 720 mode normalize the image corrdinates to 1080 mode
		if (m_eDisplayMode & eDISPLAY_MODE_HDMI_1280_BY_720)
		{
			// 1.5 * 720 = 1080
			nImgXpos *= 1.5;
			nImgYpos *= 1.5;
			nImgWidth *= 1.5;
			nImgHeight *= 1.5;
		}

		// Check to see if we are zoomed in.  If so normalize the values to the zoom area.
		if (nWidth != m_maxCaptureWidth)
		{
			float fMultipler = (float)nWidth / m_maxCaptureWidth;
			nImgXpos *= fMultipler;
			nImgYpos *= fMultipler;
			nImgWidth *= fMultipler;
			nImgHeight *= fMultipler;
		}

		if ((nImgXpos + nImgWidth > nWidth) ||
			(nImgYpos + nImgHeight > nHeight))
		{
			stiTHROWMSG(stiRESULT_ERROR, "Crop Dimensions %dx%d-%dx%d invalid with frame size %dx%d\n",
						nImgXpos, nImgYpos,
						nImgWidth, nImgHeight,
						nWidth, nHeight);
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("FrameCaptureBin::frameCapture: crop params = %dx%d-%dx%d\n", nImgXpos, nImgYpos, nImgWidth, nImgHeight);
    	);

		char croprect[CROPRECT_BUFFER_SIZE];
		sprintf (croprect, "%dx%d-%dx%d", nImgXpos, nImgYpos, nImgWidth, nImgHeight);
		m_gstElementFrameCaptureConvert.propertySet("croprect", croprect);

		//Set the output size of the image.
		char szOutputSize[OUTPUT_SIZE_BUFFER_SIZE];
		sprintf (szOutputSize, "%dx%d", imageSize.n32Width, imageSize.n32Height);
		m_gstElementFrameCaptureConvert.propertySet("output-size", szOutputSize);
	}
	else
	{
		m_gstElementFrameCaptureConvert.propertySet("croprect", "0x0-0x0");

		//Set the output size of the image.
		char szOutputSize[OUTPUT_SIZE_BUFFER_SIZE];
		sprintf (szOutputSize, "%dx%d", m_columns, m_rows);
		m_gstElementFrameCaptureConvert.propertySet("output-size", szOutputSize);
	}

	stiASSERT (!gstBuffer.flagIsSet (GST_BUFFER_FLAG_GAP));

	{
		// Copy the buffer and send it to the Capture Frame pipeline.
		GStreamerBuffer bufferCopy (gstBuffer.sizeGet ()); // create a fresh new buffer

		bufferCopy.dataCopy(gstBuffer);
		bufferCopy.metaDataCopy (gstBuffer);

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("FrameCaptureBin::frameCapture: calling pushBuffer ()\n");
		);

		GstFlowReturn flowReturn;
		flowReturn = m_gstElementFrameCaptureAppSrc.pushBuffer (bufferCopy);

		if (flowReturn != GST_FLOW_OK)
		{
			stiTrace ("FrameCaptureBin::frameCapture: FAIL: %s pushBuffer ()\n", __FUNCTION__);
			stiASSERT (false);
		}
	}

STI_BAIL:;
}


void FrameCaptureBin::frameCaptureJpegEncDropSet(
	bool frameCapture)
{
	if (frameCapture)
	{
		m_gstElementFrameCaptureJpegEnc.propertySet("drop", FALSE);
	}
}
