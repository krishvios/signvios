#include "CstiBSPInterface.h"
#include "CstiVideoOutput.h"
#include "CstiAudioHandler.h"
#include "CstiFileIO.h"
#include "stiTools.h"
#include <regex>
#include <fstream>
#include "constants.h"
#include <cstdio>
#include <unistd.h>

CFilePlayGS *CstiBSPInterface::m_filePlay = nullptr;
//CstiMatter *CstiBSPInterface::m_matter = nullptr;
CstiMatterStubs *CstiBSPInterface::m_matter = nullptr;
using EstiRestartReason = CstiBSPInterface::EstiRestartReason;

std::shared_ptr<AudioTest> CstiBSPInterface::m_audioTest {nullptr};


CstiBSPInterface::CstiBSPInterface ()
:
	CstiBSPInterfaceBase()
{
	m_cecPort = "Linux";
	m_osdName = "Lumina2";
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


IAudioTest* IAudioTest::InstanceGet()
{
	stiASSERT (CstiBSPInterface::m_audioTest);

	return CstiBSPInterface::m_audioTest.get ();
}

IMatter *IMatter::InstanceGet ()
{
	stiASSERT (CstiBSPInterface::m_matter);

	return CstiBSPInterface::m_matter;
}

stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	serialNumberRead ();
	
	hResult = CstiBSPInterfaceBase::Initialize ();
	stiTESTRESULT ();

	m_socketListener = sci::make_unique<SocketListenerBase>(m_pMonitorTask);
	m_socketListener->initialize ();
	m_socketListener->startup ();

	m_audioTest = std::make_shared<AudioTest> ();
	m_audioTest->initialize (m_pAudioHandler);

	// required even though Lumina2 has no fans
	m_fanControl = std::make_shared<FanControlBase> ();

	BluetoothPairedDevicesLog ();

	m_signalConnections.push_back (m_socketListener->testModeSetSignal.Connect (
		[this](bool testMode)
		{
			testModeSetSignal.Emit (testMode);
		}));

	m_signalConnections.push_back (m_pMonitorTask->microphoneConnectedSignal.Connect (
		[this](bool connected)
		{
			microphoneConnectedSignal.Emit (connected);
		}));

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

/*
	//
	// Create and initialize Matter hub.
	//
	m_matter = new CstiMatter ();

	stiTESTCOND (m_matter, stiRESULT_ERROR);

	hResult = m_matter->Initialize ();

	stiTESTRESULT ();

	hResult = m_matter->Startup ();

	stiTESTRESULT ();
*/
	//
	// Create and initialize Matter hub (just stubs in lumina1).
	//
	m_matter = new CstiMatterStubs ();

	stiTESTCOND (m_matter, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult CstiBSPInterface::Uninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_socketListener = nullptr;

   	delete m_filePlay;
   	m_filePlay = nullptr;
	
	m_audioTest = nullptr;

	hResult = CstiBSPInterfaceBase::Uninitialize ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult CstiBSPInterface::lightRingSetup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto serialNumber = serialNumberGet ();

	m_pLightRing = new CstiLightRing ();
	stiTESTCOND (nullptr != m_pLightRing, stiRESULT_MEMORY_ALLOC_ERROR);

	hResult = m_pLightRing->Initialize (m_helios, serialNumber);
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

	hResult = m_pStatusLED->Initialize (m_helios);
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

stiHResult CstiBSPInterface::userInputSetup()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_pUserInput = new CstiUserInput ();
	stiTESTCOND (nullptr != m_pUserInput, stiRESULT_MEMORY_ALLOC_ERROR);
	
	hResult = m_pUserInput->Initialize (m_pMonitorTask, m_cecControl);
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
		vpe::trace ("CstiBSPInterface::cpuSpeedSet: cpuSpeedSet currently does nothing on lumina2\n");
	);
	
	return stiRESULT_SUCCESS;
}

std::string CstiBSPInterface::serialNumberGet () const
{
	std::string serialNumber {};
	{
		std::lock_guard<std::mutex> lock (m_mutex);
		serialNumber = m_serialNumber;
	}
	return serialNumber;
}

stiHResult CstiBSPInterface::serialNumberSet (const std::string &serialNumber)
{
	auto hResult = stiRESULT_SUCCESS;
	
	auto localSerialNumber {serialNumber};
	Trim (&localSerialNumber);
	
	auto match = std::regex_match (localSerialNumber, std::regex (SERIAL_NUMBER_REGEX));
	stiTESTCONDMSG (match, stiRESULT_ERROR, "serial number is formatted incorrectly");

	{
		int fd = open ("/sys/class/i2c-eeprom/serial-number/value", O_WRONLY);
		if (fd > -1)
		{
			ssize_t bytesWritten = write (fd, localSerialNumber.c_str (), localSerialNumber.length ());
			close (fd);

			if (bytesWritten != static_cast<ssize_t>(localSerialNumber.length ()))
			{
				stiTHROWMSG (stiRESULT_ERROR, "Failed to write complete serial number: %s\n", strerror (errno));
			}
		}
		else
		{
			stiTHROWMSG (stiRESULT_ERROR, "Can't open serial number file for writing: %s\n", strerror (errno));
		}
	}

	{
		std::lock_guard<std::mutex> lock (m_mutex);
		m_serialNumber = localSerialNumber;
	}

STI_BAIL:

	return hResult;
}

void CstiBSPInterface::serialNumberRead ()
{
	auto hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG (hResult);
	char buffer[33];
	int fd {-1};

	memset (buffer, '\0', sizeof (buffer));

	fd = open ("/sys/class/i2c-eeprom/serial-number/value", O_RDONLY);
	if (fd > -1)
	{
		// read up to 32 characters
		ssize_t bytesRead = read (fd, buffer, sizeof(buffer) - 1);
		close (fd);
		
		if (bytesRead < 0)
		{
			stiTHROWMSG (stiRESULT_ERROR, "Can't read product serial number: %s\n", strerror (errno));
		}
		
		// Remove any trailing newline/whitespace that sysfs might include
		for (ssize_t i = bytesRead - 1; i >= 0 && (buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == ' ' || buffer[i] == '\t'); --i)
		{
			buffer[i] = '\0';
		}
		
		m_serialNumber = buffer;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "Can't open product serial number: %s\n", strerror (errno));
	}

STI_BAIL:

	return;
}

stiHResult CstiBSPInterface::audioHandlerSetup ()
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

void CstiBSPInterface::rebootSystem ()
{
	if (system ("/sbin/shutdown -r now"))
	{
		stiASSERTMSG (false, "error: /sbin/shutdown -r now\n");
	}
}

void CstiBSPInterface::systemSleep ()
{
	dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet())->selfViewEnabledSet (false);
	dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet())->systemSleepSet (true);
	//TODO: Work out means to turn HDMI off on
	//system("echo off > /sys/class/drm/card0/card0-HDMI-A-1/status");
	cpuSpeedSet (estiCPU_SPEED_LOW);
}

void CstiBSPInterface::systemWake ()
{
	cpuSpeedSet (estiCPU_SPEED_MED);
	//TODO: Work out means to turn HDMI  on
	//system("echo on > /sys/class/drm/card0/card0-HDMI-A-1/status");
	dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet())->systemSleepSet (false);
	dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet())->selfViewEnabledSet (true);
}

stiHResult CstiBSPInterface::serialNumberGet (
	std::stringstream &callStatus,
	const SstiAdvancedStatus &advancedStatus) const
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!advancedStatus.mpuSerialNumber.empty ())
	{
		callStatus << "\tSerialNumber = " << advancedStatus.mpuSerialNumber << "\n";
	}
	
	return hResult;
}

void CstiBSPInterface::currentTimeSet (
	const time_t currentTime)
{
	dynamic_cast<IVideoInputVP2*>(IstiVideoInput::InstanceGet())->currentTimeSet (currentTime);
	m_logging->currentTimerSet (true);
}
