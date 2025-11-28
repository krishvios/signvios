// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019-2025 Sorenson Communications, Inc. -- All rights reserved

#define ENABLE_GSTREAMER_VIDEO_INPUT 1

#include "CstiBSPInterfaceBase.h"
#include "CstiVideoOutput.h"
#include "IstiVideoInput.h"
#include "CstiAudioHandler.h"
#include "CstiNetwork.h"
#include "CstiAdvancedStatus.h"
#include "CstiLightRing.h"
#include "CstiAudibleRinger.h"
#include "CstiSLICRinger.h"
#include "CstiHelios.h"
#include "CstiMonitorTask.h"
#include "CstiCECControl.h"
#include "CstiUpdate.h"
#include <type_traits>
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"


template <typename enumType>
auto toInteger(enumType const value)
	-> typename std::underlying_type<enumType>::type
{
	return static_cast<typename std::underlying_type<enumType>::type>(value);
}

CstiStatusLED *CstiBSPInterfaceBase::m_pStatusLED = nullptr;
CstiLightRing *CstiBSPInterfaceBase::m_pLightRing = nullptr;
CstiUserInput *CstiBSPInterfaceBase::m_pUserInput = nullptr;
CstiAudibleRinger *CstiBSPInterfaceBase::m_pAudibleRinger = nullptr;
CstiNetwork *CstiBSPInterfaceBase::m_pNetwork = nullptr;
CstiBluetooth *CstiBSPInterfaceBase::m_pBluetooth = nullptr;
std::shared_ptr<CstiHelios> CstiBSPInterfaceBase::m_helios = nullptr;
CstiVideoOutput *CstiBSPInterfaceBase::m_pVideoOutput = nullptr;
CstiAudioHandler *CstiBSPInterfaceBase::m_pAudioHandler = nullptr;
CstiAudioInputSharedPtr CstiBSPInterfaceBase::m_pAudioInput = nullptr;
CstiAudioOutputSharedPtr CstiBSPInterfaceBase::m_pAudioOutput = nullptr;
CstiAdvancedStatus *CstiBSPInterfaceBase::m_pAdvancedStatus = nullptr;
CstiMonitorTask *CstiBSPInterfaceBase::m_pMonitorTask = nullptr;
CstiVideoInput *CstiBSPInterfaceBase::m_pVideoInput = nullptr;
CstiSLICRinger *CstiBSPInterfaceBase::m_slicRinger = nullptr;
std::shared_ptr<CstiCECControl> CstiBSPInterfaceBase::m_cecControl = nullptr;
std::shared_ptr<FanControlBase> CstiBSPInterfaceBase::m_fanControl {nullptr};
std::shared_ptr<CstiUpdate> CstiBSPInterfaceBase::m_update = nullptr;
CstiLogging *CstiBSPInterfaceBase::m_logging = nullptr;

// todo: Expose the constants from the kernel so that we can
// use those instead of our own constants
#define stiKERNEL_RESET_FILE_NAME "/sys/class/reset/tegra/reason"
#define stiKERNEL_RESET_REASON_POWER_ON_RESET "power on reset"
#define stiKERNEL_RESET_REASON_WATCHDOG_RESET "watchdog reset"
#define stiKERNEL_RESET_REASON_OVER_TEMPERATURE_RESET "over temperature reset"
#define stiKERNEL_RESET_REASON_SOFTWARE_RESET "software reset"
#define stiKERNEL_RESET_REASON_LOW_POWER_EXIT "low power exit"


IstiStatusLED *IstiStatusLED::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pStatusLED != nullptr);

	return CstiBSPInterfaceBase::m_pStatusLED;
}


IstiLightRing *IstiLightRing::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pLightRing != nullptr);

	return CstiBSPInterfaceBase::m_pLightRing;
}


IstiUserInput *IstiUserInput::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pUserInput != nullptr);

	return CstiBSPInterfaceBase::m_pUserInput;
}

IstiAudibleRinger *IstiAudibleRinger::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pAudibleRinger != nullptr);

	return CstiBSPInterfaceBase::m_pAudibleRinger;
}

IVideoOutputVP *IVideoOutputVP::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pVideoOutput != nullptr);

	return CstiBSPInterfaceBase::m_pVideoOutput;
}

IstiVideoOutput *IstiVideoOutput::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pVideoOutput != nullptr);

	return CstiBSPInterfaceBase::m_pVideoOutput;
}

IstiVideoInput *IstiVideoInput::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pVideoInput != nullptr);

	return CstiBSPInterfaceBase::m_pVideoInput;
}


IstiAudioInput *IstiAudioInput::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pAudioInput != nullptr);

	return CstiBSPInterfaceBase::m_pAudioInput.get();
}

IstiAudioOutput *IstiAudioOutput::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_pAudioOutput != nullptr);

	return CstiBSPInterfaceBase::m_pAudioOutput.get();
}

ISLICRinger *ISLICRinger::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_slicRinger != nullptr);

	return CstiBSPInterfaceBase::m_slicRinger;
}

IstiNetwork *IstiNetwork::InstanceGet()
{
	stiASSERT (CstiBSPInterfaceBase::m_pNetwork != nullptr);

	return CstiBSPInterfaceBase::m_pNetwork;
}

IstiBluetooth *IstiBluetooth::InstanceGet()
{
	stiASSERT (CstiBSPInterfaceBase::m_pBluetooth != nullptr);

	return CstiBSPInterfaceBase::m_pBluetooth;
}

IstiHelios *IstiHelios::InstanceGet()
{
	stiASSERT (CstiBSPInterfaceBase::m_helios);

	return CstiBSPInterfaceBase::m_helios.get ();
}

IFanControl* IFanControl::InstanceGet()
{
	stiASSERT (CstiBSPInterfaceBase::m_fanControl);

	return CstiBSPInterfaceBase::m_fanControl.get ();
}

IUpdate *IUpdate::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_update);

	return CstiBSPInterfaceBase::m_update.get ();
}

IstiLogging *IstiLogging::InstanceGet ()
{
	stiASSERT (CstiBSPInterfaceBase::m_logging);

	return CstiBSPInterfaceBase::m_logging;
}

stiHResult CstiBSPInterfaceBase::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiVideoInput *pVideoInput = nullptr;
	CstiVideoOutput *pVideoOutput = nullptr;
	GError *pError = nullptr;

	gst_init_check (nullptr, nullptr, &pError);

	if (pError)
	{
		stiASSERTMSG (false, pError->message);

		g_error_free (pError);
		pError = nullptr;
	}
	
	RestartReasonDetermine ();

	MainLoopStart ();

	//
	// Create and start the monitor task
	//
	m_pMonitorTask = new CstiMonitorTask ();
	stiTESTCOND (nullptr != m_pMonitorTask, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pMonitorTask->Initialize ();
	stiTESTRESULT();

	m_signalConnections.push_back (m_pMonitorTask->hdmiInStatusChangedSignal.Connect (
		[this](int status)
		{
			hdmiInStatusChangedSignalGet ().Emit (status);
		}));

	hResult = m_pMonitorTask->Startup ();
	stiTESTRESULT ();

	//
	// create the CEC controller
	//
	m_cecControl = std::make_shared<CstiCECControl> ();
	stiTESTCOND (nullptr != m_cecControl, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_cecControl->Initialize(m_pMonitorTask, m_cecPort, m_osdName);
	stiTESTRESULT();

	hResult = m_cecControl->Startup();
	stiTESTRESULT();

	//
	// Create and start video input
	//
	pVideoInput = new CstiVideoInput ();
	stiTESTCOND (nullptr != pVideoInput, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = pVideoInput->Initialize (m_pMonitorTask);
	stiTESTRESULT ();
	
	hResult = pVideoInput->Startup ();
	stiTESTRESULT ();
	
	//
	// Now that the setup is complete, allow the object to be exposed to the world.
	//
	m_pVideoInput = pVideoInput;
	pVideoInput = nullptr;

	//
	// Create and start video output
	//
	pVideoOutput = new CstiVideoOutput ();
	stiTESTCOND (nullptr != pVideoOutput, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = pVideoOutput->Initialize (m_pMonitorTask, m_pVideoInput, m_cecControl);
	stiTESTRESULT ();

	hResult = pVideoOutput->Startup ();
	stiTESTRESULT ();

	//
	// Now that the setup is complete, allow the object to be exposed to the world.
	//
	m_pVideoOutput = pVideoOutput;
	pVideoOutput = nullptr;

	//
	// Create the UserInput object
	//
	hResult = userInputSetup();
	stiTESTRESULT();

	//
	// Create the bluetooth object
	//
	m_pBluetooth = new CstiBluetooth ();
	stiTESTCOND (nullptr != m_pBluetooth, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pBluetooth->Initialize ();
	stiTESTRESULT ();

	hResult = m_pBluetooth->Startup ();
	stiTESTRESULT ();

	//
	// Initialize the network object
	//
	m_pNetwork = new CstiNetwork (m_pBluetooth);
	stiTESTCOND (nullptr != m_pNetwork, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pNetwork->Initialize ();
	stiTESTRESULT ();

	//
	// Creates a task that listens for when the audio jack is plugged/unplugged
	//
	hResult = audioHandlerSetup();
	stiTESTRESULT ();

	//
	// create the AudioOutput object (not a task)
	//
	m_pAudioOutput = std::make_shared<CstiAudioOutput> ();
	stiTESTCOND (nullptr != m_pAudioOutput, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_pAudioOutput->Initialize (m_pAudioHandler, estiAUDIO_NONE);
	stiTESTRESULT ();

	//
	// Create the Helios object
	//
	m_helios = std::make_shared<CstiHelios> ();
	stiTESTCOND (m_helios, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_helios->Initialize (m_pBluetooth);
	stiTESTRESULT ();

	hResult = m_helios->Startup ();
	stiTESTRESULT ();

	//
	// Create the StatusLED device
	//
	hResult = statusLEDSetup ();
	stiTESTRESULT ();

	//
	// Create the LightRing device
	//
	hResult = lightRingSetup ();
	stiTESTRESULT();

	//
	// Create AudioInput object
	//
	hResult = audioInputSetup ();
	stiTESTRESULT ();

	//
	// Create the AudibleRinger device
	//
	m_pAudibleRinger = new CstiAudibleRinger ();
	stiTESTCOND (nullptr != m_pAudibleRinger, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_pAudibleRinger->Initialize (m_pAudioHandler);
	stiTESTRESULT ();
	
	m_slicRinger = new CstiSLICRinger ();
	stiTESTCOND (nullptr != m_slicRinger, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_slicRinger->startup ();
	stiTESTRESULT ();

	//
	// Create AdvancedStatus object
	//
	m_pAdvancedStatus = new CstiAdvancedStatus ();
	stiTESTCOND(nullptr != m_pAdvancedStatus, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_pAdvancedStatus->initialize (
				m_pMonitorTask,
				m_pAudioHandler,
				m_pAudioInput.get(),
				m_pVideoInput,
				m_pVideoOutput,
				m_pBluetooth);
	stiTESTRESULT();
	
	m_signalConnections.push_back (m_pMonitorTask->headphoneConnectedSignal.Connect (
		[this](bool connected)
		{
			headphoneConnectedSignal.Emit (connected);
		}));

	//
	// Initialize update.
	//
	m_update = std::make_shared<CstiUpdate> ();

	stiTESTCOND (m_update, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_update->Initialize ();
	stiTESTRESULT ();

	hResult = m_update->Startup ();
	stiTESTRESULT ();

	//
	// Create Logging object
	//
	m_logging = new CstiLogging ();
	stiTESTCOND(nullptr != m_logging, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_logging->Initialize ();
	stiTESTRESULT ();

STI_BAIL:

	if (pVideoInput)
	{
		delete pVideoInput;
		pVideoInput = nullptr;
	}

	if (pVideoOutput)
	{
		delete pVideoOutput;
		pVideoOutput = nullptr;
	}


	return hResult;
}


void CstiBSPInterfaceBase::ringStart ()
{
	if (m_slicRinger)
	{
		m_slicRinger->start ();
	}
}


void CstiBSPInterfaceBase::ringStop ()
{
	if (m_slicRinger)
	{
		m_slicRinger->stop ();
	}
}


bool CstiBSPInterfaceBase::webServiceIsRunning ()
{
	try
	{
		auto session = sci::make_unique<Poco::Net::HTTPClientSession>("localhost");

		if (session)
		{
			Poco::Net::HTTPRequest request("GET", "/health");
			session->sendRequest (request);

			Poco::Net::HTTPResponse response;
			session->receiveResponse (response);

			return (response.getStatus () < 300);
		}
	}
	catch (...)
	{
	}

	return false;
}


void CstiBSPInterfaceBase::webServiceStart ()
{	
	if (system ("vpdashboard-init"))
	{
		stiASSERTMSG (false, "error: executing system command: vpdashboard-init\n");
	}
}


stiHResult CstiBSPInterfaceBase::Uninitialize ()
{
	if (m_pNetwork)
	{
		delete m_pNetwork;
		m_pNetwork = nullptr;
	}
	
	if (m_pBluetooth)
	{
		delete m_pBluetooth;
		m_pBluetooth = nullptr;
	}

	if (m_pAudibleRinger)
	{
		delete m_pAudibleRinger;
		m_pAudibleRinger = nullptr;
	}

	m_pAudioInput = nullptr;

	m_pAudioOutput = nullptr;

	if (m_slicRinger)
	{
		delete m_slicRinger;
		m_slicRinger = nullptr;
	}
	
	if (m_pAudioHandler)
	{
		delete m_pAudioHandler;
		m_pAudioHandler = nullptr;
	}
	
	if (m_pVideoInput)
	{
		delete m_pVideoInput;
		m_pVideoInput = nullptr;
	}

	if (m_pVideoOutput)
	{
		delete m_pVideoOutput;
		m_pVideoOutput = nullptr;
	}

	if (m_pStatusLED)
	{
		m_pStatusLED->Uninitialize ();
		delete m_pStatusLED;
		m_pStatusLED = nullptr;
	}

	if (m_pLightRing)
	{
		delete m_pLightRing;
		m_pLightRing = nullptr;
	}

	m_helios = nullptr;

	if (m_pUserInput)
	{
		delete m_pUserInput;
		m_pUserInput = nullptr;
	}
	
	if (m_pAdvancedStatus)
	{
		delete m_pAdvancedStatus;
		m_pAdvancedStatus = nullptr;
	}

	if (m_pMonitorTask)
	{
		delete m_pMonitorTask;
		m_pMonitorTask = nullptr;
	}

	if (m_logging)
	{
		delete m_logging;
		m_logging = nullptr;
	}
		
	m_cecControl = nullptr;

	m_fanControl = nullptr;
	
	m_update = nullptr;

	if (m_pMainLoop)
	{
		g_main_loop_quit(m_pMainLoop);
		
		while (m_pMainLoop)
		{
			stiTrace ("Waiting for main loop to exit\n");
			sleep (1);
		}
	}

	gst_deinit ();


	return stiRESULT_SUCCESS;
}


stiHResult CstiBSPInterfaceBase::RestartSystem (
	EstiRestartReason eRestartReason)
{
	//
	// Write the restart reason code so that it can be reported on bootup.
	//
	std::string oFile;
	
	stiOSDynamicDataFolderGet (&oFile);
	
	oFile += "crash/RestartReason";
	
	FILE *pFile = fopen (oFile.c_str(), "w");

	if (pFile)
	{
		fprintf (pFile, "%d\n", eRestartReason);

		fclose (pFile);
	}

	rebootSystem ();
	
	return stiRESULT_SUCCESS;
}


/*!\brief Determine restart reason from the restart reason file or system register.
 *
 */
stiHResult CstiBSPInterfaceBase::RestartReasonDetermine ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	EstiRestartReason restartReason = estiRESTART_REASON_UNKNOWN;

	std::string oFile;
	
	stiOSDynamicDataFolderGet (&oFile);
	
	oFile += "crash/RestartReason";
	
	FILE *pFile = fopen (oFile.c_str(), "r");

	if (pFile)
	{
		int nLen = fscanf (pFile, "%d", reinterpret_cast<int*>(&restartReason));
		
		(void)nLen; // Silence the compiler about the unused variable.

		fclose (pFile);
		pFile = nullptr;

		unlink (oFile.c_str());
	}
	else 
	{ 
		restartReason = hardwareReasonGet();
	}

	m_eRestartReason = restartReason;

	return hResult;
}


/*!\brief Reboot the system
 *
 */
void CstiBSPInterfaceBase::rebootSystem ()
{
	if (system ("reboot"))
	{
		stiASSERTMSG (false, "error: reboot\n");
	}
}


/*!\brief Retrieve the restart reason
 *
 */
stiHResult CstiBSPInterfaceBase::RestartReasonGet (
	EstiRestartReason *peRestartReason)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*peRestartReason = m_eRestartReason;

	return hResult;
}


///
/// \brief: Interfaces with hardware to write the MAC address
///
stiHResult CstiBSPInterfaceBase::MacAddressWrite (const char *pszMac)
{
	//
	// For the softphone, simply return.
	//
	
	return stiRESULT_SUCCESS;
}

std::string CstiBSPInterfaceBase::mpuSerialNumberGet ()
{
	return (m_pAdvancedStatus->mpuSerialGet ());
}

std::string CstiBSPInterfaceBase::rcuSerialNumberGet ()
{
	return (m_pAdvancedStatus->rcuSerialGet ());
}

stiHResult CstiBSPInterfaceBase::HdmiInStatusGet (
	int *pnHdmiInStatus)
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiBSPInterfaceBase::HdmiInResolutionGet (
	int *pnHdmiInResolution)
{
	*pnHdmiInResolution = 1080;

	return stiRESULT_SUCCESS;
}

stiHResult CstiBSPInterfaceBase::HdmiPassthroughSet (
	bool bHdmiPassthrough)
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiBSPInterfaceBase::BluetoothPairedDevicesLog ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_pBluetooth)
	{
		hResult = m_pBluetooth->PairedDevicesLog ();
	}

	return hResult;
}

/*!\brief Adds in additional stats for call statistics.
 *
 */
void CstiBSPInterfaceBase::callStatusAdd (
	std::stringstream &callStatus)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiAdvancedStatus advancedStatus;

	auto videoInput = dynamic_cast<IVideoInputVP2 *>(IstiVideoInput::InstanceGet ());
	uint32_t panPercentage = 0;
	uint32_t tiltPercentage = 0;
	uint32_t zoomPercentage = 0;
	auto ambiance = IVideoInputVP2::AmbientLight::UNKNOWN;
	float rawAmbienceValue = 0;

	videoInput->cameraPTZGet (&panPercentage, &tiltPercentage, &zoomPercentage, nullptr, nullptr);
	videoInput->ambientLightGet (&ambiance, &rawAmbienceValue);
	
	hResult = m_pAdvancedStatus->advancedStatusGet (advancedStatus);
	stiTESTRESULT ();

	callStatus << "\nCamera Information:\n";
	callStatus << "\tSaturation = " << videoInput->SaturationGet () << "\n";
	callStatus << "\tBrightness = " << videoInput->BrightnessGet () << "\n";
	callStatus << "\tZoomPercentage = " << zoomPercentage << "\n";
	callStatus << "\tPanPercentage = " << panPercentage << "\n";
	callStatus << "\tTiltPercentage = " << tiltPercentage << "\n";
	callStatus << "\tFocusPosition = " << videoInput->FocusPositionGet () << "\n";

	hResult = serialNumberGet (callStatus, advancedStatus);
	stiTESTRESULT ();
	
	if (!advancedStatus.rcuLensType.empty ())
	{
		callStatus << "\tRCULensType = \"" << advancedStatus.rcuLensType << "\"\n";
	}

	callStatus << "\tLightLevel = " << toInteger(ambiance) << "\n";
	callStatus << "\tRawLightLevel = " << rawAmbienceValue << "\n";

	callStatus << "\nWireless Information:\n";
	callStatus << "\tWirelessConnected = " << advancedStatus.wirelessConnected << "\n";
	callStatus << "\tWirelessFrequency = " << advancedStatus.wirelessFrequency << " GHz" << "\n";
	callStatus << "\tWirelessDataRate = " << advancedStatus.wirelessDataRate << " Mbit/s" << "\n";
	callStatus << "\tWirelessBandwidth = " << advancedStatus.wirelessBandwidth << " MHz" << "\n";
	callStatus << "\tWirelessProtocol = " << advancedStatus.wirelessProtocol << "\n";
	callStatus << "\tWirelessSignalStrength = " << advancedStatus.wirelessSignalStrength << " dBm" << "\n";
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	callStatus << "\tWirelessTransmitPower = " << advancedStatus.wirelessTransmitPower << " dBm" << "\n";
#elif APPLICATION == APP_NTOUCH_VP2
	// "iw" tool unable to get TX power metric from VP2 wireless hardware
	callStatus << "\tWirelessTransmitPower = N/A\n";
#endif
	callStatus << "\tWirelessChannel = " << advancedStatus.wirelessChannel << "\n";

STI_BAIL:

	return;
}


/*!\brief Starts the main loop
 *
 */
stiHResult CstiBSPInterfaceBase::MainLoopStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTaskID TaskId = stiOS_TASK_INVALID_ID;

	stiDEBUG_TOOL(g_stiNetworkDebug,
		stiTrace("spawning the MainLoopTask\n");
	);

	m_pMainLoop = g_main_loop_new(nullptr, FALSE);
	stiTESTCOND (m_pMainLoop, stiRESULT_ERROR);

	TaskId = stiOSTaskSpawn("PlatformMainLoop",
					stiOS_TASK_DEFAULT_PRIORITY,
					stiOS_TASK_DEFAULT_STACK_SIZE,
					MainLoopTask,
					this);
	
	if (TaskId == stiOS_TASK_INVALID_ID)
	{
		g_main_loop_unref (m_pMainLoop);
		m_pMainLoop = nullptr;

		stiTrace ("CstiBSPInterfaceBase::MainLoopStart: Couldn't start MainLoopTask\n");
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*
 * This actually does the initialization.  it sets up the basic proxies,
 * gets the technologies and services, then sets up it's own mainloop so
 * that we get DBUS messages in a threadsafe manner.
 */
void * CstiBSPInterfaceBase::MainLoopTask (
	void * param)
{
	auto pThis = reinterpret_cast<CstiBSPInterfaceBase *>(param); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	g_main_loop_run(pThis->m_pMainLoop);

	g_main_loop_unref(pThis->m_pMainLoop);
	pThis->m_pMainLoop = nullptr;

	return (nullptr);
}


stiHResult CstiBSPInterfaceBase::audioHandlerSetup ()
{
        stiHResult hResult = stiRESULT_SUCCESS;

        m_pAudioHandler = new CstiAudioHandler ();
        stiTESTCOND (nullptr != m_pAudioHandler, stiRESULT_MEMORY_ALLOC_ERROR);

        hResult = m_pAudioHandler->Initialize (estiAUDIO_G711_MULAW, m_pMonitorTask, m_pBluetooth->AudioGet());
        stiTESTRESULT ();

        m_signalConnections.push_back (m_pAudioHandler->micMagnitudeSignal.Connect (
                [this](std::vector<float> &left, std::vector<float> &right)
                        {
                                micMagnitudeSignal.Emit (left, right);
                        }
                ));

        hResult = m_pAudioHandler->Startup ();

STI_BAIL:
        return hResult;
}
