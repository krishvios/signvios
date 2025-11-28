// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoInput.h"
#include "stiTrace.h"
#include "CstiMonitorTask.h"
#include "CstiCameraControl.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "CstiGStreamerVideoGraph.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/interfaces/propertyprobe.h>

constexpr const int FOCUS_WINDOW_WIDTH = 300;
constexpr const int FOCUS_WINDOW_HEIGHT = 300;

constexpr const int AE_OUTER_WINDOW_WIDTH = 1920;
constexpr const int AE_OUTER_WINDOW_HEIGHT = 1072;
constexpr const int AE_INNER_WINDOW_WIDTH = 1152;
constexpr const int AE_INNER_WINDOW_HEIGHT = 864;

constexpr const int SENSOR_IMAGE_WIDTH = 1920;
constexpr const int SENSOR_IMAGE_HEIGHT = 1080;

constexpr const int MIN_RED_OFFSET = 0;
constexpr const int MAX_RED_OFFSET = 256;
constexpr const int DEFAULT_RED_OFFSET = 125;
constexpr const int MIN_GREEN_OFFSET = 0;
constexpr const int MAX_GREEN_OFFSET = 256;
constexpr const int DEFAULT_GREEN_OFFSET = 125;
constexpr const int MIN_BLUE_OFFSET = 0;
constexpr const int MAX_BLUE_OFFSET = 256;
constexpr const int DEFAULT_BLUE_OFFSET = 125;

const int MIN_ZOOM_LEVEL = 128; // Allows up to a 2x zoom

const int DEFAULT_SELFVIEW_WIDTH = 1920;
const int DEFAULT_SELFVIEW_HEIGHT = 1072;

CstiVideoInput::CstiVideoInput ()
:
	m_gstreamerVideoGraph(
		RcuConnectedGet())
{
	m_signalConnections.push_back (m_autoFocusTimer.timeoutEvent.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoInput::EventAutoFocusTimerTimeout, this));
		}));
}

CstiVideoInput::~CstiVideoInput ()
{
	// Shutdown the timers so we don't get any events from them.
	m_autoFocusTimer.Stop();

	NoLockCameraClose ();
}

stiHResult CstiVideoInput::Initialize (
	CstiMonitorTask *pMonitorTask)
{
	stiLOG_ENTRY_NAME (CstiVideoInput::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug,
		stiTrace("CstiVideoInput::Initialize:\n");
	);
	
	m_monitorTask = pMonitorTask;

	hResult = monitorTaskInitialize ();
	stiTESTRESULT ();

	m_signalConnections.push_back (m_monitorTask->hdmiInResolutionChangedSignal.Connect (
		[this](int resolution) {
			PostEvent(
				std::bind(&CstiVideoInput::EventHdmiInResolutionChanged, this, resolution));
		}));

	hResult = rcuStatusInitialize ();
	stiTESTRESULT ();

	hResult = bufferElementsInitialize ();
	stiTESTRESULT ();

	hResult = gstreamerVideoGraphInitialize ();
	stiTESTRESULT ();

	m_signalConnections.push_back (m_gstreamerVideoGraph.aspectRatioChanged.Connect (
		[this]
		{
			videoRecordSizeChangedSignal.Emit ();
		}));

	{
		int nResolution = 0;

		hResult = pMonitorTask->HdmiInResolutionGet (&nResolution);
		stiTESTRESULT ();

		hResult = m_gstreamerVideoGraph.HdmiInResolutionSet (nResolution);
		stiTESTRESULT ();
	}

	hResult = cameraControlInitialize (m_gstreamerVideoGraph.maxCaptureWidthGet(),
			m_gstreamerVideoGraph.maxCaptureHeightGet(),
									   MIN_ZOOM_LEVEL,
									   DEFAULT_SELFVIEW_WIDTH,
									   DEFAULT_SELFVIEW_HEIGHT);

	if (RcuAvailableGet())
	{
		struct stat st;

		if (stat (g_szCameraDeviceName, &st) < 0)
		{
			stiTHROW (stiRESULT_ERROR);
		}

		if (!S_ISCHR(st.st_mode))
		{
			stiTHROW (stiRESULT_ERROR);
		}

		hResult = NoLockCameraOpen ();
		stiTESTRESULT ();
	}

	m_frameTimer.Start();

STI_BAIL:
	return hResult;
}

MonitorTaskBase *CstiVideoInput::monitorTaskBaseGet()
{
	return m_monitorTask;
}


CstiGStreamerVideoGraphBase &CstiVideoInput::videoGraphBaseGet ()
{
	return m_gstreamerVideoGraph;
}


const CstiGStreamerVideoGraphBase &CstiVideoInput::videoGraphBaseGet () const
{
	return m_gstreamerVideoGraph;
}


bool CstiVideoInput::RcuAvailableGet () const
{
	return RcuConnectedGet () && !m_bHdmiPassthrough;
}

stiHResult CstiVideoInput::BrightnessSet (
	int brightness)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::%s brightness: %d\n", __func__, brightness);
	);

	m_gstreamerVideoGraph.brightnessSet (brightness);

	return hResult;
}


stiHResult CstiVideoInput::SaturationSet (
	int saturation)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();

	m_gstreamerVideoGraph.saturationSet (saturation);

	Unlock ();

	return hResult;
}


/*!\brief Returns the coordinates and dimensions of the focus window.
 *
 */
stiHResult CstiVideoInput::FocusWindowGet (
	int *pnStartX,
	int *pnStartY,
	int *pnWidth,
	int *pnHeight)
{
	int nStartX = 0;
	int nStartY = 0;
	int nWidth = 0;
	int nHeight = 0;

	Lock ();

	//
	// Compute the new focus origin and dimensions based on the focus origin and dimensions for the max capture size.
	//
#if 0
	nStartX = m_PtzSettings.horzPos + (stiFOCUS_WINDOW_ORIGIN_X / (float)stiMAX_CAPTURE_WIDTH) * m_PtzSettings.horzZoom + 0.5f;
	nStartY = m_PtzSettings.vertPos + (stiFOCUS_WINDOW_ORIGIN_Y / (float)stiMAX_CAPTURE_HEIGHT) * m_PtzSettings.vertZoom + 0.5f;
	nWidth = (stiFOCUS_WINDOW_WIDTH / (float)stiMAX_CAPTURE_WIDTH) * m_PtzSettings.horzZoom + 0.5f;
	nHeight = (stiFOCUS_WINDOW_HEIGHT / (float)stiMAX_CAPTURE_HEIGHT) * m_PtzSettings.vertZoom + 0.5f;
#else
	nStartX = m_PtzSettings.horzPos + (m_PtzSettings.horzZoom / 2) - FOCUS_WINDOW_WIDTH / 2;
	nStartY = m_PtzSettings.vertPos + (m_PtzSettings.vertZoom / 2) - FOCUS_WINDOW_HEIGHT / 2;

	//
	// Since the self view is mirrored we need to flip the X component of the origin to the other side.
	//
	nStartX = stiMAX_CAPTURE_WIDTH - 1 - (nStartX + FOCUS_WINDOW_WIDTH - 1);

	nWidth = FOCUS_WINDOW_WIDTH;
	nHeight = FOCUS_WINDOW_HEIGHT;

	//
	// If the focus window is larger in either height or width we may need to clip the window to the edge
	// of the capture image depending on pan/tilt position.
	//
	if (FOCUS_WINDOW_WIDTH > m_PtzSettings.horzZoom || FOCUS_WINDOW_HEIGHT > m_PtzSettings.vertZoom)
	{
		//
		// Clip the window to the capture image.
		//
		if (nStartX < 0)
		{
			nWidth += nStartX;
			nStartX = 0;
		}

		if (nStartX + nWidth > stiMAX_CAPTURE_WIDTH)
		{
			nWidth -= nStartX + nWidth - stiMAX_CAPTURE_WIDTH;
		}

		if (nStartY < 0)
		{
			nHeight += nStartY;
			nStartY = 0;
		}

		if (nStartY + nHeight > stiMAX_CAPTURE_HEIGHT)
		{
			nHeight -= nStartY + nHeight - stiMAX_CAPTURE_HEIGHT;
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			stiTrace ("Clipped Focus Window: %d, %d, %d, %d\n", nStartX, nStartY, nWidth, nHeight);
		);
	}
#endif

#if 0
	//
	// The sum of X and Y must be odd.  So, if both X and Y are even then subtract
	// one from the X coordinate if X is non-zero and add one if X is zero to make X odd.
	// If Y is odd and X is odd then we will modify X to be even.
	//
	if (((nStartY) & 0x01) == 0)
	{
		if (((nStartX) & 0x01) == 0)
		{
			//
			// X and Y are both even.  Make X odd.
			//
			if (nStartX == 0)
			{
				nStartX = 1;
			}
			else
			{
				nStartX -= 1;
			}
		}
	}
	else if (((nStartX) & 0x01) == 1)
	{
		//
		// X and Y are both odd.  Make X even.
		//
		nStartX = (nStartX) & ~1;
	}
#endif

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("Zoom Window: X: %d, Y: %d, W: %d, H: %d\n", stiMAX_CAPTURE_WIDTH - 1 - (m_PtzSettings.horzPos + m_PtzSettings.horzZoom - 1), m_PtzSettings.vertPos, m_PtzSettings.horzZoom, m_PtzSettings.vertZoom);
		stiTrace ("Focus Window: X: %d, Y: %d, W: %d, H: %d\n", nStartX, nStartY, nWidth, nHeight);
	);

	Unlock ();

	*pnStartX = nStartX;
	*pnStartY = nStartY;
	*pnWidth = nWidth;
	*pnHeight = nHeight;

	return (stiRESULT_SUCCESS);
}


/*!
 *\ brief set the area to focus on
 */
stiHResult CstiVideoInput::NoLockFocusWindowSet (
	uint16_t startX,
	uint16_t startY,
	uint16_t winW,
	uint16_t winH)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	struct v4l2_control ctrl;

	/*
	 * The lowest 24 bit is used to transfer the window coordinates and size.
	 *
	 * If any MSB [31:25] bit is set, the least 24 bit is coordinates.
	 * 12 MSB bits of the above mentioned 24 LSB is X coordinate
	 * and 12LSB bit the above mentioned 24 LSB is Y coordinate
	 *
	 * If none of the MSB [31:25] bit is set,the least 24 bit is Window Sizes.
	 * 12 MSB bits of the above mentioned 24 LSB is AF Window Width.
	 * and 12LSB bit the above mentioned 24 LSB is AF Window Height.
	 *
	 */

	//
	// Set the focus origin
	//
	if (RcuConnectedGet())
	{
		ctrl.id = OV640_SET_FOCUS_WINDOW;
		ctrl.value = ((startX << 12) | startY) | 0xFF000000;

		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			stiTrace ("Focus: X = %d, Y = %d, Value = 0x%0x\n", startX, startY, ctrl.value);
		);

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		if (xioctl(m_nCameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiVideoInput::NoLockFocusWindowSet: Error xoict failed\n");
		}

		//
		// Set the focus width and height
		//
		ctrl.id = OV640_SET_FOCUS_WINDOW;
		ctrl.value = ((winW << 12) | winH) & 0x00FFFFFF;

		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			stiTrace ("Focus: W = %d, H = %d, Value = 0x%0x\n", winW, winH, ctrl.value);
		);

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		if (xioctl(m_nCameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiVideoInput::NoLockFocusWindowSet: Error xoictl failed\n");
		}
	}

STI_BAIL:

	return (hResult);
}


/*!
 *\ brief start the event that performs the single-shot focus
 */
stiHResult CstiVideoInput::SingleFocus ()
{
	PostEvent (
		std::bind(&CstiVideoInput::EventSingleFocus, this));

	return stiRESULT_SUCCESS;
}


/*!
 *\ brief the actual event that performs the focus
 */
void CstiVideoInput::EventSingleFocus ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nStartX;
	int nStartY;
	int nWinW;
	int nWinH;

//	int nIoctlErr = 0;

	struct v4l2_control ctrl;

	// get the focus window
	FocusWindowGet (&nStartX, &nStartY, &nWinW, &nWinH);

	hResult = NoLockFocusWindowSet (nStartX, nStartY, nWinW, nWinH);
	stiTESTRESULT();

	// actually perform the one-shot focus
	if (RcuConnectedGet())
	{
		ctrl.id = OV640_FOCUS_CONTROL;
		ctrl.value = OV640_SINGLE_FOCUS_MODE;

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		if (xioctl(m_nCameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			//nIoctlErr = 3;
			//The ioctl call from time to time returns an error value but it seems to work.
			//stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiVideoInput::EventSingleFocus: Error xoict failed\n");
		}
	}

STI_BAIL:

	m_autoFocusTimer.Restart();
}


stiHResult CstiVideoInput::FocusRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	range->min = MIN_FOCUS_VALUE;	// start and finish values returned
	range->max = MAX_FOCUS_VALUE; // are from the driver documentation
	range->step = 1.0;

	return hResult;
}

/*
 * brief get the current focus range
 */
int CstiVideoInput::FocusPositionGet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	struct v4l2_control ctrl;
	int nFocusPosition = -1;

	Lock ();

	if (RcuConnectedGet())
	{
		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
		ctrl.value = 0;

		if (xioctl(m_nCameraControlFd, VIDIOC_G_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiVideoInput::FocusPositionGet: Error xoictl failed\n");
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			stiTrace ("Focus Position: 0x%0x\n", ctrl.value);
		);

		nFocusPosition = ctrl.value;
	}

STI_BAIL:

	Unlock ();

	if (stiIS_ERROR(hResult))
	{
		nFocusPosition = -1;
	}

	return (nFocusPosition);
}

/*!
 *\ brief set the focus (and range) at the specified value
 */
stiHResult CstiVideoInput::FocusPositionSet (
	int nFocusValue)
{
	PostEvent (
		std::bind(&CstiVideoInput::EventFocusPositionSet, this, nFocusValue));

	return stiRESULT_SUCCESS;
}

void CstiVideoInput::EventFocusPositionSet (
	int nFocusValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bPipelinePlaying = false;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CCstiVideoInput::EventFocusPositionSet\n");
	);

	if (nFocusValue < MIN_FOCUS_VALUE || nFocusValue > MAX_FOCUS_VALUE)
	{
		stiTHROWMSG (stiRESULT_INVALID_PARAMETER, "CstiVideoInput::EventFocusPositionSet: Focus value outside of range: %d\n", nFocusValue);
	}

	m_nFocusValue = nFocusValue;

	hResult = m_gstreamerVideoGraph.PipelinePlayingGet (&bPipelinePlaying);
	stiTESTRESULT ();

	if (bPipelinePlaying)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("Setting Focus Position: 0x%0x\n", m_nFocusValue);
		);

		struct v4l2_control ctrl;

		ctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
		ctrl.value = m_nFocusValue;

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		if (xioctl(m_nCameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
		{
			stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiVideoInput::EventFocusPositionSet: Error xoictl failed. m_nFocusValue = %d\n", m_nFocusValue);
		}

		m_bCachedFocusValue = false;
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("Caching Focus Position: 0x%0x\n", m_nFocusValue);
		);

		m_bCachedFocusValue = true;
	}

STI_BAIL:
	return;
}


/*!
 * \brief Get video codec
 *
 * \param pCodecs - a std::list where the codecs will be stored.
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
#ifndef stiINTEROP_EVENT
	pCodecs->push_back (estiVIDEO_H264);
#else
	if (g_stiEnableVideoCodecs & 2)
	{
		pCodecs->push_back (estiVIDEO_H264);
	}
#endif

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Get Codec Capability
 *
 * \param eCodec
 * \param pstCaps
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H264:
		{
			auto pstH264Caps = (SstiH264Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4_1;

			break;
		}

		default:
			stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


void CstiVideoInput::EventAutoFocusTimerTimeout ()
{
	uint8_t un8Register = 0;

	stiHResult hResult = NoLockOv640RegisterGet (0x80140de4, &un8Register);

	if (stiIS_ERROR (hResult) || un8Register != 0)
	{
		//
		// Fire an assert if the value is not 1 (success)
		//
		stiASSERT (un8Register == 1);

		m_nFocusValue = FocusPositionGet ();

		focusCompleteSignal.Emit(0);
	}
	else
	{
		//
		// The focus is still busy.  Restart the timer.
		//
		m_autoFocusTimer.Restart();
	}
}


/*!\brief Sets a register on the OV640
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::NoLockOv640RegisterSet (
	uint32_t un32Register,
	uint8_t un8RegValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;

	if (RcuConnectedGet())
	{
		struct v4l2_dbg_register regtr;

		memset (&regtr, 0, sizeof(regtr));

		regtr.reg  = un32Register;
		regtr.size = 1;
		regtr.val = un8RegValue;

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		nResult = xioctl (m_nCameraControlFd, VIDIOC_DBG_S_REGISTER, &regtr);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*!\brief Gets a register from the OV640
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::NoLockOv640RegisterGet (
	uint32_t un32Register,
	uint8_t *pun8RegValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nResult = 0;

	if (RcuConnectedGet())
	{
		struct v4l2_dbg_register reg;

		memset (&reg, 0, sizeof(reg));

		reg.reg  = un32Register;
		reg.size = 1;

		stiTESTCOND (m_nCameraControlFd > -1, stiRESULT_ERROR);

		nResult = xioctl (m_nCameraControlFd, VIDIOC_DBG_G_REGISTER, &reg);
		stiTESTCOND (nResult >= 0, stiRESULT_ERROR);

		*pun8RegValue = reg.val;
	}

STI_BAIL:

	return hResult;
}


/*!\brief Scales a rectangular area centered in the viewable area based on current PTZ values
 *
 */
void CstiVideoInput::NoLockWindowScale (
	int nOriginalWidth,
	int nOriginalHeight,
	int *pnX1,
	int *pnY1,
	int *pnX2,
	int *pnY2)
{
	int nWidth = 0;
	int nHeight = 0;
	int nX1 = 0;
	int nY1 = 0;
	int nX2 = 0;
	int nY2 = 0;

	nWidth = (int)((m_PtzSettings.horzZoom / (float)stiMAX_CAPTURE_WIDTH) * nOriginalWidth);
	nHeight = (int)((m_PtzSettings.vertZoom / (float)stiMAX_CAPTURE_HEIGHT) * nOriginalHeight);

	//
	// Compute the new AE origin and dimensions based on the PTZ origin and dimensions for the max capture size.
	//
	nX1 = m_PtzSettings.horzPos + (m_PtzSettings.horzZoom / 2) - nWidth / 2;
	nY1 = m_PtzSettings.vertPos + (m_PtzSettings.vertZoom / 2) - nHeight / 2;

	nX2 = nX1 + nWidth;
	nY2 = nY1 + nHeight;

	//
	// Since the self view is mirrored we need to flip the X component of the origin to the other side.
	//
	nX1 = stiMAX_CAPTURE_WIDTH - 1 - (nX1 + nWidth - 1);
	nX2 = stiMAX_CAPTURE_WIDTH - 1 - (nX2 - nWidth - 1);

	*pnX1 = nX1;
	*pnY1 = nY1;
	*pnX2 = nX2;
	*pnY2 = nY2;
}


/*!\brief Sets the position of the auto exposure windows
 *
 * \return stiHResult
 */
stiHResult CstiVideoInput::NoLockAutoExposureWindowSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nValue;

	//
	// The outer window is set to the entire capture area.
	//
	int nOuterX1 = 0;
	int nOuterY1 = 0;
	int nOuterX2 = 0;
	int nOuterY2 = 0;

	int nInnerX1 = 0;
	int nInnerY1 = 0;
	int nInnerX2 = 0;
	int nInnerY2 = 0;

	//
	// Scale and position the outer and inner windows based on the current PTZ values
	//
	NoLockWindowScale (AE_OUTER_WINDOW_WIDTH, AE_OUTER_WINDOW_HEIGHT, &nOuterX1, &nOuterY1, &nOuterX2, &nOuterY2);
	NoLockWindowScale (AE_INNER_WINDOW_WIDTH, AE_INNER_WINDOW_HEIGHT, &nInnerX1, &nInnerY1, &nInnerX2, &nInnerY2);

	//
	// The inner window's x and y are offsets from the outer window and uses width and height instead of the coordinate for the lower
	// right corner.
	//
	// Adjust the inner window so that it is sitting on the bottom edge of the outer window.
	//
	int nInnerWidth = nInnerX2 - nInnerX1;
	int nInnerHeight = nInnerY2 - nInnerY1;

	nInnerX1 -= nOuterX1;
	nInnerY1 = nOuterY2 - nInnerHeight - nOuterY1;

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("Zoom Window: X: %d, Y: %d, W: %d, H: %d\n", stiMAX_CAPTURE_WIDTH - 1 - (m_PtzSettings.horzPos + m_PtzSettings.horzZoom - 1), m_PtzSettings.vertPos, m_PtzSettings.horzZoom, m_PtzSettings.vertZoom);
		stiTrace ("AE Outer Window: Upper Left: (%d, %d), Lower Right: (%d, %d)\n", nOuterX1, nOuterY1, nOuterX2, nOuterY2);
		stiTrace ("AE Inner Window: Upper Left: (%d, %d), Width, Height: (%d, %d)\n", nInnerX1, nInnerY1, nInnerWidth, nInnerHeight);
	);

	//
	// Set outer window
	//

	// Top Left coordinates
	NoLockOv640RegisterSet(0x80226401, (nOuterX1 >> 8) & 0x1F);	// 12:8 X
	NoLockOv640RegisterSet(0x80226402, nOuterX1 & 0xFF);			// 7:0 X
	NoLockOv640RegisterSet(0x80226403, (nOuterY1 >> 8) & 0xF);	// 11:8 Y
	NoLockOv640RegisterSet(0x80226404, nOuterY1 & 0xFF);			// 7:0 Y

	//
	// Bottom Right coordinates (relative offset from lower right corner of capture area)
	//
	nValue = SENSOR_IMAGE_WIDTH - (nOuterX2 & 0x1FFF);	// Relative X
	NoLockOv640RegisterSet(0x80226405, (nValue >> 8) & 0x1F);		// 12:8 X
	NoLockOv640RegisterSet(0x80226406, nValue & 0xFF);			// 7:0 X

	nValue = SENSOR_IMAGE_HEIGHT - (nOuterY2 & 0xFFF);	// Relative Y
	NoLockOv640RegisterSet(0x80226407, (nValue >> 8) & 0xF);		// 11:8 Y
	NoLockOv640RegisterSet(0x80226408, nValue & 0xFF);			// 7:0 Y

	//
	// Set inner window
	//

	// Top Left coordinates
	NoLockOv640RegisterSet(0x80226409, (nInnerX1 >> 8) & 0x1F);	// 12:8 X
	NoLockOv640RegisterSet(0x8022640a, nInnerX1 & 0xFF);			// 7:0 X
	NoLockOv640RegisterSet(0x8022640d, (nInnerY1 >> 8) & 0xF);	// 11:8 Y
	NoLockOv640RegisterSet(0x8022640e, nInnerY1 & 0xFF);			// 7:0 Y

	//
	// Width and height of inner rectangle
	//
	NoLockOv640RegisterSet(0x80226411, (nInnerWidth >> 8) & 0x1F);	// 12:8 X
	NoLockOv640RegisterSet(0x80226412, nInnerWidth & 0xFF);			// 7:0 X

	NoLockOv640RegisterSet(0x80226415, (nInnerHeight >> 8) & 0xF);	// 11:8 Y
	NoLockOv640RegisterSet(0x80226416, nInnerHeight & 0xFF);			// 7:0 X

	return hResult;
}


stiHResult CstiVideoInput::HdmiPassthroughSet (
	bool bHdmiPassthrough)
{
	//stiDEBUG_TOOL(g_stiVideoInputDebug,
	//	stiTrace("CstiVideoInput::HdmiPassthroughSet: passthrough = %s\n", bHdmiPassthrough ? "true":"false");
	//);

	PostEvent (
		std::bind(&CstiVideoInput::EventHdmiPassthroughSet, this, bHdmiPassthrough));

	return stiRESULT_SUCCESS;
}


void CstiVideoInput::EventHdmiPassthroughSet (
	bool bHdmiPassthrough)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (bHdmiPassthrough)
	{
		if (!m_bHdmiPassthrough)
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiVideoInput::EventHdmiPassthroughSet bHdmiPassthrough = %d\n", bHdmiPassthrough);
			);

			m_bHdmiPassthrough = bHdmiPassthrough;

			hResult = m_pISPMonitor->HdmiPassthroughSet (true);
			stiTESTRESULT();

			hResult = NoLockCameraClose ();
			stiTESTRESULT();

			hResult = m_gstreamerVideoGraph.HdmiPassthroughSet (m_bHdmiPassthrough);
			stiTESTRESULT();
		}
	}
	else
	{
		if (m_bHdmiPassthrough)
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiVideoInput::EventHdmiPassthroughSet bHdmiPassthrough = %d\n", bHdmiPassthrough);
			);

			m_bHdmiPassthrough = bHdmiPassthrough;

			hResult = m_gstreamerVideoGraph.HdmiPassthroughSet (m_bHdmiPassthrough);
			stiTESTRESULT();

			hResult = NoLockCameraOpen();
			stiTESTRESULT();

			hResult = m_pISPMonitor->HdmiPassthroughSet (false);
			stiTESTRESULT();
		}
	}

STI_BAIL:
	return;
}


stiHResult CstiVideoInput::NoLockCameraSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = FocusPositionSet (m_nFocusValue);	// restore the focus
	stiTESTRESULT ();

	hResult = NoLockAutoExposureWindowSet ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInput::NoLockCameraOpen ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::NoLockCameraOpen m_nCameraControlFd = %d\n", m_nCameraControlFd);
	);

	if (m_nCameraControlFd < 0)
	{
		if ((m_nCameraControlFd = open (g_szCameraDeviceName, O_RDWR)) < 0)
		{
			stiTHROWMSG (stiRESULT_UNABLE_TO_OPEN_FILE, "Unable to open %s\n", g_szCameraDeviceName);
		}

		m_gstreamerVideoGraph.cameraControlFdSet(m_nCameraControlFd);
	}

	hResult = NoLockCameraSettingsSet ();
	stiTESTRESULT ();

	EventPTZSettingsSet (m_PtzSettings);		// restore the ptz settings

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoInput::NoLockCameraClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::NoLockCameraClose m_nCameraControlFd = %d\n", m_nCameraControlFd);
	);

	if (m_nCameraControlFd > -1)
	{
		close (m_nCameraControlFd);
		m_nCameraControlFd = -1;
	}

//STI_BAIL:

	return (hResult);
}

void CstiVideoInput::EventHdmiInResolutionChanged (int resolution)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = m_gstreamerVideoGraph.HdmiInResolutionSet (resolution);
	stiTESTRESULT()

STI_BAIL:
	return;
}

stiHResult CstiVideoInput::NoLockRcuStatusChanged ()
{

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("Enter CstiVideoInput::NoLockRcuStatusChanged - %s\n", RcuConnectedGet() ? "true" : "false");
	);

	if (!m_bHdmiPassthrough)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("CstiVideoInput::NoLockRcuStatusChanged: passthrough not set\n");
		);

		m_gstreamerVideoGraph.RcuConnectedSet (RcuConnectedGet());

		if (RcuConnectedGet())
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiVideoInput::NoLockRcuStatusChanged - connected\n");
			);

			hResult = NoLockCameraOpen ();
			stiTESTRESULT ();
		}
		else
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiVideoInput::NoLockRcuStatusChanged - not connected\n");
			);

			hResult = NoLockCameraClose ();
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoInput::NoLockCachedSettingSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bCachedFocusValue)
	{
		hResult = FocusPositionSet (m_nFocusValue);
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);

}


stiHResult CstiVideoInput::NoLockPlayingValidateStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::NoLockPlayingValidateStart\n");
	);

#ifdef ENABLE_VIDEO_INPUT
	if (RcuConnectedGet())
	{
		g_timeout_add (5000, (GSourceFunc) AsyncPlayingValidate, this);
	}
#endif

	return hResult;
}

gboolean CstiVideoInput::AsyncPlayingValidate (
	void * pUserData)
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::PlayingValidate\n");
	);

	auto pVideoInput = (CstiVideoInput *)pUserData;

	pVideoInput->PostEvent (
		std::bind(&CstiVideoInput::EventPlayingValidate, pVideoInput));

	return stiRESULT_SUCCESS;
}

void CstiVideoInput::EventPlayingValidate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::EventPlayingValidate\n");
	);

	bool bPipelinePlaying = false;
	hResult = m_gstreamerVideoGraph.PipelinePlayingGet (&bPipelinePlaying);
	stiTESTRESULT ();

	if (bPipelinePlaying)
	{
		uint64_t un64FrameCount = 0;
		m_gstreamerVideoGraph.FrameCountGet (&un64FrameCount, nullptr);

		if (un64FrameCount == 0)
		{
			stiASSERTMSG (estiFALSE, "CstiVideoInput::EventPlayingValidate: WARNING no frames counted. Calling PipelineReset\n");

			hResult = m_gstreamerVideoGraph.PipelineReset ();
			stiTESTRESULT ();
		}
		else
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiVideoInput::EventPlayingValidate: un64FrameCount = %llu\n", un64FrameCount);
			);

			// If we haved cached settings then set them now
			hResult = NoLockCachedSettingSet ();
			stiTESTRESULT ();
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("CstiVideoInput::EventPlayingValidate: Pipeline not playing\n");
		);
	}

STI_BAIL:
	return;
}

stiHResult CstiVideoInput::ambientLightGet (
	IVideoInputVP2::AmbientLight *ambiance,
	float *rawVal)
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	return m_pISPMonitor->ambientLightGet(ambiance, rawVal);
}

stiHResult CstiVideoInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiLOG_ENTRY_NAME (CstiVideoInput::VideoRecordSettingsSet);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiVideoInput::VideoRecordSettingsSet\n");
	);

	auto newVideoRecordSettings =
		std::make_shared<SstiVideoRecordSettings>(*pVideoRecordSettings);

	PostEvent (
		std::bind(&CstiVideoInput::eventEncodeSettingsSet, this, newVideoRecordSettings));

	return stiRESULT_SUCCESS;
}


void CstiVideoInput::eventEncodeSettingsSet (
	std::shared_ptr<SstiVideoRecordSettings> videoRecordSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiVideoInput::eventEncodeSettingsSet start\n");
	);

	// If we are wanting to send portrait from nVP2 we need to make sure the height is
	// valid. It was also decided to only send a 3:4 aspect ratio (75% of height).
	if (videoRecordSettings->remoteDisplayWidth < videoRecordSettings->remoteDisplayHeight)
	{
		const double fourByThreeAspectRatio = 4.0 / 3.0;
		const int macroBlockSize = 16;

		if (videoRecordSettings->unRows > static_cast<unsigned int>(m_pCameraControl->defaultSelfViewHeightGet ()))
		{
			videoRecordSettings->unRows = static_cast<unsigned int>(m_pCameraControl->defaultSelfViewHeightGet ());
		}

		videoRecordSettings->unColumns = static_cast<unsigned int>(videoRecordSettings->unRows * fourByThreeAspectRatio);
		// nVP2 needs to have width that is a multiple of 16 to prevent errors.
		videoRecordSettings->unColumns -= videoRecordSettings->unColumns % macroBlockSize;
	}

	bool bEncodeSizeChanged = false;
	bool remoteWantsPortraitVideo = (videoRecordSettings->remoteDisplayWidth < videoRecordSettings->remoteDisplayHeight);

	stiTESTCOND (videoRecordSettings, stiRESULT_ERROR);

	hResult = videoGraphBaseGet ().EncodeSettingsSet (*videoRecordSettings, &bEncodeSizeChanged);
	stiTESTRESULT ();

	if (m_portraitVideo != remoteWantsPortraitVideo)
	{
		m_portraitVideo = remoteWantsPortraitVideo;
		m_pCameraControl->RemoteDisplayOrientationChanged (
				static_cast<int>(videoRecordSettings->unColumns),
				static_cast<int>(videoRecordSettings->unRows));
	}

	if (bEncodeSizeChanged)
	{
		videoRecordSizeChangedSignal.Emit();
	}

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::eventEncodeSettingsSet end\n");
	);

STI_BAIL:
	return;
}

void CstiVideoInput::videoRecordToFile()
{

}
