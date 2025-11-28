#include "CstiGStreamerVideoGraphBase.h"
#include "GStreamerAppSink.h"
#include "GStreamerPad.h"
#include "stiTrace.h"
#include <cmath>


CstiGStreamerVideoGraphBase::CstiGStreamerVideoGraphBase(
	bool rcuConnected)
:
	m_bRcuConnected (rcuConnected)
{
}

CstiGStreamerVideoGraphBase::~CstiGStreamerVideoGraphBase()
{
	if (m_pFrameCaptureMutex)
	{
		stiOSMutexDestroy (m_pFrameCaptureMutex);
		m_pFrameCaptureMutex = nullptr;
	}
}


stiHResult CstiGStreamerVideoGraphBase::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_PtzSettings.horzPos = 0;
	m_PtzSettings.horzZoom = encodeCapturePipelineBaseGet().maxCaptureWidthGet();
	m_PtzSettings.vertPos = 0;
	m_PtzSettings.vertZoom = encodeCapturePipelineBaseGet().maxCaptureHeightGet();

	m_DisplayImageSettings.bVisible = false;
	m_DisplayImageSettings.unWidth = encodeCapturePipelineBaseGet().maxCaptureWidthGet();
	m_DisplayImageSettings.unHeight = encodeCapturePipelineBaseGet().maxCaptureHeightGet();
	m_DisplayImageSettings.unPosX = 0;
	m_DisplayImageSettings.unPosY = 0;

	hResult = cpuSpeedSet (CstiBSPInterfaceBase::estiCPU_SPEED_MED);
	stiTESTRESULT ();
	
	if (!m_pFrameCaptureMutex)
	{
		m_pFrameCaptureMutex = stiOSNamedMutexCreate ("FrameCaptureMutex");
	}

	m_frameCaptureBin = frameCaptureBinCreate();

	hResult = m_frameCaptureBin->startup();

	hResult = m_frameCaptureBin->frameCaptureBinStart ();
	stiTESTRESULT ();

	m_signalConnections.push_back (m_frameCaptureBin->frameCaptureReceivedSignal.Connect (
		[this](GStreamerBuffer &gstBuffer)
		{
			frameCaptureReceivedSignal.Emit (gstBuffer);
		}));

STI_BAIL:

	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase::cpuSpeedSet (
	CstiBSPInterfaceBase::EstiCpuSpeed cpuSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase::cpuSpeedSet cpuSpeed = %d\n", cpuSpeed);
		stiTrace ("CstiGStreamerVideoGraphBase::cpuSpeedSet m_bEncoding = %d\n", m_bEncoding);
	);
	
	auto bspInterface = dynamic_cast<CstiBSPInterfaceBase *>(IstiPlatform::InstanceGet());

	// Set the cpu speed unless in a call
	if (bspInterface)
	{
		hResult = bspInterface->cpuSpeedSet (cpuSpeed);
		stiTESTRESULT ();
	}
	else
	{
		stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
			stiTrace ("CstiGStreamerVideoGraphBase::cpuSpeedSet: Warning, no bspInterface. Not calling cpuSpeedSet\n");
		);
	}
	
STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::FrameCaptureSet (
	bool bFrameCapture,
	SstiImageCaptureSize *pImageSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiGStreamerVideoGraph::FrameCaptureSet: bFrameCapture = ", bFrameCapture, "\n");
	);

	bool crop {false};
	SstiImageCaptureSize imageSize;

	if (pImageSize && bFrameCapture)
	{
		imageSize = *pImageSize;
		crop = true;
	}

	// Set the displaymode and size for the capture frame pipeline
	m_frameCaptureBin->displayModeSet(m_eDisplayMode);
	m_frameCaptureBin->displaySizeSet(m_VideoRecordSettings.unColumns,
										   m_VideoRecordSettings.unRows);

	encodeCapturePipelineBaseGet().frameCaptureCallbackSet (
		[this, imageSize, crop] (const GStreamerPad &gstPad, GStreamerBuffer &gstBuffer)
		{
			m_frameCaptureBin->frameCapture (gstPad, gstBuffer, crop, imageSize);
		});

	return hResult;
}


stiHResult CstiGStreamerVideoGraphBase::PrivacySet (
	bool bEnable)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase::PrivacySet bEnable = %d\n", bEnable);
	);

	m_bPrivacy = bEnable;

//STI_BAIL:

	return hResult;
}

void CstiGStreamerVideoGraphBase::DisplaySettingsGet (
	IstiVideoOutput::SstiImageSettings *pImageSettings) const
{
	pImageSettings->unPosX = m_DisplayImageSettings.unPosX;
	pImageSettings->unPosY = m_DisplayImageSettings.unPosY;
	pImageSettings->unWidth = m_DisplayImageSettings.unWidth;
	pImageSettings->unHeight = m_DisplayImageSettings.unHeight;
}


stiHResult CstiGStreamerVideoGraphBase::VideoRecordSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bEncoding)
	{
		*pun32Width = m_VideoRecordSettings.unColumns;
		*pun32Height = m_VideoRecordSettings.unRows;
	}
	else
	{
		*pun32Width =  maxCaptureWidthGet ();
		*pun32Height =  maxCaptureHeightGet ();
		//*pun32Height =  1080;
	}

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiGStreamerVideoGraphBase::VideoRecordSizeGet m_bEncoding = %d\n", m_bEncoding);
		stiTrace ("CstiGStreamerVideoGraphBase::VideoRecordSizeGet *pun32Width = %d\n", *pun32Width);
		stiTrace ("CstiGStreamerVideoGraphBase::VideoRecordSizeGet *pun32Height = %d\n", *pun32Height);
	);

//STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::OutputModeSet (
	EDisplayModes eMode)
{
	m_eDisplayMode = eMode;

	return stiRESULT_SUCCESS;
}


stiHResult CstiGStreamerVideoGraphBase::PTZSettingsSet (
	const CstiCameraControl::SstiPtzSettings &ptzSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiCapturePTZDebug || g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase::PTZSettingsSet\n");
	);

	// Store teh new PTZ settings.
	m_PtzSettings = ptzSettings;

	// Set the new PTZ settings.
	hResult = PTZVideoConvertUpdate ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


#define ZOOM_MULTIPLE 2

stiHResult CstiGStreamerVideoGraphBase::PTZVideoConvertUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCameraControl::SstiPtzSettings newPtzSettings{};

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiCapturePTZDebug || g_stiVideoInputDebug,
		stiTrace("CstiGStreamerVideoGraphBase::PTZVideoConvertUpdate\n");
	);

	newPtzSettings.horzPos = m_PtzSettings.horzPos;
	newPtzSettings.horzZoom = m_PtzSettings.horzZoom;
	newPtzSettings.vertPos = m_PtzSettings.vertPos;
	newPtzSettings.vertZoom = m_PtzSettings.vertZoom;

	if (m_bEncoding)
	{
		double newRatio = static_cast<double>(m_VideoRecordSettings.unColumns) / m_VideoRecordSettings.unRows;
		double captureRatio = static_cast<double>(newPtzSettings.horzZoom) / newPtzSettings.vertZoom;

		// If the captureRatio is larger than the newRatio then adjust the Horizontal size and position.
		if ((int)(captureRatio * 10.0) > (int)(newRatio * 10.0))
		{
			newPtzSettings.horzZoom = static_cast<int>(lround(newRatio * newPtzSettings.vertZoom));
		}
		// If the captureRatio is less than the newRatio then adjust the vertical size and position.
		else if ((int)(captureRatio * 10.0) < (int)(newRatio * 10.0))
		{
			newPtzSettings.vertZoom = static_cast<int>(lround(newPtzSettings.horzZoom / newRatio));
		}

		if (newPtzSettings.horzZoom == m_PtzSettings.horzZoom)
		{
			newPtzSettings.vertPos = newPtzSettings.vertPos + (m_PtzSettings.vertZoom - newPtzSettings.vertZoom) / 2;
		}
		else if (newPtzSettings.vertZoom == m_PtzSettings.vertZoom)
		{
			newPtzSettings.horzPos = newPtzSettings.horzPos + (m_PtzSettings.horzZoom - newPtzSettings.horzZoom) / 2;
		}
	}

	newPtzSettings.horzZoom = newPtzSettings.horzZoom - (newPtzSettings.horzZoom % ZOOM_MULTIPLE);
	newPtzSettings.vertZoom = newPtzSettings.vertZoom - (newPtzSettings.vertZoom % ZOOM_MULTIPLE);

	newPtzSettings.horzPos = newPtzSettings.horzPos - (newPtzSettings.horzPos % ZOOM_MULTIPLE);
	newPtzSettings.vertPos = newPtzSettings.vertPos - (newPtzSettings.vertPos % ZOOM_MULTIPLE);

	ptzRectangleSet (newPtzSettings);

//STI_BAIL:

	return hResult;

}


CstiCameraControl::SstiPtzSettings CstiGStreamerVideoGraphBase::ptzRectangleGet () const
{
	return m_PtzSettings;
}

void CstiGStreamerVideoGraphBase::ptzRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	m_PtzSettings = ptzRectangle;
	
	encodeCapturePipelineBaseGet ().ptzRectangleSet (ptzRectangle);
}


void CstiGStreamerVideoGraphBase::FrameCountGet (
	uint64_t *frameCount,
	std::chrono::steady_clock::time_point *lastFrameTime)
{
	encodeCapturePipelineBaseGet ().frameCountGet (frameCount, lastFrameTime);
}


float CstiGStreamerVideoGraphBase::aspectRatioGet () const
{
	return encodeCapturePipelineBaseGet ().aspectRatioGet ();
}


stiHResult CstiGStreamerVideoGraphBase::KeyFrameRequest ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bEncoding)
	{
		stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiGStreamerVideoGraph::KeyFrameRequest\n");
		);

		encodeCapturePipelineBaseGet ().keyframeEventSend ();
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug || g_stiVideoInputDebug,
			vpe::trace ("WARNING: CstiGStreamerVideoGraph::KeyFrameRequest not encoding\n");
		);
	}

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::brightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().brightnessRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::brightnessGet (
	int *brightness) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().brightnessGet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().brightnessSet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::saturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().saturationRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::saturationGet (
	int *saturation) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().saturationGet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiGStreamerVideoGraphBase::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = encodeCapturePipelineBaseGet().saturationSet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

