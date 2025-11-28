// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoInputBase.h"
#include "stiTrace.h"
#include "H263Tools.h"
#include "MonitorTaskBase.h"
#include "CstiCameraControl.h"
#include "CstiGStreamerVideoGraphBase.h"
#include <cmath>

constexpr const int GSTREAMER_BITSTREAM_NUM_BUFS = 15;

CstiVideoInputBase::CstiVideoInputBase ()
:
	CstiEventQueue ("CstiVideoInput")
{
	m_signalConnections.push_back (m_frameTimer.timeoutEvent.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoInputBase::EventFrameTimerTimeout, this));
		}));
}


CstiVideoInputBase::~CstiVideoInputBase ()
{
	CstiEventQueue::StopEventLoop();

	auto pGstreamerBufferElement  = m_oGstreamerBitstreamBufferFifo.Get ();

	while (nullptr != pGstreamerBufferElement)
	{
		delete pGstreamerBufferElement;
		pGstreamerBufferElement  = m_oGstreamerBitstreamBufferFifo.Get ();
	}

	// Shutdown the timers so we don't get any events from them.
	m_frameTimer.Stop();

	if (m_pCameraControl)
	{
		delete m_pCameraControl;
		m_pCameraControl = nullptr;
	}
}


void EventHdmiOutStatusChanged ()
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase::EventHdmiOutStatusChanged\n");
	);
}


stiHResult CstiVideoInputBase::monitorTaskInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Find out if the RCU is connected and setup monitoring so we know when
	// the connection state changes.
	//
	m_signalConnections.push_back (monitorTaskBaseGet()->hdmiOutStatusChangedSignal.Connect (
		[this]() {
			PostEvent(
				[]
				{
					EventHdmiOutStatusChanged ();
				});
		}));

	return hResult;
}

stiHResult CstiVideoInputBase::rcuStatusInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bRcuConnected = false;

	hResult = monitorTaskBaseGet()->RcuConnectedStatusGet (&bRcuConnected);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
				  vpe::trace("CstiVideoInputBase::rcuStatusInitialize: bRcuConnected = ", bRcuConnected, "\n");
	);
	
	RcuConnectedSet (bRcuConnected);

	// Subscribe to the MonitorTask's event signals
	m_signalConnections.push_back (monitorTaskBaseGet()->usbRcuConnectionStatusChangedSignal.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoInputBase::EventUsbRcuConnectionStatusChanged, this));
		}));

	m_signalConnections.push_back (monitorTaskBaseGet()->usbRcuResetSignal.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoInputBase::EventUsbRcuReset, this));
		}));
STI_BAIL:

	return hResult;
}

EstiBool CstiVideoInputBase::RcuConnectedGet () const
{
	std::lock_guard<std::mutex> lock(m_RcuConnectedMutex);
	return (EstiBool)videoGraphBaseGet().rcuConnectedGet();
}

void CstiVideoInputBase::RcuConnectedSet (
	bool bRcuConnected)
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace("CstiVideoInputBase::RcuConnectedSet (", bRcuConnected, ")\n");
	);
	
	std::lock_guard<std::mutex> lock(m_RcuConnectedMutex);
	videoGraphBaseGet().rcuConnectedSet(bRcuConnected);
}

stiHResult CstiVideoInputBase::gstreamerVideoGraphInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = videoGraphBaseGet ().Initialize ();
	stiTESTRESULT ();

	m_signalConnections.push_back (videoGraphBaseGet ().pipelineResetSignal.Connect (
		[this]() {
			PostEvent(
				std::bind(&CstiVideoInputBase::eventPipelineReset, this));
		}));

	m_signalConnections.push_back (videoGraphBaseGet ().frameCaptureReceivedSignal.Connect (
		[this](GStreamerBuffer &gstBuffer)
		{
			PostEvent(
				std::bind(&CstiVideoInputBase::eventFrameCaptureReceived, this, gstBuffer));
		}));

	m_signalConnections.push_back (videoGraphBaseGet ().encodeBitstreamReceivedSignal.Connect (
		[this](GStreamerSample &gstSample)
		{
			PostEvent(
				std::bind(&CstiVideoInputBase::eventEncodeBitstreamReceived, this, gstSample));
		}));

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInputBase::bufferElementsInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	for (int i = 0; i < GSTREAMER_BITSTREAM_NUM_BUFS; i++)
	{
		GstreamerBufferElement *pGstreamerBufferElement = nullptr;

		pGstreamerBufferElement = new GstreamerBufferElement;
		stiTESTCOND (nullptr != pGstreamerBufferElement, stiRESULT_MEMORY_ALLOC_ERROR);

		pGstreamerBufferElement->FifoAssign (&m_oGstreamerBitstreamBufferFifo);
		m_oGstreamerBitstreamBufferFifo.Put (pGstreamerBufferElement);
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInputBase::cameraControlInitialize (
	int captureWidth,
	int captureHeight,
	int minZoomLevel,
	int defaultSelfViewWidth,
	int defaultSelfViewHeight)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pCameraControl = new CstiCameraControl (captureWidth, captureHeight, minZoomLevel, defaultSelfViewWidth, defaultSelfViewHeight);

	// Connect signals before initializing...CameraControl::Initialize emits a needed signal.
	m_signalConnections.push_back (m_pCameraControl->cameraMoveSignal.Connect (
		[this](const CstiCameraControl::SstiPtzSettings &ptzSettings)
		{
			PostEvent (
				std::bind(&CstiVideoInputBase::EventPTZSettingsSet, this, ptzSettings));
		}));

	m_signalConnections.push_back (m_pCameraControl->cameraMoveCompleteSignal.Connect (
		[this]() {
			cameraMoveCompleteSignal.Emit ();
		}));
		
	hResult = m_pCameraControl->Initialize ();
	stiTESTRESULT ();

	hResult = m_pCameraControl->Startup ();
	stiTESTRESULT ();

	m_PtzSettings = m_pCameraControl->ptzSettingsGet ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVideoInputBase::Startup ()
{
	return (CstiEventQueue::StartEventLoop() ?
				stiRESULT_SUCCESS :
				stiRESULT_ERROR);
}


/*! \brief Put the bitstream to the VRPacket fifo
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult CstiVideoInputBase::BitstreamFifoPut (
	GstreamerBufferElement *poGstreamerBufferElement)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::BitstreamFifoPut);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
		stiTrace("CstiVideoInputBase::BitstreamFifoPut: poGstreamerBufferElement = %p\n", poGstreamerBufferElement);
	);

	std::lock_guard<std::mutex> lock(m_BitstreamFifoMutex);

	m_oBitstreamFifo.Put (poGstreamerBufferElement);

	return hResult;
}

/*! \brief Get a bitstream from the VRPacket fifo
 *
 *  \return CBufferElement * - a pointer to the element
 */
GstreamerBufferElement *CstiVideoInputBase::BitstreamFifoGet ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::BitstreamFifoGet);

	std::lock_guard<std::mutex> lock(m_BitstreamFifoMutex);

	// Get the oldest packet in the full queue
	auto pGstreamerBufferElement = static_cast<GstreamerBufferElement *>(m_oBitstreamFifo.Get ());

	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
		stiTrace ("CstiVideoInputBase::BitstreamFifoGet: pGstreamerBufferElement = %p\n", pGstreamerBufferElement);
	);

	return pGstreamerBufferElement;
}

/*! \brief Flash out the packets in the VR fifo
 *
 *  \retval stiRESULT_SUCCESS if success
 */
stiHResult CstiVideoInputBase::BitstreamFifoFlush ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::BitstreamFifoFlush);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::mutex> lock(m_BitstreamFifoMutex);

	auto pGstreamerBufferElement = m_oBitstreamFifo.Get ();

	while (pGstreamerBufferElement)
	{
		pGstreamerBufferElement->FifoReturn ();
		pGstreamerBufferElement = m_oBitstreamFifo.Get ();
	}

	return hResult;
}

stiHResult CstiVideoInputBase::VideoRecordFrameGet (
	SstiRecordVideoFrame* pRecordVideoFrame)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoInputBase::VideoRecordFrameGet);

	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		stiTrace ("CstiVideoInputBase::VideoRecordFrameGet\n");
	);

	GstreamerBufferElement *pGstreamerBufferElement = BitstreamFifoGet ();
	stiTESTCOND (pGstreamerBufferElement, stiRESULT_ERROR);
	stiTESTCOND (pGstreamerBufferElement->DataSizeGet () <= pRecordVideoFrame->unBufferMaxSize, stiRESULT_ERROR);

#if 0
	struct timeval tv;
	uint32_t un32StartTime;
	uint32_t un32EndTime;

	gettimeofday(&tv, NULL);
	un32StartTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

	pRecordVideoFrame->keyFrame = (EstiBool)pGstreamerBufferElement->KeyFrameGet ();

	pRecordVideoFrame->eFormat = pGstreamerBufferElement->VideoFrameFormatGet ();

	pRecordVideoFrame->timeStamp = pGstreamerBufferElement->TimeStampGet();

	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		stiTrace ("CstiVideoInputBase::VideoRecordFrameGet: keyFrame = %d, eFormat = %d, timeStamp = %d\n",
				  pRecordVideoFrame->keyFrame, pRecordVideoFrame->eFormat, pRecordVideoFrame->timeStamp);
	);

	if (pGstreamerBufferElement->CodecGet () == estiVIDEO_H263)
	{
		if (estiPACKET_INFO_FOUR_BYTE_ALIGNED == pGstreamerBufferElement->VideoFrameFormatGet ())
		{
			uint32_t un32NewBitstreamSize = 0;

			hResult = H263BitstreamPacketize (
													pGstreamerBufferElement->BufferGet (), 			//src
													pGstreamerBufferElement->DataSizeGet (), 		//src size
													pRecordVideoFrame->buffer,						//dst
													pRecordVideoFrame->unBufferMaxSize,				//dst size
													videoGraphBaseGet ().MaxPacketSizeGet (), 	//max packet size for packetization
													pGstreamerBufferElement->KeyFrameGet (),  		//I frame or not
													&un32NewBitstreamSize);							//Ouput new frame size
			stiTESTRESULT ();

			pRecordVideoFrame->frameSize = un32NewBitstreamSize;

		}
		else
		{
			memcpy (pRecordVideoFrame->buffer, pGstreamerBufferElement->BufferGet (), pGstreamerBufferElement->DataSizeGet ());
			pRecordVideoFrame->frameSize = pGstreamerBufferElement->DataSizeGet ();
		}

	}
	else if (pGstreamerBufferElement->CodecGet () == estiVIDEO_H264 ||
			pGstreamerBufferElement->CodecGet () == estiVIDEO_H265)
	{
		memcpy (pRecordVideoFrame->buffer, pGstreamerBufferElement->BufferGet (), pGstreamerBufferElement->DataSizeGet ());
		pRecordVideoFrame->frameSize = pGstreamerBufferElement->DataSizeGet ();
	}

#if 0
	gettimeofday(&tv, NULL);
	un32EndTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	printf("Time for Bitstream2H264VideoPacket %ld\n", (un32EndTime - un32StartTime));
#endif

STI_BAIL:

	if (pGstreamerBufferElement)
	{
		pGstreamerBufferElement->FifoReturn ();
		pGstreamerBufferElement = nullptr;
	}

	return hResult;

}

stiHResult CstiVideoInputBase::KeyFrameRequest ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::KeyFrameRequest);

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiVideoRecordKeyframeDebug,
		stiTrace ("CstiVideoInputBase::KeyFrameRequest\n");
	);

	PostEvent(
		std::bind(&CstiVideoInputBase::EventEncodeKeyFrameRequest, this));

	return stiRESULT_SUCCESS;
}

void CstiVideoInputBase::EventEncodeKeyFrameRequest ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::KeyFrameRequest);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiVideoRecordKeyframeDebug,
		stiTrace("CstiVideoInputBase::EventKeyFrameRequest\n");
	);

	hResult = videoGraphBaseGet ().KeyFrameRequest ();
	stiTESTRESULT ();

STI_BAIL:
	return;
}

stiHResult CstiVideoInputBase::PrivacyGet (
	EstiBool *pbEnabled) const
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::PrivacyGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	*pbEnabled = (EstiBool)m_bPrivacy;

//STI_BAIL:

	return hResult;
}

stiHResult CstiVideoInputBase::PrivacySet (
	EstiBool bEnable)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::PrivacySet);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase::PrivacySet\n");
	);

	PostEvent(
		std::bind(&CstiVideoInputBase::EventEncodePrivacySet, this, bEnable));

	return stiRESULT_SUCCESS;
}

void CstiVideoInputBase::EventEncodePrivacySet (
	EstiBool bEnable)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::EventEncodePrivacySet);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (bEnable != m_bPrivacy)
	{
		m_bPrivacy = bEnable;

		//
		// Call the callbacks
		//
		videoPrivacySetSignal.Emit(bEnable);

		hResult = videoGraphBaseGet ().PrivacySet (bEnable);
		stiTESTRESULT ();
	}

STI_BAIL:
	return;
}


stiHResult CstiVideoInputBase::BrightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	Lock ();

	hResult = videoGraphBaseGet().brightnessRangeGet (range);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::BrightnessRangeGet: Brightness range: %d - %d\n", range->min, range->max);
	);

STI_BAIL:

	Unlock ();

	return hResult;
}

int CstiVideoInputBase::BrightnessGet () const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	int brightness {-1};

	Lock ();

	hResult = videoGraphBaseGet().brightnessGet (&brightness);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase::BrightnessGet: brightness = ", brightness, "\n");
	);

STI_BAIL:

	Unlock ();

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase::BrightnessGet: Error, returning brightness = ", brightness, "\n");
		);
	}

	return brightness;
}

stiHResult CstiVideoInputBase::SaturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	Lock ();

	hResult = videoGraphBaseGet().saturationRangeGet (range);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInputBase2::SaturationRangeGet: Saturation range: %d - %d\n", range->min, range->max);
	);

STI_BAIL:

	Unlock ();

	return hResult;
}


int CstiVideoInputBase::SaturationGet () const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	int saturation {-1};

	Lock ();

	hResult = videoGraphBaseGet().saturationGet (&saturation);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInputBase::SaturationGet: saturation = ", saturation, "\n");
	);

STI_BAIL:

	Unlock ();

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CstiVideoInputBase::SaturationGet: Error, returning saturation = ", saturation, "\n");
		);
	}

	return saturation;
}

void CstiVideoInputBase::DisplaySettingsGet (
	IstiVideoOutput::SstiDisplaySettings *pDisplaySettings) const
{
	uint32_t un32RecordWidth;
	uint32_t un32RecordHeight;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::DisplaySettingsGet\n");
	);

	Lock ();
	videoGraphBaseGet ().DisplaySettingsGet(&pDisplaySettings->stSelfView);
	videoGraphBaseGet ().VideoRecordSizeGet(&un32RecordWidth, &un32RecordHeight);
	Unlock ();

	unsigned int unAdjustedX = pDisplaySettings->stSelfView.unPosX;
	unsigned int unAdjustedY = pDisplaySettings->stSelfView.unPosY;
	unsigned int unAdjustedWidth = pDisplaySettings->stSelfView.unWidth;
	unsigned int unAdjustedHeight = pDisplaySettings->stSelfView.unHeight;

	double videoAspectRatio = static_cast<double>(un32RecordWidth) / un32RecordHeight;
	double maxAspectRatio = static_cast<double>(unAdjustedWidth) / unAdjustedHeight;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::DisplaySettingsGet: videoAspectRatio = %f\n", videoAspectRatio);
		stiTrace("CstiVideoInputBase::DisplaySettingsGet: maxAspectRatio = %f\n", maxAspectRatio);
	);

	if (maxAspectRatio > videoAspectRatio)
	{
		unAdjustedWidth = lround(videoAspectRatio * unAdjustedHeight);
	}
	else if (maxAspectRatio < videoAspectRatio)
	{
		unAdjustedHeight = lround(unAdjustedWidth / videoAspectRatio);
	}

	if (unAdjustedWidth == pDisplaySettings->stSelfView.unWidth)
	{
		unAdjustedY = unAdjustedY + (pDisplaySettings->stSelfView.unHeight - unAdjustedHeight) / 2;
	}
	else
	{
		unAdjustedX = unAdjustedX + (pDisplaySettings->stSelfView.unWidth - unAdjustedWidth) / 2;
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::DisplaySettingsGet: end\n");
	);

	pDisplaySettings->stSelfView.unPosX = unAdjustedX;
	pDisplaySettings->stSelfView.unPosY = unAdjustedY;
	pDisplaySettings->stSelfView.unWidth = unAdjustedWidth;
	pDisplaySettings->stSelfView.unHeight = unAdjustedHeight;
}

stiHResult CstiVideoInputBase::DisplaySettingsSet (
	IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::DisplaySettingsSet);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::DisplaySettingsSet\n");
	);

	auto newImageSettings =
		std::make_shared<IstiVideoOutput::SstiImageSettings>(*pImageSettings);

	PostEvent (
		std::bind(&CstiVideoInputBase::EventDisplaySettingsSet, this, newImageSettings));

	return stiRESULT_SUCCESS;
}

stiHResult InputImageSettingsPrint (
	IstiVideoOutput::SstiImageSettings * pImageSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTrace("\tbVisible = %d (%d, %d, %d, %d)\n",
		pImageSettings->bVisible, pImageSettings->unPosX, pImageSettings->unPosY, pImageSettings->unWidth, pImageSettings->unHeight);

	return hResult;
}


/*! \brief Event Handler for the estiVIDEO_INPUT_DISPLAY_SETTINGS_SET message
 *
 */
void CstiVideoInputBase::EventDisplaySettingsSet (
	std::shared_ptr<IstiVideoOutput::SstiImageSettings> imageSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME (CstiVideoInputBase::EventDisplaySettingsSet);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventDisplaySettingsSet start\n");
	);

	stiTESTCOND (imageSettings, stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventDisplaySettingsSet:\n");
		InputImageSettingsPrint (imageSettings.get());
	);

	hResult = videoGraphBaseGet ().DisplaySettingsSet (imageSettings.get());
	stiTESTRESULT ();

	// Notify the UI that the video size and/or position has changed.
	videoInputSizePosChangedSignal.Emit();

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventDisplaySettingsSet end\n");
	);

STI_BAIL:
	return;
}


stiHResult CstiVideoInputBase::VideoRecordSizeGet (
	uint32_t *pun32Width,
	uint32_t *pun32Height)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::VideoRecordSizeGet\n");
	);

	Lock ();

	hResult = videoGraphBaseGet ().VideoRecordSizeGet (pun32Width, pun32Height);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();

	return (hResult);
}



void CstiVideoInputBase::EventFrameTimerTimeout ()
{
	uint64_t newFrameCount = 0;
	std::chrono::steady_clock::time_point lastFrameTime;
	videoGraphBaseGet ().FrameCountGet (&newFrameCount, &lastFrameTime);
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (lastFrameTime - m_lastFrameTime);

	auto denominator = duration.count () / 1000.0F;

	if (denominator != 0)
	{
		m_fps = static_cast<float>(newFrameCount - m_un64LastFrameCount) / denominator;
	}

	if (m_fps <= 0)
	{
		m_fps = 0;
	}

	m_un64LastFrameCount = newFrameCount;
	m_lastFrameTime = lastFrameTime;

	stiDEBUG_TOOL (g_stiVideoInputDebug > 1,
		stiTrace ("Frame Rate = %f\n", m_fps);
	);

	m_frameTimer.Restart();
}


stiHResult CstiVideoInputBase::AdvancedStatusGet (
	SstiAdvancedVideoInputStatus &advancedVideoInputStatus) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	advancedVideoInputStatus.videoFrameRate = m_fps;

	Unlock ();

	return hResult;
}


EstiResult CstiVideoInputBase::cameraMove(
	uint8_t un8ActionBitMask)
{
	return m_pCameraControl->CameraMove(un8ActionBitMask);
}


EstiResult CstiVideoInputBase::cameraPTZGet (
	uint32_t *horzPercent,
	uint32_t *vertPercent,
	uint32_t *zoomPercent,
	uint32_t *zoomWidthPercent,
	uint32_t *zoomHeightPercent)
{
	return m_pCameraControl->CameraPTZGet (horzPercent, vertPercent, zoomPercent, zoomWidthPercent, zoomHeightPercent);
}

stiHResult CstiVideoInputBase::OutputModeSet (
	EDisplayModes eMode)
{
	PostEvent (
		std::bind(&CstiVideoInputBase::EventOutputModeSet, this, eMode));

	return stiRESULT_SUCCESS;
}


void CstiVideoInputBase::EventOutputModeSet (
	EDisplayModes eMode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = videoGraphBaseGet ().OutputModeSet (eMode);
	stiTESTRESULT ();

	hResult = videoGraphBaseGet ().PipelineReset ();
	stiTESTRESULT ();

STI_BAIL:
	return;
}


void CstiVideoInputBase::EventPTZSettingsSet (
	const CstiCameraControl::SstiPtzSettings &ptzSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace("CstiVideoInputBase::EventPTZSettingsSet start\n");
	);

	m_PtzSettings = ptzSettings;

	if (RcuAvailableGet())
	{
		hResult = videoGraphBaseGet ().PTZSettingsSet (ptzSettings);
		stiTESTRESULT ();

		hResult = NoLockAutoExposureWindowSet ();
		stiTESTRESULT ();
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiCapturePTZDebug,
		stiTrace("CstiVideoInputBase::EventPTZSettingsSet end\n");
	);

STI_BAIL:
	return;
}


#ifdef ENABLE_ENCODER_SETTINGS_TEST
gboolean CstiVideoInputBase::encoderSettingsTest (
	void * pUserData)
{
	stiTrace ("CstiVideoInputBase::encoderSettingsTest\n");
	
	auto videoInputBase = (CstiVideoInputBase *)pUserData;

	videoInputBase->PostEvent (
		std::bind(&CstiVideoInputBase::eventEncoderSettingsTest, videoInputBase));
	
	return TRUE;
}

void CstiVideoInputBase::eventEncoderSettingsTest ()
{
	stiTrace ("CstiVideoInputBase::eventEncoderSettingsTest\n");
	
	IstiVideoInput::SstiVideoRecordSettings VideoRecordSettings;
/*
	VideoRecordSettings.unMaxPacketSize = 1300;
	VideoRecordSettings.unTargetFrameRate = 30;
	VideoRecordSettings.unIntraRefreshRate = 0;
	VideoRecordSettings.unIntraFrameRate = 0;
	VideoRecordSettings.ePacketization = estiH264_SINGLE_NAL;
	VideoRecordSettings.eProfile = estiH264_BASELINE;
	VideoRecordSettings.unConstraints = 0;
	VideoRecordSettings.unLevel = estiH264_LEVEL_4_1;
*/
	VideoRecordSettings = videoGraphBaseGet ().EncodeSettingsGet ();

	if (m_encoderSettingsTestSwitch)
	{
		//VideoRecordSettings.unTargetBitRate = 1500000;
		VideoRecordSettings.unColumns = 1920;
		VideoRecordSettings.unRows = 1080;
		m_encoderSettingsTestSwitch = false;
	}
	else
	{
		//VideoRecordSettings.unTargetBitRate = 256000;
		VideoRecordSettings.unColumns = 1280;
		VideoRecordSettings.unRows = 740;
		m_encoderSettingsTestSwitch = true;
	}

	VideoRecordSettingsSet (&VideoRecordSettings);
}

#endif

//#define ENABLE_TEST_RESET

#ifdef ENABLE_TEST_RESET
static guint g_VideoInputResetTestID;

static gboolean VideoInputResetTest (
	CstiVideoInput * pThis)
{
	gboolean bReturn = true;
	stiTrace ("CstiVideoInputBase::VideoInputResetTest\n");

	pThis->PostEvent(
		std::bind(&CstiVideoInputBase::EventGstreamerVideoGraphCallback, pThis, (int)CstiGStreamerVideoGraph::estiMSG_PIPELINE_RESET, 0);

	return bReturn;
}
#endif

stiHResult CstiVideoInputBase::VideoRecordStart ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::VideoRecordStart);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::VideoRecordStart\n");
	);

	PostEvent(
		std::bind(&CstiVideoInputBase::EventEncodeStart, this));

	return stiRESULT_SUCCESS;
}


/*! \brief Event Handler for the estiVIDEO_INPUT_ENCODE_START message
 *
 */
void CstiVideoInputBase::EventEncodeStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoInputBase::EventRecordStart);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventEncodeStart start\n");
	);

	m_bRecording = true;
	m_bGotKeyframeAfterRecordStart = false;

	hResult = videoGraphBaseGet ().EncodeStart ();
	stiTESTRESULT ();

#ifdef ENABLE_ENCODER_SETTINGS_TEST
	stiTrace("CstiVideoInputBase::EventEncodeStop: Starting encoder settings test timer\n");
	m_encoderSettingsTestTimerID = g_timeout_add (1100, (GSourceFunc)encoderSettingsTest, this);
#endif

#ifdef ENABLE_TEST_RESET
	m_encoderSettingsTestTimerID = g_timeout_add (10000, (GSourceFunc)VideoInputResetTest, this);
#endif

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventEncodeStart end\n");
	);

STI_BAIL:
	return;
}


stiHResult CstiVideoInputBase::VideoRecordStop ()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::VideoRecordStop);

	PostEvent (
		std::bind(&CstiVideoInputBase::EventEncodeStop, this));

	return stiRESULT_SUCCESS;
}


/*! \brief Event Handler for the estiVIDEO_INPUT_ENCODE_STOP message
 *
 */
void CstiVideoInputBase::EventEncodeStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventEncodeStop start\n");
	);

	m_pCameraControl->selfViewSizeRestore ();

	m_bRecording = false;
	m_recordingToFile = false;

	// Ensure we are no longer configured for portrait video fixes bug #26865
	m_portraitVideo = false;

	//hResult = CameraClose ();
	//stiTESTRESULT ();

	hResult = videoGraphBaseGet ().EncodeStop ();
	stiTESTRESULT ();

	//hResult = CameraOpen ();
	//stiTESTRESULT ();

	videoRecordSizeChangedSignal.Emit();

	hResult = BitstreamFifoFlush ();
	stiTESTRESULT ();

#ifdef ENABLE_ENCODER_SETTINGS_TEST
	if (m_encoderSettingsTestTimerID)
	{
		stiTrace("CstiVideoInputBase::EventEncodeStop: Removing encoder settings test timer\n");
		g_source_remove (m_encoderSettingsTestTimerID);
	}
#endif

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventEncodeStop end\n");
	);

	m_bGotKeyframeAfterRecordStart = false;

STI_BAIL:
	return;
}


/*!
 * \brief Requests a frame capture
 *
 * \return stiHResult
 */
stiHResult CstiVideoInputBase::FrameCaptureRequest (
	EstiVideoCaptureDelay eDelay,
	SstiImageCaptureSize *pImageSize)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::VideoRecordStart);
	stiHResult hResult = stiRESULT_SUCCESS;
	if (eDelay == IstiVideoInput::eCaptureNow)
	{
		if (!pImageSize)
		{
			stiTHROWMSG (stiRESULT_ERROR, "No capture Image Size\n");
		}
		if (m_bRecording)
		{
			stiTHROWMSG (stiRESULT_ERROR, "Can't start CaptureNow, in recording\n");
		}
		m_ImageSize = *pImageSize;

		PostEvent (
			std::bind(&CstiVideoInputBase::EventFrameCaptureRequest, this, &m_ImageSize));
	}
	else
	{
		if (pImageSize)
		{
			stiTHROWMSG (stiRESULT_ERROR, "This image capture delay does not take a size\n");
		}

		PostEvent (
			std::bind(&CstiVideoInputBase::EventFrameCaptureRequest, this, nullptr));
	}
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}

void CstiVideoInputBase::EventFrameCaptureRequest (
	SstiImageCaptureSize *pImageSize)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::EventFrameCaptureRequest);

	//stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventFrameCaptureRequest start\n");
	);

	// Set up the capture.
	videoGraphBaseGet ().FrameCaptureSet (true, pImageSize);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::EventFrameCaptureRequest end\n");
	);
}


/*! \brief Event Handler for the estiGSTREAMER_ENCODE_BITSREAM_RECIEVED message
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
void CstiVideoInputBase::eventFrameCaptureReceived (
	GStreamerBuffer &gstBuffer)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::eventFrameCaptureReceived);
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("Enter eventFrameCaptureReceived\n");
	);

	if (gstBuffer.get () != nullptr)
	{
		std::vector<uint8_t> frame;

		frame.resize(gstBuffer.sizeGet ());
		memcpy (frame.data (), gstBuffer.dataGet (), gstBuffer.sizeGet ());

		// Notify the app that the frame is ready.
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace("eventFrameCaptureReceived: emitting frameCaptureAvailableSignal\n");
		);

		frameCaptureAvailableSignal.Emit(frame);
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("Exit eventFrameCaptureReceived\n");
	);
}


void CstiVideoInputBase::eventPipelineReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInputBase::eventPipelineReset: CstiGStreamerVideoGraph::estiMSG_PIPELINE_RESET\n");
	);

	hResult = videoGraphBaseGet ().PipelineReset ();
	stiTESTRESULT ();

STI_BAIL:
	return;
}

void CstiVideoInputBase::EventUsbRcuConnectionStatusChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bRcuConnected = false;
	
	hResult = monitorTaskBaseGet()->RcuConnectedStatusGet (&bRcuConnected);
	stiTESTRESULT ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
				  vpe::trace("CstiVideoInputBase::EventUsbRcuConnectionStatusChanged: bRcuConnected = ", bRcuConnected, "\n");
	);

	RcuConnectedSet (bRcuConnected);

	usbRcuConnectionStatusChangedSignal.Emit();

	hResult = NoLockRcuStatusChanged ();
	stiTESTRESULT ();

STI_BAIL:
	return;
}


void CstiVideoInputBase::EventUsbRcuReset ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (RcuConnectedGet())
	{
		hResult = NoLockCameraSettingsSet ();
		stiTESTRESULT ();
	}

STI_BAIL:
	return;
}

/*! \brief Event Handler for the estiGSTREAMER_ENCODE_BITSREAM_RECIEVED message
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
void CstiVideoInputBase::eventEncodeBitstreamReceived (
	GStreamerSample &gstSample)
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::NoLockEncodeBitstreamRecieved);

	stiHResult hResult = stiRESULT_SUCCESS;

	GstreamerBufferElement *pGstreamerBufferElement = nullptr;

	bool bPutBuffer = false;

	bool bKeyFrame = false;

	if (m_bRecording && !m_bPrivacy)
	{
		// Get some info from the meta data of the bitstream

		//Get a buffer from fifo if available
		pGstreamerBufferElement = m_oGstreamerBitstreamBufferFifo.WaitForBuffer (3);

		if (pGstreamerBufferElement)
		{
			stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
				stiTrace ("CstiVideoInputBase::eventEncodeBitstreamReceived\n");
			);

			auto gstBuffer = gstSample.bufferGet ();
			stiTESTCOND (gstBuffer.get () != nullptr, stiRESULT_ERROR);

			stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
				auto buffer = gstBuffer.dataGet ();
				stiTrace ("BitstreamSize = %d\n", gstBuffer.sizeGet ());
				for (int i = 0; i < 28; i++)
				{
					stiTrace ("%x ", buffer[i]);
				}
				stiTrace ("\n");
			);

			m_bRequestedKeyFrameForEmptyFifo = false;

			GstStructure *pGstStructure = nullptr;
			const char *pszCapsName = nullptr;

			auto gstCaps = gstSample.capsGet ();
			stiTESTCOND (gstCaps.get () != nullptr, stiRESULT_ERROR);

			pGstStructure = gst_caps_get_structure (gstCaps.get (), 0);
			stiTESTCOND (pGstStructure != nullptr, stiRESULT_ERROR);

			pszCapsName = gst_structure_get_name (pGstStructure);
			stiTESTCOND (pszCapsName != nullptr, stiRESULT_ERROR);

			stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug > 2,
				stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: pszCapsName = %s\n", pszCapsName);
			);

			// See if it is a key frame
			bKeyFrame = !gstBuffer.flagIsSet (GST_BUFFER_FLAG_DELTA_UNIT);

			if (bKeyFrame)
			{
				if (!m_bGotKeyframeAfterRecordStart)
				{
					stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug || g_stiVideoInputDebug || g_stiVideoRecordKeyframeDebug,
						stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Got first keyframe after record start\n");
					);
				}

				m_bGotKeyframeAfterRecordStart = true;

				stiDEBUG_TOOL (g_stiVideoRecordKeyframeDebug,
					stiTrace ( "\nCstiVideoInputBase::NoLockEncodeBitstreamRecieved Got a record keyframe\n");
				)
			};

			if (m_bGotKeyframeAfterRecordStart)
			{
				if (g_str_has_prefix (pszCapsName, "application/x-rtp"))
				{
					stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
						stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Buffer is H.263\n");
					);

					//fill buffer element with info from our new Gstreamer bitstream buffer
					hResult = pGstreamerBufferElement->GstreamerBufferSet (gstBuffer, bKeyFrame, estiVIDEO_H263, estiRTP_H263_RFC2190);
					stiTESTRESULT ();

					bPutBuffer = true;
				}
				else if (g_str_has_prefix (pszCapsName, "video/x-h263"))
				{
					stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
						stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Buffer is H.263\n");
					);

					//fill buffer element with info from our new Gstreamer bitstream buffer
					hResult = pGstreamerBufferElement->GstreamerBufferSet (gstBuffer, bKeyFrame, estiVIDEO_H263, estiPACKET_INFO_FOUR_BYTE_ALIGNED);
					stiTESTRESULT ();

					bPutBuffer = true;

				}
				else if (g_str_has_prefix (pszCapsName, "video/x-h264"))
				{
					stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
						stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Buffer is H.264\n");
					);

					//We got a bitstream buffer so fill it with info from our Gstreamer bitstream buffer
					hResult = pGstreamerBufferElement->GstreamerBufferSet (gstBuffer, bKeyFrame, estiVIDEO_H264, estiBYTE_STREAM);
					stiTESTRESULT ();

					bPutBuffer = true;
				}
				else if (g_str_has_prefix (pszCapsName, "video/x-h265"))
				{
					stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 2,
						stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Buffer is H.265\n");
					);

					//We got a bitstream buffer so fill it with info from our Gstreamer bitstream buffer
					hResult = pGstreamerBufferElement->GstreamerBufferSet (gstBuffer, bKeyFrame, estiVIDEO_H265, estiBYTE_STREAM);
					stiTESTRESULT ();

					bPutBuffer = true;
				}
				else
				{
					stiTHROWMSG (stiRESULT_ERROR, "NoLockEncodeBitstreamRecieved: recieved invalid bitstream\n");
				}

				if (bPutBuffer)
				{
					// Put the bitstream into bitsream fifo
					hResult = BitstreamFifoPut (pGstreamerBufferElement);
					stiTESTRESULT ();

					pGstreamerBufferElement = nullptr;

					//Send message about buffer availability
					//Notify the app that a frame is ready.
					packetAvailableSignal.Emit ();
				}
			}
			else
			{
				stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug || g_stiVideoInputDebug || g_stiVideoRecordKeyframeDebug,
					stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Got a non-keyframe before keyframe after start record\n");
				);
			}
		}
		else
		{
			stiASSERTMSG (estiFALSE, "CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Too slow processing bitstreams. Bitstream fifo is empty\n");

			if (!m_bRequestedKeyFrameForEmptyFifo)
			{
				stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug || g_stiVideoInputDebug || g_stiVideoRecordKeyframeDebug,
					stiTrace ("CstiVideoInputBase::NoLockEncodeBitstreamRecieved: Request a local keyframe because we had to drop an encoded buffer\n");
				);

				hResult = KeyFrameRequest ();
				stiTESTRESULT ();

				m_bRequestedKeyFrameForEmptyFifo = true;

			}
		}

	}

STI_BAIL:

	if (pGstreamerBufferElement)
	{
		pGstreamerBufferElement->FifoReturn ();
		pGstreamerBufferElement = nullptr;
	}
}


float CstiVideoInputBase::aspectRatioGet () const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	return videoGraphBaseGet ().aspectRatioGet ();
}


void CstiVideoInputBase::cameraErrorRestartPipeline ()
{
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("m_gstreamerVideoGraph.encodeCameraError: Error\n");
	);

	// There was an error with the camera on the encode pipeline
	// shutdown and restart the pipeline
	if (m_bRecording)
	{
		videoRecordEncodeCameraErrorSignal.Emit();

		// We are in a call
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("m_gstreamerVideoGraph.encodeCameraError: In a call\n");
		);

		// Stop call
		VideoRecordStop ();

		// Restart capture pipeline
		selfViewRestart ();
	}
	else
	{
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("m_gstreamerVideoGraph.encodeCameraError: Not in a call\n");
		);

		// Restart capture pipeline
		selfViewRestart ();
	}
}


void CstiVideoInputBase::selfViewRestart ()
{
	selfViewEnabledSet (false);
	selfViewEnabledSet (true);
}

