// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <gst/gst.h>
//#include "test.h"
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

typedef struct
{
	GThread * Test0Thread;
//  GThread * Test0Thread;

	GstElement * pEncodePipeline;
	GstElement * pDecodePipeline;

	GstElement * pCaptureBin;
	GstElement * pCaptureSource;
	GstElement * pPTZPreVideoConvertFilter;
	GstElement * pPTZVideoConvert;
	GstElement * pPTZPostVideoConvertFilter;

	GstElement * pEncodeBin;
	GstElement * pEncodeVideoConvert;
	GstElement * pEncodePostVideoConvertFilter;
	GstElement * pEncode;
	GstElement * pEncodePostEncodeFilter;
	GstElement * pEncodeAppSink;

	GstElement * pDecodeBin;
	GstElement * pDecodeAppSrc;
	GstElement * pDecode;

	GstElement * pDisplayBin;
	GstElement * pDisplayVideoConvert;
	GstElement * pDisplayPostVideoConvertFilter;
	GstElement * pDisplaySink;

	int nCaptureWidth;
	int nCaptureHeight;

	int nEncodeWidth;
	int nEncodeHeight;
	int nEncodeBitrate;
	int nEncodeMaxPacketSize;

	int nDisplayWidth;
	int nDisplayHeight;
	int nDisplayXPos;
	int nDisplayYPos;
	int nDisplayLayer;

	gboolean bRunning;
} TestData;

static GstFlowReturn EncodeAppSinkGstBufferPut (
	GstElement* pGstElement,
	gpointer pUser)
{
	printf ("OnNewBuffer\n");

	GstAppSink* pGstAppSink = (GstAppSink*)pGstElement;

	GstBuffer* pGstBuffer = gst_app_sink_pull_buffer(pGstAppSink);

	if (pGstBuffer)
	{
		TestData * pData = (TestData*)pUser;

		GstCaps *pCaps = GST_BUFFER_CAPS(pGstBuffer);

		printf("OnNewBuffer: caps = %s\n", gst_caps_to_string (pCaps));


		int ret = gst_app_src_push_buffer(GST_APP_SRC(pData->pDecodeAppSrc), pGstBuffer);

		if (ret != GST_FLOW_OK)
		{
			printf ("OnNewBuffer: Error pushing\n");
		}

		//gst_buffer_unref(pGstBuffer);

	}

	return GST_FLOW_OK;
}


static void EncoderBinCreate (
	TestData * pData)
{

	GstCaps* pFiltercaps;

	pData->pEncodeBin = gst_bin_new ("encode_bin");
	g_assert (pData->pEncodeBin);


	pData->pEncodeVideoConvert = gst_element_factory_make ("nvvidconv", "encode_video_convert_element");
	g_assert (pData->pEncodeVideoConvert);
	//g_object_set (G_OBJECT (pData->pEncodeVideoConvert), "force-aspect-ratio", "true", NULL);

	// Create the filter after nvvidconv
	pFiltercaps = gst_caps_new_simple("video/x-nv-yuv",
					"width", G_TYPE_INT, pData->nEncodeWidth,
					"height", G_TYPE_INT, pData->nEncodeHeight,
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
					NULL);
	g_assert (pFiltercaps);

	pData->pEncodePostVideoConvertFilter = gst_element_factory_make("capsfilter", "encode_post_video_convert_filter_element");
	g_assert(pData->pEncodePostVideoConvertFilter);
	g_object_set(G_OBJECT(pData->pEncodePostVideoConvertFilter), "caps", pFiltercaps, NULL);

	gst_caps_unref(pFiltercaps);

	// Create the encoder element
	pData->pEncode = gst_element_factory_make("nv_omx_h264enc", "encode_element");
	g_assert (pData->pEncode);
	g_object_set (G_OBJECT (pData->pEncode), "bitrate", pData->nEncodeBitrate, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "iframeinterval", 100000, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "framerate", 30, NULL);
	//g_object_set (G_OBJECT (pData->pEncode), "avc-level", 11, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "use-timestamps", TRUE, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "insert-spsppsatidr", TRUE, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "rc-mode", 0, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "bit-packetization", TRUE, NULL);
	g_object_set (G_OBJECT (pData->pEncode), "slice-header-spacing", pData->nEncodeMaxPacketSize * 8, NULL);

	pFiltercaps = gst_caps_new_simple (
					"video/x-h264",
					"width", G_TYPE_INT, pData->nEncodeWidth,
					"height", G_TYPE_INT, pData->nEncodeHeight,
					"framerate", GST_TYPE_FRACTION, 30, 1,
					"stream-format", G_TYPE_STRING, "byte-stream",
					"alignment", G_TYPE_STRING, "au",
					NULL);
	g_assert (pFiltercaps);

	pData->pEncodePostEncodeFilter = gst_element_factory_make("capsfilter", "encode_post_encode_filter_element");
	g_assert(pData->pEncodePostEncodeFilter);
	g_object_set(G_OBJECT(pData->pEncodePostEncodeFilter), "caps", pFiltercaps, NULL);

	gst_caps_unref(pFiltercaps);

	// Create the appsink element
	pData->pEncodeAppSink = gst_element_factory_make ("appsink", "encode_appsink_element");
	g_assert (pData->pEncodeAppSink);
	g_object_set (G_OBJECT (pData->pEncodeAppSink), "sync", FALSE, NULL);
	g_object_set (G_OBJECT (pData->pEncodeAppSink), "async", FALSE, NULL);
	//g_object_set (G_OBJECT (pData->pEncodeAppSink), "qos", TRUE, NULL);
	g_object_set (G_OBJECT (pData->pEncodeAppSink), "enable-last-buffer", FALSE, NULL);

	// Build and link pipeline
	gst_bin_add_many (
			GST_BIN (pData->pEncodeBin),
			pData->pEncodeVideoConvert,
			pData->pEncodePostVideoConvertFilter,
			pData->pEncode,
			pData->pEncodePostEncodeFilter,
			pData->pEncodeAppSink,
			NULL);

	// link all the elements that can be automatically linked because they have "always" pads
	bool bLinked = gst_element_link_many(
			pData->pEncodeVideoConvert,
			pData->pEncodePostVideoConvertFilter,
			pData->pEncode,
			pData->pEncodePostEncodeFilter,
			pData->pEncodeAppSink,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pEncodeBin didn't link-\n");
	}

	// Add sink ghost pad
	GstPad *pPad;
	pPad = gst_element_get_static_pad (pData->pEncodeVideoConvert, "sink");
	gst_element_add_pad (pData->pEncodeBin, gst_ghost_pad_new ("sink", pPad));
	gst_object_unref (GST_OBJECT (pPad));

	g_signal_connect (pData->pEncodeAppSink, "new-buffer", G_CALLBACK (EncodeAppSinkGstBufferPut), pData);
	g_object_set (G_OBJECT (pData->pEncodeAppSink), "emit-signals", TRUE, "sync", FALSE, NULL);

}

static void Notify (
	gpointer data)
{
	printf("Notify\n");
}

void NeedData(
	GstAppSrc *src,
	guint length,
	gpointer user_data)
{
	printf ("NeedData\n");
}

void EnoughData(
	GstAppSrc *src,
	gpointer user_data)
{
	printf ("EnoughData\n");
}


static void DecoderBinCreate (
	TestData * pData)
{
	pData->pDecodeBin = gst_bin_new ("decode_bin");
	g_assert (pData->pDecodeBin);

	pData->pDecodeAppSrc = gst_element_factory_make ("appsrc", "decode_app_src_element");
	g_assert(pData->pDecodeAppSrc);

	GstAppSrcCallbacks AppSrcCallbacks;

	AppSrcCallbacks.need_data = NeedData;
	AppSrcCallbacks.enough_data = EnoughData;
	AppSrcCallbacks.seek_data = NULL;
	gst_app_src_set_callbacks(GST_APP_SRC(pData->pDecodeAppSrc), &AppSrcCallbacks, pData, Notify);

	pData->pDecode = gst_element_factory_make("nv_omx_h264dec", "decode_element");
	g_assert(pData->pDecode);
	//g_object_set (G_OBJECT (pData->pDecodeElement), "use-timestamps", TRUE, NULL);

	// Build and link pipeline
	gst_bin_add_many (
			GST_BIN (pData->pDecodeBin),
			pData->pDecodeAppSrc,
			pData->pDecode,
			NULL);

	// link all the elements that can be automatically linked because they have "always" pads
	bool bLinked = gst_element_link_many(
			pData->pDecodeAppSrc,
			pData->pDecode,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pDecodeBin didn't link-\n");
	}

	// Add sink ghost pad
	GstPad *pPad;
	pPad = gst_element_get_static_pad (pData->pDecode, "src");
	gst_element_add_pad (pData->pDecodeBin, gst_ghost_pad_new ("src", pPad));
	gst_object_unref (GST_OBJECT (pPad));

}

static void EncoderSettingsSet (
	TestData * pData)
{
	printf("EncoderSettingsSet:\n");
	printf("pData->nEncodeWidth = %d\n", pData->nEncodeWidth);
	printf("pData->nEncodeHeight = %d\n", pData->nEncodeHeight);

	gst_element_set_state(pData->pEncodePipeline, GST_STATE_NULL);
	gst_element_set_state(pData->pDecodePipeline, GST_STATE_NULL);

	gst_bin_remove (
				GST_BIN (pData->pEncodePipeline),
				pData->pEncodeBin);


	gst_bin_remove (
					GST_BIN (pData->pDecodePipeline),
					pData->pDecodeBin);


	EncoderBinCreate (pData);

	DecoderBinCreate (pData);



	// link encode pipeline
	gst_bin_add_many (
			GST_BIN (pData->pEncodePipeline),
			pData->pEncodeBin,
			NULL);


	bool bLinked = gst_element_link_many (
			pData->pCaptureBin,
			pData->pEncodeBin,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pEncodePipeline didn't link\n");
	}

	// link decode pipeline
	gst_bin_add_many (
			GST_BIN (pData->pDecodePipeline),
			pData->pDecodeBin,
			NULL);

	bLinked = gst_element_link_many (
			pData->pDecodeBin,
			pData->pDisplayBin,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pDecodePipeline didn't link\n");
	}

	gst_element_set_state(pData->pEncodePipeline, GST_STATE_PLAYING);

	gst_element_set_state(pData->pDecodePipeline, GST_STATE_PLAYING);

}


static gpointer Test0Function (
	gpointer pUserData)
{
	TestData * pData = (TestData *)pUserData;

	g_thread_yield ();
	g_usleep (G_USEC_PER_SEC * 5);
	g_thread_yield ();

	while (pData->bRunning)
	{
		pData->nEncodeWidth = 352;
		pData->nEncodeHeight = 240;
		EncoderSettingsSet (pData);

		g_thread_yield ();
		g_usleep (G_USEC_PER_SEC * 5);
		g_thread_yield ();

		pData->nEncodeWidth = 704;
		pData->nEncodeHeight = 480;
		EncoderSettingsSet (pData);

		g_thread_yield ();
		g_usleep (G_USEC_PER_SEC * 5);
		g_thread_yield ();
	}

	return NULL;
}

static void SourceBinCreate (
	TestData * pData)
{

	GstCaps* pFiltercaps;

	pData->pCaptureBin = gst_bin_new ("capture_bin");
	g_assert (pData->pCaptureBin);

#if 0
	pData->pCaptureSource = gst_element_factory_make ("nv4l2src", "capture_source_element");
	g_assert (pData->pCaptureSource);
	g_object_set (G_OBJECT (pData->pCaptureSource), "device", "/dev/video0", NULL);
	// PTZ bin
	// Create the filter before nvvidconv
	pFiltercaps = gst_caps_new_simple("video/x-nv-yuv",
					"framerate", GST_TYPE_FRACTION, 30, 1,
					"width", G_TYPE_INT, pData->nCaptureWidth,
					"height", G_TYPE_INT, pData->nCaptureHeight,
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
					NULL);
#else
	pData->pCaptureSource = gst_element_factory_make ("videotestsrc", "capture_source_element");
	g_object_set (G_OBJECT (pData->pCaptureSource), "is-live", 1, NULL);
	g_assert (pData->pCaptureSource);
	// PTZ bin
	// Create the filter before nvvidconv
	pFiltercaps = gst_caps_new_simple("video/x-raw-yuv",
					"framerate", GST_TYPE_FRACTION, 30, 1,
					"width", G_TYPE_INT, pData->nCaptureWidth,
					"height", G_TYPE_INT, pData->nCaptureHeight,
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
					NULL);
#endif


	g_assert (pFiltercaps);

	pData->pPTZPreVideoConvertFilter = gst_element_factory_make("capsfilter", "PTZ_pre_video_convert_filter_element");
	g_assert(pData->pPTZPreVideoConvertFilter);
	g_object_set(G_OBJECT(pData->pPTZPreVideoConvertFilter), "caps", pFiltercaps, NULL);

	gst_caps_unref(pFiltercaps);

	// Create the video converter
	pData->pPTZVideoConvert = gst_element_factory_make ("nvvidconv", "PTZ_video_convert_element");
	g_assert (pData->pPTZVideoConvert);
	//g_object_set (G_OBJECT (pData->pPTZVideoConvert), "force-aspect-ratio", "true", NULL);

	// Create the filter after nvvidconv
	pFiltercaps = gst_caps_new_simple("video/x-nv-yuv",
					"framerate", GST_TYPE_FRACTION, 30, 1,
					//"width", G_TYPE_INT, pData->nCaptureWidth,
					//"height", G_TYPE_INT, pData->nCaptureHeight,
					"width", G_TYPE_INT, pData->nEncodeWidth,
					"height", G_TYPE_INT, pData->nEncodeHeight,
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
					NULL);
	g_assert (pFiltercaps);

	pData->pPTZPostVideoConvertFilter = gst_element_factory_make ("capsfilter", "PTZ_post_video_convert_filter_element");
	g_assert (pData->pPTZPostVideoConvertFilter);
	g_object_set (G_OBJECT(pData->pPTZPostVideoConvertFilter), "caps", pFiltercaps, NULL);
	gst_caps_unref(pFiltercaps);

	// Build and link pipeline
	gst_bin_add_many (
		GST_BIN (pData->pCaptureBin),
		pData->pCaptureSource,
		pData->pPTZPreVideoConvertFilter,
		pData->pPTZVideoConvert,
		pData->pPTZPostVideoConvertFilter,
		NULL);

	// link all the elements
	bool bLinked = gst_element_link_many(
			pData->pCaptureSource,
			pData->pPTZPreVideoConvertFilter,
			pData->pPTZVideoConvert,
			pData->pPTZPostVideoConvertFilter,
			NULL);

	if (!bLinked)
	{
		printf ("Capture bin didn't link\n");
	}

	// Add ghost padd
	GstPad *pPad;
	pPad = gst_element_get_static_pad (pData->pPTZPostVideoConvertFilter, "src");
	gst_element_add_pad (pData->pCaptureBin, gst_ghost_pad_new ("src", pPad));
	gst_object_unref (GST_OBJECT (pPad));

}


static void DisplayBinCreate (
	TestData * pData)
{
	GstCaps* pFiltercaps;

	// display bin
	pData->pDisplayBin = gst_bin_new ("display_bin");
	g_assert (pData->pDisplayBin);

	// Create the video converter
	pData->pDisplayVideoConvert = gst_element_factory_make ("nvvidconv", "display_video_convert_element");
	g_assert (pData->pDisplayVideoConvert);

	// Create the filter after nvvidconv
	pFiltercaps = gst_caps_new_simple("video/x-nv-yuv",
					"width", G_TYPE_INT, pData->nDisplayWidth,
					"height", G_TYPE_INT, pData->nDisplayHeight,
					"format", GST_TYPE_FOURCC, GST_MAKE_FOURCC('I','4','2','0'),
					NULL);
	g_assert (pFiltercaps);

	pData->pDisplayPostVideoConvertFilter = gst_element_factory_make("capsfilter", "display_post_convert_filter_element");
	g_assert(pData->pDisplayPostVideoConvertFilter);
	g_object_set(G_OBJECT (pData->pDisplayPostVideoConvertFilter), "caps", pFiltercaps, NULL);

	gst_caps_unref(pFiltercaps);

	pData->pDisplaySink = gst_element_factory_make ("nv_omx_hdmi_videosink", "display_sink_element");
	g_assert(G_OBJECT (pData->pDisplaySink));

	g_object_set(G_OBJECT (pData->pDisplaySink), "overlay", 1, NULL);

	g_object_set(G_OBJECT (pData->pDisplaySink), "force-aspect-ratio", TRUE, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "sync", 0, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "async", 0, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "enable-last-buffer", FALSE, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "overlay-x", pData->nDisplayXPos, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "overlay-y", pData->nDisplayYPos, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "overlay-w", pData->nDisplayWidth, NULL);
	g_object_set(G_OBJECT (pData->pDisplaySink), "overlay-h", pData->nDisplayHeight, NULL);

	// link decode display pipeline
	gst_bin_add_many (
			GST_BIN (pData->pDisplayBin),
			pData->pDisplayVideoConvert,
			pData->pDisplayPostVideoConvertFilter,
			pData->pDisplaySink,
			NULL);

	bool bLinked = gst_element_link_many (
			pData->pDisplayVideoConvert,
			pData->pDisplayPostVideoConvertFilter,
			pData->pDisplaySink,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pDisplayBin didn't link\n");
	}

	// Add ghost padd
	GstPad *pPad;
	pPad = gst_element_get_static_pad (pData->pDisplayVideoConvert, "sink");
	gst_element_add_pad (pData->pDisplayBin, gst_ghost_pad_new ("sink", pPad));
	gst_object_unref (GST_OBJECT (pPad));

}

static void SourceBinDestroy (
	TestData * pData)
{
}


static void EncoderBinDestroy (
	TestData * pData)
{

}

static void DecoderBinDestroy (
	TestData * pData)
{

}

static void DisplayBinDestroy (
	TestData * pData)
{

}
#if 0
static void PipelineCreate (
	TestData * pData)
{
	gst_object_unref (pData->pEncodePipeline);
	gst_object_unref (pData->pDecodePipeline);
}

static void PipelineDestroy (
	TestData * pData)
{
	gst_object_unref (pData->pSourcePipeline);
}
#endif

static void Setup (
	TestData * pData)
{
	pData->nCaptureWidth = 1920;
	pData->nCaptureHeight = 1072;

	pData->nEncodeWidth = 704;
	pData->nEncodeHeight = 480;
	pData->nEncodeBitrate = 256000;

	pData->nDisplayWidth = 704;
	pData->nDisplayHeight = 480;
	pData->nDisplayXPos = 0;
	pData->nDisplayYPos = 0;
	pData->nDisplayLayer = 1;

	pData->pEncodePipeline = gst_pipeline_new ("encode_pipeline");
	g_assert (pData->pEncodePipeline);

	pData->pDecodePipeline = gst_pipeline_new ("decode_pipeline");
	g_assert (pData->pDecodePipeline);

	SourceBinCreate (pData);

	EncoderBinCreate (pData);

	DecoderBinCreate (pData);

	DisplayBinCreate (pData);

	// link encode pipeline
	gst_bin_add_many (
			GST_BIN (pData->pEncodePipeline),
			pData->pCaptureBin,
			pData->pEncodeBin,
			NULL);


	bool bLinked = gst_element_link_many (
			pData->pCaptureBin,
			pData->pEncodeBin,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pSourcePipeline didn't link\n");
	}

	// link decode pipeline
	gst_bin_add_many (
			GST_BIN (pData->pDecodePipeline),
			pData->pDecodeBin,
			pData->pDisplayBin,
			NULL);

	bLinked = gst_element_link_many (
			pData->pDecodeBin,
			pData->pDisplayBin,
			NULL);

	if (!bLinked)
	{
		printf ("pData->pSourcePipeline didn't link\n");
	}

	gst_element_set_state(pData->pEncodePipeline, GST_STATE_PLAYING);

	gst_element_set_state(pData->pDecodePipeline, GST_STATE_PLAYING);

	pData->bRunning = TRUE;
	pData->Test0Thread = g_thread_create (Test0Function, pData, TRUE, NULL);
	//data->swap_thread = g_thread_create (TestFunction1, pData, TRUE, NULL);

}

static void Teardown (
		TestData * pData)
{
	pData->bRunning = FALSE;

	g_thread_join (pData->Test0Thread);
//  g_thread_join (pData->data->TestFunction1);

	gst_element_set_state(pData->pEncodePipeline, GST_STATE_NULL);
	gst_element_set_state(pData->pDecodePipeline, GST_STATE_NULL);

	SourceBinDestroy (pData);

	EncoderBinDestroy (pData);

	DecoderBinDestroy (pData);

	DisplayBinDestroy (pData);

	gst_object_unref (pData->pEncodePipeline);
	gst_object_unref (pData->pDecodePipeline);


}



void
test_encoder (void)
{
	TestData data;
	Setup (&data);

	g_usleep (G_USEC_PER_SEC * 40);

	Teardown (&data);
}

int main(int argc, char *argv[])
{

	gst_init (&argc, &argv);

	guint major;
	guint minor;
	guint micro;
	guint nano;

	gst_version(&major, &minor, &micro, &nano);

	printf("GStreamer %d.%d.%d %s\n",
		   major, minor, micro, (nano == 1)?"(CVS)":(nano == 2)?"(Prerelease)":"");

	test_encoder ();

	return 0;
}

