#include "CstiBSPInterface.h"
#include "CstiVideoOutput.h"
#include "CstiAudioHandler.h"
#include "CstiFileIO.h"
#include "stiTools.h"
#include <regex>
#include <fstream>
#include "constants.h"
#include <cstdio>
#include "FanControl.h"
#include <unistd.h>
#include <sys/io.h>

CFilePlayGS *CstiBSPInterface::m_filePlay = nullptr;
CstiMatterStubs *CstiBSPInterface::m_matter = nullptr;
using EstiRestartReason = CstiBSPInterface::EstiRestartReason;

std::shared_ptr<AudioTest> CstiBSPInterface::m_audioTest {nullptr};

static std::map<CstiBSPInterfaceBase::EstiCpuSpeed, std::string> EstiCpuSpeedMap = 
{
	{CstiBSPInterfaceBase::estiCPU_SPEED_LOW, "810000"},
	{CstiBSPInterfaceBase::estiCPU_SPEED_MED, "910000"},
	{CstiBSPInterfaceBase::estiCPU_SPEED_HIGH, "1100000"},
};


CstiBSPInterface::CstiBSPInterface ()
:
	CstiBSPInterfaceBase()
{
	m_cecPort = "GPIO";
	m_osdName = "Lumina";
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

IAudioTest* IAudioTest::InstanceGet()
{
	stiASSERT (CstiBSPInterface::m_audioTest);

	return CstiBSPInterface::m_audioTest.get ();
}

stiHResult CstiBSPInterface::Initialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	serialNumberRead ();
	hdccRead ();
	
	hResult = CstiBSPInterfaceBase::Initialize ();
	stiTESTRESULT ();

	m_socketListener = sci::make_unique<SocketListenerBase>(m_pMonitorTask);
	m_socketListener->initialize ();
	m_socketListener->startup ();

	m_tofSensor = sci::make_unique<ToFSensor>();
	m_tofSensor->initialize ();
	m_tofSensor->startup ();

	m_audioTest = std::make_shared<AudioTest> ();
	m_audioTest->initialize (m_pAudioHandler);

	m_fanControl = std::make_shared<FanControl> ();

	BluetoothPairedDevicesLog ();

	m_signalConnections.push_back (m_socketListener->testModeSetSignal.Connect (
		[this](bool testMode)
		{
			testModeSetSignal.Emit (testMode);
			
			if (m_fanControl)
			{
				dynamic_cast<FanControl*>(m_fanControl.get ())->testModeSet (testMode);
			}
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
	
	m_tofSensor = nullptr;

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

void CstiBSPInterface::tofDistanceGet (const std::function<void(int)> &onSuccess)
{
	if (m_tofSensor)
	{
		m_tofSensor->distanceGet (onSuccess);
	}
}

stiHResult CstiBSPInterface::cpuSpeedSet (
	EstiCpuSpeed eCpuSpeed)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
		vpe::trace ("CstiBSPInterface::cpuSpeedSet: called with eCpuSpeed = ", eCpuSpeed, "\n");
	);
	
	const std::string updateGovernors = "echo performance >  /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor && echo performance >  /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor";
	std::stringstream updateMaxFreq;
	bool updateCpuSpeed = false;
	FILE *fp = nullptr;

	fp = popen(updateGovernors.c_str(), "w");
	
	if (fp)
	{
		char error[PATH_MAX];
		
		if (fread(error, 1, sizeof(error), fp))
		{
			stiTHROWMSG (stiERROR_FLAG, "Error updating governor files for CPUs");
		}
		
		updateCpuSpeed = true;
		pclose(fp);
		fp = nullptr;
	} 

	if (updateCpuSpeed)
	{
		stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
			vpe::trace ("CstiBSPInterface::cpuSpeedSet: updateCpuSpeed = ", updateCpuSpeed, "\n");
		);
		
		auto updateCPUSpeed  {[](int cpuId, const std::string &speed)
						{
							stiDEBUG_TOOL (g_cpuSpeedDebug || g_stiVideoInputDebug,
								vpe::trace ("updateCPUSpeed: cpuId = ", cpuId, "\n");
								vpe::trace ("updateCPUSpeed: speed = ", speed, "\n");
							);
							
							return std::string("echo ") + speed +
						 	" > /sys/devices/system/cpu/cpu" + std::to_string(cpuId) +
						 	"/cpufreq/scaling_max_freq"; 
						}
		};

		updateMaxFreq << updateCPUSpeed(0, EstiCpuSpeedMap[eCpuSpeed]) << " && "
									<< updateCPUSpeed(1, EstiCpuSpeedMap[eCpuSpeed]);

		//Update the max_freq files to desired speed
		fp = popen(updateMaxFreq.str().c_str(), "w");
		
		if (fp)
		{
			char error[PATH_MAX];
			if (fread(error, 1, sizeof(error), fp))
			{
				stiTHROWMSG (stiERROR_FLAG, "Error updating max_freq files for CPUs");
			}
			pclose (fp);
		} 
	}

STI_BAIL:

	return hResult;
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
		std::string command {"amide.sh /SS " + localSerialNumber +  " >/dev/null 2>&1"};
		if (system (command.c_str ()))
		{
			stiTHROWMSG (false, "error setting serial number");
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
	char buffer[32];
	int fd {-1};

	memset(buffer, '\0', sizeof(buffer));

	if ((fd = open("/sys/class/dmi/id/product_serial", O_RDONLY)) > -1)
	{
		if (read(fd, buffer, sizeof(buffer)) < 0)
		{
			stiTHROWMSG(stiRESULT_ERROR, "Can't read product serial number: %s\n", strerror(errno));
		}
		m_serialNumber = buffer;
	}
	else
	{
		stiTHROWMSG(stiRESULT_ERROR, "Can't open product serial number: %s\n", strerror(errno));
	}

STI_BAIL:
	if (fd > -1)
	{
		close(fd);
	}

	return;
}

std::tuple<int, boost::optional<int>> CstiBSPInterface::hdccGet () const
{
	int hdcc {0};
	boost::optional<int> hdccOverride;

	{
		std::lock_guard<std::mutex> lock (m_mutex);
		hdcc = m_hdcc;
	}
	
	std::string hdccFilePath;
	stiOSDynamicDataFolderGet (&hdccFilePath);
	hdccFilePath += "hdcc";

	std::ifstream ifs {hdccFilePath, std::ifstream::in};

	if (ifs.is_open ())
	{
		std::string line;
		std::getline (ifs, line);

		try
		{
			hdccOverride = std::stoi (line);
		}
		catch (...)
		{
			vpe::trace (hdccFilePath, " is present but formatted incorrectly.\n");
		}
    }

	return {hdcc, hdccOverride};
}


std::tuple<stiHResult, stiHResult> CstiBSPInterface::hdccSet (boost::optional<int> hdcc, boost::optional<int> hdccOverride)
{
	auto hdccResult {stiRESULT_SUCCESS};
	auto hdccOverrideResult {stiRESULT_SUCCESS};

	if (hdcc)
	{
		hdccResult = hdccSet (*hdcc);
	}

	if (hdccOverride)
	{
		hdccOverrideResult = hdccOverrideSet (*hdccOverride);	
	}

	return {hdccResult, hdccOverrideResult};
}


stiHResult CstiBSPInterface::hdccSet (int hdcc)
{
	auto hResult {stiRESULT_SUCCESS};
	
	std::string command {"amide.sh /OS 1 hdcc:" + std::to_string (hdcc) + " >/dev/null 2>&1"};

	if (system (command.c_str ()))
	{
		stiTHROWMSG (stiRESULT_ERROR, "failed to set hdcc");
	}

	{
		std::lock_guard<std::mutex> lock (m_mutex);
		m_hdcc = hdcc;
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiBSPInterface::hdccOverrideSet (int hdccOverride)
{
	auto hResult {stiRESULT_SUCCESS};
	
	std::string hdccFilePath;
	stiOSDynamicDataFolderGet (&hdccFilePath);
	hdccFilePath += "hdcc";

	std::ofstream ofs {hdccFilePath, std::ofstream::trunc};

	if (ofs.is_open ())
	{
		ofs << hdccOverride << std::endl;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "failed to set hdccOverride");
	}

STI_BAIL:

	return hResult;
}


void CstiBSPInterface::hdccOverrideDelete ()
{
	std::string hdccFilePath;
	stiOSDynamicDataFolderGet (&hdccFilePath);
	hdccFilePath += "hdcc";

	remove (hdccFilePath.c_str ());
}


void CstiBSPInterface::hdccRead ()
{
	auto hResult {stiRESULT_SUCCESS}; stiUNUSED_ARG (hResult);
	char buffer[32];

	auto *file = popen ("amide.sh /OS 2>/dev/null | grep \"OEM string\" | sed 's/.*\"\\(.*\\)\"/\\1/'", "re");
	stiTESTCONDMSG (file, stiRESULT_UNABLE_TO_OPEN_FILE, "failed to open hdcc file");

	{
		std::string hdcc {};
		while (fgets (buffer, sizeof (buffer), file) != nullptr)
		{
			hdcc += buffer;
		}

		pclose (file);

		RightTrim (&hdcc);

		auto pos = hdcc.find ("hdcc:");
		if (pos == std::string::npos)
		{
			stiTHROWMSG (false, "hdcc value must be prefaced with \"hdcc:\"");
		}
		
		try
		{
			std::lock_guard<std::mutex> lock (m_mutex);
			m_hdcc = std::stoi (hdcc.substr (pos + 5)); // +5 to exclude "hdcc:"
		}
		catch (...)
		{
			stiTHROWMSG (false, "hdcc value is invalid: \"%s\"", hdcc.c_str ());
		}
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
	if (system ("/usr/sbin/shutdown -r now"))
	{
		stiASSERTMSG (false, "error: /usr/sbin/shutdown -r now\n");
	}
}

EstiRestartReason CstiBSPInterface::hardwareReasonGet ()
{
	const int portAddress = 0x70; //RTC
	const int disablePort = 0;
	const int enablePort = 1;
	const int bootCheck = 0xD1;
	unsigned char  val = 0;

	EstiRestartReason restartReason = estiRESTART_REASON_UNKNOWN;

	if (ioperm(portAddress, 3, enablePort) == 0) 
	{
		outb(bootCheck, portAddress);
		usleep(100000);
		val = inb(portAddress+1);
		ioperm(portAddress, 3, disablePort);
		if(val == 1)
		{
				restartReason = estiRESTART_REASON_POWER_ON_RESET;
		}
		else if(val == 0)
		{
				restartReason = estiRESTART_REASON_SOFTWARE_RESET;
		}
	}

	return restartReason;
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
