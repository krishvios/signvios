#include "CstiGStreamerVideoGraphBase2.h"
#include "GStreamerAppSink.h"
#include "GStreamerPad.h"
#include "stiDebugTools.h"
#include "CstiBSPInterfaceBase.h"
#include "MediaPipeline.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <cmath>
#include <functional>

CstiGStreamerVideoGraphBase2::CstiGStreamerVideoGraphBase2 (
	bool rcuConnected)
:
	CstiGStreamerVideoGraphBase(rcuConnected)
{
}


CstiGStreamerVideoGraphBase2::~CstiGStreamerVideoGraphBase2 ()
{
	EncodePipelineDestroy ();
}

stiHResult CstiGStreamerVideoGraphBase2::PipelineReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = EncodePipelineStart ();
	stiTESTRESULT ();
	
STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraphBase2::PipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = EncodePipelineDestroy ();
		stiTESTRESULT ();
	
STI_BAIL:

	return (hResult);
}


int32_t CstiGStreamerVideoGraphBase2::MaxPacketSizeGet ()
{
	return m_VideoRecordSettings.unMaxPacketSize;
}


stiHResult CstiGStreamerVideoGraphBase2::EncodeSettingsSet (
	const IstiVideoInput::SstiVideoRecordSettings &videoRecordSettings,
	bool *pbEncodeSizeChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::EncodeSettingsSet);
	
	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug || g_stiVideoEncodeTaskDebug,
		vpe::trace("CstiGStreamerVideoGraphBase2::EncodeSettingsSet m_bEncoding = ", m_bEncoding, "\n");
	);

	auto sizeChanged = m_VideoRecordSettings.unColumns != videoRecordSettings.unColumns ||
					   m_VideoRecordSettings.unRows != videoRecordSettings.unRows;

	auto bitrateChanged = m_VideoRecordSettings.unTargetBitRate != videoRecordSettings.unTargetBitRate;

	auto framerateChanged = m_VideoRecordSettings.unTargetFrameRate != videoRecordSettings.unTargetFrameRate;

	auto codecChanged = m_VideoRecordSettings.codec != videoRecordSettings.codec ||
						m_VideoRecordSettings.eProfile != videoRecordSettings.eProfile ||
						m_VideoRecordSettings.unConstraints != videoRecordSettings.unConstraints ||
						m_VideoRecordSettings.unLevel != videoRecordSettings.unLevel ||
						m_VideoRecordSettings.ePacketization != videoRecordSettings.ePacketization;

	m_VideoRecordSettings = videoRecordSettings;

	if (sizeChanged)
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug || g_stiVideoEncodeTaskDebug,
			vpe::trace("CstiGStreamerVideoGraphBase2::EncodeSettingsSet - Size changed: ", m_VideoRecordSettings.unColumns, "x", m_VideoRecordSettings.unRows, "\n");
		);
		
		if (m_bEncoding)
		{
			hResult = PTZVideoConvertUpdate ();
		}

		encodeCapturePipelineBase2Get().encodeSizeSet (m_VideoRecordSettings.unColumns, m_VideoRecordSettings.unRows);

		if (pbEncodeSizeChanged)
		{
			*pbEncodeSizeChanged = true;
		}
	}

	if (bitrateChanged)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiGStreamerVideoGraphBase2::EncodeSettingsSet - Bitrate changed: ", m_VideoRecordSettings.unTargetBitRate, "\n");
		);

		encodeCapturePipelineBase2Get().bitrateSet(m_VideoRecordSettings.unTargetBitRate / 1000);
	}

	if (framerateChanged)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiGStreamerVideoGraphBase2::EncodeSettingsSet - Frame rate changed: ", m_VideoRecordSettings.unTargetFrameRate, "\n");
		);

		encodeCapturePipelineBase2Get().framerateSet(m_VideoRecordSettings.unTargetFrameRate);
	}

	if (codecChanged)
	{
		encodeCapturePipelineBase2Get().codecSet(
			m_VideoRecordSettings.codec,
			m_VideoRecordSettings.eProfile,
			m_VideoRecordSettings.unConstraints,
			m_VideoRecordSettings.unLevel,
			m_VideoRecordSettings.ePacketization);
	}

	return (hResult);

}

IstiVideoInput::SstiVideoRecordSettings CstiGStreamerVideoGraphBase2::EncodeSettingsGet () const
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase2::EncodeSettingsGet\n");
	);
	
	return m_VideoRecordSettings;
}


stiHResult CstiGStreamerVideoGraphBase2::RcuConnectedSet (
	bool bRcuConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
				  vpe::trace ("CstiGStreamerVideoGraphBase2::RcuConnectedSet (", bRcuConnected, ")\n");
	);

	rcuConnectedSet (bRcuConnected);

	if (rcuConnectedGet())
	{
		hResult = EncodePipelineStart ();
		stiTESTRESULT ();
	}
	else
	{
		hResult = EncodePipelineDestroy ();
		stiTESTRESULT ();
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraphBase2::SelfViewWidgetSet (
	void *qquickItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase2::SelfViewWidgetSet: setting QQuickItem = %p\n", qquickItem);
	);

	m_qquickItem = qquickItem;
	
	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase2::DisplaySettingsSet (
	IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	m_DisplayImageSettings = *pImageSettings;

	return stiRESULT_SUCCESS;
}


void CstiGStreamerVideoGraphBase2::encodeCapturePipelineCreate (
	void *qquickItem)
{
	encodeCapturePipelineBase2Get().create (qquickItem);
}


stiHResult CstiGStreamerVideoGraphBase2::exposureWindowSet (
	const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = encodeCapturePipelineBase2Get().exposureWindowSet (exposureWindow);
	stiTESTRESULT ();
		
STI_BAIL:
	
	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::brightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().brightnessRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::brightnessGet (
	int *brightness) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().brightnessGet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().brightnessSet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::saturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().saturationRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::saturationGet (
	int *saturation) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().saturationGet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().saturationSet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::focusRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().focusRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::focusPositionGet (
	int *focusPosition) const
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = encodeCapturePipelineBase2Get().focusPositionGet (focusPosition);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::focusPositionSet (
	int focusPosition)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().focusPositionSet (focusPosition);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::singleFocus (
	int initialFocusPosition,
	int focusRegionStartX,
	int focusRegionStartY,
	int focusRegionWidth,
	int focusRegionHeight,
	std::function <void(bool success, int contrast)> callback)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().singleFocus (
		initialFocusPosition,
		focusRegionStartX,
		focusRegionStartY,
		focusRegionWidth,
		focusRegionHeight,
		callback);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::contrastLoopStart (
	std::function<void(int contrast)> callback)
{
	auto hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().contrastLoopStart (callback);
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::contrastLoopStop ()
{
	auto hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBase2Get().contrastLoopStop ();
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::ambientLightGet (
	IVideoInputVP2::AmbientLight &ambientLight,
	float &rawAmbientLight)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	if (EncodePipelineCreated())
	{
		hResult = encodeCapturePipelineBase2Get().ambientLightGet (ambientLight, rawAmbientLight);
		stiTESTRESULT ();
		
		m_lastAmbientLight = ambientLight;
		m_lastRawAmbientLight = rawAmbientLight;
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
			vpe::trace ("CstiGStreamerVideoGraphBase2::ambientLightGet: encode pipeline not created, using last values", "\n");
		);
		
		ambientLight = m_lastAmbientLight;
		
		rawAmbientLight = m_lastRawAmbientLight;
	}
	
	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		vpe::trace ("CstiGStreamerVideoGraphBase2::ambientLightGet: ambientLight = ", (int)ambientLight, " and ambientRawVal = ", rawAmbientLight, "\n");
	);
	
	
STI_BAIL:
	
	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase2::EncodePipelineCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiGStreamerVideoGraphBase2::EncodePipelineCreate begin\n");
	);

	if (!m_EncodeCapturePipelineCreated)
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
				vpe::trace("CstiGStreamerVideoGraphBase2::EncodePipelineCreate calling EncodePipelineDestroy\n");
		);
		hResult = EncodePipelineDestroy ();
		stiTESTRESULT ();

		//
		// Only create the capture pipeline if the camera is connected.
		//
		if (rcuConnectedGet())
		{
			// Setup the capture pipeline
			stiASSERT (encodeCapturePipelineBase2Get().get () == nullptr);

			encodeCapturePipelineCreate (m_qquickItem);
			stiTESTCOND (encodeCapturePipelineBase2Get().get (), stiRESULT_ERROR);

			encodeCapturePipelineBase2Get().ptzRectangleSet (ptzRectangleGet());
			
			m_encodeCameraErrorConnection = encodeCapturePipelineBase2Get ().cameraErrorSignal.Connect (
				[this]
				{
					encodeCameraError.Emit ();
				});

			m_aspectRationChangedConnection = encodeCapturePipelineBase2Get().aspectRatioChanged.Connect (
				[this]
				{
					aspectRatioChanged.Emit ();
				});

			m_encodeBitstreamReceivedConnection =  encodeCapturePipelineBase2Get().encodeBitstreamReceivedSignal.Connect (
				[this](GStreamerSample &gstSample)
				{
					encodeBitstreamReceivedSignal.Emit (gstSample);
				});

			hResult = PTZVideoConvertUpdate ();
			stiTESTRESULT ();

			if (m_frameCaptureBin->get())
			{
				encodeCapturePipelineBase2Get().elementAdd(*m_frameCaptureBin);
			}
		}

		m_EncodeCapturePipelineCreated = true;
	}

STI_BAIL:

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiGStreamerVideoGraphBase2::EncodePipelineCreate end\n");
	);

	return (hResult);
}


stiHResult CstiGStreamerVideoGraphBase2::EncodePipelineDestroy ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineDestroy: begin\n");
	);

	m_bEncodePipelinePlaying = false;

	if (encodeCapturePipelineBase2Get().get ())
	{
		auto StateChangeReturn = encodeCapturePipelineBase2Get().stateSet (GST_STATE_NULL);
		stiTESTCONDMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR,
				"CstiGStreamerVideoGraphBase2::EncodePipelineDestroy: Could not gst_element_set_state (m_pGstElementEncodeCapturePipeline, GST_STATE_NULL): %d\n", StateChangeReturn);

		encodeCapturePipelineBase2Get().clear ();
	}
	
	m_EncodeCapturePipelineCreated = false;

STI_BAIL:

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineDestroy end\n");
	);

	return (hResult);
}


stiHResult CstiGStreamerVideoGraphBase2::EncodePipelineStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineStart\n");
	);

	if (rcuConnectedGet())
	{
		// Reset all record settings to create a reproducible
		// pipeline every time
		EncodeSettingsSet (defaultVideoRecordSettingsGet (), nullptr);

		hResult = EncodePipelineCreate ();
		stiTESTRESULT ();

		if (encodeCapturePipelineBase2Get().get ())
		{
			GstStateChangeReturn StateChangeReturn;

			StateChangeReturn = encodeCapturePipelineBase2Get().stateSet (GST_STATE_PLAYING);
			stiTESTCONDMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR, "CstiGStreamerVideoGraphBase2::EncodePipelineStart: GST_STATE_CHANGE_FAILURE: gst_element_set_state (encodeCapturePipelineBase2Get(), GST_STATE_PLAYING)\n");
		}

	}

	m_bEncodePipelinePlaying = true;

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraphBase2::EncodePipelineStatePlaySet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineStatePlaySet\n");
	);

	if (m_EncodeCapturePipelineCreated)
	{
		if (encodeCapturePipelineBase2Get().get ())
		{
			GstStateChangeReturn StateChangeReturn;
			
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineStatePlaySet: Calling stateSet\n");
			);

			StateChangeReturn = encodeCapturePipelineBase2Get().stateSet (GST_STATE_PLAYING);
			stiTESTCONDMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR, "CstiGStreamerVideoGraphBase2::EncodePipelineStatePlaySet: GST_STATE_CHANGE_FAILURE: gst_element_set_state (encodeCapturePipelineBase2Get(), GST_STATE_PLAYING)\n");
		}
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraphBase2::EncodePipelineStatePauseSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineStatePauseSet\n");
	);

	if (m_EncodeCapturePipelineCreated)
	{
		if (encodeCapturePipelineBase2Get().get ())
		{
			GstStateChangeReturn StateChangeReturn;
			
			stiDEBUG_TOOL (g_stiVideoInputDebug,
				stiTrace("CstiGStreamerVideoGraphBase2::EncodePipelineStatePauseSet: Calling stateSet\n");
			);

			StateChangeReturn = encodeCapturePipelineBase2Get().stateSet (GST_STATE_PAUSED);
			stiTESTCONDMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR, "CstiGStreamerVideoGraphBase2::EncodePipelineStatePauseSet: GST_STATE_CHANGE_FAILURE: gst_element_set_state (encodeCapturePipelineBase2Get(), GST_STATE_PAUSED)\n");
		}
	}

STI_BAIL:

	return (hResult);
}

stiHResult CstiGStreamerVideoGraphBase2::KeyFrameForce ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
    auto result = false;
	GstEvent *event = nullptr;

	stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiGStreamerVideoGraph::KeyFrameForce\n");
	);

	event = gst_video_event_new_downstream_force_key_unit (GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, TRUE, 1);
    stiTESTCOND(event, stiRESULT_ERROR);

	result = encodeCapturePipelineBase2Get().eventSend (event);
    stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase2::EncodeStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraph::EncodeStart);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiGStreamerVideoGraph::EncodeStart\n");
	);

	// Set the cpu speed to high for calls
	hResult = cpuSpeedSet (CstiBSPInterfaceBase::estiCPU_SPEED_HIGH);
	stiTESTRESULT();
	
	hResult = KeyFrameForce();
	stiTESTRESULT();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiGStreamerVideoGraph::EncodeStart: before PTZVideoConvertUpdate\n");
	);
	//
	// We want to indicate we are encoding regardless of the outcome
	// of setting up the encode pipeline. If the camera is unplugged it
	// will get corrected, and start recording, once it is plugged back in.
	//
	m_bEncoding = true;

	encodeCapturePipelineBase2Get().encodingStart();

	{
		auto ptzRectangle = ptzRectangleGet();

		// Adjust the PTZ setting for the current encode settings.
		if (m_VideoRecordSettings.unColumns != static_cast<unsigned int>(ptzRectangle.horzZoom) ||
			m_VideoRecordSettings.unRows != static_cast<unsigned int>(ptzRectangle.vertZoom))
		{
			hResult = PTZVideoConvertUpdate ();
			stiTESTRESULT();
		}
	}
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiGStreamerVideoGraph::EncodeStart: end\n");
	);

STI_BAIL:
	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase2::EncodeStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiGStreamerVideoGraphBase2::EncodeStop);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase2::EncodeStop\n");
	);
	
	m_bEncoding = false;

	// Check to see if we are recording a video.
	encodeCapturePipelineBase2Get().encodingStop ();

	hResult = EncodePipelineStart ();

	hResult = PTZVideoConvertUpdate ();
	
	// Set the cpu speed to medium when not in a call
	hResult = cpuSpeedSet (CstiBSPInterfaceBase::estiCPU_SPEED_MED);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase2::PlaySet (
	bool play)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiGStreamerVideoGraphBase2::PlaySet:\n");
	);
	
	hResult = EncodePipelineStart ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;

}


stiHResult CstiGStreamerVideoGraphBase2::videoRecordToFile()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiGStreamerVideoGraphBase2::videoRecordToFile \n");
	);
	
	stiTESTCOND (encodeCapturePipelineBase2Get().get (), stiRESULT_ERROR);

	encodeCapturePipelineBase2Get().recordBinInitialize(m_VideoRecordSettings.recordFastStartFile);

STI_BAIL:

	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase2::KeyFrameRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bEncoding)
	{
		stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiGStreamerVideoGraphBase2::KeyFrameRequest\n");
		);

		hResult = KeyFrameForce ();
		stiTESTRESULT ();
		
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
			vpe::trace ("WARNING: CstiGStreamerVideoGraphBase2::KeyFrameRequest not encoding\n");
		);
	}

STI_BAIL:

	return hResult;
}



void CstiGStreamerVideoGraphBase2::ptzRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	m_PtzSettings = ptzRectangle;
	
	encodeCapturePipelineBase2Get ().ptzRectangleSet (ptzRectangle);
}
