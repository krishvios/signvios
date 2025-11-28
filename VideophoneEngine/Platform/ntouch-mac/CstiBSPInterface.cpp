#include "CstiBSPInterface.h"
#include "AppleVideoOutput.h"
#if	APPLICATION == APP_NTOUCH_IOS
#include "AudioIO.h"
#else
#include "CstiAudio.h"
#endif
#include "AppleVideoInput.h"
#include "CstiAudibleRinger.h"
#include "AppleNetwork.h"
#include "BaseBluetooth.h"
#include "CstiBluetooth.h"
#include "CstiDisabledBluetooth.h"
#include "cmPropertyNameDef.h"
#include "CFilePlay.h"


CstiAudibleRinger *g_pAudibleRinger = NULL;
vpe::AppleVideoOutput *g_pDisplay = NULL;
#if	APPLICATION == APP_NTOUCH_IOS
std::shared_ptr<vpe::AudioIO> g_pAudio;
#else
CstiAudioSharedPtr g_pAudio;
#endif
vpe::AppleNetworkSP g_Network;

vpe::AppleVideoInput *g_pVideoInput = NULL;
BaseBluetooth *g_pBluetooth = NULL;
CFilePlay g_messageViewer;


IstiPlatform *IstiPlatform::InstanceGet ()
{
	static CstiBSPInterface localBSPInterface;

	return &localBSPInterface;
}


//IstiWireless *IstiWireless::InstanceGet ()
//{
//	return NULL;//g_pWireless;
//}


IstiStatusLED *IstiStatusLED::InstanceGet ()
{
	return g_pBluetooth;
}

IstiBluetooth *IstiBluetooth::InstanceGet ()
{
	return g_pBluetooth;
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
	return g_pAudio.get ();
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	return g_pAudio.get ();
}

IstiNetwork *IstiNetwork::InstanceGet ()
{
	return g_Network.get ();
}

IstiLightRing *IstiLightRing::InstanceGet ()
{
	return g_pBluetooth;
}

IstiMessageViewer *IstiMessageViewer::InstanceGet ()
{
	return &g_messageViewer;
}


CstiBSPInterface::CstiBSPInterface ()
//	:
	//m_pDisplayTask (NULL)
{
}


CstiBSPInterface::~CstiBSPInterface()
{
	Uninitialize();
	
/*	if (m_pDisplayTask)
	{
		delete m_pDisplayTask;
		m_pDisplayTask = NULL;
	}*/
	if (g_pBluetooth)
	{
		delete g_pBluetooth;
		g_pBluetooth = NULL;
	}

	if (g_pAudibleRinger)
	{
		delete g_pAudibleRinger;
		g_pAudibleRinger = NULL;
	}
}

EstiResult CstiBSPInterface::ErrorReport (
										  const SstiErrorLog *pstErrorLog) ///> Pointer to the structure containing the error data
{
	EstiResult eResult = estiOK;
	
	// Send the message to the client application. Should we also send the
	// message to the user?
	if (NULL != pstErrorLog)
	{
		stiTrace("Error: task name = %s (%z) line = %d stierr = %ld errno = %ld\n",
				 pstErrorLog->szTaskName,
				 pstErrorLog->tidTaskId,
				 pstErrorLog->nLineNbr,
				 pstErrorLog->un32StiErrorNbr,
				 pstErrorLog->un32SysErrorNbr);
		
	} // end if
	
	return (eResult);
} // CVideoDeviceController::ErrorReport


/*!\brief Callback function from sub-tasks
 *
 * \retval estiOK if success
 * \note This is a static function
 */
stiHResult CstiBSPInterface::ThreadCallback(
											int32_t n32Message, 	///< holds the type of message
											size_t MessageParam,	///< holds data specific to the message
											size_t CallbackParam,	///< points to the instantiated CCI object
											size_t CallbackFromId)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiResult eResult = estiOK;
	
	// retrieve pointer to CCI object from pParam
	CstiBSPInterface *pThis = (CstiBSPInterface *)CallbackParam;
	
	switch (n32Message)
	{
		case estiMSG_CB_ERROR_REPORT:
		{
			// Extract data and call ErrorReport
			SstiErrorLog *psStiErrorLog = (SstiErrorLog*)MessageParam;
			eResult = pThis->ErrorReport(psStiErrorLog);
			hResult = stiRESULT_CONVERT (eResult);
			break;
		}
		default:
			break;
	}
	
	return hResult;
}


stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	int nPulseEnabled;
	WillowPM::PropertyManager *propertyManager = WillowPM::PropertyManager::getInstance();
	propertyManager->getPropertyInt(PULSE_ENABLED, &nPulseEnabled, stiPULSE_ENABLED_DEFAULT, propertyManager->Persistent);
	bool bPulseEnabled = (nPulseEnabled == 1);
	
	g_pDisplay = new vpe::AppleVideoOutput;
	hResult = g_pDisplay->Initialize ();

	stiTESTRESULT ();
	hResult = g_pDisplay->Startup();
	stiTESTRESULT();

	g_pVideoInput = new vpe::AppleVideoInput;

	//
	// Create the StatusLED device (do we need to initialize it also?)
	//
	//g_pStatusLED = new CstiStatusLED;

	//
	// Create the LightRing device (do we need to initialize it also?)
	//
#if APPLICATION == APP_NTOUCH_IOS
#ifdef stiENABLE_LIGHT_CONTROL
	hResult = g_pLightRing->Initialize ();
	stiTESTRESULT ();
		
	hResult = g_pLightRing->Startup ();
	stiTESTRESULT ();
#endif
#endif
	//
	// Create the AudibleRinger device
	//
	g_pAudibleRinger = new CstiAudibleRinger;

	#if APPLICATION == APP_NTOUCH_IOS
	g_pAudio = std::make_shared<vpe::AudioIO> ();
	#else
	g_pAudio = std::make_shared<CstiAudio> ();
	#endif
	
	hResult = g_pAudio->Initialize ();
	stiTESTRESULT ();
	
	g_Network = std::make_shared<vpe::AppleNetwork>();
	
	if (bPulseEnabled) {
		g_pBluetooth = new CstiBluetooth;
	}
	else {
		g_pBluetooth = new CstiDisabledBluetooth;
	}

	hResult = g_messageViewer.Initialize ();
	stiTESTRESULT ();
	
	hResult = g_messageViewer.Startup ();
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::Uninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	hResult = g_pAudio->Uninitialize();
	g_pAudio = NULL;
	stiASSERT (!stiIS_ERROR (hResult));

	delete g_pVideoInput;
	g_pVideoInput = NULL;

	g_messageViewer.StopEventLoop ();
	hResult = g_messageViewer.Shutdown ();
	stiASSERT (!stiIS_ERROR (hResult));
	
	return hResult;
}


/*!
 * \brief Restart System
 *
 * \return stiHResult
 */
stiHResult CstiBSPInterface::RestartSystem (EstiRestartReason eRestartReason)
{
	//
	// Write the restart reason code so that it can be reported on bootup.
	// This is being opened in "append" mode so that subsequent calls to
	// restart the system don't over write the initial reason for restarting the system.
	//
	//
	std::stringstream RestartFile;
	std::string DynamicDataFolder;
	
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	RestartFile << DynamicDataFolder << "RestartReason";
	FILE *pFile = fopen (RestartFile.str().c_str(), "a");
	
	if (pFile)
	{
		fprintf (pFile, "%d\n", eRestartReason);
		
		fclose (pFile);
	}

#if APPLICATION != APP_NTOUCH_IOS 
	if (eRestartReason != estiRESTART_REASON_TERMINATED)
	{
		exit (0);
	}
#endif
	
	return stiRESULT_SUCCESS;
}

stiHResult CstiBSPInterface::RestartReasonGet (EstiRestartReason *peRestartReason)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nRestartReason = estiRESTART_REASON_UNKNOWN;
	
	
	std::stringstream RestartFile;
	std::string DynamicDataFolder;
	
	stiOSDynamicDataFolderGet (&DynamicDataFolder);
	RestartFile << DynamicDataFolder << "RestartReason";

	FILE *pFile = fopen (RestartFile.str().c_str(), "r");
	
	if (pFile)
	{
		fscanf (pFile, "%d", &nRestartReason);
		
		fclose (pFile);
		pFile = NULL;
		
		unlink (RestartFile.str().c_str());
	}
	
	*peRestartReason = (EstiRestartReason)nRestartReason;
	
	return hResult;
}

void CstiBSPInterface::callStatusAdd (std::stringstream & callStatus)
{
	
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

void CstiBSPInterface::ringStart ()
{
#if APPLICATION == APP_NTOUCH_IOS
	IstiLightRing::InstanceGet ()->PatternSet (IstiLightRing::Pattern::Flash, IstiLightRing::EstiLedColor::estiLED_COLOR_WHITE, 15, 3);
	IstiLightRing::InstanceGet ()->Start (0);
#endif
}

void CstiBSPInterface::ringStop ()
{
#if APPLICATION == APP_NTOUCH_IOS
    IstiLightRing::InstanceGet ()->Stop ();
#endif
}

#if APPLICATION == APP_NTOUCH_IOS

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

#endif
