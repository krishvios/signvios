// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <cstdio>
#include "CstiVideoOutput.h"

#define stiDISPLAY_MAX_MESSAGES_IN_QUEUE 24
#define stiDISPLAY_MAX_MSG_SIZE 512
#define stiDISPLAY_TASK_NAME "CstiVideoOutputTask"
#define stiDISPLAY_TASK_PRIORITY 151
#define stiDISPLAY_STACK_SIZE 4096

#define SLICE_TYPE_NON_IDR	1
#define SLICE_TYPE_IDR		5

#define	MAX_FRAME_BUFFERS	6

//#define WRITE_BITSTREAM_TO_FILE

#define DECODE_LATENCY_IN_NS      (70 * GST_MSECOND)

#define PIPELINE_CREATION_DELAY 750 // 750 milliseconds

static const IstiVideoOutput::SstiDisplaySettings g_DefaultDisplaySettings =
{
	IstiVideoOutput::eDS_SELFVIEW_VISIBILITY |
	IstiVideoOutput::eDS_SELFVIEW_SIZE |
	IstiVideoOutput::eDS_SELFVIEW_POSITION |
	IstiVideoOutput::eDS_REMOTEVIEW_VISIBILITY |
	IstiVideoOutput::eDS_REMOTEVIEW_SIZE |
	IstiVideoOutput::eDS_REMOTEVIEW_POSITION |
	IstiVideoOutput::eDS_TOPVIEW,
	{
		0,  // self view visibility
		1920,  // self view width
		1072,  // self view height
		0,  // self view position X
		0  // self view position Y
	},
	{
		0,   // remote view visibility
		CstiVideoOutputBase::DEFAULT_DECODE_WIDTH, // remote view width
		CstiVideoOutputBase::DEFAULT_DECODE_HEIGHT, // remote view height
		0,   // remote view position X
		0    // remote view position Y
	},
	IstiVideoOutput::eSelfView
};



CstiVideoOutput::CstiVideoOutput ()
:
	CstiVideoOutputBase2 (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT, NUM_DEC_BITSTREAM_BUFS),
	m_delayDecodePiplineCreationTimer (PIPELINE_CREATION_DELAY, this)
{
	m_DisplaySettings = g_DefaultDisplaySettings;

	m_un32DecodeWidth = DEFAULT_DECODE_WIDTH;
	m_un32DecodeHeight = DEFAULT_DECODE_HEIGHT;
}


stiHResult CstiVideoOutput::Initialize (
	CstiMonitorTask *monitorTask,
	CstiVideoInput *pVideoInput,
	std::shared_ptr<CstiCECControl> cecControl)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (monitorTask, stiRESULT_ERROR);
	stiTESTCOND (pVideoInput, stiRESULT_ERROR);

	m_pMonitorTask = monitorTask;

	m_pVideoInput = pVideoInput;

	if (nullptr == m_Signal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_Signal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);
		stiOSSignalSet2 (m_Signal, 0);
	}

	CECStatusCheck();

	m_videoOutputConnections.push_back (m_delayDecodePiplineCreationTimer.timeoutSignal.Connect (
		[this]{
			stiHResult hResult = stiRESULT_SUCCESS;

			hResult = videoDecodePipelineCreate ();
			stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutput::platformVideoDecodePipelineCreate: can't create output pipeline\n");

			hResult = videoDecodePipelineReady ();
			stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutput::videoDecodePipelineReady: can't ready output pipeline\n");

			hResult = videoDecodePipelinePlay ();
			stiASSERTMSG(!stiIS_ERROR(hResult), "CstiVideoOutput::videoDecodePipelineReady: can't play output pipeline\n");
			ChangeStateWait (m_videoDecodePipeline, 1);

			if (m_videoPlaybackStarting)
			{
				m_videoPlaybackStarting = false;

				eventVideoPlaybackStart ();
			}
		}));

STI_BAIL:

	return hResult;
}


std::string CstiVideoOutput::decodeElementName(EstiVideoCodec eCodec) 
{
	switch (eCodec)
	{
		case estiVIDEO_H264:
			return "avdec_h264";

		case estiVIDEO_H265:
			return "avdec_h265";
		
		default:
			stiASSERTMSG (estiFALSE, "CstiVideoOutput::decodeElementName: WARNING eCodec is unknown\n");
			return "";
	}
}


///\brief Reports to the calling object what codecs this device is capable of.
///\brief Add codecs in the order that you would like preference to be given during channel
///\brief negotiations.
stiHResult CstiVideoOutput::VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs)
{
	// Add the supported video codecs to the list
	pCodecs->push_back (estiVIDEO_H264);
	pCodecs->push_back (estiVIDEO_H263);
	
	return stiRESULT_SUCCESS;
}


///\brief Fills in the video capabilities structure with the specific capabilities of the given codec.
///\return stiRESULT_SUCCESS unless a request for an unsupported codec is made.
stiHResult CstiVideoOutput::CodecCapabilitiesGet (
	EstiVideoCodec eCodec,	// The codec for which we are inquiring
	SstiVideoCapabilities *pstCaps)		// A structure to load with the capabilities of the given codec
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiProfileAndConstraint stProfileAndConstraint;
	
	switch (eCodec)
	{
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			auto pstH264Caps = dynamic_cast<SstiH264Capabilities*>(pstCaps);

			stProfileAndConstraint.eProfile = estiH264_EXTENDED;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_BASELINE;
			stProfileAndConstraint.un8Constraints = 0xe0;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			pstH264Caps->eLevel = estiH264_LEVEL_4;

			break;
		}

		default:
			hResult = stiRESULT_ERROR;
	}

	return hResult;
}


/*!
 * NOTE: This WILL work on x86 linux, but only if you have a compatible
 *  HDMI display and install the cec-utils package
*/
stiHResult CstiVideoOutput::DisplayPowerSet (bool bOn)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int result = 0;
	
	if (m_bCECSupported)
	{
		if (bOn && !m_bTelevisionPowered)
		{
			// Tell TV to turn on
			result = system ("echo 'on 0' | cec-client -s -d 1 H2C");

			// Wait for the TV to turn on
			EstiPowerStatus eStatus = ePowerStatusUnknown;
			for (int nFailsafe = 0; nFailsafe < 2 && eStatus != ePowerStatusOn; ++nFailsafe)
			{
				eStatus = CECDisplayPowerGet ();
			}
			// Set the TV input source to us
			if (eStatus == ePowerStatusOn)
			{
				result = system ("echo 'as' | cec-client -s -d 1 H2C"); 
				m_bTelevisionPowered = true;
			}
		}
		else if (!bOn && m_bTelevisionPowered)
		{
			// this assumes that turning it off just works......
			result = system ("echo 'standby 0' | cec-client -s -d 1 H2C");
			m_bTelevisionPowered = false;
		}
	}
	
//STI_BAIL:
	stiUNUSED_ARG(result);
	
	return (hResult);
}


/*!
 * NOTE: This WILL work on x86 linux, but only if you have a compatible
 *  HDMI display and install the cec-utils package
*/
bool CstiVideoOutput::DisplayPowerGet ()
{
	return(m_bTelevisionPowered);
}

CstiVideoOutput::EstiPowerStatus CstiVideoOutput::CECDisplayPowerGet ()
{
	EstiPowerStatus eReturnValue = ePowerStatusUnknown;
	char * result = nullptr;

	FILE *pPipe = popen ("echo 'pow 0' | cec-client -d 1 -s H2C", "r");
	if (pPipe)
	{
		char pszResult[128];
		while (!feof (pPipe))
		{
			result = fgets (pszResult, sizeof(pszResult), pPipe);
			if (result && 0 == strncmp (pszResult, "power status: ", 14))
			{
				eReturnValue = ePowerStatusOff;
				if (0 == strcmp (&pszResult[14], "on\n"))
				{
					eReturnValue = ePowerStatusOn;
				}
				else if (0 == strcmp(&pszResult[14], "unknown\n"))
				{
					eReturnValue = ePowerStatusUnknown;
				}
				break;
			}
		}
		pclose (pPipe);
	}

	return(eReturnValue);
}

stiHResult CstiVideoOutput::CECStatusCheck()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	EstiPowerStatus eStatus = CECDisplayPowerGet();
	if (eStatus != ePowerStatusUnknown)
	{
		m_bCECSupported = true;
		m_bTelevisionPowered = eStatus == ePowerStatusOn;
		cecSupportedSignal.Emit (m_bCECSupported);
	}

	return(hResult);
}

bool CstiVideoOutput::CECSupportedGet()
{
	return(m_bCECSupported);
}



#if APPLICATION == APP_NTOUCH_VP2
void CstiVideoOutput::DisplaySettingsGet (
	SstiDisplaySettings * pDisplaySettings) const
{
	Lock ();

	if (pDisplaySettings->un32Mask & (IstiVideoOutput::eDS_SELFVIEW_SIZE | IstiVideoOutput::eDS_SELFVIEW_POSITION))
	{
		if (m_pVideoInput)
		{
			m_pVideoInput->DisplaySettingsGet(pDisplaySettings);
		}
	}
	else if (pDisplaySettings->un32Mask & (IstiVideoOutput::eDS_REMOTEVIEW_SIZE | IstiVideoOutput::eDS_REMOTEVIEW_POSITION))
	{
		auto adjustedX = static_cast<int>(m_DisplaySettings.stRemoteView.unPosX);
		auto adjustedY = static_cast<int>(m_DisplaySettings.stRemoteView.unPosY);
		auto adjustedWidth = static_cast<int>(m_DisplaySettings.stRemoteView.unWidth);
		auto adjustedHeight = static_cast<int>(m_DisplaySettings.stRemoteView.unHeight);

		auto videoAspectRatio = static_cast<float>(m_un32DecodeWidth) / m_un32DecodeHeight;
		auto maxAspectRatio = static_cast<float>(adjustedWidth) / adjustedHeight;

		if (maxAspectRatio > videoAspectRatio)
		{
			adjustedWidth = lroundf(videoAspectRatio * adjustedHeight);
		}
		else if (maxAspectRatio < videoAspectRatio)
		{
			adjustedHeight = lroundf(adjustedWidth / videoAspectRatio);
		}

		if (adjustedWidth == static_cast<int>(m_DisplaySettings.stRemoteView.unWidth))
		{
			adjustedY += (m_DisplaySettings.stRemoteView.unHeight - adjustedHeight) / 2;
		}
		else
		{
			adjustedX += (m_DisplaySettings.stRemoteView.unWidth - adjustedWidth) / 2;
		}

		pDisplaySettings->stRemoteView.unPosX = adjustedX;
		pDisplaySettings->stRemoteView.unPosY = adjustedY;
		pDisplaySettings->stRemoteView.unWidth = adjustedWidth;
		pDisplaySettings->stRemoteView.unHeight = adjustedHeight;
	}

	Unlock ();
}


stiHResult CstiVideoOutput::DisplaySettingsSet (
	const SstiDisplaySettings *pDisplaySettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::DisplaySettingsSet\n");
	);

	auto spNewDisplaySettings = std::make_shared<SstiDisplaySettings>(*pDisplaySettings);

	PostEvent (
		std::bind (&CstiVideoOutput::EventDisplaySettingsSet, this, spNewDisplaySettings));

	return hResult;
}


void CstiVideoOutput::EventDisplaySettingsSet (
	std::shared_ptr<SstiDisplaySettings> displaySettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet\n");
	);

	bool bSelfViewImageSettingsValid = false;
	bool bRemoteViewImageSettingsValid = false;
	SstiImageSettings PreValidateSelfViewImageSettings{};
	SstiImageSettings PreValidateRemoteViewImageSettings{};

	stiTESTCOND (displaySettings != nullptr, stiRESULT_ERROR);

	PreValidateSelfViewImageSettings = displaySettings->stSelfView;

	if (displaySettings->un32Mask & (eDS_SELFVIEW_VISIBILITY | eDS_SELFVIEW_SIZE | eDS_SELFVIEW_POSITION))
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: SelfView:\n");
			OutputImageSettingsPrint (&displaySettings->stSelfView);
		);

		bSelfViewImageSettingsValid = ImageSettingsValidate (&displaySettings->stSelfView);

		if (bSelfViewImageSettingsValid)
		{
			if (!ImageSettingsEqual (&m_DisplaySettings.stSelfView, &displaySettings->stSelfView))
			{
				m_DisplaySettings.stSelfView = displaySettings->stSelfView;

				if (m_pVideoInput)
				{
					stiDEBUG_TOOL (g_stiVideoOutputDebug,
						vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Calling m_pVideoInput->DisplaySettingsSet\n");
						OutputImageSettingsPrint (&m_DisplaySettings.stSelfView);
					);

					hResult = m_pVideoInput->DisplaySettingsSet (&m_DisplaySettings.stSelfView);
					stiTESTRESULT ();
				}

				if (m_DisplaySettings.stSelfView.bVisible
					&& m_DisplaySettings.stSelfView.unWidth >= maxDisplayWidthGet ()
					&& m_DisplaySettings.stSelfView.unHeight >= 1072)
				{
					stiDEBUG_TOOL (g_stiVideoOutputDebug,
						vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: FULLSCREEN self view\n");
					);

					if (!m_bHidingRemoteView)
					{
						//The self view is full screen in 1080p mode
						//We can make the remote view not visible
						//because it is always behind the self view
						if (m_bPlaying)
						{
							if (m_DisplaySettings.stRemoteView.bVisible)
							{
								stiDEBUG_TOOL (g_stiVideoOutputDebug,
									vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Hiding remote view and calling DecodeDisplaySinkMove\n");
								);

								m_DisplaySettings.stRemoteView.bVisible = false;

								hResult = DecodeDisplaySinkMove ();
								stiTESTRESULT ();

								m_bHidingRemoteView = true;
							}
						}
					}
				}
				else
				{
					if (m_bHidingRemoteView)
					{
						if (m_bPlaying)
						{
							stiDEBUG_TOOL (g_stiVideoOutputDebug,
								vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Un-hiding remote view and calling DecodeDisplaySinkMove\n");
								OutputImageSettingsPrint (&m_DisplaySettings.stRemoteView);
							);

							m_DisplaySettings.stRemoteView.bVisible = true;

							hResult = DecodeDisplaySinkMove ();
							stiTESTRESULT ();
						}

						m_bHidingRemoteView = false;
					}
				}
			}
			else
			{
				// Notify the UI that a size/pos change request was made.
				m_pVideoInput->NotifyUISizePosChanged();
			}
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Self View Settings: "
					"Mask = %x, bVisible = %d (%d, %d, %d, %d)\n",
					displaySettings->un32Mask,
					displaySettings->stSelfView.bVisible,
					displaySettings->stSelfView.unPosX,
					displaySettings->stSelfView.unPosY,
					displaySettings->stSelfView.unWidth,
					displaySettings->stSelfView.unHeight);
		}
	}

	PreValidateRemoteViewImageSettings = displaySettings->stRemoteView;

	if (displaySettings->un32Mask & (eDS_REMOTEVIEW_VISIBILITY | eDS_REMOTEVIEW_SIZE | eDS_REMOTEVIEW_POSITION))
	{
		stiDEBUG_TOOL (g_stiVideoOutputDebug,
			vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: RemoteView:\n");
			OutputImageSettingsPrint (&displaySettings->stRemoteView);
		);

		bRemoteViewImageSettingsValid = ImageSettingsValidate (&displaySettings->stRemoteView);

		if (bRemoteViewImageSettingsValid)
		{
			if (!ImageSettingsEqual (&m_DisplaySettings.stRemoteView, &displaySettings->stRemoteView))
			{
				m_DisplaySettings.stRemoteView = displaySettings->stRemoteView;

				stiDEBUG_TOOL (g_stiVideoOutputDebug,
					vpe::trace ("CstiVideoOutput::EventDisplaySettingsSet: Calling DecodeDisplaySinkMove\n");
				);

				hResult = DecodeDisplaySinkMove ();
				stiTESTRESULT ();
			}

			// Notify the UI that a size/pos change request was made.
			videoRemoteSizePosChangedSignal.Emit ();
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Remote View Settings: "
					"Mask = %x, bVisible = %d (%d, %d, %d, %d)\n",
					displaySettings->un32Mask,
					displaySettings->stRemoteView.bVisible,
					displaySettings->stRemoteView.unPosX,
					displaySettings->stRemoteView.unPosY,
					displaySettings->stRemoteView.unWidth,
					displaySettings->stRemoteView.unHeight);
		}
	}

STI_BAIL:;
}

bool CstiVideoOutput::ImageSettingsValidate (
	const IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	bool bValid = true;

	if (pImageSettings->bVisible)
	{
		if ((int)pImageSettings->unPosX < 0)
		{
			bValid = false;
		}

		if (pImageSettings->unPosX > maxDisplayWidthGet ())
		{
			bValid = false;
		}

		if (pImageSettings->unPosY < 0)
		{
			bValid = false;
		}

		if (pImageSettings->unPosY > maxDisplayHeightGet ())
		{
			bValid = false;
		}

		if (pImageSettings->unWidth < minDisplayWidthGet ())
		{
			bValid = false;
		}

		if (pImageSettings->unWidth > maxDisplayWidthGet ())
		{
			bValid = false;
		}

		if (pImageSettings->unHeight < minDisplayHeightGet ())
		{
			bValid = false;
		}

		if (pImageSettings->unHeight > maxDisplayHeightGet ())
		{
			bValid = false;
		}
	}

	return (bValid);

}

/*
 * Brief - return the bitmask of the capapabilites of the last connected hdmi device
 */
stiHResult CstiVideoOutput::DisplayModeCapabilitiesGet (
	uint32_t *pun32CapBitMask)
{
	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayModeCapabilitiesGet\n");
	);

	*pun32CapBitMask = 0;

	return stiRESULT_SUCCESS;
}

/*
 * Brief - return the bitmask of the capapabilites of the last connected hdmi device
 */
stiHResult CstiVideoOutput::DisplayModeSet (
	EDisplayModes eDisplayMode)
{
	stiDEBUG_TOOL (g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::DisplayModeSet: eDisplayMode = ", eDisplayMode, "\n");
	);

	return stiRESULT_SUCCESS;
}
#endif

void CstiVideoOutput::eventDisplayResolutionSet (
	uint32_t width,
	uint32_t height)
{
	stiDEBUG_TOOL(g_stiDisplayDebug,
		vpe::trace ("CstiVideoOutput::EventResolutionSet\n");
	);

	if (m_eDisplayHeight != height
		|| m_eDisplayWidth != width)
	{
		m_eDisplayHeight = height;
		m_eDisplayWidth = width;
		
		keyframeNeededSignal.Emit ();
	}
	
}


void CstiVideoOutput::platformVideoDecodePipelineCreate ()
{
	stiLOG_ENTRY_NAME (CstiVideoOutput::platformVideoDecodePipelineCreate);

	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		vpe::trace ("CstiVideoOutput::platformVideoDecodePipelineCreate\n");
	);

	// On startup we need to wait for the system to be completely up before we create the output pipeline.
	m_delayDecodePiplineCreationTimer.restart ();
}
