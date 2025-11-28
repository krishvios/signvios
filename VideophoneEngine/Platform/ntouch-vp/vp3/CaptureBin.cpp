// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gst/video/video.h>
#include <gst/gstcamera3ainterface.h>

#include "CaptureBin.h"
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"
#include "GStreamerElementFactory.h"

#include "ICamera.h"

using namespace icamera;

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
	m_cropScaleBin("capture_ptz", false)
{
	focusVcmOpen ();
}

CaptureBin::~CaptureBin ()
{
	focusVcmClose ();
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
		m_cropScaleBin.cropRectangleSet(ptzRectangle, maxCaptureWidthGet(), maxCaptureHeightGet());
	}

	return hResult;
}

stiHResult CaptureBin::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	int baseBrightness = 0;

	hResult = CaptureBinBase2::brightnessSet(brightness);
	stiTESTRESULT();

	if (!empty())
	{
		auto cameraElement = elementGetByName (CAMERA_ELEMENT_NAME);
		stiTESTCONDMSG (cameraElement.get () != nullptr, stiRESULT_ERROR, "CaptureBin::brightnessSet: Error - No camera element\n");

		hResult = brightnessGet(&baseBrightness);
		stiTESTRESULT();

		cameraElement.propertySet ("brightness", static_cast<guint64>(baseBrightness));
	}
	
STI_BAIL:
	
	return hResult;
}


stiHResult CaptureBin::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	int baseSaturation = 0;
	
	hResult = CaptureBinBase2::saturationSet(saturation);
	stiTESTRESULT();

	if (!empty())
	{
		auto cameraElement = elementGetByName (CAMERA_ELEMENT_NAME);
		stiTESTCONDMSG (cameraElement.get () != nullptr, stiRESULT_ERROR, "CaptureBin::saturationSet: Error - No camera element\n");

		hResult = saturationGet (&baseSaturation);
		stiTESTRESULT();

		cameraElement.propertySet ("saturation", static_cast<guint64>(baseSaturation));
	}
		
STI_BAIL:
	
	return hResult;
}


stiHResult CaptureBin::exposureWindowSet (const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = CaptureBinBase2::exposureWindowSet(exposureWindow);
	stiTESTRESULT();

	if (!empty())
	{
		auto cameraElement = elementGetByName (CAMERA_ELEMENT_NAME);
		stiTESTCONDMSG (cameraElement.get () != nullptr, stiRESULT_ERROR, "CaptureBin::exposureWindowSet: Error - No camera element\n");
		
		exposureWindowApply(cameraElement, exposureWindow);
	}
		
STI_BAIL:
	
	return hResult;
}

void CaptureBin::exposureWindowApply(
	GStreamerElement cameraElement,
	const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
	// The window values already consider mirroring of selfview
	auto aeRegionString {
		std::to_string (exposureWindow.horzPos) + ',' +
		std::to_string (exposureWindow.vertPos) + ',' +
		std::to_string (exposureWindow.horzPos + exposureWindow.horzZoom) + ',' +
		std::to_string (exposureWindow.vertPos + exposureWindow.vertZoom) + ',' +
		std::to_string (0) + ';'}; // Weight, this parameter does nothing. Semicolon is required.

	cameraElement.propertySet ("ae-region", aeRegionString);
}

stiHResult CaptureBin::focusVcmOpen ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusVcmOpen\n");
	);
	
	if (m_vcmFd < 0)
	{
		FILE *file = popen ("find /sys/devices/pci0000\\:00/0000\\:00\\:03.0/intel-ipu4-mmu0/intel-ipu4-isys0/video4linux -name name -exec grep 'vcm-mcp4725 b' {} /dev/null \\; | sed 's_.*\\(v4l-subdev\\)\\([0-9]*\\).*_/dev/\\1\\2_'", "re");
		
		if (file)
		{
			char subdevice[32];
			
			if (1 == fscanf (file, "%s", subdevice))
			{
				m_vcmFd = open (subdevice, O_RDWR);
			}

			pclose (file);
		}

		if (m_vcmFd < 0)
		{
			stiTHROWMSG(stiRESULT_ERROR, "CaptureBin::focusVcmOpen: (m_vcmFd < 0)\n");
		}
	}

STI_BAIL:

	return (hResult);
}


void CaptureBin::focusVcmClose ()
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusVcmClose\n");
	);
	
	if (m_vcmFd > -1)
	{
		close (m_vcmFd);
		m_vcmFd = -1;
	}
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
		stiTESTCONDMSG (!m_runContrastLoop, stiRESULT_TASK_STARTUP_FAILED, "cannot focus while contrast loop is running");

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

/*
 * brief get the current focus position
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

	v4l2_control ctrl {V4L2_CID_FOCUS_ABSOLUTE, -1};

	stiTESTCOND_NOLOG (m_vcmFd > -1, stiRESULT_ERROR);

	if (ioctl (m_vcmFd, VIDIOC_G_CTRL, &ctrl) < 0)
	{
		stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::focusPositionGet - Error, ioctl failed\n");
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("CaptureBin::focusPositionGet - Focus Position: %d\n", ctrl.value);
	);

	*focusPosition = ctrl.value;
	
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

	stiTESTCOND_NOLOG (m_vcmFd > -1, stiRESULT_ERROR);

	{
		v4l2_control ctrl {V4L2_CID_FOCUS_ABSOLUTE, focusPosition};
		
		if (ioctl (m_vcmFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CaptureBin::focusPositionSet: Error ioctl failed. focusPosition = %d\n", focusPosition);
		}
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
	
	GstCamerasrc3A *cameraSrc3a;
	GstCamerasrc3AInterface *cameraSrc3aInterface;
	
	icamera::camera_window_t focusRegion;
	icamera::camera_window_list_t focusRegions;
	icamera::camera_af_trigger_t focusTrigger; 
	icamera::camera_af_state_t focusState {icamera::AF_STATE_FAIL};
	int contrast {0};
	gboolean focusReturn {false};
	int tries {0};
	
	auto cameraElement = elementGetByName (CAMERA_ELEMENT_NAME);
	stiTESTCONDMSG (cameraElement.get () != nullptr, stiRESULT_ERROR, "CaptureBin::focusTask: Error - No camera element\n");
			
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: elementGetByName successful\n");
	);
		
	cameraSrc3a = GST_CAMERASRC_3A (cameraElement.get ());
	stiTESTCOND (cameraSrc3a != nullptr, stiRESULT_ERROR);
	
	cameraSrc3aInterface = GST_CAMERASRC_3A_GET_INTERFACE(cameraSrc3a);
	stiTESTCOND (cameraSrc3aInterface != nullptr, stiRESULT_ERROR);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: GST_CAMERASRC_3A_GET_INTERFACE successful\n");
	);
	
	focusReturn = cameraSrc3aInterface->set_scene_mode (cameraSrc3a, icamera::SCENE_MODE_ULL);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: set_scene_mode returned: ", focusReturn, "\n");
	);
	
	// Set mode
	focusReturn = cameraSrc3aInterface->set_af_mode (cameraSrc3a, icamera::AF_MODE_AUTO);
		
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: set_af_mode returned: ", focusReturn, "\n");
	);
		
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
	focusRegion.left = focusRegionStartX;
	focusRegion.top = focusRegionStartY;
	focusRegion.right = focusRegionStartX + focusRegionWidth;
	focusRegion.bottom = focusRegionStartY + focusRegionHeight;
	focusRegion.weight = 1;
	focusRegions.push_back(focusRegion);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusTask: calling set_af_region\n");
		vpe::trace ("focusRegion.left = ", focusRegion.left, "\n");
		vpe::trace ("focusRegion.top = ", focusRegion.top, "\n");
		vpe::trace ("focusRegion.right = ", focusRegion.right, "\n");
		vpe::trace ("focusRegion.bottom = ", focusRegion.bottom, "\n");
		vpe::trace ("focusRegion.weight = ", focusRegion.weight, "\n");
	);
	
	
	focusReturn = cameraSrc3aInterface->set_af_region (cameraSrc3a, focusRegions);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: set_af_region returned: ", focusReturn, "\n");
	);
	
	/*
	focusReturn = cameraSrc3aInterface->get_af_region (cameraSrc3a, focusRegions);
	
	focusRegion = focusRegions.at(0);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::focusTask: get_af_region returned: ", focusReturn, "\n");
		vpe::trace ("focusRegion.left = ", focusRegion.left, "\n");
		vpe::trace ("focusRegion.top = ", focusRegion.top, "\n");
		vpe::trace ("focusRegion.right = ", focusRegion.right, "\n");
		vpe::trace ("focusRegion.bottom = ", focusRegion.bottom, "\n");
		vpe::trace ("focusRegion.weight = ", focusRegion.weight, "\n");
	);
	*/

	// Set AF_TRIGGER_START
	focusReturn = cameraSrc3aInterface->set_af_trigger (cameraSrc3a, icamera::AF_TRIGGER_START);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: set_af_trigger AF_TRIGGER_START returned: ", focusReturn, "\n");
	);
	
	focusReturn = cameraSrc3aInterface->get_af_trigger (cameraSrc3a, focusTrigger);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: get_af_trigger returned: ", focusReturn, "\n");
	);
	
	if (icamera::AF_TRIGGER_START != focusTrigger)
	{
		vpe::trace ("CaptureBin::focusTask: icamera::AF_TRIGGER_START != focusTrigger: focusTrigger = ", focusTrigger, "\n");
	}
	
	usleep (40000);
	
	// Set AF_TRIGGER_IDLE
	focusReturn = cameraSrc3aInterface->set_af_trigger (cameraSrc3a, icamera::AF_TRIGGER_IDLE);
		
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: set_af_trigger AF_TRIGGER_IDLE returned: ", focusReturn, "\n");
	);
	
	focusReturn = cameraSrc3aInterface->get_af_trigger (cameraSrc3a, focusTrigger);
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::focusTask: get_af_trigger returned: ", focusReturn, "\n");
	);
	
	if (icamera::AF_TRIGGER_IDLE != focusTrigger)
	{
		vpe::trace ("CaptureBin::focusTask: icamera::AF_TRIGGER_IDLE != focusTrigger: focusTrigger = ", focusTrigger, "\n");
	}
		
	while (tries++ < MAX_FOCUS_TRIES)
	{
		usleep (FOCUS_WAIT_TIME_IN_MICROSECONDS); //Sleep for approximately one frame

		// Check af state
		focusReturn = cameraSrc3aInterface->get_af_state (cameraSrc3a, focusState, contrast);
		
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CaptureBin::focusTask: try = ", tries, " focusReturn = ", focusReturn, " contrast = ", contrast, "\n");
			
			switch (focusState)
			{
				case AF_STATE_IDLE:
					vpe::trace ("AF_STATE_IDLE\n");
					break;
				case AF_STATE_LOCAL_SEARCH:
					vpe::trace ("AF_STATE_LOCAL_SEARCH\n");
					break;
				case AF_STATE_EXTENDED_SEARCH:
					vpe::trace ("AF_STATE_EXTENDED_SEARCH\n");
					break;
				case AF_STATE_SUCCESS:
					vpe::trace ("AF_STATE_SUCCESS\n");
					break;
				case AF_STATE_FAIL:
					vpe::trace ("AF_STATE_FAIL\n");
					break;
			}
		);

		// AF_STATE_SUCCESS means finished
		// AF_STATE_FAIL means failed
		if (focusReturn)
		{
			if (icamera::AF_STATE_SUCCESS == focusState)
			{
				stiDEBUG_TOOL (g_stiVideoInputDebug,
					vpe::trace ("CaptureBin::focusTask: auto focus successful after ", tries * FOCUS_WAIT_TIME_IN_MICROSECONDS / 1000000.0, " seconds\n");
				);
				break;
			}
		
			if (icamera::AF_STATE_FAIL == focusState)
			{
				stiDEBUG_TOOL (g_stiVideoInputDebug,
					vpe::trace ("CaptureBin::focusTask: auto focus failed after ", tries * FOCUS_WAIT_TIME_IN_MICROSECONDS / 1000000.0, " seconds\n");
				);
				break;
			}
		}
	}
	
	if (icamera::AF_STATE_SUCCESS != focusState &&
		icamera::AF_STATE_FAIL != focusState)
	{
		// If timed out, stop the focus routine
		cameraSrc3aInterface->set_af_trigger (cameraSrc3a, icamera::AF_TRIGGER_CANCEL);
		
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CaptureBin::focusTask: focus timed out after ", tries * FOCUS_WAIT_TIME_IN_MICROSECONDS / 1000000.0, " seconds\n");
		);
	}
	
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

	callback (focusReturn == 1 && icamera::AF_STATE_SUCCESS == focusState, contrast);

	return;
}

stiHResult CaptureBin::contrastLoopStart (
	std::function<void(int contrast)> callback)
{
	auto hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::contrastLoopStart\n");
	);

	std::lock_guard<std::mutex> lock (m_cameraSrcMutex);

	stiTESTCONDMSG (!m_runContrastLoop, stiRESULT_TASK_ALREADY_STARTED, "contrast loop is already running");
	stiTESTCONDMSG (!m_focusInProcess, stiRESULT_TASK_STARTUP_FAILED, "cannot run contrast loop while focus is in process");

	m_runContrastLoop = true;
	m_contrastThread = std::thread ([this, callback]
									{
										contrastTask (callback);
									});

STI_BAIL:

	return hResult;
}

stiHResult CaptureBin::contrastLoopStop ()
{
	auto hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CaptureBin::contrastLoopStop\n");
	);

	{
		// Scoping is necessary to properly shut down thread
		std::lock_guard<std::mutex> lock (m_cameraSrcMutex);

		stiTESTCONDMSG (m_runContrastLoop, stiRESULT_ERROR, "contrast loop is not running");

		m_runContrastLoop = false;
		m_contrastCondVar.notify_one ();
	}

	if (m_contrastThread.joinable ())
	{
		m_contrastThread.join ();
	}

STI_BAIL:

	return hResult;
}

void CaptureBin::contrastTask (
	std::function<void(int contrast)> callback)
{
	std::unique_lock<std::mutex> lock (m_cameraSrcMutex);
	
	auto hResult {stiRESULT_SUCCESS};
	stiUNUSED_ARG (hResult);
	
	GStreamerElement cameraElement;
	GstCamerasrc3A *cameraSrc3a;
	GstCamerasrc3AInterface *cameraSrc3aInterface;

	icamera::camera_af_state_t focusState {icamera::AF_STATE_FAIL};
	auto contrast {0};

	stiTESTCONDMSG (!m_focusInProcess, stiRESULT_ERROR, "Trying to get contrast while focusing is not allowed");

	cameraElement = elementGetByName (CAMERA_ELEMENT_NAME);
	stiTESTCONDMSG (cameraElement.get () != nullptr, stiRESULT_ERROR, "CaptureBin::contrastTask: Error - No camera element\n");

	cameraSrc3a = GST_CAMERASRC_3A (cameraElement.get ());
	stiTESTCOND (cameraSrc3a != nullptr, stiRESULT_ERROR);

	cameraSrc3aInterface = GST_CAMERASRC_3A_GET_INTERFACE(cameraSrc3a);
	stiTESTCOND (cameraSrc3aInterface != nullptr, stiRESULT_ERROR);

	// Set auto focus mode
	cameraSrc3aInterface->set_af_mode (cameraSrc3a, icamera::AF_MODE_AUTO);

	// Set mfg mode
	cameraSrc3aInterface->set_af_manufacture (cameraSrc3a, true);

	while (m_runContrastLoop)
	{
		// Set AF_TRIGGER_START
		cameraSrc3aInterface->set_af_trigger (cameraSrc3a, icamera::AF_TRIGGER_START);

		// Wait 40ms before setting trigger to idle
		if (m_contrastCondVar.wait_for (lock, std::chrono::milliseconds (40), [this]{return !m_runContrastLoop;}))
		{
			// If m_runContrastLoop is no longer set
			break;
		}

		// Set AF_TRIGGER_IDLE
		cameraSrc3aInterface->set_af_trigger (cameraSrc3a, icamera::AF_TRIGGER_IDLE);

		while (m_runContrastLoop)
		{
			if (m_contrastCondVar.wait_for (lock, std::chrono::milliseconds (500), [this]{return !m_runContrastLoop;}))
			{
				// If m_runContrastLoop is no longer set
				break;
			}

			cameraSrc3aInterface->get_af_state (cameraSrc3a, focusState, contrast);

			callback (contrast);

			// If state is returned as AF_STATE_SUCCESS or AF_STATE_FAIL, the routine stops
			// updating the contrast value. Auto focus should be triggered again.
			if (focusState == icamera::AF_STATE_SUCCESS ||
				focusState == icamera::AF_STATE_FAIL)
			{
				break;
			}
		}
	}

	cameraSrc3aInterface->set_af_manufacture (cameraSrc3a, false);

STI_BAIL:

	return;
}

/**
 * Get camera name which uses imx274_mcp4725-2 as default if not provided.
 * Return camera name which is never NULL.
 */
const std::string currentCameraNameGet ()
{
	const char* CAMERA_INPUT = "cameraInput";
	const char *input = getenv(CAMERA_INPUT);
	
	if (!input)
	{
		input = "imx274_mcp4725-2";
	}

	return input;
}

int currentCameraIdGet ()
{
	int cameraId = 0;
	const std::string input = currentCameraNameGet ();
	int count = get_number_of_cameras();
	int id = 0;
	
	for (; id < count; id++)
	{
		camera_info_t info;
		memset (&(info), 0, sizeof (info));
		
		get_camera_info (id, info);
		
		if (input.compare(info.name) == 0)
		{
			cameraId = id;
			break;
		}
	}

	return cameraId;
}

stiHResult CaptureBin::ambientLightGet (
	IVideoInputVP2::AmbientLight &ambientLight,
	float &rawAmbientLight)
{
	auto hResult {stiRESULT_SUCCESS};
	
	int ret = 0;
	int cameraId = 0;
	float sensitivity = 0.0;
	int32_t iso = 0;
	float isoInverse = 0.0;
	IVideoInputVP2::AmbientLight newAmbientLight = IVideoInputVP2::AmbientLight::HIGH;
	Parameters parameter;
	
	// This is left here for information
	// If the gstreamer pipeline is not created then this must be called
	// As is, we will fail with the camera_get_parameters call in this case
	// but later if we want to init and de-init this is how you would do that
	//ret = camera_hal_init ();
	//stiTESTCONDMSG (ret == 0, stiRESULT_ERROR, "CaptureBin::ambientLightGet: Error - camera_hal_init\n");
	
	cameraId = currentCameraIdGet ();
	ret = camera_get_parameters (cameraId, parameter);
	stiTESTCONDMSG (ret == 0, stiRESULT_ERROR, "CaptureBin::ambientLightGet: Error - device is not open\n");
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::ambientLightGet: device is open", "\n");
	);
		
	// This code is left here to show that there are other
	// values that can be set/got
	//int64_t exposure;
	//float fps;
	//if (parameter.getExposureTime(exposure) == 0)
	//{
	//	int64_t time = exposure / 1000; // us -> ms
	//	vpe::trace ("CaptureBin::ambientLightGet: exposure time = ", time, "ms\n");
	//}
	
	//if (parameter.getFrameRate(fps) == 0)
	//{
	//	int64_t duration = MSEC_PER_SEC / fps;
	//	vpe::trace ("CaptureBin::focusTask: frame duration = ", duration, "ms\n");
	//}

	if (parameter.getSensitivityGain(sensitivity) == 0)
	{
		iso = round(pow(10, (sensitivity / 20))); // db -> ISO
	}

	if (iso >= 1)
	{
		isoInverse = 1.0 / iso;
	}
	else
	{
		isoInverse = 1.0;
	}

	if (!std::isfinite(sensitivity))
	{
		newAmbientLight = m_lastAmbientLight;
	}
	else if (isoInverse <= AMBIENT_LIGHT_LIMIT_LOW)
	{
		newAmbientLight = IVideoInputVP2::AmbientLight::LOW;
	}
	else if (isoInverse >= AMBIENT_LIGHT_LIMIT_HIGH)
	{
		newAmbientLight = IVideoInputVP2::AmbientLight::HIGH;
	}
	else if ((m_lastAmbientLight == IVideoInputVP2::AmbientLight::LOW && isoInverse >= AMBIENT_LIGHT_LIMIT_MIDLOW) ||
			(m_lastAmbientLight == IVideoInputVP2::AmbientLight::HIGH && isoInverse <= AMBIENT_LIGHT_LIMIT_MIDHIGH))
	{
		newAmbientLight = IVideoInputVP2::AmbientLight::MEDIUM;
	}
	else
	{
		newAmbientLight = m_lastAmbientLight;
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CaptureBin::ambientLightGet:\n\tsensitivity = ", sensitivity, \
		"\n\tsensitivity gain (iso) = ", iso, \
		"\n\tsensitivity gain (iso) inverse = ", isoInverse, \
		"\n\tlast ambient light = ", (int)m_lastAmbientLight, \
		"\n\tnew ambient light = ", (int)newAmbientLight, "\n");
	);
	
	m_lastAmbientLight = newAmbientLight;
	
	ambientLight = newAmbientLight;
	
	rawAmbientLight = isoInverse;
	
STI_BAIL:

//	ret = camera_hal_deinit ();

	return hResult;
}


GStreamerElement CaptureBin::cameraElementGet()
{
	int brightness {-1};
	int saturation {-1};
	GStreamerElementFactory icameraSrcFactory ("icamerasrc");

	// Create the capture source
	GStreamerElement cameraSource = icameraSrcFactory.createElement (CAMERA_ELEMENT_NAME); 

	cameraSource.propertySet ("device-name", 30);
	cameraSource.propertySet ("scene-mode", 2);
	cameraSource.propertySet ("blc-area-mode", 0);
	cameraSource.propertySet ("io-mode", 4);
	cameraSource.propertySet ("num-buffers", -1);
	cameraSource.propertySet ("af-mode", 1);
	cameraSource.propertySet ("buffer-count", 10);
	brightnessGet (&brightness);
	cameraSource.propertySet ("brightness", brightness);
	saturationGet (&saturation);
	cameraSource.propertySet ("saturation", saturation);
	cameraSource.propertySet ("converge-speed", 2); //0-NORMAL, 1-MID, 2-LOW

	return cameraSource;
}

GStreamerCaps CaptureBin::captureCapsGet()
{
	std::stringstream str;

	str << "video/x-raw,format=NV12,width=" << maxCaptureWidthGet()
		<< ",height=" << maxCaptureHeightGet()
		<< ",framerate=30/1,tiled=false";

	return GStreamerCaps (str.str());
}
