// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

// Enable one kind of input

#define ENABLE_FILE_VIDEO_INPUT 1

#include "CstiBSPInterface.h"
#include "CstiDisplay.h"
#include "IstiVideoInput.h"
#include "CstiAudioInput.h"
#include "CstiAudioOutput.h"
#include "CstiNetwork.h"

#include "CstiFileVideoInput.h"

#include "CstiStatusLED.h"
#include "CstiLightRing.h"
#include "CstiUserInput.h"
#include "CstiAudibleRinger.h"

CstiStatusLED *g_pStatusLED = NULL;
CstiLightRing *g_pLightRing = NULL;
CstiUserInput *g_pUserInput = NULL;
CstiAudibleRinger *g_pAudibleRinger = NULL;
CstiDisplay *g_pDisplay = NULL;
CstiAudioInputSharedPtr g_pAudioInput;
CstiAudioOutputSharedPtr g_pAudioOutput;

CstiFileVideoInput *g_pVideoInput = NULL;
CstiNetwork *g_pNetwork = NULL;


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
		0,  // self view visibility
		352,  // self view width
		288,  // self view height
		0,  // self view position X
		0  // self view position Y
	},
	{
		0,  // remote view visibility
		352,  // remote view width
		288,  // remote view height
		352,  // remote view position X
		0   // remote view position Y
	},
	IstiVideoOutput::eSelfView
};


IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiBSPInterface localBSPInterface;

	return &localBSPInterface;
}


IstiStatusLED *IstiStatusLED::InstanceGet ()
{
	return g_pStatusLED;
}


IstiLightRing *IstiLightRing::InstanceGet ()
{
	return g_pLightRing;
}


IstiUserInput *IstiUserInput::InstanceGet ()
{
	return g_pUserInput;
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
	return g_pAudioInput.get();
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return g_pAudioOutput.get();
}

IstiNetwork * IstiNetwork::InstanceGet ()
{
	return g_pNetwork;
}


CstiBSPInterface::CstiBSPInterface ()
{
}


CstiBSPInterface::~CstiBSPInterface()
{
	Uninitialize ();
}


stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiTrace ("CstiBSPInterface::Initialize\n");

	g_pVideoInput = new CstiFileVideoInput;
	
	hResult = g_pVideoInput->Initialize ();
	stiTESTRESULT ();
	hResult = g_pVideoInput->Startup ();
	stiTESTRESULT ();

	g_pDisplay = new CstiDisplay();
	hResult = g_pDisplay->Initialize ();
	stiTESTRESULT ();

	//
	// Create the StatusLED device (do we need to initialize it also?)
	//
	g_pStatusLED = new CstiStatusLED;

	//
	// Create the LightRing device (do we need to initialize it also?)
	//
	g_pLightRing = new CstiLightRing;

	//
	// Create the UserInput device
	//
	g_pUserInput = new CstiUserInput;

	//
	// Create the AudibleRinger device
	//
	g_pAudibleRinger = new CstiAudibleRinger;

	g_pAudioInput = std::make_shared<CstiAudioInput> ();
	hResult = g_pAudioInput->Initialize ();
	stiTESTRESULT ();

	hResult = g_pAudioInput->Startup ();
	stiTESTRESULT ();
	
	g_pAudioOutput = std::make_shared<CstiAudioOutput> ();

	hResult = g_pAudioOutput->Initialize ();
	stiTESTRESULT ();
	
	g_pNetwork = new CstiNetwork;

	
STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{
	if (g_pAudioInput)
	{
		g_pAudioInput->Shutdown ();
		g_pAudioInput->WaitForShutdown ();

		// This is a smart pointer.  Just assign the pointer to NULL to release it.
		g_pAudioInput = NULL;
	}
	
	if (g_pAudioOutput)
	{
		// This is a smart pointer.  Just assign the pointer to NULL to release it.
		g_pAudioOutput = NULL;
	}
	
	//
	// Shutdown and destroy the video object.
	//
	if (g_pVideoInput)
	{
		delete g_pVideoInput;
		g_pVideoInput = NULL;
	}

	if (g_pDisplay)
	{
		delete g_pDisplay;
		g_pDisplay = NULL;
	}
	
	if (g_pStatusLED)
	{
		delete g_pStatusLED;
		g_pStatusLED = NULL;
	}

	if (g_pLightRing)
	{
		delete g_pLightRing;
		g_pLightRing = NULL;
	}

	if (g_pUserInput)
	{
		delete g_pUserInput;
		g_pUserInput = NULL;
	}

	if (g_pAudibleRinger)
	{
		delete g_pAudibleRinger;
		g_pAudibleRinger = NULL;
	}
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBSPInterface::RestartSystem (
	EstiRestartReason eRestartReason)
{
	//
	// Write the restart reason code so that it can be reported on bootup.
	//
	FILE *pFile = fopen ("/data/ntouch/crash/RestartReason", "w");

	if (pFile)
	{
		fprintf (pFile, "%d\n", eRestartReason);

		fclose (pFile);
	}

	//
	// For the softphone, simply exit.
	//
	stiTrace ("Application exiting to simulate rebooting.\n");
	exit (0);
	
	return stiRESULT_SUCCESS;
}


stiHResult CstiBSPInterface::RestartReasonGet (
	EstiRestartReason *peRestartReason)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRestartReason = estiRESTART_REASON_UNKNOWN;

	FILE *pFile = fopen ("/data/ntouch/crash/RestartReason", "r");

	if (pFile)
	{
		int nLen = fscanf (pFile, "%d", &nRestartReason);
		(void)nLen; // Silence the compiler about the unused variable.

		fclose (pFile);
		pFile = NULL;

		unlink ("/data/ntouch/crash/RestartReason");
	}

	*peRestartReason = (EstiRestartReason)nRestartReason;

	return hResult;
}

