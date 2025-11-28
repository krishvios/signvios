#include "EncodePipeline.h"
#include "stiSVX.h"
#include "stiTools.h"

#define stiMAX_CAPTURE_WIDTH 1920
#define stiMAX_CAPTURE_HEIGHT 1072 //because its the value under 1080 thats a multiple of 16

EncodePipeline::~EncodePipeline ()
{
	// Bug 24358: disable the video when destroying the Encode pipeline
	m_displaySink.propertySet ("disablevideo", TRUE);

	g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "active-pad", m_pGstPadEncodeOuputSelectorFakeSrc, NULL);

	GstStateChangeReturn StateChangeReturn;

	StateChangeReturn = gst_element_set_state (get (), GST_STATE_NULL);
	stiASSERTMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE,
			"EncodePipeline::~EncodePipeline: Could not gst_element_set_state (get (), GST_STATE_NULL): %d\n", StateChangeReturn);

	DisplaySinkBinRemove ();

	EncodeSinkBinRemove ();
}

void EncodePipeline::create (
	const IstiVideoOutput::SstiImageSettings &displaySettings,
	bool privacy,
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	clear ();
	GStreamerPipeline::create ("encode_capture_pipeline");

	m_captureBin.create();

	elementAdd(m_captureBin);

	hResult = EncodeSinkBinCreate (videoRecordSettings);
	stiTESTRESULT ();

	hResult = EncodeSinkBinAddAndLink ();
	stiTESTRESULT ();

	hResult = DisplaySinkBinCreate (displaySettings, privacy);
	stiTESTRESULT ();

	hResult = DisplaySinkBinAddAndLink ();
	stiTESTRESULT ();

	busWatchEnable ();

	// Add a probe
	displaySinkProbeAdd ();

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear ();
	}
}

void EncodePipeline::rcuConnectedSet(bool rcuConnected)
{
	m_captureBin.rcuConnectedSet(rcuConnected);
}

stiHResult EncodePipeline::DisplaySinkBinRemove ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (get () && m_displaySink.get ())
	{
		gst_bin_remove (GST_BIN (get ()), m_displaySink.get ());
		m_displaySink.clear ();
	}

//STI_BAIL:

	return hResult;
}


stiHResult SrcGhostPadCreate (
	GstElement *pGstElementBin,
	GstElement *pGstElement)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstGhostPad = nullptr;
	GstPad *pGstPadSrc = nullptr;
	gboolean bResult = false;

	pGstPadSrc = gst_element_get_static_pad (pGstElement, "src");
	stiTESTCOND (pGstPadSrc, stiRESULT_ERROR);

	pGstGhostPad = gst_ghost_pad_new ("src", pGstPadSrc);
	stiTESTCOND (pGstGhostPad, stiRESULT_ERROR);

	bResult = gst_pad_set_active (pGstGhostPad, TRUE);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bResult = gst_element_add_pad (pGstElementBin, pGstGhostPad);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	gst_object_unref (pGstPadSrc);
	pGstPadSrc = nullptr;

	//Maybe
	//gst_object_unref (pGstGhostPad);
	//pGstGhostPad = NULL;

STI_BAIL:

	return hResult;

}


stiHResult EncodePipeline::EncodeSinkBinCreate (
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Encode bin
	stiASSERT (m_pGstElementEncodeSinkBin == nullptr);

	m_pGstElementEncodeSinkBin = gst_bin_new ("encode_sink_bin");
	stiTESTCOND (m_pGstElementEncodeSinkBin, stiRESULT_ERROR);

	m_pGstElementEncodeConvert = gst_element_factory_make ("nvvidconv", "encode_convert_element");
	stiTESTCOND (m_pGstElementEncodeConvert, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementEncodeConvert), "output-buffers", 2, NULL);

	char szOutputSize[64];
	sprintf (szOutputSize, "%dx%d", stiMAX_CAPTURE_WIDTH, stiMAX_CAPTURE_HEIGHT);
	g_object_set (G_OBJECT (m_pGstElementEncodeConvert), "output-size", szOutputSize, NULL);

	m_pGstElementEncodeOutputSelector = gst_element_factory_make ("output-selector", "encode_output_selector_element");
	stiTESTCOND (m_pGstElementEncodeOutputSelector, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "pad-negotiation-mode", 1, NULL);


	m_pGstElementEncodeFakeSink = gst_element_factory_make ("fakesink", "encode_fakesink_element");
	stiTESTCOND (m_pGstElementEncodeFakeSink, stiRESULT_ERROR);

	hResult = H264EncodeBinCreate (videoRecordSettings);
	stiTESTRESULT ();

	// Create the funnel element
	m_pGstElementEncodeFunnel = gst_element_factory_make ("funnel", "encode_funnel_element");
	stiTESTCOND (m_pGstElementEncodeFunnel, stiRESULT_ERROR);

	// Create the appsink element
	m_gstElementEncodeAppSink = GStreamerAppSink ("encode_appsink_element");
	stiTESTCOND (m_gstElementEncodeAppSink.get () != nullptr, stiRESULT_ERROR);

	//g_object_set (m_gstElementEncodeAppSink.get (), "sync", FALSE, NULL);
	g_object_set (m_gstElementEncodeAppSink.get (), "sync", TRUE, NULL);

	g_object_set (m_gstElementEncodeAppSink.get (), "async", FALSE, NULL);
	g_object_set (m_gstElementEncodeAppSink.get (), "qos", FALSE, NULL);
	g_object_set (m_gstElementEncodeAppSink.get (), "enable-last-buffer", FALSE, NULL);
	g_object_set (m_gstElementEncodeAppSink.get (), "max-buffers", 4, NULL);

STI_BAIL:

	return hResult;
}


stiHResult SinkGhostPadCreate (
	GstElement *pGstElementBin,
	GstElement *pGstElementLinkSink)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstGhostPad = nullptr;
	GstPad *pGstPadSink = nullptr;
	gboolean bResult = false;

	// create ghost pad for display bin sink
	pGstPadSink = gst_element_get_static_pad (pGstElementLinkSink, "sink");
	stiTESTCOND (pGstPadSink, stiRESULT_ERROR);

	pGstGhostPad = gst_ghost_pad_new ("sink", pGstPadSink);
	stiTESTCOND (pGstGhostPad, stiRESULT_ERROR);

	bResult = gst_pad_set_active (pGstGhostPad, TRUE);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bResult = gst_element_add_pad (pGstElementBin, pGstGhostPad);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	gst_object_unref (pGstPadSink);
	pGstPadSink = nullptr;

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::EncodeSinkBinAddAndLink ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstPad = nullptr;

	gboolean bLinked;
	gboolean bResult = false;

	// Build and link pipeline
	gst_bin_add_many (GST_BIN (m_pGstElementEncodeSinkBin),
			m_pGstElementEncodeConvert,
			m_pGstElementEncodeOutputSelector,
			m_pGstElementEncodeFakeSink,
			m_pGstElementEncodeFunnel,
			m_gstElementEncodeAppSink.elementGet (),
			NULL);

	// link all the elements that can be automatically linked because they have "always" pads
	bLinked = gst_element_link_many (
				m_pGstElementEncodeConvert,
				m_pGstElementEncodeOutputSelector,
				NULL);

	stiTESTCOND (bLinked, stiRESULT_ERROR);

	m_pGstPadEncodeOuputSelectorFakeSrc = gst_element_get_request_pad (m_pGstElementEncodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadEncodeOuputSelectorFakeSrc);

	m_pGstPadEncodeOuputSelectorH264Src = gst_element_get_request_pad (m_pGstElementEncodeOutputSelector, "src%d");
	stiASSERT (m_pGstPadEncodeOuputSelectorH264Src);

	// link ouput-selector to fakesrc
	pGstPad = gst_element_get_static_pad (m_pGstElementEncodeFakeSink, "sink");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	bLinked = gst_pad_link (m_pGstPadEncodeOuputSelectorFakeSrc, pGstPad);
	stiTESTCONDMSG (bLinked == GST_PAD_LINK_OK, stiRESULT_ERROR, "m_pGstPadEncodeOuputSelectorFakeSrc and m_pGstElementEncodeFakeSink didn't link\n");

	gst_object_unref (pGstPad);
	pGstPad = nullptr;

	// link ouput-selector to H264
	hResult = H264EncodeBinAddAndLink (m_pGstElementEncodeSinkBin, m_pGstPadEncodeOuputSelectorH264Src);
	stiTESTRESULT ();

	//link encoders to funnel
	m_pGstPadEncodeFunnelH264Sink = gst_element_get_request_pad (m_pGstElementEncodeFunnel, "sink%d");
	stiASSERT (m_pGstPadEncodeFunnelH264Sink);

	// link H264 to funnel
	pGstPad = gst_element_get_static_pad (m_pGstElementEncodeH264Bin, "src");
	stiTESTCOND (pGstPad, stiRESULT_ERROR);

	bLinked = gst_pad_link (pGstPad, m_pGstPadEncodeFunnelH264Sink);
	stiTESTCONDMSG (bLinked == GST_PAD_LINK_OK, stiRESULT_ERROR, "m_pGstElementEncodeH264Bin and m_pGstPadEncodeFunnelH264Sink didn't link\n");

	gst_object_unref (pGstPad);
	pGstPad = nullptr;

	gst_object_unref (pGstPad);
	pGstPad = nullptr;

	// link funnel to app sink
	bLinked = gst_element_link (m_pGstElementEncodeFunnel, m_gstElementEncodeAppSink.elementGet ());
	stiTESTCONDMSG (bLinked, stiRESULT_ERROR, "m_pGstElementEncodeFunnel and m_gstElementEncodeAppSink didn't link\n");

	// and to pipeline
	bResult = gst_bin_add (GST_BIN (get ()), m_pGstElementEncodeSinkBin);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	{
		hResult = SinkGhostPadCreate (
					m_pGstElementEncodeSinkBin,
					m_pGstElementEncodeConvert);
		stiTESTRESULT ();

		GStreamerElement encodeBin (m_pGstElementEncodeSinkBin, true);
		auto result = m_captureBin.linkPads ("encodesrc", encodeBin, "sink");
		stiTESTCOND (result, stiRESULT_ERROR);
	}

	m_gstElementEncodeAppSink.newBufferCallbackSet(
		[this](GStreamerAppSink appSink)
		{
			sampleFromAppSinkPull (appSink);
		});
//	m_gstElementEncodeAppSink.propertySet ("emit-signals", TRUE);

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::DisplaySinkBinCreate (
	const IstiVideoOutput::SstiImageSettings &displaySettings,
	bool privacy)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiASSERT (m_displaySink.get () == nullptr);

#ifdef ENABLE_FAKESINK
	m_displaySink = GStreamerElement ("fakesink", "fake_sink_element");
	stiTESTCOND (m_displaySink.get () != nullptr, stiRESULT_ERROR);
#else
	m_displaySink = GStreamerElement ("nv_omx_videosink", "display_sink_element");
	stiTESTCOND (m_displaySink.get () != nullptr, stiRESULT_ERROR);

	m_displaySink.propertySet ("mirror", TRUE);

	m_displaySink.propertySet ("overlay", 1);
	m_displaySink.propertySet ("hdmi", TRUE);
	//m_displaySink.propertySet ("sync", TRUE);
	m_displaySink.propertySet ("sync", FALSE);
	m_displaySink.propertySet ("async", FALSE);
	m_displaySink.propertySet ("force-aspect-ratio", TRUE);
	m_displaySink.propertySet ("enable-last-buffer", FALSE);
	m_displaySink.propertySet ("qos", FALSE);
	m_displaySink.propertySet ("force-fullscreen", FALSE);

	if (privacy)
	{
		m_displaySink.propertySet ("max-lateness", (gint64) -1);
	}
	else
	{
		m_displaySink.propertySet ("max-lateness", 100 * GST_MSECOND);
	}

	char win[64];
	sprintf(win, "%dx%d-%dx%d",
			displaySettings.unPosX,
			displaySettings.unPosY,
			displaySettings.unWidth,
			displaySettings.unHeight);

//	stiTrace ("***** display window: %s\n", win);

	// I dont know why but these settings must be set before the disablevideo setting will take effect
	m_displaySink.propertySet ("window", win);

	if (displaySettings.bVisible)
	{
		m_displaySink.propertySet ("disablevideo", FALSE);
	}
	else
	{
		m_displaySink.propertySet ("disablevideo", TRUE);
	}
#endif

STI_BAIL:

	return hResult;
}

stiHResult EncodePipeline::DisplaySinkBinAddAndLink ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//stiTrace ("EncodePipeline::DisplaySinkBinAddAndLink\n");
	gboolean bLinked;

	// link decode display pipeline
	gst_bin_add_many(GST_BIN (get ()),
			m_displaySink.get (), NULL);

	bLinked = m_captureBin.linkPads ("selfviewsrc", m_displaySink, "sink");
	stiTESTCOND (bLinked, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::EncodeSinkBinRemove ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (get () && m_pGstElementEncodeSinkBin)
	{
		hResult = H264EncodeBinRemove (m_pGstElementEncodeSinkBin);
		stiTESTRESULT ();

		gst_bin_remove (GST_BIN (get ()), m_pGstElementEncodeSinkBin);
		m_pGstElementEncodeFakeSink = nullptr;
		m_pGstElementEncodeSinkBin = nullptr;
		m_pGstElementEncodeConvert = nullptr;
		m_pGstElementEncodeOutputSelector = nullptr;
		m_pGstElementEncodeFunnel = nullptr;
		m_gstElementEncodeAppSink.clear ();
	}

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::EncodeConvertResize (
	uint32_t nWidth,
	uint32_t nHeight)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace("EncodePipeline::EncodeConvertResize: nWidth = %d, nHeight = %d\n", nWidth, nHeight);
	);

	stiTESTCOND (m_pGstElementEncodeConvert, stiRESULT_ERROR);
	stiTESTCOND (nWidth >= 16, stiRESULT_ERROR);
	stiTESTCOND (nHeight >= 16, stiRESULT_ERROR);

	char szOutputSize[64];
	sprintf (szOutputSize, "%dx%d", nWidth, nHeight);
	g_object_set (G_OBJECT (m_pGstElementEncodeConvert), "output-size", szOutputSize, NULL);

STI_BAIL:

	return (hResult);
}


void EncodePipeline::codecSet (
	EstiVideoCodec codec,
	EstiVideoProfile profile,
	unsigned int constraints,
	unsigned int level,
	EstiPacketizationScheme packetizationMode)
{
//	if (m_encodeBin)
//	{
//		m_encodeBin->codecSet (codec, profile, constraints, level, packetizationMode);
//	}
}


void EncodePipeline::bitrateSet(unsigned kbps)
{
	if (m_pGstElementEncodeH264)
	{
		g_object_set (G_OBJECT(m_pGstElementEncodeH264), "bitrate", kbps, NULL);
	}
}


void EncodePipeline::framerateSet(unsigned fps)
{
//	if (m_encodeBin)
//	{
//		m_encodeBin->framerateSet (fps);
//	}
}


GStreamerElement EncodePipeline::displaySinkGet()
{
	return m_displaySink;
}


void EncodePipeline::encodeSizeSet(unsigned width, unsigned height)
{
//	if (m_encodeBin)
//	{
//		m_encodeBin->sizeSet (width, height);
//	}
}

void EncodePipeline::encodingStart ()
{
	g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "active-pad", m_pGstPadEncodeOuputSelectorH264Src, NULL);

	m_encoding = true;
}


void EncodePipeline::encodingStop ()
{
	g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "active-pad", m_pGstPadEncodeOuputSelectorFakeSrc, NULL);

	m_encoding = false;
}


stiHResult EncodePipeline::brightnessSet (int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = m_captureBin.brightnessSet (brightness);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


stiHResult EncodePipeline::saturationSet (int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = m_captureBin.saturationSet (saturation);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}


void EncodePipeline::displaySettingsSet (
	const IstiVideoOutput::SstiImageSettings &displaySettings)
{
	char win[50];
	sprintf (win, "%dx%d-%dx%d",
			displaySettings.unPosX, displaySettings.unPosY,
			displaySettings.unWidth, displaySettings.unHeight);

	// I dont know why but these settings must be set before the disablevideo setting will take effect
	m_displaySink.propertySet ("window", win);

	if (displaySettings.bVisible)
	{
		m_displaySink.propertySet ("disablevideo", FALSE);
	}
	else
	{
		m_displaySink.propertySet ("disablevideo", TRUE);
	}
}


stiHResult EncodePipeline::H264EncodeBinCreate (
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pGstElementEncodeH264Bin = gst_bin_new ("H264_encode_bin");
	stiTESTCOND (m_pGstElementEncodeH264Bin, stiRESULT_ERROR);

	// Create the encoder element
	m_pGstElementEncodeH264 = gst_element_factory_make ("nv_omx_h264enc", "h264_encode_element");
	stiTESTCOND (m_pGstElementEncodeH264, stiRESULT_ERROR);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "bitrate", videoRecordSettings.unTargetBitRate, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "iframeinterval", 100000, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "framerate", 30, NULL);
	//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "use-timestamps", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "insert-spsppsatidr", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "rc-mode", 0, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "bit-packetization", TRUE, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "slice-header-spacing", videoRecordSettings.unMaxPacketSize * 8, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "app-type", 1, NULL);
	//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "low-latency", 1, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "output-buffers", 4, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "input-buffers", 1, NULL);
	g_object_set (G_OBJECT (m_pGstElementEncodeH264), "qp-range", "0-51:33-51", NULL);

	hResult = H264EncodeSettingsSet (videoRecordSettings);
	stiTESTRESULT ();

	{
		GStreamerCaps gstCaps ("video/x-h264",
						"stream-format", G_TYPE_STRING, "byte-stream",
						"alignment", G_TYPE_STRING, "au",
						NULL);
		stiTESTCOND (gstCaps.get () != nullptr, stiRESULT_ERROR);

		m_pGstElementEncodePostH264Filter = gst_element_factory_make("capsfilter", "encode_post_h264_encode_filter_element");
		stiTESTCOND (m_pGstElementEncodePostH264Filter, stiRESULT_ERROR);

		g_object_set (G_OBJECT(m_pGstElementEncodePostH264Filter), "caps", gstCaps.get (), NULL);
	}

STI_BAIL:

	return hResult;
}


stiHResult SinkGhostPadCreateAndLink (
	GstElement *pGstElementBin,
	GstPad *pGstPadSrc,
	GstElement *pGstElementLinkSink)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	GstPad *pGstGhostPad = nullptr;
	GstPad *pGstPadSink = nullptr;
	gboolean bLinked;
	gboolean bResult = false;

	// create ghost pad for display bin sink
	pGstPadSink = gst_element_get_static_pad (pGstElementLinkSink, "sink");
	stiTESTCOND (pGstPadSink, stiRESULT_ERROR);

	pGstGhostPad = gst_ghost_pad_new ("sink", pGstPadSink);
	stiTESTCOND (pGstGhostPad, stiRESULT_ERROR);

	bResult = gst_pad_set_active (pGstGhostPad, TRUE);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	bResult = gst_element_add_pad (pGstElementBin, pGstGhostPad);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	gst_object_unref (pGstPadSink);
	pGstPadSink = nullptr;

	bLinked = gst_pad_link (pGstPadSrc, pGstGhostPad);
	stiTESTCONDMSG (bLinked == GST_PAD_LINK_OK, stiRESULT_ERROR, "pGstGhostPad didn't link\n");

	//Maybe
	//gst_object_unref (pGstGhostPad);
	//pGstGhostPad = NULL;

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::H264EncodeBinAddAndLink (
	GstElement *pGstElementPipeline,
	GstPad *pGstLinkPad)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	gboolean bLinked;
	gboolean bResult = false;

	// Build and link pipeline
	gst_bin_add_many (GST_BIN (m_pGstElementEncodeH264Bin),
			m_pGstElementEncodeH264,
			m_pGstElementEncodePostH264Filter,
			NULL);

	// link all the elements that can be automatically linked because they have "always" pads
	bLinked = gst_element_link_many (
				m_pGstElementEncodeH264,
				m_pGstElementEncodePostH264Filter,
				NULL);
	stiTESTCOND (bLinked, stiRESULT_ERROR);

	// add bin to pipeline
	bResult = gst_bin_add (GST_BIN (pGstElementPipeline), m_pGstElementEncodeH264Bin);
	stiTESTCOND (bResult, stiRESULT_ERROR);

	// Add sink ghost pad and link to src passed in
	hResult = SinkGhostPadCreateAndLink (
		m_pGstElementEncodeH264Bin,
		pGstLinkPad,
		m_pGstElementEncodeH264);
	stiTESTRESULT ();

	hResult = SrcGhostPadCreate (m_pGstElementEncodeH264Bin, m_pGstElementEncodePostH264Filter);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult EncodePipeline::H264EncodeBinRemove (
	GstElement *pGstElementPipeline)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	gst_bin_remove (GST_BIN (pGstElementPipeline), m_pGstElementEncodeH264Bin);
	m_pGstElementEncodeH264 = nullptr;
	m_pGstElementEncodePostH264Filter = nullptr;

//STI_BAIL:

	return hResult;
}


gboolean EncodePipeline::keyframeEventSend ()
{
	KeyFrameForce (m_pGstElementEncodeH264Bin);

	return true;
}


stiHResult EncodePipeline::KeyFrameForce (
	GstElement *pGstElementPipeline)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	const char* sig = "force-IDR";
	GstPad *pGstPad = NULL;
	GstPad *pPeerGstPad = NULL;
	GstElement* pGstElement = NULL;
	gboolean done = FALSE;
	GstIterator* pGstIterator = NULL;

	stiTESTCOND (pGstElementPipeline, stiRESULT_ERROR);

	pGstIterator = gst_bin_iterate_elements (GST_BIN (pGstElementPipeline));
	stiTESTCOND (pGstIterator, stiRESULT_ERROR);

	while (!done)
	{
		switch (gst_iterator_next (pGstIterator, (void**)&pGstElement))
		{
			case GST_ITERATOR_OK:
			{
				GstElementFactory* pGstElementFactory = gst_element_get_factory (pGstElement);

				if (pGstElementFactory)
				{
					if (strncmp("ffenc_", GST_PLUGIN_FEATURE_NAME(pGstElementFactory), 6) == 0)
					{
						pGstPad = gst_element_get_static_pad (pGstElement, "src");

						if (pGstPad)
						{
							pPeerGstPad = gst_pad_get_peer (pGstPad);

							if (pPeerGstPad)
							{
								stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
									stiTrace ("KeyFrameForce: emit 'GstForceKeyUnit' event to src pad of '%s'\n", GST_OBJECT_NAME (pGstElement));
								);

								GstStructure *pStructure = gst_structure_new ("GstForceKeyUnit", "all-headers", G_TYPE_BOOLEAN, TRUE, NULL);
								stiTESTCOND (pStructure != NULL, stiRESULT_ERROR);

								GstEvent *pEvent = gst_event_new_custom (GST_EVENT_CUSTOM_UPSTREAM, pStructure);
								stiTESTCOND (pEvent != NULL, stiRESULT_ERROR);

								gboolean bResult = gst_pad_push_event (pPeerGstPad, pEvent);
								stiTESTCOND (bResult, stiRESULT_ERROR);

								gst_object_unref (pPeerGstPad);
								pPeerGstPad = NULL;
							}

							gst_object_unref (pGstPad);
							pGstPad = NULL;
						}
					}
					else if (strcmp ("nv_omx_h264enc", GST_PLUGIN_FEATURE_NAME (pGstElementFactory)) == 0 ||
							strcmp ("nv_omx_h263enc", GST_PLUGIN_FEATURE_NAME (pGstElementFactory)) == 0)
					{
						stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
							stiTrace ("KeyFrameForce: emit '%s' signal to element '%s'\n", sig, GST_OBJECT_NAME (pGstElement));
						);

						g_signal_emit_by_name (G_OBJECT(pGstElement), sig, NULL);
					}
				}

				gst_object_unref (pGstElement);
				pGstElement = NULL;

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (pGstIterator);
				break;

			case GST_ITERATOR_ERROR:
			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	gst_iterator_free (pGstIterator);

STI_BAIL:

	if (pGstElement)
	{
		gst_object_unref (pGstElement);
		pGstElement = NULL;
	}

	if (pGstPad)
	{
		gst_object_unref (pGstPad);
		pGstPad = NULL;
	}

	if (pPeerGstPad)
	{
		gst_object_unref (pPeerGstPad);
		pPeerGstPad = NULL;
	}

	return hResult;
}


stiHResult EncodePipeline::H264EncodeSettingsSet (
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Setting the profile is not supported in the codec
	// Apparently baseline is the only profile supported

	switch (videoRecordSettings.unLevel)
	{
		case estiH264_LEVEL_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x01, NULL);
			break;
		case estiH264_LEVEL_1_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level1.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x04, NULL);
			break;
		case estiH264_LEVEL_1_2:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level1.2", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x08, NULL);
			break;
		case estiH264_LEVEL_1_3:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level1.3", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x10, NULL);
			break;
		case estiH264_LEVEL_2:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level2", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x20, NULL);
			break;
		case estiH264_LEVEL_2_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level2.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x40, NULL);
			break;
		case estiH264_LEVEL_2_2:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level2.2", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x80, NULL);
			break;
		case estiH264_LEVEL_3:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level3", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x100, NULL);
			break;
		case estiH264_LEVEL_3_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level3.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x200, NULL);
			break;
		case estiH264_LEVEL_3_2:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level3.2", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x400, NULL);
			break;
		case estiH264_LEVEL_4:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level4", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x800, NULL);
			break;
		case estiH264_LEVEL_4_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level4.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x1000, NULL);
			break;
		case estiH264_LEVEL_4_2:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level4.2", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x2000, NULL);
			break;
		case estiH264_LEVEL_5:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level5", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x4000, NULL);
			break;
		case estiH264_LEVEL_5_1:
			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level5.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x8000, NULL);
			break;
		default:

			//g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", "level4.1", NULL);
			g_object_set (G_OBJECT (m_pGstElementEncodeH264), "level", 0x1000, NULL);
			break;
	}

//STI_BAIL:

	return hResult;

}


GstStateChangeReturn EncodePipeline::stateSet (GstState state)
{
	auto stateChange = GStreamerElement::stateSet (state);

	if (state == GST_STATE_PLAYING)
	{
		if (m_encoding)
		{
			g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "active-pad", m_pGstPadEncodeOuputSelectorH264Src, NULL);
		}
		else
		{
			g_object_set (G_OBJECT (m_pGstElementEncodeOutputSelector), "active-pad", m_pGstPadEncodeOuputSelectorFakeSrc, NULL);
		}
	}

	return stateChange;
}


void EncodePipeline::cameraControlFdSet(int cameraControlFd)
{
	m_captureBin.cameraControlFdSet(cameraControlFd);
}
