// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gst/video/video.h>
//#include <gst/gstcamera3ainterface.h>

#include "CaptureBin.h"
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"
#include "GStreamerElementFactory.h"


#define CAPTURE_SRC_VP4 "/dev/video2"

#define PTZ_IMX_ELEMENT_NAME "capture_ptz_imx_element"


#define stiMAX_CAPTURE_WIDTH 3840
#define stiMAX_CAPTURE_HEIGHT 2160

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
	m_cropScaleBin ("capture_ptz", false)

{
	// trace level default=1 (errors); can go up to 3 (debug)
	m_vvcam.traceLevelSet(MIN((int)g_stiVideoInputDebug + 1, 3));

	m_vvcam.initialize();
}

CaptureBin::~CaptureBin ()
{
}

int CaptureBin::maxCaptureWidthGet () const
{
	if (g_stiCaptureWidth)
	{
		return g_stiCaptureWidth;
	}

	return stiMAX_CAPTURE_WIDTH;
}

int CaptureBin::maxCaptureHeightGet () const
{
	if (g_stiCaptureHeight)
	{
		return g_stiCaptureHeight;
	}

	return stiMAX_CAPTURE_HEIGHT;
}

stiHResult CaptureBin::cropRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::cropRectangleSet\n");
	);
	
	CaptureBinBase2::cropRectangleSet(ptzRectangle);

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
	GStreamerElement cameraSource = GStreamerElement ("v4l2src", CAMERA_ELEMENT_NAME);

	cameraSource.propertySet ("device", CAPTURE_SRC_VP4);

	return cameraSource;
}

GStreamerCaps CaptureBin::captureCapsGet()
{
	std::stringstream str;

	str << "video/x-raw,format=NV12,width=" << maxCaptureWidthGet()
		<< ",height=" << maxCaptureHeightGet()
		<< ",framerate=30/1";

	return GStreamerCaps (str.str());
}

stiHResult CaptureBin::brightnessGet (
	int *brightness) const
{
	auto hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBinBase::brightnessGet:\n");
	);

	int vvcamBrightness {0};

	if (!empty())
	{
		if (m_vvcam.brightnessGet(vvcamBrightness))
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::brightnessGet: Error vvcam failed.\n");
		}
	}

	*brightness = vvcamBrightness;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBin::brightnessGet: brightness = %d\n", brightness);
	);

STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::brightnessSet (
	int brightness)
{
	auto hResult {stiRESULT_SUCCESS};

	hResult = CaptureBinBase2::brightnessSet(brightness);
	stiTESTRESULT();

	if (!empty())
	{
		if (m_vvcam.brightnessSet(brightness))
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::brightnessSet: Error vvcam failed. brightness = %d\n", brightness);
		}
	}

STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::saturationGet (
	int *saturation) const
{
	auto hResult {stiRESULT_SUCCESS};

	float vvcamSaturation {1.0};

	if (!empty())
	{
		if (m_vvcam.saturationGet(vvcamSaturation))
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::saturationGet: Error vvcam failed.\n");
		}
	}

	*saturation = (int)(vvcamSaturation * 100.0);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBin::saturationGet: vvcamSaturation = %f\n", vvcamSaturation);
		stiTrace ("CaptureBin::saturationGet: saturation = %d\n", saturation);
	);

STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::saturationSet (
	int saturation)
{
	auto hResult {stiRESULT_SUCCESS};
	float vvcamSaturation {0.0};

	hResult = CaptureBinBase2::saturationSet (saturation);
	stiTESTRESULT();

	vvcamSaturation = (float)saturation / 100.0;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBin::saturationSet - vvcamSaturation: %f\n", vvcamSaturation);
		stiTrace ("CaptureBin::saturationSet - saturation: %d\n", saturation);
	);

	if (!empty())
	{
		if (m_vvcam.saturationSet(vvcamSaturation))
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::saturationSet: Error vvcam failed. saturation = %d\n", saturation);
		}
	}

STI_BAIL:

	return hResult;
}

/*
 * brief get the focus range
 */
stiHResult CaptureBin::focusRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	range->min = MIN_FOCUS_VALUE;
	range->max = MAX_FOCUS_VALUE;
	range->step = (100.0 / (MAX_FOCUS_VALUE - MIN_FOCUS_VALUE)) * DAC_STEP_SIZE;
	range->nDefault = DEFAULT_FOCUS_VALUE;
	
	return hResult;
};

/*
 * brief get the current focus position
 */
stiHResult CaptureBin::focusPositionGet (
	int *focusPosition) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	uint32_t focusPos = 0;
	if (m_vvcam.focusGet(focusPos))
	{
		stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::focusPositionGet - Error, vvcam failed\n");
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("CaptureBin::focusPositionGet - Focus Position: %u\n", focusPos);
	);

	*focusPosition = focusPos;
	
STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::focusPositionSet (
	int focusPosition)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBin::focusPositionSet - focusValue: %d\n", focusPosition);
	);
	
	if (focusPosition < MIN_FOCUS_VALUE || focusPosition > MAX_FOCUS_VALUE)
	{
		stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "CaptureBin::focusPositionSet - Focus position outside of range: %d\n", focusPosition);
	}

	if (m_vvcam.focusSet(focusPosition))
	{
		stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::focusPositionSet: Error vvcam failed. focusPosition = %d\n", focusPosition);
	}

STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::singleFocus (
	int initialFocusPosition,
	int focusRegionStartX,
	int focusRegionStartY,
	int focusRegionWidth,
	int focusRegionHeight,
	std::function<void(bool success, int contrast)> callback)
{
	auto hResult {stiRESULT_SUCCESS};

	if (!empty())
	{
		std::lock_guard<std::mutex> lock (m_cameraSrcMutex);

		stiTESTCONDMSG (!m_focusInProcess, stiRESULT_TASK_ALREADY_STARTED, "focus is already running");
		//stiTESTCONDMSG (!m_runContrastLoop, stiRESULT_TASK_STARTUP_FAILED, "cannot focus while contrast loop is running");

		m_focusInProcess = true;
		std::thread ([this, initialFocusPosition, focusRegionStartX, focusRegionStartY, focusRegionWidth, focusRegionHeight, callback]
							{
								focusTask (
									initialFocusPosition,
									focusRegionStartX,
									focusRegionStartY,
									focusRegionWidth,
									focusRegionHeight,
									callback);
							}
						).detach ();
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "CaptureBin::singleFocus() - captureBin is empty");
	}

STI_BAIL:

	return hResult;
}

void CaptureBin::focusTask (
	int initialFocusPosition,
	int focusRegionStartX,
	int focusRegionStartY,
	int focusRegionWidth,
	int focusRegionHeight,
	std::function<void(bool success, int contrast)> callback)
{
	auto hResult {stiRESULT_SUCCESS};
	stiUNUSED_ARG (hResult);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusTask\n");
	);
	
 	int contrast {0};
 	int focusReturn {1};
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: calling focusPositionSet with initialFocusPosition = ", initialFocusPosition, "\n");
	);
	
	// If initialFocusPosition is -1 that means we should
	// just start from the current position
	if (initialFocusPosition > 0)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
				vpe::trace ("CaptureBin::focusTask: calling focusPositionSet with initialFocusPosition = ", initialFocusPosition, "\n");
		);

		// Set position
		focusPositionSet (initialFocusPosition);
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			int currentFocusPosition {0};
			focusPositionGet (&currentFocusPosition);
			vpe::trace ("CaptureBin::focusTask: starting from current position = ", currentFocusPosition, "\n");
		);
	}
	
	// Set region
	camdev::CalibAf::MeasureWindow win;
	win.startX = focusRegionStartX;
    win.startY = focusRegionStartY;
    win.width = focusRegionWidth;
    win.height = focusRegionHeight;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusTask: using AF window:\n");
		vpe::trace ("focusRegion.left = ", win.startX, "\n");
		vpe::trace ("focusRegion.top = ", win.startY, "\n");
		vpe::trace ("focusRegion.width = ", win.width, "\n");
		vpe::trace ("focusRegion.height = ", win.height, "\n");
	);
	
    bool avail;
    focusReturn = m_vvcam.afAvailableGet(avail);
 	stiTESTCONDMSG (!focusReturn && avail, stiRESULT_ERROR, "CaptureBin::focusTask: Error - focus controls unavailable\n");
	
	focusReturn = m_vvcam.focusOneShot(win);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusTask: focusReturn = ", focusReturn, "\n");
	);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
			int focusPosition {0};
			focusPositionGet (&focusPosition);
			vpe::trace ("CaptureBin::focusTask: focus position after focus = ", focusPosition, "\n");
		);

STI_BAIL:

	{
		std::lock_guard<std::mutex> lock (m_cameraSrcMutex);
		m_focusInProcess = false;
	}

 	callback (focusReturn == 0, contrast);

 	return;
}
