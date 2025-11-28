// Uncomment this to turn off the use of FFMPEG
#define DISABLE_FFMPEG 1

#include "CstiPlatform.h"
#include "CstiDisplay.h"
// #include "CDisplayTask.h"
// #include "stiError.h"
// #include "CVideoDeviceController.h"
// #include "VideoControl.h"
// #include "stiTrace.h"
// #include "stiDebugMacros.h"
// #include "stiError.h"
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
#include "IstiVideoInput.h"
#ifdef DISABLE_FFMPEG
#include "CstiVideoInput.h"
#else
#include "CstiFFMpegVideoInput.h"
#endif
#include "CstiAudibleRinger.h"

static std::unique_ptr<CstiPlatform> g_platformInterface;

CstiAudibleRinger *g_pAudibleRinger = NULL;
CstiDisplay *g_pDisplay = NULL;
CstiAudioInput *g_pAudioInput = NULL;
CstiAudioOutput *g_pAudioOutput = NULL;
#ifdef DISABLE_FFMPEG
CstiVideoInput *g_pVideoInput = NULL;
#else
CstiFFMpegVideoInput *g_pVideoInput = NULL;
#endif


static const IstiVideoOutput::SstiDisplaySettings g_stDisplaySettings =
	{
		IstiVideoOutput::eDS_SELFVIEW_VISIBILITY |
			IstiVideoOutput::eDS_SELFVIEW_SIZE |
			IstiVideoOutput::eDS_SELFVIEW_POSITION |
			IstiVideoOutput::eDS_REMOTEVIEW_VISIBILITY |
			IstiVideoOutput::eDS_REMOTEVIEW_SIZE |
			IstiVideoOutput::eDS_REMOTEVIEW_POSITION |
			IstiVideoOutput::eDS_TOPVIEW,
		{
			0, // self view visibility
			352, // self view width
			288, // self view height
			0, // self view position X
			0 // self view position Y
		},
		{
			0, // remote view visibility
			352, // remote view width
			288, // remote view height
			352, // remote view position X
			0 // remote view position Y
		},
		IstiVideoOutput::eSelfView};

IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiPlatform localBSPInterface;

	return &localBSPInterface;
}

IstiAudibleRinger *IstiAudibleRinger::InstanceGet ()
{
	return g_pAudibleRinger;
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	return g_pDisplay;
}


IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	return g_pVideoInput;
}

IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	return g_pAudioInput;
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return g_pAudioOutput;
}

IstiNetwork *IstiNetwork::InstanceGet ()
{
	if (g_platformInterface == nullptr)
	{
		g_platformInterface = std::make_unique<CstiPlatform> ();
	}
	return &g_platformInterface->network;
}

CstiPlatform::CstiPlatform ()
{
}


CstiPlatform::~CstiPlatform ()
{
	if (g_pDisplay)
	{
		delete g_pDisplay;
		g_pDisplay = NULL;
	}

	if (g_pVideoInput)
	{
		delete g_pVideoInput;
		g_pVideoInput = NULL;
	}

	// Uninitialize the CstiVideoInput static list
	CstiVideoInput::UninitList ();

	if (g_pAudibleRinger)
	{
		delete g_pAudibleRinger;
		g_pAudibleRinger = NULL;
	}

	if (g_pAudioInput)
	{
		delete g_pAudioInput;
		g_pAudioInput = NULL;
	}

	if (g_pAudioOutput)
	{
		// Parent object takes care of this...delete g_pAudioOutput;
		g_pAudioOutput = NULL;
	}
}

stiHResult CstiPlatform::Initialize ()
{
	//NOTE: this may need to be implemented more.
	return CstiPlatform::Initialize (20);
}

stiHResult CstiPlatform::Initialize (int nMaxCalls)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	g_pDisplay = new CstiDisplay ();

	hResult = g_pDisplay->Initialize ();

	stiTESTRESULT ();

#ifdef DISABLE_FFMPEG
	g_pVideoInput = new CstiVideoInput;

	hResult = g_pVideoInput->Initialize ();
#else
	g_pVideoInput = new CstiFFMpegVideoInput;
	hResult = g_pVideoInput->Initialize ();
	stiTESTRESULT ();

	hResult = g_pVideoInput->Startup ();
#endif

	stiTESTRESULT ();

	// Initialize a list of CstiVideoInput objects (used by CstiVideoRecord to interface with QuickTime files)
	CstiVideoInput::InitList (nMaxCalls);

	//
	// Create the AudibleRinger device
	//
	g_pAudibleRinger = new CstiAudibleRinger;

	g_pAudioInput = new CstiAudioInput;

	g_pAudioOutput = new CstiAudioOutput;

	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiPlatform::Uninitialize ()
{
	delete g_pAudioInput;
	g_pAudioInput = NULL;

#ifndef DISABLE_FFMPEG
	// Cleanup the FFMpeg video classes.
	g_pVideoInput->Uninitialize ();

	//
	// Shutdown and destroy the video object.
	//
	g_pVideoInput->Shutdown ();

	g_pVideoInput->WaitForShutdown ();
#endif

	delete g_pVideoInput;
	g_pVideoInput = NULL;

	return stiRESULT_SUCCESS;
}


stiHResult CstiPlatform::RestartSystem ()
{
	return RestartSystem(EstiRestartReason::estiRESTART_REASON_UNKNOWN);
}

///
/// \brief: Interfaces with hardware to write the MAC address
///
stiHResult CstiPlatform::MacAddressWrite (const char *pszMac)
{
	// Only called from CCI, not used by Holdserver
	return stiRESULT_SUCCESS;
}

stiHResult CstiPlatform::RestartSystem (EstiRestartReason eRestartReason)
{
	// Raise a signal which eventually causes the hold server to restart
	platformRestartSignal.Emit(eRestartReason);
	return stiRESULT_SUCCESS;
}

stiHResult CstiPlatform::RestartReasonGet (EstiRestartReason *peRestartReason)
{
	// Only called from CCI, not used by Holdserver
	return stiRESULT_SUCCESS;
}

void CstiPlatform::callStatusAdd (std::stringstream &callStatus)
{
	// Only called from CCI, not used by Holdserver
}
