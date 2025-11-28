// Uncomment this to turn off the use of FFMPEG
#define DISABLE_FFMPEG 1

#include "CstiBSPInterface.h"

#include "CstiVideoInput.h"
#include "CstiDisplay.h"

#include "IstiWireless.h"
#include "IstiAudioInput.h"
#include "IstiAudioOutput.h"
#include "IstiStatusLED.h"
#include "IstiLightRing.h"
#include "IstiUserInput.h"
#include "CstiAudibleRinger.h"


//CstiWireless *g_pWireless = NULL;
//CstiStatusLED *g_pStatusLED = NULL;
//CstiLightRing *g_pLightRing = NULL;
//CstiUserInput *g_pUserInput = NULL;
CstiAudibleRinger *g_pAudibleRinger = NULL;
CstiDisplay *g_pDisplay = NULL;
//CstiAudioInput *g_pAudioInput = NULL;
//CstiAudioOutput *g_pAudioOutput = NULL;
//#ifdef DISABLE_FFMPEG
	CstiVideoInput *g_pVideoInput = NULL;
//#else
//	CstiFFMpegVideoInput *g_pVideoInput = NULL;
//#endif


//static const IstiVideoOutput::SstiDisplaySettings g_stDisplaySettings =
//{
//	IstiVideoOutput::eDS_SELFVIEW_VISIBILITY |
//	IstiVideoOutput::eDS_SELFVIEW_SIZE |
//	IstiVideoOutput::eDS_SELFVIEW_POSITION |
//	IstiVideoOutput::eDS_REMOTEVIEW_VISIBILITY |
//	IstiVideoOutput::eDS_REMOTEVIEW_SIZE |
//	IstiVideoOutput::eDS_REMOTEVIEW_POSITION |
//	IstiVideoOutput::eDS_TOPVIEW,
//	{
//		0,  // self view visibility
//		352,  // self view width
//		288,  // self view height
//		0,  // self view position X
//		0  // self view position Y
//	},
//	{
//		0,  // remote view visibility
//		352,  // remote view width
//		288,  // remote view height
//		352,  // remote view position X
//		0   // remote view position Y
//	},
//	IstiVideoOutput::eSelfView
//};

//std::vector<CstiBSPInterface*> gktestInterfaces;


IstiPlatform *IstiPlatform::InstanceGet ()
{
	//CstiBSPInterface *newInterface = new CstiBSPInterface;
	//gktestInterfaces.push_back(newInterface);
	static CstiBSPInterface bspInst;
	return &bspInst;
}

IstiWireless *IstiWireless::InstanceGet ()
{
	return NULL; //g_pWireless;
}


IstiStatusLED *IstiStatusLED::InstanceGet ()
{
	return NULL; //g_pStatusLED;
}


IstiLightRing *IstiLightRing::InstanceGet ()
{
	return NULL; //g_pLightRing;
}


IstiUserInput *IstiUserInput::InstanceGet ()
{
	return  NULL; //g_pUserInput;
}

IstiAudibleRinger *IstiAudibleRinger::InstanceGet ()
{
	return  g_pAudibleRinger;
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	return g_pDisplay;
}

IstiVideoInput *IstiVideoInput::InstanceGet()
{
 return g_pVideoInput;
}


IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	return  NULL; //g_pAudioInput;
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return  NULL; //g_pAudioOutput;
}


CstiBSPInterface::CstiBSPInterface ()
//	:
	//m_pDisplayTask (NULL)
{
}


CstiBSPInterface::~CstiBSPInterface()
{
/*	if (m_pDisplayTask)
	{
		delete m_pDisplayTask;
		m_pDisplayTask = NULL;
	}*/

	Uninitialize ();
}


stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	g_pDisplay = new CstiDisplay;
	hResult = g_pDisplay->Initialize ();

	stiTESTRESULT ();


	g_pVideoInput = new CstiVideoInput;
	hResult = g_pVideoInput->Initialize();
	
	stiTESTRESULT();
	hResult = g_pVideoInput->Startup();
	stiTESTRESULT();

//	//
//	// Create the StatusLED device (do we need to initialize it also?)
//	//
//	g_pStatusLED = new CstiStatusLED;
//
//	//
//	// Create the LightRing device (do we need to initialize it also?)
//	//
//	g_pLightRing = new CstiLightRing;
//
//	//
//	// Create the UserInput device
//	//
//	g_pUserInput = new CstiUserInput;
//
//	//
//	// Create the AudibleRinger device
//	//
	g_pAudibleRinger = new CstiAudibleRinger;
//
//	g_pAudioInput = new CstiAudioInput;
//	hResult = g_pAudioInput->Initialize ();
//	stiTESTRESULT ();
//
//	hResult = g_pAudioInput->Startup ();
//	stiTESTRESULT ();
//	
//	g_pAudioOutput = new CstiAudioOutput;
//
//	hResult = g_pAudioOutput->Initialize ();
//	stiTESTRESULT ();


STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{

	g_pDisplay->Uninitialize();
	g_pVideoInput->Uninitialize();
	delete g_pDisplay;
	delete g_pVideoInput;

//	g_pAudioInput->Shutdown ();
//	g_pAudioInput->WaitForShutdown ();
//	
//	if (g_pAudioInput)
//	{
//		delete g_pAudioInput;
//		g_pAudioInput = NULL;
//	}
//	
//	if (g_pAudioOutput)
//	{
//		delete g_pAudioOutput;
//		g_pAudioOutput = NULL;
//	}
//
//
//#ifndef DISABLE_FFMPEG
//	// Cleanup the FFMpeg video classes.
//	g_pVideoInput->Uninitialize();
//
//	//
//	// Shutdown and destroy the video object.
//	//
//	g_pVideoInput->Shutdown ();
//
//	g_pVideoInput->WaitForShutdown ();
//#endif
//
//	if (g_pVideoInput)
//	{
//		delete g_pVideoInput;
//		g_pVideoInput = NULL;
//	}
//
//	if (g_pDisplay)
//	{
//		delete g_pDisplay;
//		g_pDisplay = NULL;
//	}
//
//	if (g_pStatusLED)
//	{
//		delete g_pStatusLED;
//		g_pStatusLED = NULL;
//	}
//
//	if (g_pLightRing)
//	{
//		delete g_pLightRing;
//		g_pLightRing = NULL;
//	}
//
//	if (g_pUserInput)
//	{
//		delete g_pUserInput;
//		g_pUserInput = NULL;
//	}
//
	if (g_pAudibleRinger)
	{
		delete g_pAudibleRinger;
		g_pAudibleRinger = NULL;
	}
	
	return stiRESULT_SUCCESS;
}

//IstiVideoInput* CstiBSPInterface::VideoInputInstanceGet()
//{
//	return NULL;
//}


stiHResult CstiBSPInterface::RestartSystem ()
{
	//
	// For the softphone, simply exit.
	//
	stiTrace ("Application exiting to simulate rebooting.\n");
	exit (0);
	
	return stiRESULT_SUCCESS;
}

///
/// \brief: Interfaces with hardware to write the MAC address
///
stiHResult CstiBSPInterface::MacAddressWrite (const char *pszMac)
{
	//
	// For the softphone, simply return.
	//
	
	return stiRESULT_SUCCESS;
}
