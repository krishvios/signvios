// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoInputBase2.h"
#include "stiTrace.h"
#include "CstiMonitorTask.h"
#include "CstiCameraControl.h"
#include "IstiPlatform.h"
#include "IPlatformVP.h"
#include "IVideoOutputVP.h"
#include "CstiGStreamerVideoGraph.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
//#include <gst/interfaces/propertyprobe.h>
#include <boost/optional.hpp>

constexpr const int FOCUS_WINDOW_WIDTH = 600;
constexpr const int FOCUS_WINDOW_HEIGHT = 500;

constexpr const int AE_INNER_WINDOW_WIDTH = 600;
constexpr const int AE_INNER_WINDOW_HEIGHT = 600;

const int MIN_ZOOM_LEVEL = 64; // Allows up to a 4x zoom

const int DEFAULT_SELFVIEW_WIDTH = 1920;
const int DEFAULT_SELFVIEW_HEIGHT = 1080;

const int SELFVIEW_DISABLED_INTERVAL = 30 * 60 * 1000;
const int SELFVIEW_DISABLED_START = 1500;
const int SELFVIEW_DISABLED_STOP = 2500;
const int SELFVIEW_TRANSITION = 1500;

CstiVideoInputBase2::CstiVideoInputBase2 ()
:
	m_encodeSettingsSetTimer (0, this),
	m_selfviewDisabledIntervalTimer (SELFVIEW_DISABLED_INTERVAL, this),
	m_selfviewDisabledStartTimer (SELFVIEW_DISABLED_START, this),
	m_selfviewDisabledStopTimer (SELFVIEW_DISABLED_STOP, this),
	m_selfviewTransitionTimer (SELFVIEW_TRANSITION,this)
{
	m_signalConnections.push_back (m_autoFocusTimer.timeoutEvent.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					eventAutoFocusTimerTimeout ();
				});
		}));
	
	// Connect the timers to some handlers
	m_signalConnections.push_back (m_encodeSettingsSetTimer.timeoutSignal.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					eventEncodeSettingsSet ();
				});
		}));
	
	m_signalConnections.push_back (m_selfviewDisabledIntervalTimer.timeoutSignal.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					if (!m_selfviewEnabled && !m_systemSleepEnabled)
					{	
						pipelineEnabledSet (true);
						m_selfviewDisabledStartTimer.start ();
						m_selfviewDisabledStopTimer.start ();
					}
				});
		}));

	m_signalConnections.push_back (m_selfviewDisabledStartTimer.timeoutSignal.Connect (
		[this]
		{
			PostEvent (
				[this]
				{	
					auto ambiance = IVideoInputVP2::AmbientLight::UNKNOWN;
					float rawAmbientLight = 0;

					m_gstreamerVideoGraph.ambientLightGet (ambiance, rawAmbientLight);

					if (ambiance != IVideoInputVP2::AmbientLight::UNKNOWN)
					{
						m_lastAmbientLight = ambiance;
					}
				});
		}));

	m_signalConnections.push_back (m_selfviewDisabledStopTimer.timeoutSignal.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					if (!m_selfviewEnabled && !m_systemSleepEnabled)
					{
						pipelineEnabledSet (false);
						m_selfviewDisabledIntervalTimer.start ();
					}
					
				});
		}));
	
	m_signalConnections.push_back (m_selfviewTransitionTimer.timeoutSignal.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					m_allowAmbientLightGet = true;
				});
		}));
}

CstiVideoInputBase2::~CstiVideoInputBase2()
{
	NoLockCameraClose ();
}

stiHResult CstiVideoInputBase2::Initialize (
	CstiMonitorTask *pMonitorTask)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase2::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::Initialize:\n");
	);
	
	m_monitorTask = pMonitorTask;

	hResult = monitorTaskInitialize ();
	stiTESTRESULT ();
	
	hResult = rcuStatusInitialize ();
	stiTESTRESULT ();

	hResult = bufferElementsInitialize ();
	stiTESTRESULT ();

	hResult = gstreamerVideoGraphInitialize ();
	stiTESTRESULT ();
	
	m_lastSelfViewEnableTime = std::chrono::steady_clock::now ();
	
	m_lastEncodeSettingsSetTime = std::chrono::steady_clock::now ();
	
	m_signalConnections.push_back (m_gstreamerVideoGraph.encodeCameraError.Connect (
		[this]
		{
			PostEvent (
				[this]
				{
					// There was an error with the camera on the encode pipeline
					// shutdown and restart the pipeline
					cameraErrorRestartPipeline ();
				});
		}));
	
	m_signalConnections.push_back (m_gstreamerVideoGraph.aspectRatioChanged.Connect (
		[this]
		{
			videoRecordSizeChangedSignal.Emit ();
		}));

	hResult = cameraControlInitialize (
		m_gstreamerVideoGraph.maxCaptureWidthGet(),
		m_gstreamerVideoGraph.maxCaptureHeightGet (),
		MIN_ZOOM_LEVEL,
		DEFAULT_SELFVIEW_WIDTH,
		DEFAULT_SELFVIEW_HEIGHT);
	stiTESTRESULT ();

	m_frameTimer.Start();

STI_BAIL:

	return hResult;
}


MonitorTaskBase *CstiVideoInputBase2::monitorTaskBaseGet ()
{
	return m_monitorTask;
}


CstiGStreamerVideoGraphBase &CstiVideoInputBase2::videoGraphBaseGet ()
{
	return m_gstreamerVideoGraph;
}


const CstiGStreamerVideoGraphBase &CstiVideoInputBase2::videoGraphBaseGet () const
{
	return m_gstreamerVideoGraph;
}


bool CstiVideoInputBase2::RcuAvailableGet () const
{
	return RcuConnectedGet ();
}

stiHResult CstiVideoInputBase2::BrightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	Lock ();

	hResult = m_gstreamerVideoGraph.brightnessRangeGet (range);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::BrightnessRangeGet: Brightness range: %d - %d\n", range->min, range->max);
	);

STI_BAIL:

	Unlock ();

	return hResult;
}

/*
 * brief get the brightness
 */
int CstiVideoInputBase2::BrightnessGet () const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	int brightness {-1};

	Lock ();

	hResult = m_gstreamerVideoGraph.brightnessGet (&brightness);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::BrightnessGet: brightness = ", brightness, "\n");
	);

STI_BAIL:

	Unlock ();

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::BrightnessGet: Error, returning brightness = ", brightness, "\n");
		);
	}

	return brightness;
}

stiHResult CstiVideoInputBase2::BrightnessSet (
	int brightness)
{
	PostEvent (
		[this, brightness]
		 {
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				vpe::trace ("CstiVideoInputBase2::eventBrightnessSet: brightness = ", brightness, "\n");
			);

			m_gstreamerVideoGraph.brightnessSet (brightness);
		 });
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiVideoInputBase2::SaturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	Lock ();

	hResult = m_gstreamerVideoGraph.saturationRangeGet (range);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::SaturationRangeGet: Saturation range: %d - %d\n", range->min, range->max);
	);

STI_BAIL:

	Unlock ();

	return hResult;
}

/*
 * brief get the saturation
 */
int CstiVideoInputBase2::SaturationGet () const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	int saturation {-1};

	Lock ();

	hResult = m_gstreamerVideoGraph.saturationGet (&saturation);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::SaturationGet: saturation = ", saturation, "\n");
	);

STI_BAIL:

	Unlock ();

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::SaturationGet: Error, returning saturation = ", saturation, "\n");
		);
	}

	return saturation;
}

stiHResult CstiVideoInputBase2::SaturationSet (
	int saturation)
{
	PostEvent (
		[this, saturation]
		{
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				vpe::trace ("CstiVideoInputBase2::eventSaturationSet: saturation = ", saturation, "\n");
			);

			m_gstreamerVideoGraph.saturationSet (saturation);
		});
	
	return stiRESULT_SUCCESS;
}


/*!\brief Returns the coordinates and dimensions of the focus window.
 *
 */
stiHResult CstiVideoInputBase2::focusWindowGet (
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

	nStartX = m_PtzSettings.horzPos + (m_PtzSettings.horzZoom / 2) - FOCUS_WINDOW_WIDTH / 2;
	nStartY = m_PtzSettings.vertPos + (m_PtzSettings.vertZoom / 2) - FOCUS_WINDOW_HEIGHT / 2;

	//
	// Since the self view is mirrored we need to flip the X component of the origin to the other side.
	//
	nStartX = m_gstreamerVideoGraph.maxCaptureWidthGet() - 1 - (nStartX + FOCUS_WINDOW_WIDTH - 1);

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

		if (nStartX + nWidth > m_gstreamerVideoGraph.maxCaptureWidthGet())
		{
			nWidth -= nStartX + nWidth - m_gstreamerVideoGraph.maxCaptureWidthGet();
		}

		if (nStartY < 0)
		{
			nHeight += nStartY;
			nStartY = 0;
		}

		if (nStartY + nHeight > m_gstreamerVideoGraph.maxCaptureHeightGet())
		{
			nHeight -= nStartY + nHeight - m_gstreamerVideoGraph.maxCaptureHeightGet();
		}

		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			stiTrace ("Clipped Focus Window: %d, %d, %d, %d\n", nStartX, nStartY, nWidth, nHeight);
		);
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("Zoom Window: X: %d, Y: %d, W: %d, H: %d\n", m_gstreamerVideoGraph.maxCaptureWidthGet() - 1 - (m_PtzSettings.horzPos + m_PtzSettings.horzZoom - 1), m_PtzSettings.vertPos, m_PtzSettings.horzZoom, m_PtzSettings.vertZoom);
		stiTrace ("Focus Window: X: %d, Y: %d, W: %d, H: %d\n", nStartX, nStartY, nWidth, nHeight);
	);

	Unlock ();

	*pnStartX = nStartX;
	*pnStartY = nStartY;
	*pnWidth = nWidth;
	*pnHeight = nHeight;

	return (stiRESULT_SUCCESS);
}

void CstiVideoInputBase2::ipuFocusCompleteCallback (bool success, int contrast)
{
	PostEvent (
		[this, success, contrast] {
			m_focusPosition = FocusPositionGet ();
			eventFocusComplete (success, contrast);
		}
	);
}


void CstiVideoInputBase2::eventFocusComplete (bool success, int contrast)
{
	stiASSERTMSG (success, "Auto focus failed");
	m_autoFocusTimer.Stop ();	
	focusCompleteSignal.Emit (success ? contrast : 0);
}


stiHResult CstiVideoInputBase2::FocusRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	Lock ();
	
	hResult = m_gstreamerVideoGraph.focusRangeGet (range);
	stiTESTRESULT ();
			
	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace ("CstiVideoInputBase2::FocusRangeGet: Focus range: %d - %d\n", range->min, range->max);
	);

STI_BAIL:

	Unlock ();

	return hResult;
}

/*
 * brief get the current focus position
 */
int CstiVideoInputBase2::FocusPositionGet ()
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	int focusPosition {-1};

	Lock ();
	
	hResult = m_gstreamerVideoGraph.focusPositionGet (&focusPosition);
	stiTESTRESULT ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		vpe::trace ("CstiVideoInputBase2::FocusPositionGet: focusPosition = ", focusPosition, "\n");
	);

STI_BAIL:

	Unlock ();
	
	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
			vpe::trace ("CstiVideoInputBase2::FocusPositionGet: Error, returning focusPosition = ", focusPosition, "\n");
		);
	}

	return focusPosition;
}

/*!
 *\ brief set the focus (and range) at the specified value
 */
stiHResult CstiVideoInputBase2::FocusPositionSet (
	int focusPosition)
{
	PostEvent (
		std::bind(&CstiVideoInputBase2::eventFocusPositionSet, this, focusPosition));

	return stiRESULT_SUCCESS;
}

void CstiVideoInputBase2::eventFocusPositionSet (
	int focusPosition)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::EventFocusPositionSet: focusPosition = ", focusPosition, "\n");
	);
	
	m_focusPosition = focusPosition;
	hResult = m_gstreamerVideoGraph.focusPositionSet (focusPosition);
	stiTESTRESULT ();
	
STI_BAIL:

	return;
}

stiHResult CstiVideoInputBase2::contrastLoopStart (
	std::function<void(int contrast)> callback)
{
	Lock ();

	auto hResult = m_gstreamerVideoGraph.contrastLoopStart (callback);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	return hResult;
}

stiHResult CstiVideoInputBase2::contrastLoopStop ()
{
	Lock ();

	auto hResult = m_gstreamerVideoGraph.contrastLoopStop ();
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	return hResult;
}


void CstiVideoInputBase2::selfViewEnabledSet (
	bool enabled)
{
	PostEvent ([this, enabled]()
	{
		auto hResult = stiRESULT_SUCCESS;
		stiUNUSED_ARG (hResult);
		stiTESTCOND (!(m_systemSleepEnabled && enabled), stiRESULT_ERROR);

		m_selfviewEnabled = enabled;

		if (enabled)
		{
			m_selfviewDisabledIntervalTimer.stop ();
			m_selfviewTransitionTimer.start();
			
			pipelineEnabledSet (true);
		}
		else
		{
			m_allowAmbientLightGet = false;

			if (m_systemSleepEnabled)
			{
				m_selfviewDisabledIntervalTimer.stop ();
			}
			else
			{
				m_selfviewDisabledIntervalTimer.start ();
			}

			pipelineEnabledSet (false);
		}

		if (m_videoOutputSignalConnections.empty())
		{
			IVideoOutputVP *m_videoOutput = IVideoOutputVP::InstanceGet ();
			if (m_videoOutput)
			{
				m_videoOutputSignalConnections.push_back (m_videoOutput->videoFileClosedSignalGet ().Connect (
					[this] (const std::string)
					{
						PostEvent ([this]
						{
							// If the selfview is disabled then reset the enable wait time so that the video playback pipeline
							// has time to complete shutdown.  Otherwise the selfview may crash when restarting the pipeline.
							if (!m_selfviewEnabled)
							{
								m_lastSelfViewEnableTime = std::chrono::steady_clock::now ();
							}
						});
					}));
			}
		}
STI_BAIL:;

	});
}

void CstiVideoInputBase2::pipelineEnabledSet (
	bool enabled)
{
	std::chrono::milliseconds limit(PIPELINE_SHUTDOWN_WAIT_TIME_MS);
	
	// according to Hui Li, There needs to be a 300 ms sleep to allow the camera time to close

	std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now ();
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastSelfViewEnableTime);

	// check for a number  that doesn't make sense
	if (diff.count() < 0)
	{
		diff = std::chrono::milliseconds(0);
	}

	if (diff < limit)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("CstiVideoInputBase2::pipelineEnabledSet: Sleep for %d\n", limit - diff);
		);
		
		std::this_thread::sleep_for(limit - diff);
	}

	if (enabled)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::pipelineEnabledSet: Calling PipelineReset\n");
		);

		m_gstreamerVideoGraph.PipelineReset ();
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::pipelineEnabledSet: Calling PipelineDestroy\n");
		);

		m_gstreamerVideoGraph.PipelineDestroy ();
	}

	m_lastSelfViewEnableTime = std::chrono::steady_clock::now ();
}

/*!
 * \brief Get video codec
 *
 * \param pCodecs - a std::list where the codecs will be stored.
 *
 * \return stiHResult
 */
stiHResult CstiVideoInputBase2::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	
#ifndef stiINTEROP_EVENT
	pCodecs->push_back (estiVIDEO_H265);
	pCodecs->push_back (estiVIDEO_H264);
#else
	if (g_stiEnableVideoCodecs & 4)
	{
		pCodecs->push_back (estiVIDEO_H265);
	}
	
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
stiHResult CstiVideoInputBase2::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;

	switch (eCodec)
	{
		case estiVIDEO_H264:
		{
			auto *pstH264Caps = (SstiH264Capabilities*)pstCaps;

			stProfileAndConstraint.eProfile = estiH264_HIGH;
			stProfileAndConstraint.un8Constraints = 0x0c;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4_1;

			break;
		}
		
		case estiVIDEO_H265:
			stProfileAndConstraint.eProfile = estiH265_PROFILE_MAIN;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		default:
			stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


void CstiVideoInputBase2::eventAutoFocusTimerTimeout ()
{
	stiASSERTMSG (false, "auto focus timed out");
	
	m_focusPosition = FocusPositionGet ();

	focusCompleteSignal.Emit (0);
}


/*!\brief Scales a rectangular area centered in the viewable area based on current PTZ values
 *
 */
void CstiVideoInputBase2::NoLockWindowScale (
	int originalWidth,
	int originalHeight,
	CstiCameraControl::SstiPtzSettings *exposureWindow)
{
	int width = (int)((m_PtzSettings.horzZoom / (float)m_gstreamerVideoGraph.maxCaptureWidthGet()) * originalWidth);
	int height = (int)((m_PtzSettings.vertZoom / (float)m_gstreamerVideoGraph.maxCaptureHeightGet()) * originalHeight);

	//
	// Compute the new AE origin and dimensions based on the PTZ origin and dimensions for the max capture size.
	//
	int left = m_PtzSettings.horzPos + (m_PtzSettings.horzZoom / 2) - width / 2;
	int top = m_PtzSettings.vertPos + (m_PtzSettings.vertZoom / 2);

	//
	// Since the self view is mirrored we need to flip the X component of the origin to the other side.
	//
	left = m_gstreamerVideoGraph.maxCaptureWidthGet() - left - width;

	exposureWindow->horzPos = left;
	exposureWindow->horzZoom = width;
	exposureWindow->vertPos = top;
	exposureWindow->vertZoom = height;
}


/*!\brief Sets the position of the auto exposure windows
 *
 * \return stiHResult
 */
stiHResult CstiVideoInputBase2::NoLockAutoExposureWindowSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCameraControl::SstiPtzSettings exposureWindow {};

	NoLockWindowScale (AE_INNER_WINDOW_WIDTH, AE_INNER_WINDOW_HEIGHT, &exposureWindow);

	hResult = m_gstreamerVideoGraph.exposureWindowSet (exposureWindow);
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInputBase2::NoLockCameraSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = FocusPositionSet (m_focusPosition);	// restore the focus
	stiTESTRESULT ();

	hResult = NoLockAutoExposureWindowSet ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInputBase2::NoLockCameraOpen ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::NoLockCameraOpen\n");
	);
	
	hResult = NoLockCameraSettingsSet ();
	stiTESTRESULT ();

	EventPTZSettingsSet (m_PtzSettings);		// restore the ptz settings

STI_BAIL:

	return (hResult);
}


stiHResult CstiVideoInputBase2::NoLockCameraClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::NoLockCameraClose\n");
	);

//STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoInputBase2::NoLockRcuStatusChanged ()
{

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("Enter CstiVideoInputBase2::NoLockRcuStatusChanged - %s\n", RcuConnectedGet() ? "true" : "false");
	);

	m_gstreamerVideoGraph.RcuConnectedSet (RcuConnectedGet());

	if (RcuConnectedGet())
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("CstiVideoInputBase2::NoLockRcuStatusChanged - connected\n");
		);

		hResult = NoLockCameraOpen ();
		stiTESTRESULT ();
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("CstiVideoInputBase2::NoLockRcuStatusChanged - not connected\n");
		);

		hResult = NoLockCameraClose ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiVideoInputBase2::SelfViewWidgetSet (
	void * QQuickItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::SelfViewWidgetSet: before Lock\n");
	);
	
	Lock ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::SelfViewWidgetSet: after Lock\n");
	);

	hResult = m_gstreamerVideoGraph.SelfViewWidgetSet (QQuickItem);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::SelfViewWidgetSet: after Unlock\n");
	);
	
	return hResult;
}

stiHResult CstiVideoInputBase2::PlaySet (
	bool play)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();

	hResult = m_gstreamerVideoGraph.PlaySet (play);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	return hResult;
}

stiHResult CstiVideoInputBase2::ambientLightGet (
	IVideoInputVP2::AmbientLight *ambiance,
	float *rawVal)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	Lock ();
	
	IVideoInputVP2::AmbientLight ambientLight = IVideoInputVP2::AmbientLight::UNKNOWN;
	float rawAmbientLight = 0;
	
	if (m_selfviewEnabled)
	{
		if (m_allowAmbientLightGet)
		{			
			hResult = m_gstreamerVideoGraph.ambientLightGet (ambientLight, rawAmbientLight);
			stiTESTRESULT ();

			if (ambiance)
			{
				*ambiance = ambientLight;

				m_lastAmbientLight = ambientLight;
				
				stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
					vpe::trace ("CstiVideoInputBase2::ambientLightGet: returning ambientLight = ", (int)ambientLight, "\n");
				)
			}
			
			if (rawVal)
			{
				*rawVal = rawAmbientLight;
				
				stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
					vpe::trace ("CstiVideoInputBase2::ambientLightGet: returning rawAmbientLight = ", rawAmbientLight, "\n");
				);
			}
		}
		else
		{
			*ambiance = m_lastAmbientLight;
		}
	}
	else
	{
		*ambiance = m_lastAmbientLight;
	}

STI_BAIL:

	Unlock ();
	
	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::ambientLightGet: Error, returning ambiance = ", ambiance, " and rawVal = ", rawVal, "\n");
		);
	}

	return stiRESULT_SUCCESS;
}

void CstiVideoInputBase2::currentTimeSet (
	const time_t currentTime)
{
	Lock ();
	
	timespec currentTimespec {currentTime, 0};
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
					char buf[100];
					double timeDelta = 0.0;
					timespec systemTimespec {};
					
					clock_gettime (CLOCK_REALTIME, &systemTimespec);
					std::strftime (buf, sizeof buf, "%D %T", std::gmtime(&systemTimespec.tv_sec));
					stiTrace ("CstiVideoInputBase2::currentTimeSet: current time is: %s.%09ld UTC\n", buf, systemTimespec.tv_nsec);
					
					std::strftime (buf, sizeof buf, "%D %T", std::gmtime(&currentTimespec.tv_sec));
					stiTrace ("CstiVideoInputBase2::currentTimeSet: setting current time to: %s.%09ld UTC\n", buf, currentTimespec.tv_nsec);
					
					timeDelta = difftime (currentTime, systemTimespec.tv_sec);
					timeDelta = abs (timeDelta);
					stiTrace ("CstiVideoInputBase2::currentTimeSet: timeDelta = %f\n", timeDelta);
	);
	
	// TODO: Intel bug. This is needed because gstreamer/icamerasrc/libcamhal
	// have a problem when we set system time. (libcamhal timeout)
	m_gstreamerVideoGraph.EncodePipelineStatePauseSet ();
	clock_settime (CLOCK_REALTIME, &currentTimespec);
	m_gstreamerVideoGraph.EncodePipelineStatePlaySet ();
	
	Unlock ();
	
}

stiHResult CstiVideoInputBase2::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase2::VideoRecordSettingsSet);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::VideoRecordSettingsSet\n");
	);
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase2::VideoRecordSettingsSet: after lock\n");
	);
	
	m_videoRecordSettings = *pVideoRecordSettings;
	
	if (m_encodeSettingsSetQueued)
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::VideoRecordSettingsSet: eventEncodeSettingsSet already queued\n");
		);
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::VideoRecordSettingsSet: eventEncodeSettingsSet not queued, adding to queue\n");
		);
		
		m_encodeSettingsSetQueued = true;
		PostEvent (std::bind(&CstiVideoInputBase2::eventMarshalEncodeSettingsSet, this));
	}
	
	return stiRESULT_SUCCESS;
}	
	
void CstiVideoInputBase2::eventMarshalEncodeSettingsSet ()
{
	
	// The encode pipeline (icamersrc) seems to have trouble
	// if a size change happens too frequently
	std::chrono::milliseconds limit(PIPELINE_SIZE_CHANGE_WAIT_TIME_MS);
	
	std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now ();
	auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastEncodeSettingsSetTime);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::eventMarshalEncodeSettingsSet: Time since last encode settings call = %d\n", diff);
	);
	
	// Make it zero if negative
	if (diff.count() < 0)
	{
		diff = std::chrono::milliseconds(0);
	}

	if (diff < limit)
	{
		long timerWaitTime = limit.count() - diff.count();
		
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			stiTrace ("CstiVideoInputBase2::eventMarshalEncodeSettingsSet: starting timer to add eventEncodeSettingsSet to queue in %d milliseconds\n", timerWaitTime);
		);
			
		m_encodeSettingsSetTimer.timeoutSet (timerWaitTime);
		m_encodeSettingsSetTimer.start ();
		
	}
	else
	{
	
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase2::eventMarshalEncodeSettingsSet: adding eventEncodeSettingsSet to queue\n");
		);
			
		PostEvent (std::bind(&CstiVideoInputBase2::eventEncodeSettingsSet, this));
	}
	
	return;
}

void CstiVideoInputBase2::eventEncodeSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::eventEncodeSettingsSet start\n");
	);

	// If we are wanting to send portrait from nVP2 we need to make sure the height is
	// valid. It was also decided to only send a 3:4 aspect ratio (75% of height).
	if (m_videoRecordSettings.remoteDisplayWidth < m_videoRecordSettings.remoteDisplayHeight)
	{
		const double fourByThreeAspectRatio = 4.0 / 3.0;
		const int macroBlockSize = 16;

		if (m_videoRecordSettings.unRows > static_cast<unsigned int>(m_pCameraControl->defaultSelfViewHeightGet ()))
		{
			m_videoRecordSettings.unRows = static_cast<unsigned int>(m_pCameraControl->defaultSelfViewHeightGet ());
		}

		m_videoRecordSettings.unColumns = static_cast<unsigned int>(m_videoRecordSettings.unRows * fourByThreeAspectRatio);
		// nVP2 needs to have width that is a multiple of 16 to prevent errors.
		m_videoRecordSettings.unColumns -= m_videoRecordSettings.unColumns % macroBlockSize;
	}

	bool bEncodeSizeChanged = false;
	bool remoteWantsPortraitVideo = (m_videoRecordSettings.remoteDisplayWidth < m_videoRecordSettings.remoteDisplayHeight);

	hResult = videoGraphBaseGet ().EncodeSettingsSet (m_videoRecordSettings, &bEncodeSizeChanged);
	stiTESTRESULT ();
	
	m_lastEncodeSettingsSetTime = std::chrono::steady_clock::now ();

	if (m_portraitVideo != remoteWantsPortraitVideo)
	{
		m_portraitVideo = remoteWantsPortraitVideo;
		m_pCameraControl->RemoteDisplayOrientationChanged (
				static_cast<int>(m_videoRecordSettings.unColumns),
				static_cast<int>(m_videoRecordSettings.unRows));
	}

	if (bEncodeSizeChanged)
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			stiTrace("CstiVideoInputBase2::eventEncodeSettingsSet: size changed\n");
		);

		if (!m_recordingToFile)
		{
			videoRecordSizeChangedSignal.Emit();
		}
	}

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::eventEncodeSettingsSet end\n");
	);

	m_encodeSettingsSetQueued = false;

	if (m_videoRecordToFileSet)
	{
		m_videoRecordToFileSet = false;
		m_recordingToFile = true;
		m_gstreamerVideoGraph.videoRecordToFile();
	}

	if (m_videoEncodeStart)
	{
		m_videoEncodeStart = false;
		EventEncodeStart ();
	}

STI_BAIL:
	return;
}

void CstiVideoInputBase2::videoRecordToFile()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::videoRecordToFile);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::videoRecordToFile\n");
	);

	PostEvent (
		[this]
		{
			if (!m_encodeSettingsSetQueued)
			{
				m_videoRecordToFileSet = false;
				m_recordingToFile = true;
				m_gstreamerVideoGraph.videoRecordToFile();
			}
			else
			{
				m_videoRecordToFileSet = true;
			}
		});
}


stiHResult CstiVideoInputBase2::EncodePipelineReset ()
{
	return	m_gstreamerVideoGraph.PipelineReset();
}


stiHResult CstiVideoInputBase2::EncodePipelineDestroy ()
{
	return	m_gstreamerVideoGraph.PipelineDestroy();
}

void CstiVideoInputBase2::systemSleepSet (bool enabled)
{
	m_selfviewDisabledIntervalTimer.stop ();
	m_systemSleepEnabled = enabled;
}

stiHResult CstiVideoInputBase2::VideoRecordStart ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase2::VideoRecordStart);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase2::VideoRecordStart\n");
	);

	PostEvent (
		[this]
		{
			if (!m_encodeSettingsSetQueued)
			{
				m_videoEncodeStart = false;
				EventEncodeStart ();
			}
			else
			{
				m_videoEncodeStart = true;
			}
		});

	return stiRESULT_SUCCESS;
}

