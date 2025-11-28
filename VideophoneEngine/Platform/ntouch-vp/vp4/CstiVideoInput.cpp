// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2024 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoInput.h"

/*!
 *\ brief start the event that performs the single-shot focus
 */
stiHResult CstiVideoInput::SingleFocus ()
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInput::SingleFocus\n");
	);

	PostEvent (
		[this]
		{
			// Just call eventSingleFocus without getting tof distance
			PostEvent (
				[this]
				{
					eventSingleFocus (0);
				});
	
			m_autoFocusTimer.Restart ();
		});

	return hResult;
}


/*!
 *\ brief the actual event that performs the focus
 */
void CstiVideoInput::eventSingleFocus (
	int distance)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	int focusRegionStartX;
	int focusRegionStartY;
	int focusRegionWidth;
	int focusRegionHeight;
	
	// get the focus window
	hResult = focusWindowGet (&focusRegionStartX, &focusRegionStartY, &focusRegionWidth, &focusRegionHeight);
	stiTESTRESULT ();
	
	//call the auto-focus routine in libcamhal from
	//icamerasrc gstreamer interface with an invalid
	//initial position and current focus region
	
	hResult = m_gstreamerVideoGraph.singleFocus (
		-1,
		focusRegionStartX,
		focusRegionStartY,
		focusRegionWidth,
		focusRegionHeight,
		[this](bool success, int contrast){ipuFocusCompleteCallback (success, contrast);});
	stiTESTRESULT ();
		
STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "CstiVideoInput::eventSingleFocus: failed with hResult = 0x%0x\n", hResult);
	}
}

