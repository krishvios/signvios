// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2024 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoInput.h"


const std::vector<tofFocusPosition> CstiVideoInput::m_tofFocusVector { {{ 160, 2000},
																		{ 191, 1880},
																		{ 231, 1845},
																		{ 296, 1795},
																		{ 379, 1762},
																		{ 465, 1730},
																		{ 551, 1710},
																		{ 678, 1685},
																		{ 817, 1665},
																		{ 959, 1653},
																		{1121, 1644},
																		{1263, 1635},
																		{1465, 1630},
																		{1727, 1627},
																		{2012, 1620},
																		{2012, 1610}} };



/*!
 *\ brief start the event that performs the single-shot focus
 */
stiHResult CstiVideoInput::SingleFocus ()
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInput::SingleFocus\n");
	);

	if (g_stiVideoDisableTOFAutoFocus && g_stiVideoDisableIPUAutoFocus)
	{
		focusCompleteSignal.Emit(0);
		stiTHROWMSG (estiFALSE, "CstiVideoInput::SingleFocus: both auto focus methods are disabled.\n");
	}
	
	PostEvent (
		[this]
		{
			if (g_stiVideoDisableTOFAutoFocus)
			{
				// Just call eventSingleFocus without getting tof distance
				PostEvent (
					[this]
					{
						eventSingleFocus (0);
					});
			}
			else
			{
				// Run tof then call eventSingleFocus with tof distance
				auto onSuccess {
					[this](int distance)
					{
						PostEvent (
							[this, distance]
							{
								eventSingleFocus (distance);
							});
					}
				};

				dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet())->tofDistanceGet (onSuccess);
			}

			m_autoFocusTimer.Restart ();
		});

STI_BAIL:

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
	
	if (g_stiVideoDisableTOFAutoFocus && g_stiVideoDisableIPUAutoFocus)
	{
		stiASSERTMSG (estiFALSE, "CstiVideoInput::eventSingleFocus: both auto focus methods are disabled.\n");
	}
	
	if (g_stiVideoDisableTOFAutoFocus)
	{
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
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInput::eventSingleFocus: distance: ", distance, "\n");
		);
		// Calculate focus position
		auto tofFocusPosition {m_tofFocusVector.back ().position};

		auto i = 0u;
		for (; i < m_tofFocusVector.size () - 1; i++)
		{
			if (distance < m_tofFocusVector[i].distance)
			{
				tofFocusPosition = m_tofFocusVector[i].position;
				break;
			}
		}

		// Apply board specific correction
		int hdcc {0};
		boost::optional<int> hdccOverride;
		std::tie (hdcc, hdccOverride) = dynamic_cast<IPlatformVP*>(IstiPlatform::InstanceGet ())->hdccGet ();

		if (hdccOverride)
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
					vpe::trace ("---Using hdcc override---\n");
					vpe::trace ("hdcc: ", hdcc, ", hdccOverride: ", *hdccOverride, "\n");
			);
			hdcc = *hdccOverride;
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("tof: ", distance, ", hdcc: ", hdcc, ", ratio of hdcc: ", i, "/", m_tofFocusVector.size()-1, "\n");
			vpe::trace ("focusPostion BEFORE hdcc: ", tofFocusPosition, "\n");
		);

		tofFocusPosition += hdcc * static_cast<int>(i) / static_cast<int>(m_tofFocusVector.size () - 1);

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("focusPostion AFTER hdcc: ", tofFocusPosition, "\n");
		);
		
		if (g_stiVideoDisableIPUAutoFocus)
		{
			// Use the tof distance for a final focus position
			eventFocusPositionSet (tofFocusPosition);
			eventFocusComplete (true, -1);
		}
		else
		{
			// get the focus window
			hResult = focusWindowGet (&focusRegionStartX, &focusRegionStartY, &focusRegionWidth, &focusRegionHeight);
			stiTESTRESULT ();
			
			//Now that we have a good starting point call
			//call the auto-focus routine in libcamhal from
			//icamerasrc gstreamer interface with the initial position
			//and focus current region
			
			hResult = m_gstreamerVideoGraph.singleFocus (
				tofFocusPosition,
				focusRegionStartX,
				focusRegionStartY,
				focusRegionWidth,
				focusRegionHeight,
				[this](bool success, int contrast){ipuFocusCompleteCallback (success, contrast);});
			stiTESTRESULT ();
		}
	}
		
STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "CstiVideoInput::eventSingleFocus: failed with hResult = 0x%0x\n", hResult);
	}
}

