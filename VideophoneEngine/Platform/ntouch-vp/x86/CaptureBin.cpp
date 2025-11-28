#include "CaptureBin.h"

#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"

CaptureBin::CaptureBin ()
:
	CaptureBinBase2 (PropertyRange (CaptureBin::MIN_BRIGHTNESS_VALUE,
									CaptureBin::MAX_BRIGHTNESS_VALUE,
									CaptureBin::DEFAULT_BRIGHTNESS_VALUE,
									CaptureBin::DEFAULT_BRIGHTNESS_STEP_SIZE),
					 PropertyRange (CaptureBin::MIN_SATURATION_VALUE,
									CaptureBin::MAX_SATURATION_VALUE,
									CaptureBin::DEFAULT_SATURATION_VALUE,
									CaptureBin::DEFAULT_SATURATION_STEP_SIZE)),
	m_cropScaleBin("capture_ptz", false, false)
{
}


stiHResult CaptureBin::cropRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	CaptureBinBase2::cropRectangleSet (ptzRectangle);

	if (!empty())
	{
		hResult = m_cropScaleBin.cropRectangleSet(ptzRectangle, maxCaptureWidthGet(), maxCaptureHeightGet());
		stiTESTRESULT();
	}

STI_BAIL:
	
	return hResult;
}


void CaptureBin::exposureWindowApply(
	GStreamerElement cameraElement,
	const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
}


GStreamerElement CaptureBin::cameraElementGet()
{
	// Create the capture source
	GStreamerElement cameraSource = GStreamerElement ("videotestsrc", CAMERA_ELEMENT_NAME);

	cameraSource.propertySet ("is-live", true);
	cameraSource.propertySet ("pattern", static_cast<guint64>(0));
	cameraSource.propertySet ("horizontal-speed", static_cast<guint64>(10));

	return cameraSource;
}

GStreamerCaps CaptureBin::captureCapsGet()
{
	return GStreamerCaps ("video/x-raw,format=NV12");
}
