////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiCameraControl
//
//  File Name:	CstiCameraControl.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	This class contains the functions for supporting the near end 
//				(local) and far end camera control
//
////////////////////////////////////////////////////////////////////////////////


//
// Includes
//
#include "CstiCameraControl.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "stiTaskInfo.h"

//
// Constants
//
const int PTZ_SETTINGS_TIMEOUT = 5000;		// 5 seconds (in milliseconds)
const int LOCAL_MOVEMENT_INTERVAL = 100;	// 100 milliseconds

#define ZOOM_INCREMENT 4
#define PT_STEP_FACTOR 2

#define ZOOM_LEVEL_DENOMINATOR		256

#define MINIMUM_HORZ_POS	0
#define MINIMUM_VERT_POS	0

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//

////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::CstiCameraControl
//
// Description: Constructor
//
// Abstract:
//	This function is the default constructor
//
// Returns:
//	None.
//
CstiCameraControl::CstiCameraControl (
	int captureWidth,
	int captureHeight,
	int minZoomLevel,
	int defaultSelfViewWidth,
	int defaultSelfViewHeight)
:
	CstiEventQueue ("CameraControl"),
	m_ptzSettingsTimer (PTZ_SETTINGS_TIMEOUT, this),
	m_localMovementTimer (LOCAL_MOVEMENT_INTERVAL, this),
	m_captureWidth (captureWidth),
	m_captureHeight (captureHeight),
	m_minZoomLevel (minZoomLevel),
	m_defaultSelfViewWidth (defaultSelfViewWidth),
	m_defaultSelfViewHeight (defaultSelfViewHeight)
{
	m_signalConnections.push_back (m_ptzSettingsTimer.timeoutSignal.Connect (
			[this](){ SavePTZSettings (); }));

	m_signalConnections.push_back (m_localMovementTimer.timeoutSignal.Connect (
			[this](){ StartLocalMovement (); }));

} // end CstiCameraControl::CstiCameraControl


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::~CstiCameraControl
//
// Description: Class destructor
//
// Abstract:
//	This function is the default destructor.
//
// Returns:
//	None.
//
CstiCameraControl::~CstiCameraControl ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::~CstiCameraControl);

	CstiEventQueue::StopEventLoop();

	Shutdown();

} // end CstiCameraControl::~CstiCameraControl ()

//:-----------------------------------------------------------------------------
//:
//:	Task functions
//:
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::Startup
//
// Description: Start the control data task.
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiCameraControl::Startup ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Start the task
	//
	auto result = CstiEventQueue::StartEventLoop ();
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	return (hResult);

}


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::Shutdown
//
// Description: Starts the shutdown process of the control data task
//
// Abstract:
//	This method is responsible for doing all clean up necessary to gracefully
//	terminate the control data task. Note that once this method exits,
//	there will be no more message received from the message queue since the
//	message queue will be deleted and the control data task will go away.
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
void CstiCameraControl::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::ShutdownHandle);

	SavePTZSettings ();

	CstiEventQueue::StopEventLoop();
} // end CstiCameraControl::ShutdownHandle


//:-----------------------------------------------------------------------------
//:
//: Data flow functions
//:
//:-----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::CameraMove
//
// Description: Control the camera movement (P/T/Z/F)
//
// Abstract:
//
// Returns:
//	estiOK when successfull, otherwise estiERROR
//
EstiResult CstiCameraControl::CameraMove (
	uint8_t actionBitMask)
{
	stiLOG_ENTRY_NAME (CstiCameraControl::CameraSelect);

	// Send the message
	PostEvent(
		std::bind(&CstiCameraControl::eventCameraControl, this, actionBitMask));

	return estiOK;
} // end CstiCameraControl::CameraMove


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::Initialize
//
// Description: First stage of initialization of the control data task.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiCameraControl::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	uint32_t un32CameraCommandTimeout = 0;
	uint8_t un8CameraStep = 0;

	{
		const int nTIMEOUT_INCREMENT = 50;
		int nIrRepeatFrequency;
		int nCameraStep;

		// Obtain the IR Repeat frequency from the Property Manager
		//int nIrRepeatFreq = 120;
		WillowPM::PropertyManager::getInstance()->getPropertyInt (cmIR_REPEAT_FREQUENCY, &nIrRepeatFrequency, 200);

		// What is the step size?
		//int nStep = 2;
		WillowPM::PropertyManager::getInstance()->getPropertyInt (cmCAMERA_STEP_SIZE, &nCameraStep, 2);

		// Step up to the next timeout increment if necessary.  Are we evenly divisable
		// by the timeout increment?
		div_t stDiv = div (nIrRepeatFrequency, nTIMEOUT_INCREMENT);
		if (0 != stDiv.rem)
		{
			nIrRepeatFrequency = (stDiv.quot + 1) * nTIMEOUT_INCREMENT;
		} // end if

		// Add 200 ms to the timeout to comply with the specification H.281 wherein it states
		// that to receive in continuous mode, the "Continue" commands must arrive prior to
		// the repeat value minus 200 ms.
		un32CameraCommandTimeout = nIrRepeatFrequency + 200;  // Time after which the command becomes invalid
		un8CameraStep = nCameraStep;
	}

	m_un32CameraCommandTimeout = un32CameraCommandTimeout;
	m_un8CameraStep = ((un8CameraStep + 1) & ~1) * PT_STEP_FACTOR;

	int horzZoom = 0;
	int vertZoom = 0;
	int horzPos = MINIMUM_HORZ_POS;
	int vertPos = MINIMUM_VERT_POS;

#ifndef stiDISABLE_PROPERTY_MANAGER
	int nResult = 0;

	// Get instance of the property manager
	m_poPM = WillowPM::PropertyManager::getInstance();
	stiTESTCOND (m_poPM, stiRESULT_ERROR);

	m_poPM->propertyDefaultSet (cmVIDEO_SOURCE1_HORZ_CTR, m_captureWidth / 2);
	m_poPM->propertyDefaultSet (cmVIDEO_SOURCE1_VERT_CTR, m_captureHeight / 2);
	m_poPM->propertyDefaultSet (cmVIDEO_SOURCE1_ZOOM, ZOOM_LEVEL_DENOMINATOR);
	m_poPM->propertyDefaultSet (cmVIDEO_SOURCE1_HORZ_ZOOM, m_captureWidth);
#endif

	m_selfViewWidth = m_defaultSelfViewWidth;
	m_selfViewHeight = m_defaultSelfViewHeight;

	m_bPTZChanged = false;

	//
	// Get and validate the PTZ settings
	//

	// PTZ zoom size
	// The more zoom in, the bigger of zoom size.
	// HorzZoom = (SelfViewHorzSize * ZoomDen) / ZoomNum;
	// VertZoom = (SelfViewVertSize * ZoomDen) / ZoomNum;
	// HorzZoom <= m_nCaptureWidth and VertZoom <= m_nCaptureHeight
	//
	// PTZ pan and tilt position
	// HorzPos + HorzZoom <= m_nCaptureWidth
	// VertPos + VertZoom <= m_nCaptureHeight

	//
	// Retrieve the zoom level property from the propery table.
	//
#ifndef stiDISABLE_PROPERTY_MANAGER
	nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_ZOOM, &m_nZoomLevelNumerator);

	if (nResult == WillowPM::PM_RESULT_NOT_FOUND)
	{
		//
		// The Zoom Level property may not be in the property table because it is new.  If it is not
		// in the property table then try using the old property.
		//
		nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_HORZ_ZOOM, &horzZoom);

		if (nResult != WillowPM::PM_RESULT_NOT_FOUND)
		{
			m_nZoomLevelNumerator = (horzZoom * ZOOM_LEVEL_DENOMINATOR + (m_captureWidth / 2)) / m_captureWidth;

			m_poPM->propertySet (cmVIDEO_SOURCE1_ZOOM, m_nZoomLevelNumerator);
		}
	}

	//
	// Make sure the zoom level is not out of bounds
	//
	if (m_nZoomLevelNumerator < m_minZoomLevel)
	{
		m_nZoomLevelNumerator = m_minZoomLevel;
	}
	else if (m_nZoomLevelNumerator > ZOOM_LEVEL_DENOMINATOR)
	{
		m_nZoomLevelNumerator = ZOOM_LEVEL_DENOMINATOR;
	}

	SelfViewZoomDimensionsCompute (&horzZoom, &vertZoom);

	nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_HORZ_CTR, &m_nHorzCenter);

	if (nResult == WillowPM::PM_RESULT_NOT_FOUND)
	{
		int defaultHorzPos = (m_captureWidth - horzZoom) / 2;

		nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_HORZ_POS, &horzPos);

		if (nResult == WillowPM::PM_RESULT_NOT_FOUND)
		{
			horzPos = defaultHorzPos;
		}
		else
		{
			if (horzPos + horzZoom > m_captureWidth)
			{
				horzPos = defaultHorzPos;
			}

			horzPos &= ~1; // need to be even

			m_nHorzCenter = horzPos + ((horzZoom + 1) / 2);

			m_poPM->propertySet (cmVIDEO_SOURCE1_HORZ_CTR, m_nHorzCenter);
		}
	}

	nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_VERT_CTR, &m_nVertCenter);

	if (nResult == WillowPM::PM_RESULT_NOT_FOUND)
	{
		int defaultVerzPos = (m_captureHeight - vertZoom) / 2;

		nResult = m_poPM->propertyGet (cmVIDEO_SOURCE1_VERT_POS, &vertPos);

		if (nResult == WillowPM::PM_RESULT_NOT_FOUND)
		{
			vertPos = defaultVerzPos;
		}
		else
		{
			if (vertPos + vertZoom > m_captureHeight)
			{
				vertPos = defaultVerzPos;
			}

			// These variables help to zoom to center
			m_nVertCenter = vertPos + ((vertZoom + 1) / 2);

			m_poPM->propertySet (cmVIDEO_SOURCE1_VERT_CTR, m_nVertCenter);
		}
	}
#endif //stiDISABLE_PROPERTY_MANAGER

	// Set the PTZ information
	m_stPtzSettings.horzZoom = horzZoom;
	m_stPtzSettings.vertZoom = vertZoom;
	m_stPtzSettings.horzPos = horzPos;
	m_stPtzSettings.vertPos = vertPos;

	SelfViewPositionCompute (&m_stPtzSettings);

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("Zoom Level = %d\n", m_nZoomLevelNumerator);
		stiTrace ("\thPos = %d hZoom = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
		stiTrace ("\tvPos = %d vZoom = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
	);

	cameraMoveSignal.Emit(m_stPtzSettings);

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::StartLocalMovement
//
// Description:: Start the timer for sending camera control command to driver
//
// Abstract:
//
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
void CstiCameraControl::StartLocalMovement ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::StartLocalMovement);

	// Cancel save settings timer
	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::StartLocalMovement - m_ptzSettingsTimer timer cancel ...\n");
	);
	m_ptzSettingsTimer.stop ();

	if(m_nTimeoutCounter > 0)
	{
		CameraMove ();

		m_nTimeoutCounter --;

		stiDEBUG_TOOL (g_stiFECCDebug,
				stiTrace ("CstiCameraControl::StartLocalMovement bitmask = 0x%x %d\n", m_un8CurrActionBitMask, m_nTimeoutCounter);
		);

		//
		// The behavior of the camera movements can be changed to be single movements
		// instead of the usual deceleration by setting a debug flag to a non-zero value.
		//
		if (g_stiDiscreteCameraMovements == 0)
		{
			m_localMovementTimer.restart ();
		}
	}
	else
	{
		StopLocalMovement ();
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::StopLocalMovement
//
// Description:: Stop the timer for sending camera control command to driver
//
// Abstract:
//
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
void CstiCameraControl::StopLocalMovement ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::StopLocalMovement);

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::StopLocalMovement\n");
	);

	m_localMovementTimer.stop ();

	m_nTimeoutCounter = 0;

	// Start the save settings timer
	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::StopLocalMovement - m_ptzSettingsTimer timer Start ...\n");
	);
	m_ptzSettingsTimer.restart ();

	cameraMoveCompleteSignal.Emit ();
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::eventCameraControl
//
// Description: For near end camera control
//
// Abstract: Pass the camera control command from local terminal to control
//  the local camera
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
void CstiCameraControl::eventCameraControl (
	uint8_t actionBitBask)
{
	stiLOG_ENTRY_NAME (CstiCameraControl::eventCameraControl);

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::eventCameraControl - START\n");
	);

	// Start a new time out period
	StopLocalMovement ();

	m_un8CurrActionBitMask = actionBitBask;

	// convert timeout to 50 milliseconds intervals units
	m_nTimeoutCounter = (uint8_t) ((m_un32CameraCommandTimeout + LOCAL_MOVEMENT_INTERVAL - 1) / LOCAL_MOVEMENT_INTERVAL);

	StartLocalMovement ();
}


void CstiCameraControl::zoomIn (
	SstiPtzSettings* pPtzSettings)
{
	if (m_nZoomLevelNumerator > m_minZoomLevel)
	{
		m_nZoomLevelNumerator -= ZOOM_INCREMENT;

		if (m_nZoomLevelNumerator < m_minZoomLevel)
		{
			m_nZoomLevelNumerator = m_minZoomLevel;
		}

		SelfViewZoomDimensionsCompute (&pPtzSettings->horzZoom, &pPtzSettings->vertZoom);
		SelfViewPositionCompute (pPtzSettings);
	}
}


void CstiCameraControl::zoomOut (
	SstiPtzSettings* pPtzSettings)
{
	if (m_nZoomLevelNumerator < ZOOM_LEVEL_DENOMINATOR)
	{
		m_nZoomLevelNumerator += ZOOM_INCREMENT;

		if (m_nZoomLevelNumerator > ZOOM_LEVEL_DENOMINATOR)
		{
			m_nZoomLevelNumerator = ZOOM_LEVEL_DENOMINATOR;
		}

		SelfViewZoomDimensionsCompute (&pPtzSettings->horzZoom, &pPtzSettings->vertZoom);
		SelfViewPositionCompute (pPtzSettings);
	}
}


void CstiCameraControl::panRight (
	SstiPtzSettings* pPtzSettings)
{
	// Pan right if we can
	if (pPtzSettings->horzPos < m_captureWidth - pPtzSettings->horzZoom)
	{
		pPtzSettings->horzPos += m_un8CameraStep;
		m_nHorzCenter += m_un8CameraStep;

		if (pPtzSettings->horzPos > m_captureWidth - pPtzSettings->horzZoom)
		{
			pPtzSettings->horzPos = m_captureWidth - pPtzSettings->horzZoom;
			m_nHorzCenter = m_captureWidth - (pPtzSettings->horzZoom + 1) / 2;
		}
	}
}


void CstiCameraControl::panLeft (
	SstiPtzSettings* pPtzSettings)
{
	// Pan left if we can
	if (pPtzSettings->horzPos > MINIMUM_HORZ_POS)
	{
		pPtzSettings->horzPos -= m_un8CameraStep;
		m_nHorzCenter -= m_un8CameraStep;

		if ((int)pPtzSettings->horzPos < MINIMUM_HORZ_POS)
		{
			pPtzSettings->horzPos = MINIMUM_HORZ_POS;
			m_nHorzCenter = MINIMUM_HORZ_POS + (pPtzSettings->horzZoom + 1) / 2;
		}
	}
}


void CstiCameraControl::tiltDown (
	SstiPtzSettings* pPtzSettings)
{
	// Tilt down if we can
	if (pPtzSettings->vertPos < m_captureHeight - pPtzSettings->vertZoom)
	{
		pPtzSettings->vertPos += m_un8CameraStep;
		m_nVertCenter += m_un8CameraStep;

		if (pPtzSettings->vertPos > m_captureHeight - pPtzSettings->vertZoom)
		{
			pPtzSettings->vertPos = m_captureHeight - pPtzSettings->vertZoom;
			m_nVertCenter = m_captureHeight - (pPtzSettings->vertZoom + 1) / 2;
		}
	}
}


void CstiCameraControl::tiltUp (
	SstiPtzSettings* pPtzSettings)
{
	// Tilt up if we can
	if ((int)pPtzSettings->vertPos > MINIMUM_VERT_POS)
	{
		pPtzSettings->vertPos -= m_un8CameraStep;
		m_nVertCenter -= m_un8CameraStep;

		if ((int)pPtzSettings->vertPos < MINIMUM_VERT_POS)
		{
			pPtzSettings->vertPos = MINIMUM_VERT_POS;
			m_nVertCenter = MINIMUM_VERT_POS + (pPtzSettings->vertZoom + 1) / 2;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::CameraMove
//
// Description: Calculate the camera movement (position and size ) based on
//  the command bitmask and step size.
//
// Abstract:
// uint8_t un8Bitmask - bitMask described the movement
//
//	7          0
//	+----------+
//	| PxTxZxFx |
//	+----------+
//
//  Bit 7: Pan on = 1 / off = 0
//  Bit 6: Pan Right = 1 / Left = 0
//  Bit 5: Tilt on = 1 / off = 0
//  Bit 4: Tilt Up = 1 / Down = 0
//  Bit 3: Zoom on = 1 / off = 0
//  Bit 2: Zoom  In = 1 / Out = 0
//  Bit 1: Focus on = 1 / off = 0
//  Bit 0: Focus In = 1 / Out = 0
//
// un8StepSize un8StepSize - speed of movement
//
// Returns:
//	estiOK if successful, estiERROR if not moving (the settings not changed).
//
EstiResult CstiCameraControl::CameraMove ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::CameraMove);

	EstiResult eResult = estiOK;

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::CameraMove\n");
		stiTrace ("\nBitMask 0x%X nStepSize = %d eCtrTerminal = %d\n", m_un8CurrActionBitMask, m_un8CameraStep);
		stiTrace ("Before Movement: \n");
		stiTrace ("\tnStepSize = %d\n", m_un8CameraStep);
		stiTrace ("\thPos = %d width = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
		stiTrace ("\tvPos = %d height = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
		stiTrace ("\tZoom Ratio = %d/%d\n", m_nZoomLevelNumerator, ZOOM_LEVEL_DENOMINATOR);
	);

	SstiPtzSettings stPtzSettings = m_stPtzSettings;

	// Zoom
	if (0 != (m_un8CurrActionBitMask & 0x08))
	{
		if (0 != (m_un8CurrActionBitMask & 0x04))
		{
			// Zooom in
			zoomIn (&stPtzSettings);
		}
		else
		{
			// Zoom out
			zoomOut (&stPtzSettings);
		}
	}

	// Pan
	if (0 != (m_un8CurrActionBitMask & 0x80))
	{
		bool right = (0 != (m_un8CurrActionBitMask & 0x40));

		if (right)
		{
			// Pan Right
			panRight (&stPtzSettings);
		}
		else
		{
			// Pan Left
			panLeft (&stPtzSettings);
		}

	}

	// Tilt
	if (0 != (m_un8CurrActionBitMask & 0x20))
	{
		bool up = (0 != (m_un8CurrActionBitMask & 0x10));

		if (up)
		{
			// Tilt Up
			tiltUp (&stPtzSettings);
		}
		else
		{
			// Tilt Down
			tiltDown (&stPtzSettings);
		}
	}

	if ((stPtzSettings.horzPos != m_stPtzSettings.horzPos) ||
			(stPtzSettings.vertPos != m_stPtzSettings.vertPos) ||
			(stPtzSettings.horzZoom != m_stPtzSettings.horzZoom) ||
			(stPtzSettings.vertZoom != m_stPtzSettings.vertZoom))
	{
		m_bPTZChanged = true;

		m_stPtzSettings = stPtzSettings;

		cameraMoveSignal.Emit(m_stPtzSettings);
	}

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("After Movement: \n");
		stiTrace ("\thPos = %d width = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
		stiTrace ("\tvPos = %d height = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
		stiTrace ("\tZoom Ratio = %d/%d\n", m_nZoomLevelNumerator, ZOOM_LEVEL_DENOMINATOR);
	);

	return eResult;
}

EstiResult CstiCameraControl::CameraPTZGet(
	uint32_t *pun32HorzPercent,
	uint32_t *pun32VertPercent,
	uint32_t *pun32ZoomPercent,
	uint32_t *pun32ZoomWidthPercent,
	uint32_t *pun32ZoomHeightPercent)
{
	EstiResult eResult = estiOK;
	int zoomWidth, zoomHeight;

	SelfViewZoomDimensionsCompute(&zoomWidth, &zoomHeight);

	if (pun32HorzPercent)
	{
		if (m_captureWidth > zoomWidth)
		{
			*pun32HorzPercent = (uint32_t)((double(m_stPtzSettings.horzPos - MINIMUM_HORZ_POS) / double(m_captureWidth - zoomWidth - MINIMUM_HORZ_POS)) * 100.0);
		}
		else
		{
			*pun32HorzPercent = 50;
		}
		
	}

	if (pun32VertPercent)
	{
		if (m_captureHeight > zoomHeight)
		{
			*pun32VertPercent = (uint32_t)((double(m_stPtzSettings.vertPos - MINIMUM_VERT_POS) / double(m_captureHeight - zoomHeight - MINIMUM_VERT_POS)) * 100.0);
		}
		else
		{
			*pun32VertPercent = 50;
		}
		
	}

	if (pun32ZoomPercent)
	{
		*pun32ZoomPercent = 100 - (uint32_t)((double(m_nZoomLevelNumerator - m_minZoomLevel) / double(ZOOM_LEVEL_DENOMINATOR - m_minZoomLevel)) * 100.0);
	}

	if (pun32ZoomWidthPercent)
	{
		*pun32ZoomWidthPercent = (uint32_t)(double(zoomWidth) / double(m_captureWidth) * 100.0);
	}

	if (pun32ZoomHeightPercent)
	{
		*pun32ZoomHeightPercent = (uint32_t)(double(zoomHeight) / double(m_captureHeight) * 100.0);
	}

	return eResult;
}


/*!
 * \brief Restores values needed for sending video in landscape orientation.
 *
 */
void CstiCameraControl::ResetSelfViewProperties ()
{
#ifndef stiDISABLE_PROPERTY_MANAGER
	m_poPM->propertyGet (cmVIDEO_SOURCE1_HORZ_CTR, &m_nHorzCenter);
	m_poPM->propertyGet (cmVIDEO_SOURCE1_VERT_CTR, &m_nVertCenter);
	m_poPM->propertyGet (cmVIDEO_SOURCE1_ZOOM, &m_nZoomLevelNumerator);
#endif
}


/*!
 * \brief Handles signals of remote endpoints preferred display size changes.
 *
 * \return Success or failure.
 */
void CstiCameraControl::RemoteDisplayOrientationChanged (
	int width,
	int height)
{
	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::RemoteDisplayOrientationChanged\n");
	);
	
	CameraSelfViewSizeSet (width, height, false);
}


void CstiCameraControl::SelfViewZoomDimensionsCompute (
	int *horzZoom,
	int *vertZoom)
{
	//
	// The zoom window is the capture size multiplied by the zoom level factor (stored as an integer numerator and an integer denominator).
	// The zoom window is then: capture size * zoom level numerator / zoom level denominator where capture size is either width or height.
	// The zoom window, of course, has the same aspect ratio of the full capture size.
	//
	int nZoomWindowWidth = ((m_captureWidth * m_nZoomLevelNumerator + ZOOM_LEVEL_DENOMINATOR / 2) / ZOOM_LEVEL_DENOMINATOR);
	int nZoomWindowHeight = ((m_captureHeight * m_nZoomLevelNumerator + ZOOM_LEVEL_DENOMINATOR / 2) / ZOOM_LEVEL_DENOMINATOR);

	//
	// We want to compute a factor so that we can grow or shrink the self view size to fit into the zoom window.
	// This self view aspect ratio as it is passed in may have a different aspect ratio than our capture size.  Because
	// of this we may need to compute the zoom factor twice.  We will do it once using the width of the self view first.
	// If self view doesn't fit well then we will compute the self view zoom factor again using the height of the self
	// view.
	//
	// Note: the zoom numerator needs to be even so we zero out the least significant bit to enforce this.
	//
	int nSelfViewZoomNumerator = (nZoomWindowWidth * ZOOM_LEVEL_DENOMINATOR + m_selfViewWidth / 2) / m_selfViewWidth;

	//
	// Compute the zoom self view height to make sure it doesn't exceed our zoom window.  If it does
	// then compute a new zoom numerator using the self view height instead.
	//
	int nZoomSelfViewHeight = (m_selfViewHeight * nSelfViewZoomNumerator + ZOOM_LEVEL_DENOMINATOR / 2) / ZOOM_LEVEL_DENOMINATOR;

	if (nZoomSelfViewHeight > nZoomWindowHeight)
	{
		//
		// The height is too large so re-compute the self view zoom numerator using the height instead.
		//
		nSelfViewZoomNumerator = (nZoomWindowHeight * ZOOM_LEVEL_DENOMINATOR + m_selfViewHeight / 2) / m_selfViewHeight;
	}

	//
	// Now compute the zoomed self view height and width.
	//
	*horzZoom = (m_selfViewWidth * nSelfViewZoomNumerator + ZOOM_LEVEL_DENOMINATOR / 2) / ZOOM_LEVEL_DENOMINATOR;
	*vertZoom = (m_selfViewHeight * nSelfViewZoomNumerator + ZOOM_LEVEL_DENOMINATOR / 2) / ZOOM_LEVEL_DENOMINATOR;

	// We can never return a zoom of greater width or height than we can capture.
	if (*horzZoom > m_captureWidth)
	{
		*horzZoom = m_captureWidth;
	}
	if (*vertZoom > m_captureHeight)
	{
		*vertZoom = m_captureHeight;
	}
}


void CstiCameraControl::SelfViewPositionCompute (
	SstiPtzSettings *pPtzSettings)
{
	// Adjust horizontal position to zoom to center of picture
	if (m_nHorzCenter - ((int)(pPtzSettings->horzZoom + 1) / 2) < MINIMUM_HORZ_POS)
	{
		//
		// The zoom rectangle is off the left side of the frame.
		//
		pPtzSettings->horzPos = MINIMUM_HORZ_POS;
		m_nHorzCenter = MINIMUM_HORZ_POS + ((pPtzSettings->horzZoom + 1) / 2);
	}
	else if (m_nHorzCenter + ((int)(pPtzSettings->horzZoom + 1) / 2) > m_captureWidth)
	{
		//
		// The zoom rectangle is off the right side of the frame.
		//
		pPtzSettings->horzPos = m_captureWidth - pPtzSettings->horzZoom;
		// Must be even
		pPtzSettings->horzPos = pPtzSettings->horzPos & ~1;
		m_nHorzCenter = m_captureWidth - ((pPtzSettings->horzZoom + 1) / 2);
	}
	else
	{
		//
		// The zoom rectangle is between the left and right sides of the frame.
		//
		pPtzSettings->horzPos = m_nHorzCenter - ((pPtzSettings->horzZoom + 1) / 2);
		if (pPtzSettings->horzPos < MINIMUM_HORZ_POS)
		{
			pPtzSettings->horzPos = MINIMUM_HORZ_POS;
		}
		// Must be even
		pPtzSettings->horzPos = pPtzSettings->horzPos & ~1;
	}

	// Adjust vertical position to zoom to center of picture
	if (m_nVertCenter - ((int)(pPtzSettings->vertZoom + 1) / 2) < MINIMUM_VERT_POS)
	{
		//
		// The zoom rectangle is off the top of the frame.
		//
		pPtzSettings->vertPos = MINIMUM_VERT_POS;
		m_nVertCenter = MINIMUM_VERT_POS + ((pPtzSettings->vertZoom + 1) / 2);
	}
	else if (m_nVertCenter + ((int)(pPtzSettings->vertZoom + 1) / 2) > m_captureHeight)
	{
		//
		// The zoom rectangle is off the bottom of the frame.
		//
		pPtzSettings->vertPos = m_captureHeight - pPtzSettings->vertZoom;
#if APPLICATION == APP_NTOUCH_VP2
		// Must be even
		pPtzSettings->vertPos = pPtzSettings->vertPos & ~1;
#endif
		m_nVertCenter = m_captureHeight - ((pPtzSettings->vertZoom + 1) / 2);
	}
	else
	{
		//
		// The zoom rectangle is between the top and the bottom of the frame
		//
		pPtzSettings->vertPos = m_nVertCenter - ((pPtzSettings->vertZoom + 1) / 2);
		if (pPtzSettings->vertPos < MINIMUM_VERT_POS)
		{
			pPtzSettings->vertPos = MINIMUM_VERT_POS;
		}
#if APPLICATION == APP_NTOUCH_VP2
		// Must be even
		pPtzSettings->vertPos = pPtzSettings->vertPos & ~1;
#endif
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::CameraSelfViewSizeSet
//
// Description: Set the camera self view size. PTZ settings needs to be modified
// accordingly because of the change in self view size.
//
// uint32_t horzSize - horizonal size for self view
// uint32_t vertSize - vertical size for self view
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
EstiResult CstiCameraControl::CameraSelfViewSizeSet (
	int horzSize,
	int vertSize,
	bool restore)
{
	EstiResult eResult = estiOK;
	SstiPtzSettings PtzSettings{};

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("Before CameraSelfViewSizeSet: \n");
		stiTrace ("\tNew horzSize = %d vertSize = %d\n", horzSize, vertSize);
		stiTrace ("\thPos = %d hZoom = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
		stiTrace ("\tvPos = %d vZoom = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
	);

	PtzSettings = m_stPtzSettings;

	m_selfViewWidth = horzSize;
	m_selfViewHeight = vertSize;

	SelfViewZoomDimensionsCompute (&m_stPtzSettings.horzZoom, &m_stPtzSettings.vertZoom);
	SelfViewPositionCompute (&m_stPtzSettings);

	if (restore)
	{
		if (PtzSettings.horzPos != m_stPtzSettings.horzPos
		 || PtzSettings.horzZoom != m_stPtzSettings.horzZoom
		 || PtzSettings.vertPos != m_stPtzSettings.vertPos
		 || PtzSettings.vertZoom != m_stPtzSettings.vertZoom)
		{
			m_bPTZChanged = true;

			// Save to property table if any of the settings changed because of changes in self view size.
			SavePTZSettings ();
		}
	}

	// Set the PTZ information to driver
	cameraMoveSignal.Emit(m_stPtzSettings);

	cameraMoveCompleteSignal.Emit ();

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("After CameraSelfViewSizeSet: \n");
		stiTrace ("\thPos = %d hZoom = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
		stiTrace ("\tvPos = %d vZoom = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
	);

	return eResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCameraControl::SavePTZSettings
//
// Description::Save the PTZ settings to property table
//
// Abstract:
//
//
// Returns:
//	estiOK if successful, estiERROR if not.
//
void CstiCameraControl::SavePTZSettings ()
{
	stiLOG_ENTRY_NAME (CstiCameraControl::SavePTZSettings);

	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::SavePTZSettings\n");
	);

#ifndef stiDISABLE_PROPERTY_MANAGER
	if (m_bPTZChanged)
	{
		WillowPM::PropertyManager *poPM = m_poPM;

		poPM->propertySet (cmVIDEO_SOURCE1_ZOOM, m_nZoomLevelNumerator, WillowPM::PropertyManager::Persistent);
		poPM->propertySet (cmVIDEO_SOURCE1_VERT_CTR, (int) m_nVertCenter, WillowPM::PropertyManager::Persistent);
		poPM->propertySet (cmVIDEO_SOURCE1_HORZ_CTR, (int) m_nHorzCenter, WillowPM::PropertyManager::Persistent);

		stiDEBUG_TOOL (g_stiFECCDebug,
			stiTrace ("Zoom Level = %d\n", m_nZoomLevelNumerator);
			stiTrace ("\thPos = %d hZoom = %d hCenter = %d\n", m_stPtzSettings.horzPos, m_stPtzSettings.horzZoom, m_nHorzCenter);
			stiTrace ("\tvPos = %d vZoom = %d vCenter = %d\n", m_stPtzSettings.vertPos, m_stPtzSettings.vertZoom, m_nVertCenter);
		);
	}

#endif  // stiDISABLE_PROPERTY_MANAGER
}


/*!
 * \brief Disconnects signal for remote display size changes and resets selfview size.
 *
 * \return Nothing.
 */
void CstiCameraControl::selfViewSizeRestore ()
{
	stiDEBUG_TOOL (g_stiFECCDebug,
		stiTrace ("CstiCameraControl::selfViewSizeRestore\n");
	);
			
	// Save the PTZ settings before restoring landscape values.
	SavePTZSettings ();
	ResetSelfViewProperties ();
	CameraSelfViewSizeSet (
		m_captureWidth,
		m_captureHeight,
		false);
}


CstiCameraControl::SstiPtzSettings CstiCameraControl::ptzSettingsGet () const
{
	return m_stPtzSettings;
}


int CstiCameraControl::defaultSelfViewWidthGet () const
{
	return m_defaultSelfViewWidth;
}


int CstiCameraControl::defaultSelfViewHeightGet () const
{
	return m_defaultSelfViewHeight;
}
