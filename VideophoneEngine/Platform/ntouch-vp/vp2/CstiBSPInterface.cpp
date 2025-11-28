// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2025 Sorenson Communications, Inc. -- All rights reserved

#define ENABLE_GSTREAMER_VIDEO_INPUT 1

#include "CstiBSPInterface.h"
#include "CstiISPMonitor.h"
#include "CstiVideoInput.h"

using EstiRestartReason = CstiBSPInterface::EstiRestartReason;

//
// these two are defined in the kernel in drivers/media/video/tc358743_regs.h
// as MASK_S_V_FORMAT_1080P and MASK_S_V_FORMAT_1080P
// other resolutions are not being supported.
#define HDMI_RESOLUTION_720 12
#define HDMI_RESOLUTION_1080 15


template <typename enumType>
auto toInteger(enumType const value)
	-> typename std::underlying_type<enumType>::type
{
	return static_cast<typename std::underlying_type<enumType>::type>(value);
}

CstiISPMonitor *CstiBSPInterface::m_pISPMonitor = nullptr;
CFilePlay *CstiBSPInterface::m_filePlay = nullptr;
CstiMatterStubs *CstiBSPInterface::m_matter = nullptr;

// todo: Expose the constants from the kernel so that we can
// use those instead of our own constants
#define stiKERNEL_RESET_FILE_NAME "/sys/class/reset/tegra/reason"
#define stiKERNEL_RESET_REASON_POWER_ON_RESET "power on reset"
#define stiKERNEL_RESET_REASON_WATCHDOG_RESET "watchdog reset"
#define stiKERNEL_RESET_REASON_OVER_TEMPERATURE_RESET "over temperature reset"
#define stiKERNEL_RESET_REASON_SOFTWARE_RESET "software reset"
#define stiKERNEL_RESET_REASON_LOW_POWER_EXIT "low power exit"


CstiBSPInterface::CstiBSPInterface() : CstiBSPInterfaceBase()
{
	m_cecPort = "H2C";
	m_osdName = "ntouch VP2";
}


IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiBSPInterface localBSPInterface;

	return &localBSPInterface;
}

IstiMessageViewer *IstiMessageViewer::InstanceGet()
{
	stiASSERT (CstiBSPInterface::m_filePlay != nullptr);

	return CstiBSPInterface::m_filePlay;
}

IMatter *IMatter::InstanceGet ()
{
	stiASSERT (CstiBSPInterface::m_matter);

	return CstiBSPInterface::m_matter;
}

stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = CstiBSPInterfaceBase::Initialize ();
	stiTESTRESULT ();

	//
	// Create and start the ISP Monitor.
	//
	m_pISPMonitor = new CstiISPMonitor ();
	stiTESTCOND (nullptr != m_pISPMonitor, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pISPMonitor->Initialize (m_pMonitorTask);
	stiTESTRESULT ();

	m_pISPMonitor->Startup ();

	m_pVideoInput->SetISPMonitor (m_pISPMonitor);

	//
	// Create and initialize FilePlay.
	//
	m_filePlay = new CFilePlay ();

	stiTESTCOND (m_filePlay, stiRESULT_ERROR);

	hResult = m_filePlay->Initialize ();

	stiTESTRESULT ();

	hResult = m_filePlay->Startup ();

	if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
	{
		stiTESTRESULT ();
	}

	//
	// Create and initialize Matter hub (just stubs in VP2).
	//
	m_matter = new CstiMatterStubs ();

	stiTESTCOND (m_matter, stiRESULT_ERROR);

	m_fanControl = std::make_shared<FanControlBase> ();

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{
	
	if (m_pISPMonitor)
	{
		delete m_pISPMonitor;
		m_pISPMonitor = nullptr;
	}
	
	if (m_filePlay)
	{
		delete m_filePlay;
		m_filePlay = nullptr;
	}
	
	CstiBSPInterfaceBase::Uninitialize ();

	return stiRESULT_SUCCESS;
}

stiHResult CstiBSPInterface::lightRingSetup()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pLightRing = new CstiLightRing ();
	stiTESTCOND (nullptr != m_pLightRing, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pLightRing->Initialize (m_pMonitorTask, m_helios);
	stiTESTRESULT();

	hResult = m_pLightRing->Startup ();
	stiTESTRESULT();

STI_BAIL:
	return hResult;
}

stiHResult CstiBSPInterface::statusLEDSetup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_pStatusLED = new CstiStatusLED ();
	stiTESTCOND (nullptr != m_pStatusLED, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pStatusLED->Initialize (m_pMonitorTask, m_helios);
	stiTESTRESULT();

	hResult = m_pStatusLED->Startup ();
	stiTESTRESULT();

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::audioInputSetup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pAudioInput = std::make_shared<CstiAudioInput> ();
	stiTESTCOND (nullptr != m_pAudioInput, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pAudioInput->Initialize (m_pAudioHandler);
	stiTESTRESULT ();

	hResult = m_pAudioInput->Startup ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::HdmiInStatusGet (
	int *pnHdmiInStatus)
{
	return (m_pMonitorTask->HdmiInStatusGet (pnHdmiInStatus));
}

stiHResult CstiBSPInterface::HdmiInResolutionGet (
	int *pnHdmiInResolution)
{
	int nResolution;
	stiHResult hResult = m_pMonitorTask->HdmiInResolutionGet (&nResolution);

	switch (nResolution)
	{
	case HDMI_RESOLUTION_1080:
		*pnHdmiInResolution = 1080;
		break;
	case HDMI_RESOLUTION_720:
		*pnHdmiInResolution = 720;
		break;
	default:
		*pnHdmiInResolution = 0;
		break;
	}

	return (hResult);
}

stiHResult CstiBSPInterface::HdmiPassthroughSet (
	bool bHdmiPassthrough)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = m_pVideoInput->HdmiPassthroughSet (bHdmiPassthrough);
	stiTESTRESULT ();
	
//	hResult = m_pAudioHandler->HdmiPassthroughSet (bHdmiPassthrough);
//	stiTESTRESULT ();
	
STI_BAIL:
	return hResult;
}

stiHResult CstiBSPInterface::userInputSetup()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pUserInput = new CstiUserInput ();
	stiTESTCOND (nullptr != m_pUserInput, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_pUserInput->Initialize (m_pMonitorTask);
	stiTESTRESULT();

	hResult = m_pUserInput->Startup ();
	stiTESTRESULT();

STI_BAIL:
	return hResult;
}

stiHResult CstiBSPInterface::cpuSpeedSet (
	EstiCpuSpeed eCpuSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiBSPInterface::cpuSpeedSet: called with eCpuSpeed = ", eCpuSpeed, "\n");
	);

	switch (eCpuSpeed)
	{
		case estiCPU_SPEED_LOW:
			//NOTE: /etc/init.d/cpu-speed is a script that we 
			//provide in VP2 1.5
			stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
				vpe::trace ("CstiBSPInterface::cpuSpeedSet: setting cpu speed to low.\n");
			);
			
			if (system ("/etc/init.d/cpu-speed low"))
			{
				stiTHROWMSG (stiRESULT_ERROR, "error: running /etc/init.d/cpu-speed low");
			}

			break;

		case estiCPU_SPEED_MED:

			stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
				vpe::trace ("CstiBSPInterface::cpuSpeedSet: setting cpu speed to med.\n");
			);
			
			if (system ("/etc/init.d/cpu-speed med"))
			{
				stiTHROWMSG (stiRESULT_ERROR, "error: running /etc/init.d/cpu-speed med");
			}

			break;

		case estiCPU_SPEED_HIGH:
			
			stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
				vpe::trace ("CstiBSPInterface::cpuSpeedSet: setting cpu speed to high.\n");
			);

			if (system ("/etc/init.d/cpu-speed high"))
			{
				stiTHROWMSG (stiRESULT_ERROR, "error: running /etc/init.d/cpu-speed high");
			}

			break;
	}
	
STI_BAIL:

	return hResult;
}

EstiRestartReason CstiBSPInterface::hardwareReasonGet () 
{
	FILE *pFile;
	EstiRestartReason restartReason = estiRESTART_REASON_UNKNOWN;

	if ((pFile = fopen(stiKERNEL_RESET_FILE_NAME, "r")) != nullptr)
	{
		char szReason[128];
		char *cPtr = nullptr;
		memset(szReason, '\0', sizeof(szReason));
		if (fread(szReason, sizeof(char), sizeof(szReason), pFile) > 0)
		{
			if ((cPtr = strchr(szReason, '\n')) != nullptr)
			{
				*cPtr = '\0';
			};

			if (!strcmp(szReason, stiKERNEL_RESET_REASON_POWER_ON_RESET))
			{
				restartReason = estiRESTART_REASON_POWER_ON_RESET;
			}
			else if (!strcmp(szReason, stiKERNEL_RESET_REASON_WATCHDOG_RESET))
			{
				restartReason = estiRESTART_REASON_WATCHDOG_RESET;
			}
			else if (!strcmp(szReason, stiKERNEL_RESET_REASON_OVER_TEMPERATURE_RESET))
			{
				restartReason = estiRESTART_REASON_OVER_TEMPERATURE_RESET;
			}
			else if (!strcmp(szReason, stiKERNEL_RESET_REASON_SOFTWARE_RESET))
			{
				restartReason = estiRESTART_REASON_SOFTWARE_RESET;
			}
			else if (!strcmp(szReason, stiKERNEL_RESET_REASON_LOW_POWER_EXIT))
			{
				restartReason = estiRESTART_REASON_LOW_POWER_EXIT;
			}
		}
		fclose(pFile);
		pFile = nullptr;
	}

	return restartReason;
}

void CstiBSPInterface::currentTimeSet (
	const time_t currentTime)
{
	timespec currentTimespec {currentTime, 0};
	
	stiDEBUG_TOOL (g_stiCCIDebug,
					char buf[100];
					std::strftime (buf, sizeof buf, "%D %T", std::gmtime(&currentTimespec.tv_sec));
					stiTrace ("CstiBSPInterface::currentTimeSet: setting current time to: %s.%09ld UTC\n", buf, currentTimespec.tv_nsec);
	);
	
	clock_settime (CLOCK_REALTIME, &currentTimespec);
}

stiHResult CstiBSPInterface::serialNumberGet (
	std::stringstream &callStatus,
	const SstiAdvancedStatus &advancedStatus) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!advancedStatus.rcuSerialNumber.empty ())
	{
		callStatus << "\tRCUSerialNumber = " << advancedStatus.rcuSerialNumber << "\n";
	}
	
	if (!advancedStatus.mpuSerialNumber.empty ())
	{
		callStatus << "\tMPUSerialNumber = " << advancedStatus.mpuSerialNumber << "\n";
	}
	
	return hResult;
}
