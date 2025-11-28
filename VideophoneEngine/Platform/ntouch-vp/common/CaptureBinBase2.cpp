#include "CaptureBinBase2.h"

#include <gst/video/video.h>
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"
#include "GStreamerCaps.h"
#include "GStreamerPad.h"
#include "GStreamerElementFactory.h"
#include <sstream>
#include "CropScaleBin.h"

//#define CAPTURE_SRC_ICAMERA 1
//#define CAPTURE_SRC_FAKE 2
//#define CAPTURE_SRC_GLTEST 3
//#define CAPTURE_SRC CAPTURE_SRC_FAKE

#define CAPTURE_TEST_SRC_VIDEO_TEST_SRC 1
#define CAPTURE_TEST_SRC_GL_TEST_SRC 2
#define CAPTURE_TEST_SRC CAPTURE_TEST_SRC_VIDEO_TEST_SRC
//#define CAPTURE_TEST_SRC CAPTURE_TEST_SRC_GL_TEST_SRC

CaptureBinBase2::CaptureBinBase2 (
	const PropertyRange &brightness,
	const PropertyRange &saturation)
:
	CaptureBinBase(brightness, saturation)
{
}

void CaptureBinBase2::create ()
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;
	GStreamerElement captureSource;
	GStreamerElement captureFilter;
	GStreamerElement tee;
	GStreamerElement queue0;
	GStreamerElement queue1;
	GStreamerPad selfViewSrcPad;
	GStreamerPad encodeSrcPad;
	GStreamerCaps gstCaps;

	clear ();
	GStreamerBin::create ("capture_bin");

	cropScaleBinBaseGet().create();
	stiTESTCOND(cropScaleBinBaseGet().get (), stiRESULT_ERROR);

	if (!g_stiVideoUseTestSource)
	{
		captureSource = cameraElementGet();
		stiTESTCOND(captureSource.get (), stiRESULT_ERROR);
	}
	else
	{
		captureSource = GStreamerElement ("videotestsrc", CAMERA_ELEMENT_NAME);
		stiTESTCOND(captureSource.get (), stiRESULT_ERROR);

		captureSource.propertySet ("is-live", true);
		captureSource.propertySet ("pattern", static_cast<guint64>(0));
		captureSource.propertySet ("horizontal-speed", static_cast<guint64>(10));
	}

	exposureWindowApply(captureSource, m_exposureWindow);

	gstCaps = captureCapsGet();
	stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);

	captureFilter = GStreamerElement ("capsfilter", "capture_filter_element");
	stiTESTCOND(captureFilter.get (), stiRESULT_ERROR);

	captureFilter.propertySet ("caps", gstCaps);
	gstCaps.clear ();
	
	elementAdd (captureSource);
	elementAdd (captureFilter);

	result = captureSource.link (captureFilter);
	stiTESTCOND(result, stiRESULT_ERROR);

	elementAdd(cropScaleBinBaseGet());

	result = captureFilter.link (cropScaleBinBaseGet());
	stiTESTCOND(result, stiRESULT_ERROR);

	tee = GStreamerElement ("tee", "capture_video_tee");
	stiTESTCOND(tee.get (), stiRESULT_ERROR);

	queue0 = GStreamerElement ("queue", "capture_queue0_element");
	stiTESTCOND(queue0.get (), stiRESULT_ERROR);

	queue1 = GStreamerElement ("queue", "capture_queue1_element");
	stiTESTCOND(queue1.get (), stiRESULT_ERROR);

	// Add elements to the bin

	elementAdd (tee);
	elementAdd (queue0);
	elementAdd (queue1);

	result = cropScaleBinBaseGet().link (tee);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = tee.link (queue0);
	stiTESTCOND(result, stiRESULT_ERROR);
	
	result = tee.link (queue1);
	stiTESTCOND(result, stiRESULT_ERROR);
	
	// Create source pads for the bin
	selfViewSrcPad = queue0.staticPadGet ("src");
	stiTESTCOND(selfViewSrcPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("selfviewsrc", selfViewSrcPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	encodeSrcPad = queue1.staticPadGet ("src");
	stiTESTCOND(encodeSrcPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("encodesrc", encodeSrcPad);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}


stiHResult CaptureBinBase2::exposureWindowSet (
	const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
	m_exposureWindow = exposureWindow;

	return stiRESULT_SUCCESS;
}

