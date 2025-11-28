#include "CstiBSPInterface.h"

std::shared_ptr<AudioTest> CstiBSPInterface::m_audioTest {nullptr};

IAudioTest* IAudioTest::InstanceGet()
{
	stiASSERT (CstiBSPInterface::m_audioTest);

	return CstiBSPInterface::m_audioTest.get ();
}


CFilePlayGS *CstiBSPInterface::m_filePlay = nullptr;

IstiMessageViewer *IstiMessageViewer::InstanceGet()
{
	stiASSERT (CstiBSPInterface::m_filePlay != nullptr);

	return CstiBSPInterface::m_filePlay;
}

IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiBSPInterface localBSPInterface;

	return &localBSPInterface;
}

CstiMatterStubs *CstiBSPInterface::m_matter = nullptr;

IMatter *IMatter::InstanceGet ()
{
	stiASSERT (CstiBSPInterface::m_matter);

	return CstiBSPInterface::m_matter;
}

stiHResult CstiBSPInterface::Initialize ()
{
	auto hResult {stiRESULT_SUCCESS};
	
	hResult = CstiBSPInterfaceBase::Initialize ();
	stiTESTRESULT ();

	m_audioTest = std::make_shared<AudioTest> ();
	m_audioTest->initialize (m_pAudioHandler);

	m_socketListener = sci::make_unique<SocketListenerBase>(m_pMonitorTask);
	m_socketListener->initialize ();
	m_socketListener->startup ();

	//
	// Create and initialize FilePlay.
	//
	m_filePlay = new CFilePlayGS ();

	stiTESTCOND (m_filePlay, stiRESULT_ERROR);

	hResult = m_filePlay->Initialize ();

	stiTESTRESULT ();

	hResult = m_filePlay->Startup ();

	if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
	{
		stiTESTRESULT ();
	}

	//
	// Create and initialize Matter hub (just stubs in x86).
	//
	m_matter = new CstiMatterStubs ();

	stiTESTCOND (m_matter, stiRESULT_ERROR);

	m_fanControl = std::make_shared<FanControlBase> ();

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_socketListener = nullptr;

	m_audioTest = nullptr;

	if (m_filePlay)
	{
		delete m_filePlay;
		m_filePlay = nullptr;
	}
	
	hResult = CstiBSPInterfaceBase::Uninitialize ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::statusLEDSetup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pStatusLED = new CstiStatusLED ();
	stiTESTCOND (nullptr != m_pStatusLED, stiRESULT_MEMORY_ALLOC_ERROR);

//	hResult = m_pStatusLED->Initialize (m_pMonitorTask, m_helios);
//	stiTESTRESULT();

//	hResult = m_pStatusLED->Startup ();
//	stiTESTRESULT();

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

stiHResult CstiBSPInterface::lightRingSetup ()
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

/*!\brief On the softphone, exit the application
 *
 */
void CstiBSPInterface::rebootSystem ()
{
	exit (0);
}


stiHResult CstiBSPInterface::userInputSetup ()
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
	stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiBSPInterface::cpuSpeedSet: called with eCpuSpeed = ", eCpuSpeed, "\n");
		vpe::trace ("CstiBSPInterface::cpuSpeedSet: cpuSpeedSet does nothing on x86\n");
	);
	
	return stiRESULT_SUCCESS;
}
