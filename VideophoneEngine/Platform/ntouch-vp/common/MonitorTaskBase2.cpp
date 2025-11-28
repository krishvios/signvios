// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include <libudev.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <array>

#include <linux/input.h>

#include "stiSVX.h"
#include "stiTrace.h"
#include "MonitorTaskBase2.h"
#include "stiRemoteLogEvent.h"

#define BITS_PER_LONG			(sizeof(long) * 8)
#define NBITS(x)				((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)					((x) % BITS_PER_LONG)
#define BIT(x)					(1UL<<OFF(x))
#define LONG(x)					((x)/BITS_PER_LONG)
#define TEST_BIT(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

constexpr char UDEV_SUBSYSTEM_I2C[] = "i2c";
constexpr char UDEV_SUBSYSTEM_USB[] = "usb";
constexpr char UDEV_SUBSYSTEM_SWITCH[] = "switch";
constexpr char UDEV_SUBSYSTEM_VIDEO[] = "nvhost";
constexpr char UDEV_SUBSYSTEM_INPUT[] = "input";

constexpr char HEADSET_NAME[] = "skl_hda_card Headphone";
constexpr char MICROPHONE_NAME[] = "skl_hda_card Mic";
constexpr int stiMAX_EVENT_FILES = 12;
constexpr int TYPE_HEADSET = 0;
constexpr int TYPE_MIC = 1;


/*! \brief Destructor
 *
 * This is the default destructor for the MonitorTaskBase2 class.
 *
 * \return None
 */
MonitorTaskBase2::~MonitorTaskBase2 ()
{
	stiLOG_ENTRY_NAME (MonitorTaskBase2::~MonitorTaskBase2);

	Shutdown();

	// Cleanup the udev stuff
	if (m_pUdevMonitor)
	{
		udev_monitor_unref (m_pUdevMonitor);
		m_pUdevMonitor = nullptr;
	}

	if (m_pUdev)
	{
		udev_unref (m_pUdev);
		m_pUdev = nullptr;
	}
}

#define SYSTEM_EXECUTE_MAX_LINE_SIZE 512

std::string MonitorTaskBase2::systemExecute (
	const std::string &cmd)
{
	std::array<char, SYSTEM_EXECUTE_MAX_LINE_SIZE> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str (), "r"), pclose);
	
	if (!pipe) 
	{
		vpe::trace ("system_execute: popen() failed! for command = ", cmd, "\n");
	}
	
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}

	return result;
}

/*! \brief Initialize the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult MonitorTaskBase2::Initialize ()
{
	stiLOG_ENTRY_NAME (MonitorTaskBase2::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_bRcuConnected = isCameraAvailable ();
	
	stiDEBUG_TOOL (g_stiMonitorDebug,
		vpe::trace ("MonitorTaskBase2::Initialize: Camera connected = ", m_bRcuConnected, "\n");
	);
	
	// Initialize udev and file descriptor
	m_pUdev = udev_new ();
	stiTESTCOND (m_pUdev, stiRESULT_ERROR);

	m_pUdevMonitor = udev_monitor_new_from_netlink (m_pUdev, "udev");
	stiTESTCOND (m_pUdevMonitor, stiRESULT_ERROR);

	udev_monitor_filter_add_match_subsystem_devtype (m_pUdevMonitor, UDEV_SUBSYSTEM_I2C, nullptr);
	udev_monitor_filter_add_match_subsystem_devtype (m_pUdevMonitor, UDEV_SUBSYSTEM_USB, nullptr);
	udev_monitor_filter_add_match_subsystem_devtype (m_pUdevMonitor, UDEV_SUBSYSTEM_SWITCH, nullptr);
	udev_monitor_filter_add_match_subsystem_devtype (m_pUdevMonitor, UDEV_SUBSYSTEM_VIDEO, nullptr);
	udev_monitor_filter_add_match_subsystem_devtype (m_pUdevMonitor, UDEV_SUBSYSTEM_INPUT, nullptr);

	udev_monitor_enable_receiving (m_pUdevMonitor);

	m_udevFd = udev_monitor_get_fd (m_pUdevMonitor);
	stiTESTCOND (m_udevFd >= 0, stiRESULT_ERROR);

	// Tell the event loop to monitor this file descriptor
	if (!FileDescriptorAttach (
		m_udevFd,
		std::bind (&MonitorTaskBase2::EventUdevReadHandler, this)))
	{
		stiASSERTMSG(estiFALSE, "%s: Can't attach udevFd %d\n", __func__, m_udevFd);
	}

	HdmiOutHotPlugCheck ();

	m_headphoneFd = getHeadsetFd (TYPE_HEADSET);
	
	if (m_headphoneFd < 0)
	{
		vpe::trace ("WARNING: Can't find headphone device. Headphone capabilities will not work.\n");
	}
	else
	{
		headsetCurrentStateGet(TYPE_HEADSET, m_headphoneFd);

		stiDEBUG_TOOL(g_stiMonitorDebug,
			stiTrace("MonitorTaskBase2::Initialize: Headphone is %sconnected\n", m_headphone?"":"dis");
		);

		if (!FileDescriptorAttach (
			m_headphoneFd, std::bind (&MonitorTaskBase2::headphoneEventReadHandler, this)))
		{
			stiASSERTMSG (estiFALSE, "%s: Can't attach headphoneFd %d\n", __func__, m_headphoneFd);
		}
	}

	m_microphoneFd = getHeadsetFd (TYPE_MIC);
	
	if (m_microphoneFd < 0)
	{
		vpe::trace ("WARNING: Can't find mic device. Microphone capabilities will not work.\n");
	}
	else
	{
		headsetCurrentStateGet(TYPE_MIC, m_microphoneFd);

		stiDEBUG_TOOL(g_stiMonitorDebug,
			stiTrace("MonitorTaskBase2::Initialize: Micropone is %sconnected\n", m_microphone?"":"dis");
		);

		if (!FileDescriptorAttach (m_microphoneFd, std::bind (&MonitorTaskBase2::micEventReadHandler, this)))
		{
			stiASSERTMSG (estiFALSE, "%s: can't attach microphoneFD %d\n", __func__, m_microphoneFd);
		}
	}

	m_bInitialized = true;

STI_BAIL:

	return (hResult);

}//end MonitorTaskBase2::Initialize ()


/*! \brief Start the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult MonitorTaskBase2::Startup ()
{
	stiLOG_ENTRY_NAME (MonitorTaskBase2::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (!m_bInitialized)
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("MonitorTaskBase2::Startup: Need to call initialize before calling Startup\n");
		);

		stiTHROW (stiRESULT_ERROR);
	}

	if (!StartEventLoop ())
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return (hResult);

}

/*! \brief shutdown the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult MonitorTaskBase2::Shutdown ()
{
	stiLOG_ENTRY_NAME (MonitorTaskBase2::Shutdown);

	// Stop the event loop
	StopEventLoop ();

	return stiRESULT_SUCCESS;
}

// Handler for udev events
void MonitorTaskBase2::EventUdevReadHandler ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("udev says there should be udev data\n");
	);

	// Make the call to receive the device
	// select() from the event loop ensures this won't block
	udev_device *pUdevDevice = udev_monitor_receive_device (m_pUdevMonitor);
	
	if (!pUdevDevice)
	{
		stiTrace ("ERROR: No Device from udev_monitor_receive_device ()\n");
		stiASSERT (false);
	}

	const char *pszAction = udev_device_get_action (pUdevDevice);
	const char *pszDevicePath = udev_device_get_devpath (pUdevDevice);
	const char *pszDeviceName = udev_device_get_property_value(pUdevDevice, "NAME");

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("Got Device\n");
		stiTrace ("   Node: %s\n", udev_device_get_devnode (pUdevDevice));
		stiTrace ("   Subsystem: %s\n", udev_device_get_subsystem (pUdevDevice));
		stiTrace ("   Devtype: %s\n", udev_device_get_devtype (pUdevDevice));
		stiTrace ("   Action: %s\n", pszAction);
		stiTrace ("   Devpath: %s\n", pszDevicePath);
		stiTrace ("   DevName: %s\n", pszDeviceName);
	);

	#if 0
	//TODO: For VP3, find trigger that camera has plugged in or out
	else if (!strcmp(pszDevicePath, UDEV_DEVICE_PATH_RCU))
	{
		bool bStatusChanged = false;

		if (!strcmp(pszAction, UDEV_ACTION_REMOVE))
		{
			if (m_bRcuConnected)
			{
				m_bRcuConnected = false;
				bStatusChanged = true;
			}
		}
		else if (!strcmp(pszAction, UDEV_ACTION_ADD))
		{
			if (!m_bRcuConnected)
			{
				m_bRcuConnected = true;
				bStatusChanged = true;
			}
		}

		if (bStatusChanged)
		{
			usbRcuConnectionStatusChangedSignal.Emit ();
		}
	}
	else
	{
		stiTrace("UDEV event from other device: %s\n", pszDevicePath);
	}
#endif
	
	if (pUdevDevice)
	{
		udev_device_unref (pUdevDevice);
		pUdevDevice = nullptr;
	}
}

stiHResult MonitorTaskBase2::HdmiInStatusGet (
	int *pnHdmiInStatus)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// For VP3 there is no HDMI in capability
	*pnHdmiInStatus = 0;

//STI_BAIL:

	return hResult;
}

stiHResult MonitorTaskBase2::RcuConnectedStatusGet (
	bool *pbRcuConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	*pbRcuConnected = m_bRcuConnected;

	Unlock ();

	return hResult;
}

/* \brief Check the state of the hdmi hot plug pin
 *
 * this is seperate from the hdmi state, because we can be hot plugged
 * but still not have hdmi connectivity.
 *
 * \return none
 */
void MonitorTaskBase2::HdmiOutHotPlugCheck ()
{

	bool bDetected = false;

	std::ifstream file ("/sys/class/drm/card0/card0-HDMI-A-1/status", std::ifstream::in);
	if (!file.is_open ())
	{
		stiASSERTMSG(estiFALSE, "Can't open hdmi plug state\n");
	}
	else
	{
		std::string plugState;
		std::getline (file, plugState);

		bDetected = plugState == "connected";
		if (m_bHotPlugDetected != bDetected)
		{
			m_bHotPlugDetected = bDetected;
			hdmiHotPlugStateChangedSignal.Emit ();
		}
	}
}

/* \brief get the status of the hdmi hot plug pin
 *
 * \return true if the hot plug pin is high, else false
 */
bool MonitorTaskBase2::HdmiHotPlugGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return (m_bHotPlugDetected);
}

void MonitorTaskBase2::headsetInitialStateGet()
{
	headsetCurrentStateGet(TYPE_HEADSET, m_headphoneFd);
	headsetCurrentStateGet(TYPE_MIC, m_microphoneFd);
}

void MonitorTaskBase2::headsetCurrentStateGet(int type, int tmpFd)
{

	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	unsigned long state[KEY_CNT] = {0};

	memset(bit, 0, sizeof(bit));
	ioctl(tmpFd, EVIOCGBIT(0, EV_MAX), bit[0]);
	ioctl(tmpFd, EVIOCGSW(sizeof(state)), state);
	ioctl(tmpFd, EVIOCGBIT(EV_SW, KEY_MAX), bit[EV_SW]);
	for (int code = 0; code < KEY_MAX; code++)
	{
		if (TEST_BIT(code, bit[EV_SW]))
		{
			if (type == TYPE_HEADSET)
			{
				m_headphone = TEST_BIT(code, state);
			}
			else
			{
				m_microphone = TEST_BIT(code, state);
			}
			break;
		}
	}
}

int MonitorTaskBase2::getHeadsetFd (
	int type)
{

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::getHeadsetFd: type = %s\n", type == TYPE_HEADSET?"headphone":"microphone");
	);

	const char *searchName = nullptr;
	int tmpFd = -1;

	if (type == TYPE_HEADSET)
	{
		searchName = HEADSET_NAME;
	}
	else
	{
		searchName = MICROPHONE_NAME;
	}

	for (int i = 0; i <= stiMAX_EVENT_FILES; i++)
	{
		char fileName[32];
		char deviceName[64];
		sprintf(fileName, "/dev/input/event%d", i);

		if ((tmpFd = open(fileName, O_RDONLY | O_NONBLOCK)) < 0)
		{
			break;
		}

		if (ioctl(tmpFd, EVIOCGNAME(sizeof(deviceName)), deviceName) < 0)
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("\tMonitorTaskBase2::getHeadsetFd: get device name for %s failed\n", fileName);
			);

			close (tmpFd);
			tmpFd = -1;
			continue;
		}

		if (!strncmp (deviceName, searchName, strlen(searchName)))
		{
			break;
		}
		close(tmpFd);
		tmpFd = -1;
	}

	stiDEBUG_TOOL(g_stiMonitorDebug,
		vpe::trace ("MonitorTaskBase2::getHeadsetFd: returning ", tmpFd, " for ", type, "\n");
	);

	return(tmpFd);
}

int MonitorTaskBase2::readHeadsetState (
	int fd)
{
	
	struct input_event inputEvent{};

	int bytesRead;
	int retVal = -1;

	while (true)
	{
		bytesRead = read(fd, &inputEvent, (signed int)sizeof(struct input_event));

		if (bytesRead <= 0)
		{
			break;			// nothing to read, we're done
		}
		else if (bytesRead < (signed int)sizeof(struct input_event))
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("MonitorTaskBase2::readHeadsetState: failed to read full packet - %d\n", errno);
			);
			break;
		}
		else
		{
			if (inputEvent.type == EV_SYN)
			{
				stiDEBUG_TOOL(g_stiMonitorDebug,
					stiTrace("MonitorTaskBase2::readHeadsetState: type is EV_SYN; skipping\n");
				);
				continue;
			}
			else if (inputEvent.type == EV_SW)
			{
				retVal = inputEvent.value;
				break;
			}
		}
	}
	
	stiDEBUG_TOOL(g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::readHeadsetState: returning %d\n", retVal);
	);
	
	return(retVal);
}

void MonitorTaskBase2::headphoneEventReadHandler ()
{
	stiDEBUG_TOOL(g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::headphoneEventReadHandler: enter\n");
	);

	int value = readHeadsetState(m_headphoneFd);

	stiDEBUG_TOOL(g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::headphoneEventReadHandler: current state = %d, read state = %d\n", m_headphone, value);
	);

	if (value > -1 && value != m_headphone)
	{
		m_headphone = value;
		stiDEBUG_TOOL(g_stiMonitorDebug,
			vpe::trace ("emitting headphone state ", m_headphone, "\n");
		);
		headphoneConnectedSignal.Emit(m_headphone);
	}
}

bool MonitorTaskBase2::headphoneConnectedGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return (m_headphone > 0);
}

void MonitorTaskBase2::micEventReadHandler ()
{
	stiDEBUG_TOOL(g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::micEventReadHandler: enter\n");
	);

	int value = readHeadsetState(m_microphoneFd);

	stiDEBUG_TOOL(g_stiMonitorDebug,
		stiTrace("MonitorTaskBase2::micEventReadHandler: current state = %d, read state = %d\n", m_microphone, value);
	);

	if (value > -1 && value != m_microphone)
	{
		m_microphone = value;
		stiDEBUG_TOOL(g_stiMonitorDebug,
			vpe::trace ("emitting microphone state ", m_microphone, "\n");
		);
		microphoneConnectedSignal.Emit(m_microphone);
	}
}

bool MonitorTaskBase2::microphoneConnectedGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return (m_microphone > 0);
}
