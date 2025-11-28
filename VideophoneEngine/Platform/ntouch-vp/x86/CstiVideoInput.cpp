// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved

#include "CstiExtendedEvent.h"
#include "CstiVideoInput.h"
#include "CstiMonitorTask.h"
#include "CstiGStreamerVideoGraph.h"

#define stiMAX_MESSAGES_IN_QUEUE 12
#define stiMAX_MSG_SIZE 512
#define stiTASK_NAME "CstiVideoInput"
#define stiTASK_PRIORITY 151
#define stiSTACK_SIZE 4096

#define DEFAULT_RED_OFFSET_VALUE 0
#define RED_OFFSET_MIN_VALUE (-200) //-1024
#define RED_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_GREEN_OFFSET_VALUE 0
#define GREEN_OFFSET_MIN_VALUE (-200) //-1024
#define GREEN_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_BLUE_OFFSET_VALUE 0
#define BLUE_OFFSET_MIN_VALUE (-200) //-1024
#define BLUE_OFFSET_MAX_VALUE 200 //1023

#define DEFAULT_ENCODE_SLICE_SIZE 1300
#define DEFAULT_ENCODE_BIT_RATE 10000000
#define DEFAULT_ENCODE_FRAME_RATE 30
//#define DEFAULT_ENCODE_FRAME_RATE_NUM 30000
//#define DEFAULT_ENCODE_FRAME_RATE_DEN 1001
#define DEFAULT_ENCODE_WIDTH 1920
#define DEFAULT_ENCODE_HEIGHT 1072

const int MIN_ZOOM_LEVEL = 128; // Allows up to a 2x zoom

const int DEFAULT_SELFVIEW_WIDTH = 1920;
const int DEFAULT_SELFVIEW_HEIGHT = 1072;


stiHResult CstiVideoInput::Initialize (
	CstiMonitorTask *monitorTask)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_monitorTask = monitorTask;

	hResult = cameraControlInitialize (
		m_gstreamerVideoGraph.maxCaptureWidthGet(),
		m_gstreamerVideoGraph.maxCaptureHeightGet (),
		MIN_ZOOM_LEVEL,
		DEFAULT_SELFVIEW_WIDTH,
		DEFAULT_SELFVIEW_HEIGHT);
	stiTESTRESULT ();

	hResult = bufferElementsInitialize ();
	stiTESTRESULT ();

	hResult = gstreamerVideoGraphInitialize ();
	stiTESTRESULT ();

	m_signalConnections.push_back (m_gstreamerVideoGraph.encodeCameraError.Connect(
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

#if 0
	m_wdFrameTimer = stiOSWdCreate ();
	stiTESTCOND (m_wdFrameTimer != nullptr, stiRESULT_ERROR);
	
	m_pInputFile264 = fopen ("/etc/Test.264", "rb");

	if (!m_pInputFile264)
	{
		stiTrace ("Was not able to open /etc/Test.264\n");
	}

	m_pInputFile263 = fopen ("/etc/Test.263", "rb");

	if (!m_pInputFile263)
	{
		stiTrace ("Was not able to open /etc/Test.263\n");
	}

	m_pInputFileJpg = fopen("/etc/Test.jpg", "rb");

	if (!m_pInputFileJpg)
	{
		stiTrace ("Was not able to open /etc/Test.jpg\n");
	}
#endif

STI_BAIL:

	return hResult;
}

CstiVideoInput::CstiVideoInput ()
{
}

CstiVideoInput::~CstiVideoInput ()
{
	if (m_pInputFile264)
	{
		fclose (m_pInputFile264);
		m_pInputFile264 = nullptr;
	}

	if (m_pInputFile263)
	{
		fclose (m_pInputFile263);
		m_pInputFile263 = nullptr;
	}
	
	if (m_wdFrameTimer)
	{
		stiOSWdDelete (m_wdFrameTimer);
		m_wdFrameTimer = nullptr;
	}

	NoLockCameraClose ();
}


MonitorTaskBase *CstiVideoInput::monitorTaskBaseGet ()
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


stiHResult CstiVideoInput::BrightnessSet (
		int brightness)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_gstreamerVideoGraph.brightnessSet (brightness);

	return hResult;
}


stiHResult CstiVideoInput::SaturationSet (
		int saturation)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_gstreamerVideoGraph.saturationSet (saturation);

	return hResult;
}


EstiBool CstiVideoInput::RcuConnectedGet () const
{
	return estiTRUE;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiVideoPlayback::TimerHandle
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK if successful, estiERROR otherwise.
//
stiHResult CstiVideoInput::TimerHandle (
	CstiEvent *poEvent) // Event
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiLOG_ENTRY_NAME (CstiVideoPlayback::TimerHandle);

	Lock ();

	auto  Timer = reinterpret_cast<stiWDOG_ID>((dynamic_cast<CstiExtendedEvent *>(poEvent))->ParamGet (0)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	if (Timer == m_wdFrameTimer)
	{
//		stiTrace ("------------------------------------------------------------- Frame Timer Fired.\n");
		
//		CstiOsTask::TimerStart (m_wdFrameTimer, 33, 0);
		
		// Notify the app that a frame is ready.
		packetAvailableSignal.Emit();
	}

	Unlock ();

	return (hResult);
} // end CstiVideoPlayback::TimerHandle


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
	pCodecs->push_back (estiVIDEO_H264);
	pCodecs->push_back (estiVIDEO_H263);

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
		case estiVIDEO_H263:
			stProfileAndConstraint.eProfile = estiH263_ZERO;
			pstCaps->Profiles.push_back (stProfileAndConstraint);
			break;

		case estiVIDEO_H264:
		{
			auto pstH264Caps = dynamic_cast<SstiH264Capabilities*>(pstCaps);

	#if 1
			stProfileAndConstraint.eProfile = estiH264_EXTENDED;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);

			stProfileAndConstraint.eProfile = estiH264_MAIN;
			stProfileAndConstraint.un8Constraints = 0x00;
			pstH264Caps->Profiles.push_back (stProfileAndConstraint);
	#endif
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


stiHResult CstiVideoInput::FocusRangeGet (
	PropertyRange *range) const
{
	range->min = 0;
	range->max = 0;
	range->step = 1.0;

	return stiRESULT_SUCCESS;
}


stiHResult CstiVideoInput::SingleFocus()
{
	return stiMAKE_ERROR(stiRESULT_ERROR);
}


stiHResult CstiVideoInput::FocusPositionSet(int nPosition)
{
	stiUNUSED_ARG (nPosition);

	return (stiRESULT_SUCCESS);
}


int CstiVideoInput::FocusPositionGet()
{
	return (0);
}


bool CstiVideoInput::RcuAvailableGet () const
{
	return (true);
}


void CstiVideoInput::selfViewEnabledSet(bool enabled)
{
	PostEvent ([this, enabled]()
	{
		if (enabled)
		{
			m_gstreamerVideoGraph.PipelineReset ();
		}
		else
		{
			m_gstreamerVideoGraph.PipelineDestroy ();
		}
	});
}


stiHResult CstiVideoInput::NoLockAutoExposureWindowSet ()
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiVideoInput::VideoRecordSettingsSet (
	const SstiVideoRecordSettings *pVideoRecordSettings)
{
	stiLOG_ENTRY_NAME (CstiVideoInput::VideoRecordSettingsSet);

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInput::VideoRecordSettingsSet\n");
	);
	
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiVideoInput::VideoRecordSettingsSet: after lock\n");
	);
	
	m_videoRecordSettings = *pVideoRecordSettings;
	
	PostEvent ([this]{
		eventEncodeSettingsSet ();
		});

	return stiRESULT_SUCCESS;
}


void CstiVideoInput::eventEncodeSettingsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace ("CstiVideoInput::eventEncodeSettingsSet start\n");
	);

	bool encodeSizeChanged = false;

	hResult = videoGraphBaseGet ().EncodeSettingsSet (m_videoRecordSettings, &encodeSizeChanged);
	stiTESTRESULT ();
	
	if (encodeSizeChanged)
	{
		stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
			stiTrace("CstiVideoInput::eventEncodeSettingsSet: size changed\n");
		);

		if (!m_recordingToFile)
		{
			videoRecordSizeChangedSignal.Emit();
		}
	}

	stiDEBUG_TOOL (g_stiVideoSizeDebug || g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::eventEncodeSettingsSet end\n");
	);

STI_BAIL:
	return;
}

#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
stiHResult CstiVideoInput::SelfViewWidgetSet (
	void * QQickItem)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::SelfViewWidgetSet: before Lock\n");
	);
	
	Lock ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::SelfViewWidgetSet: after Lock\n");
	);

	hResult = m_gstreamerVideoGraph.SelfViewWidgetSet (QQickItem);
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace("CstiVideoInput::SelfViewWidgetSet: after Unlock\n");
	);
	
	return hResult;
}

stiHResult CstiVideoInput::PlaySet (
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
#endif


void CstiVideoInput::videoRecordToFile()
{
	stiLOG_ENTRY_NAME (CstiVideoInputBase::videoRecordToFile);

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiVideoInpu::videoRecordToFile\n");
	);

	PostEvent (
		[this]
		{
			m_gstreamerVideoGraph.videoRecordToFile();
		});
}
