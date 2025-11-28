#include "CstiBSPInterface.h"
#include "CstiAndroidDisplay.h"
#include "CstiDisplay.h"
#include "CstiAudio.h"

#include "IstiVideoOutput.h"
#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiAudioOutput.h"
#include "IstiAudibleRinger.h"
#include "IstiPlatform.h"
#include "CstiBSPInterface.h"

#ifndef TAG
#define TAG "CstiBSPInterface"
#endif

#include <android/log.h>
extern bool g_debug;

extern CstiAndroidDisplay *getCstiDisplayFromJava();
extern CstiVideoInput *getCstiVideoInputFromJava();
extern void initializeDisplay();


class CstiAudibleRinger : public IstiAudibleRinger
{
public:
    CstiAudibleRinger() /*: m_sound(NULL)*/ {
        //        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        //        m_sound = [MstiAudibleRinger new];
        //        [pool release];
    }
    ~CstiAudibleRinger() {
        //        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        //        [m_sound release];
        //        [pool release];
    }
    
    virtual stiHResult Start() {
        //        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        //        [m_sound start];
        //        [pool release];
        return stiRESULT_SUCCESS;
    }
    virtual stiHResult Stop() {
        //        NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
        //        [m_sound stop];
        //        [pool release];
        return stiRESULT_SUCCESS;
    }
    
    virtual stiHResult TonesSet (
                                 EstiTone eTone,
                                 const SstiTone *pstiTone,	/// Pointer to the array of tones structures
                                 unsigned int unCount,		/// Number of entries in the array of tones
                                 unsigned int unRepeatCount	/// Number of times the array of tones should be played before stopping
                                 ) { return stiRESULT_SUCCESS; }
    
protected:
    //    MstiAudibleRinger *m_sound;
};


//CstiWireless *g_pWireless = NULL;
//CstiStatusLED *g_pStatusLED = NULL;
//CstiLightRing *g_pLightRing = NULL;
//CstiUserInput *g_pUserInput = NULL;
CstiAudibleRinger *g_pAudibleRinger = NULL;
CstiSoftwareDisplay *g_pDisplay = NULL;
CstiAudioSharedPtr g_pAudio;
CstiVideoInput *g_pVideoInput = NULL;
CstiNetwork *g_Network = NULL;
CFilePlay *g_FilePlay = NULL;


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

//IstiWireless *IstiWireless::InstanceGet ()
//{
//	return NULL;//g_pWireless;
//}


//IstiStatusLED *IstiStatusLED::InstanceGet ()
//{
//	return g_pStatusLED;
//}


//IstiLightRing *IstiLightRing::InstanceGet ()
//{
//	return g_pLightRing;
//}


//IstiUserInput *IstiUserInput::InstanceGet ()
//{
//	return g_pUserInput;
//}

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
	return g_pAudio.get();
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return g_pAudio.get();
}

IstiNetwork *IstiNetwork::InstanceGet ()
{
	return g_Network;
}

IstiMessageViewer* IstiMessageViewer::InstanceGet ()
{
	return g_FilePlay;
}

CstiVideoInput *CstiBSPInterface::VideoInputGet()
{
	return g_pVideoInput;
}

CstiSoftwareDisplay *CstiBSPInterface::VideoOutputGet()
{
	return g_pDisplay;
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
//	if (g_pStatusLED)
//	{
//		delete g_pStatusLED;
//		g_pStatusLED = NULL;
//	}

//	if (g_pLightRing)
//	{
//		delete g_pLightRing;
//		g_pLightRing = NULL;
//	}

//	if (g_pUserInput)
//	{
//		delete g_pUserInput;
//		g_pUserInput = NULL;
//	}

	if (g_pAudibleRinger)
	{
		delete g_pAudibleRinger;
		g_pAudibleRinger = NULL;
	}
	if (g_FilePlay) {
		delete g_FilePlay;
		g_FilePlay = NULL;
	}
}


stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

    CstiAndroidDisplay *display = getCstiDisplayFromJava();
    if (display != NULL)
    {
        hResult = display->Initialize ();
        stiTESTRESULT ();
        
        hResult = display->Startup();
        stiTESTRESULT();
        
        g_pDisplay = display;
    }
    else
    {
        CstiDisplay *display = new CstiDisplay;
        
        hResult = display->Initialize ();
        stiTESTRESULT ();
        
        hResult = display->Startup();
        stiTESTRESULT();
        
        g_pDisplay = display;
    }

    initializeDisplay();

	g_pVideoInput = getCstiVideoInputFromJava();
    if (g_pVideoInput == NULL)
    {
        g_pVideoInput = new CstiVideoInput;
    }
	hResult = g_pVideoInput->Initialize ();
	stiTESTRESULT ();

	//
	// Create the AudibleRinger device
	//
	g_pAudibleRinger = new CstiAudibleRinger;


	g_pAudio = std::make_shared<CstiAudio> ();
	hResult = g_pAudio->Initialize ();
	stiTESTRESULT ();

	g_Network = new CstiNetwork;

	//
	// Create and start CFilePlay
	//
	g_FilePlay = new CFilePlay();

	stiTESTCOND (g_FilePlay, stiRESULT_ERROR);

	hResult = g_FilePlay->Initialize();

	stiTESTRESULT ();

	hResult = g_FilePlay->Startup();

	if (stiRESULT_CODE (hResult) != stiRESULT_TASK_ALREADY_STARTED)
	{
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{
	g_pAudio->Uninitialize();
	g_pAudio = NULL;
    
	// Cleanup the FFMpeg video classes.
    g_pVideoInput->Uninitialize();
    
	delete g_pVideoInput;
	g_pVideoInput = NULL;

	return stiRESULT_SUCCESS;
}


stiHResult CstiBSPInterface::RestartSystem (
		EstiRestartReason eRestartReason)
{
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


/*!
 * \brief Receives geolocation information and creates a geolocation string per RFC 5870
 *	with the following format, "geo:latitude,longitude,altitude;uncertainty" where
 *		latitude: required (degrees)
 *		longitude: required (degrees)
 *		altitude: optional (meters)
 *		uncertainty: optional (radius of uncertainty, meters)
 *
 *		for example, "geo:40.679310,-111.917226,1297;u=65"
 */
void CstiBSPInterface::geolocationUpdate (
	bool geolocationViable,
	double latitude,
	double longitude,
	int uncertainty,
	bool altitudeViable,
	int altitude)
{
	if (geolocationViable)
	{
		// Build geolocation string, geo:latitude,longitude,altitude;u=uncertainty
		std::ostringstream oss;
		oss << std::fixed;
		oss.precision (6); // Set precision to six decimal places for latitude and longitude

		// Start string with latitude and longitude
		oss << "geo:" << latitude << "," << longitude;

		// Set precision to zero decimal places for remainder of string
		oss.precision(0);

		// If altitude is valid, add it
		if (altitudeViable) {
			oss << "," << altitude;
		}

		// If uncertainty is valid, add it
		if (uncertainty >= 0)
		{
			oss << ";u=" << uncertainty;
		}

		geolocationChangedSignal.Emit (oss.str());
	}
}

void CstiBSPInterface::geolocationClear ()
{
	geolocationClearSignal.Emit ();
}

void CstiBSPInterface::callStatusAdd (std::stringstream & callStatus)
{
}

void CstiBSPInterface::ringStart ()
{
}

void CstiBSPInterface::ringStop ()
{
}
