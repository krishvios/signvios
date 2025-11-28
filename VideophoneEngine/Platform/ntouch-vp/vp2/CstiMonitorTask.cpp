// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include <libudev.h>
#include <fcntl.h>
#include <rcu_api.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include <media/ov640_v4l2.h>

#include "stiSVX.h"
#include "stiTrace.h"
#include "CstiMonitorTask.h"
#include "stiOVUtils.h"
#include "stiRemoteLogEvent.h"

constexpr char UDEV_DEVICE_PATH_HDMI_IN[]   = "/devices/platform/tegra-i2c.2/i2c-2/2-000f";
constexpr char UDEV_DEVICE_PATH_RCU[]       = "/devices/platform/tegra-ehci.2/usb1/1-1/1-1:1.1";
constexpr char UDEV_DEVICE_PATH_HDMI_OUT[]  = "/devices/virtual/switch/hdmi";
constexpr char UDEV_DEVICE_PATH_VI[]        = "/devices/vi";
constexpr char UDEV_DEVICE_PATH_HEADSET[]   = "/devices/virtual/switch/h2w";
constexpr char UDEV_DEVICE_PATH_TEMP[]      = "/devices/platform/tegra-i2c.4/i2c-4/4-004c";

constexpr char UDEV_DEVICE_PATH_HEADSET_STATE[] = "/sys/devices/virtual/switch/h2w/state";

constexpr char UDEV_SUBSYSTEM_I2C[]         = "i2c";
constexpr char UDEV_SUBSYSTEM_USB[]         = "usb";
constexpr char UDEV_SUBSYSTEM_SWITCH[]      = "switch";
constexpr char UDEV_SUBSYSTEM_VIDEO[]       = "nvhost";
constexpr char UDEV_SUBSYSTEM_INPUT[]       = "input";

constexpr char UDEV_ACTION_CHANGE[]         = "change";
constexpr char UDEV_ACTION_REMOVE[]         = "remove";
constexpr char UDEV_ACTION_ADD[]            = "add";

constexpr int HDMI_OUT_FRAME_RATE_MIN = 30.0;
constexpr int EPROM_BUFFER_LENGTH     = 64;
constexpr int LENS_TYPE_BLOCK         = 19;

// these two are defined in the kernel in drivers/media/video/tc358743-regs.h
// as H2C_VI_SIGNAL_720P and H2C_VI_SIGNAL_1080P
// other resolutions are not being supported.
constexpr int stiHDMI_RESOLUTION_720  = 12;
constexpr int stiHDMI_RESOLUTION_1080 = 15;

constexpr int HDMI_BOUNCE_TIMEOUT_MS  = 100;   // wait 100 milliseconds

constexpr char TEMPERATURE_FILE[] = "/sys/devices/platform/tegra-i2c.4/i2c-4/4-004c/ext_temperature";


/*! \brief Constructor
 *
 * This is the default constructor for the CstiMonitorTask class.
 *
 * \return None
 */
CstiMonitorTask::CstiMonitorTask ()
	:
	m_bounceTimer(HDMI_BOUNCE_TIMEOUT_MS)
{
	stiLOG_ENTRY_NAME (CstiMonitorTask::CstiMonitorTask);
}

/*! \brief Destructor
 *
 * This is the default destructor for the CstiMonitorTask class.
 *
 * \return None
 */
CstiMonitorTask::~CstiMonitorTask ()
{
	stiLOG_ENTRY_NAME (CstiMonitorTask::~CstiMonitorTask);

	Shutdown();

	// Make sure the timer doesn't send an event.
	m_bounceTimer.Stop();

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

static std::string readState (
	const std::string &path)
{
	std::string state;

	std::ifstream file (path, std::ifstream::in);
	if (!file.is_open ())
	{
		stiASSERTMSG(estiFALSE, "Can't open hdmi plug state\n");
	}
	else
	{
		std::getline (file, state);
	}

	return state;
}


/*! \brief Initialize the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult CstiMonitorTask::Initialize ()
{
	stiLOG_ENTRY_NAME (CstiMonitorTask::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = HdmiInResolutionUpdate (nullptr);
	stiTESTRESULT ();

	hResult = HdmiInStatusUpdate (nullptr);
	stiTESTRESULT ();

	// Is the resolution supported?
	if (m_nHdmiInStatus &&
		(m_nHdmiInResolution != stiHDMI_RESOLUTION_720 &&
		 m_nHdmiInResolution != stiHDMI_RESOLUTION_1080))
	{
		m_nHdmiInStatus = false;
	}

	hResult = HdmiOutCapabilitiesUpdate (nullptr);
	stiTESTRESULT ();

	// this directory structure will be present if the RCU camera is plugged in
	struct stat st;

	if (stat("/sys/devices/platform/tegra-ehci.2/usb1/1-1", &st) > -1 && S_ISDIR(st.st_mode))
	{
		m_bRcuConnected = true;
		// the RCU can be connected, but mfddrvd not running.
		// if the RCU is not connected mfddrv will never be running
		if (RcuApiInit() > -1)
		{
			hResult = RcuLensTypeSet ();
			stiTESTRESULT();
			m_bMfddrvdRunning = true;
		}
		else
		{
			m_bMfddrvdRunning = false;
		}
	}
	else
	{
		m_bRcuConnected = false;
	}

	if (!m_bHdmiFixed)
	{
		m_bounceTimerConnection = m_bounceTimer.timeoutEvent.Connect (
			[this]() {
				PostEvent (
					std::bind (&CstiMonitorTask::EventBounceTimerTimeout, this));
			});
	}

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
		std::bind (&CstiMonitorTask::EventUdevReadHandler, this)))
	{
		stiASSERTMSG (estiFALSE, "%s: Can't attach udevFd %d\n", __func__, m_udevFd);
	}

	HdmiOutHotPlugCheck();

	{
		auto state = readState (UDEV_DEVICE_PATH_HEADSET_STATE);

		m_headphone = state == "1";
	}

	m_bInitialized = true;

STI_BAIL:

	return (hResult);

}//end CstiMonitorTask::Initialize ()


/*! \brief Start the monitor task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult CstiMonitorTask::Startup ()
{
	stiLOG_ENTRY_NAME (CstiMonitorTask::Startup);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (!m_bInitialized)
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::Startup: Need to call initialize before calling Startup\n");
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
stiHResult CstiMonitorTask::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiMonitorTask::Shutdown);

	// Stop the event loop
	StopEventLoop ();

	return stiRESULT_SUCCESS;
}

// Handler for udev events
void CstiMonitorTask::EventUdevReadHandler()
{
	stiHResult hResult = stiRESULT_SUCCESS;

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

	if (!strcmp (pszDevicePath, UDEV_DEVICE_PATH_HDMI_IN))
	{
		if (!strcmp(pszAction, UDEV_ACTION_CHANGE))
		{
			if (!m_bHdmiFixed)
			{
				stiDEBUG_TOOL(g_stiMonitorDebug,
					stiTrace("CstiMonitorTask: Hardware fix not detected, setting bounce timer\n");
				);
				if (!m_bInBounce)
				{
					m_bInBounce = true;
					m_bounceTimer.Start ();
				}
			}
			else
			{
				stiDEBUG_TOOL(g_stiMonitorDebug,
					stiTrace("CstiMonitorTask: Hardware fix detected: sending message\n");
				);
				hResult = HdmiInStatusChanged();
			}
		}
	}
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
	else if (!strcmp(pszDevicePath, UDEV_DEVICE_PATH_HDMI_OUT))
	{
		if (!strcmp(pszAction, UDEV_ACTION_CHANGE))
		{
			hResult = HdmiOutCapabilitiesChanged ();

			if (stiIS_ERROR (hResult))
			{
				stiASSERTMSG (estiFALSE, "hResult = 0x%0x\n", hResult);
			}
		}
	}
	else if (!strcmp (pszDevicePath, UDEV_DEVICE_PATH_VI))
	{
		if (!strcmp(pszAction, UDEV_ACTION_CHANGE))
		{
			usbRcuResetSignal.Emit ();
		}
	}
	//
	// mfddrv creates a udev device /devices/virtual/input/inputnn.
	// where each invocation increments nn.  if something else someday
	// starts to make virtual input devices we'll have to check further
	//
	else if (pszDeviceName && !strcmp(pszDeviceName, "\"icd_uinput\""))
	{
		bool bStatusChanged = false;

		stiDEBUG_TOOL(g_stiMonitorDebug,
			stiTrace("Managing MFDDRV\n");
		);

		if (!strcmp(pszAction, UDEV_ACTION_ADD))
		{
			if (m_bMfddrvdRunning)
			{
				stiTrace("UDEV add virtual input device while mfddrvd is running\n");
			}
			else
			{
				if (RcuApiInit () > -1)
				{
					RcuLensTypeSet ();
					m_bMfddrvdRunning = true;
					bStatusChanged = true;
				}
			}
		}
		else if (!strcmp(pszAction, UDEV_ACTION_REMOVE))
		{
			if (!m_bMfddrvdRunning)
			{
				stiTrace("UDEV remove virtual input device while mfddrv is not running\n");
			}
			else
			{
				RcuApiUninit ();
				m_bMfddrvdRunning = false;
				bStatusChanged = true;
			}
		}
		else
		{
			stiTrace("UDEV virtual input device unknown action: %s\n", pszAction);
		}
		if (bStatusChanged)
		{
			mfddrvdStatusChangedSignal.Emit ();
		}
	}
	else if (!strncmp(pszDevicePath, UDEV_DEVICE_PATH_HEADSET, sizeof(UDEV_DEVICE_PATH_HEADSET) - 1))
	{
		if (!strcmp(pszAction, UDEV_ACTION_CHANGE))
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("Headset changed\n");
			);

			auto state = readState (UDEV_DEVICE_PATH_HEADSET_STATE);

			bool connected = state == "1";
			if (connected != m_headphone)
			{
				m_headphone = connected;
				vpe::trace ("emitting headphone connected signal: ", m_headphone, "\n");
				headphoneConnectedSignal.Emit(m_headphone);
				microphoneConnectedSignal.Emit(m_headphone);
			}
		}
	}
	else if (!strcmp(pszDevicePath, UDEV_DEVICE_PATH_TEMP))
	{
		if (!strcmp(pszAction, UDEV_ACTION_CHANGE))
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("Temperature changed\n");
			);

			std::ifstream file (TEMPERATURE_FILE, std::ifstream::in);

			if (file.is_open ())
			{
				std::string temperature;

				std::getline (file, temperature);

				stiRemoteLogEventSend ("EventType=Temperature  Temperature=%s", temperature.c_str ());
			}
		}
	}
	else
	{
		stiTrace("UDEV event from other device: %s\n", pszDevicePath);
	}

	if (pUdevDevice)
	{
		udev_device_unref (pUdevDevice);
		pUdevDevice = nullptr;
	}
}


//\brief Called when HDMI in changes
///
stiHResult CstiMonitorTask::HdmiInStatusChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bStatusChanged = false;
	bool bResolutionChanged = false;

	hResult = HdmiInStatusUpdate (&bStatusChanged);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiInStatusChanged: m_nHdmiInStatus = %d\n", m_nHdmiInStatus);
	);

	// When checking the stauts of the Resolution it may be out of date because we only
	// read it when the HdmiInStatus changes in the kernel. To fix this we need notify the platfrom
	// layer with a different message than the one used to notify us that the HdmiInStatus changed.

	hResult = HdmiInResolutionUpdate (&bResolutionChanged);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiInStatusChanged: m_nHdmiInResolution = %d\n", m_nHdmiInResolution);
	);

	if (bResolutionChanged && (m_nHdmiInResolution == stiHDMI_RESOLUTION_720 || m_nHdmiInResolution == stiHDMI_RESOLUTION_1080))
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::HdmiInStatusChanged: Signaling HDMI Input resolution changed m_nHdmiInResolution = %d\n", m_nHdmiInResolution);
		);

		hdmiInResolutionChangedSignal.Emit (m_nHdmiInResolution);
	}

	if (!m_bHdmiFixed)
	{
		stiDEBUG_TOOL( g_stiMonitorDebug,
			stiTrace("CstiMonitorTask: hardware not fixed, checking resolution\n");
		);

		if (bStatusChanged == estiTRUE || bResolutionChanged == estiTRUE)
		{
			if (m_nHdmiInStatus)
			{
				if (m_nHdmiInResolution == stiHDMI_RESOLUTION_720 || m_nHdmiInResolution == stiHDMI_RESOLUTION_1080)
				{
					stiDEBUG_TOOL (g_stiMonitorDebug,
						stiTrace ("CstiMonitorTask::HdmiInStatusChanged: Calling back platform with estiMSG_HDMI_IN_STATUS_CHANGED m_nHdmiInStatus = %d\n", m_nHdmiInStatus);
					);

					// only notify that the hdmi input is available if the resolution is supported
					hdmiInStatusChangedSignal.Emit (m_nHdmiInStatus);
				}
				else
				{
					stiDEBUG_TOOL(g_stiMonitorDebug,
						stiTrace ("CstiMonitorTask::HdmiInStatusChanged: Calling back platform with estiMSG_HDMI_IN_STATUS_CHANGED because unsupported resolution\n");
					)
					m_nHdmiInStatus = false;
					hdmiInStatusChangedSignal.Emit (m_nHdmiInStatus);
				}
			}
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiMonitorDebug,
			stiTrace("CstiMonitorTask: Hardware fix detected checking resolution, statusChanged = %s\n", bStatusChanged ? "true" : "false");
		)
		if (bStatusChanged && (!m_nHdmiInStatus || m_nHdmiInResolution == stiHDMI_RESOLUTION_720 || m_nHdmiInResolution == stiHDMI_RESOLUTION_1080))
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("CstiMonitorTask: sending STATUSCHANGED = %s\n", m_nHdmiInStatus ? "true" : "false");
		)
			hdmiInStatusChangedSignal.Emit (m_nHdmiInStatus);
		}
		else if (bResolutionChanged && m_nHdmiInResolution != stiHDMI_RESOLUTION_720 && m_nHdmiInResolution != stiHDMI_RESOLUTION_1080)
		{
			stiDEBUG_TOOL(g_stiMonitorDebug,
				stiTrace("sending HDMI_IN_STATUS_CHANGED (off), because of unknown resolution\n");
			)
			m_nHdmiInStatus = false;
			hdmiInStatusChangedSignal.Emit (m_nHdmiInStatus);
		}
		else
		{
			// if you plugged in the hdmi cable, the status will change, but the resolution might not be supported
			// so tell it that we aren't in.  so when the resolution changes to something we support we will also
			// send the status change.
			m_nHdmiInStatus = 0;
		}
	}

STI_BAIL:

	return hResult;
}

//\brief Called when HDMI in changes
//
// \note This is only called within the EventLoop, so no manual locking
///
stiHResult CstiMonitorTask::HdmiInStatusUpdate (
	bool *pbStatusChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char szHdmiInStatus [10];
	int nFd = -1;
	int nReadLength = 0;
	int nCurrentStatus = m_nHdmiInStatus;

	std::stringstream ss;
	ss << "/sys" << UDEV_DEVICE_PATH_HDMI_IN << "/video_present";

	nFd = open (ss.str().c_str(), O_RDONLY | O_CLOEXEC);

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiInStatusUpdate: nFd = %d\n", nFd);
	);
	if (nFd >= 0)
	{
		nReadLength = stiOSRead (nFd, szHdmiInStatus, sizeof(szHdmiInStatus));

		close (nFd);
		nFd = -1;

		stiTESTCOND (nReadLength > 0, stiRESULT_ERROR);

		szHdmiInStatus[nReadLength] = 0;

		stiDEBUG_TOOL(g_stiMonitorDebug,
			stiTrace("CstiMonitorTask::HdmiInStatusUpdate: string read = %s\n", szHdmiInStatus);
		);
		m_nHdmiInStatus = atoi (szHdmiInStatus);
	}
	else
	{
		m_nHdmiInStatus = false;
	}

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiInStatusUpdate: m_nHdmiInStatus = %d, nCurrentStatus = %d\n", m_nHdmiInStatus,nCurrentStatus);
	);

	if (pbStatusChanged)
	{
		if (m_nHdmiInStatus != nCurrentStatus)
		{
			*pbStatusChanged = true;
		}
		else
		{
			*pbStatusChanged = false;
		}
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::HdmiInStatusUpdate: *pbStatusChanged = %s\n", *pbStatusChanged ? "true" : "false");
		);
	}

STI_BAIL:

	return hResult;
}

//\brief Called when HDMI in resolution changes
///
stiHResult CstiMonitorTask::HdmiInResolutionUpdate (
	bool *pbResolutionChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	char szHdmiInResolution [10];
	int nFd = -1;
	int nReadLength = 0;
	int nCurrentResolution = m_nHdmiInResolution;

	std::stringstream ss;
	ss << "/sys" << UDEV_DEVICE_PATH_HDMI_IN << "/video_res";

	nFd = open (ss.str().c_str(), O_RDONLY | O_CLOEXEC);

	if (nFd >= 0)
	{
		nReadLength = stiOSRead (nFd, szHdmiInResolution, sizeof(szHdmiInResolution));

		close (nFd);
		nFd = -1;

		stiTESTCOND (nReadLength > 0, stiRESULT_ERROR);

		szHdmiInResolution[nReadLength] = 0;

		m_nHdmiInResolution = atoi (szHdmiInResolution);
	}
	else
	{
		m_nHdmiInResolution = 0;
	}

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiInResolutionUpdate: m_nHdmiInResolution = %d\n", m_nHdmiInResolution);
	);

	if (pbResolutionChanged)
	{
		if (m_nHdmiInResolution != nCurrentResolution)
		{
			*pbResolutionChanged = true;
		}
		else
		{
			*pbResolutionChanged = false;
		}
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiMonitorTask::HdmiInStatusGet (
	int *pnHdmiInStatus)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	//
	// only say our status is plugged in if we also support the resolution
	//
	if ((m_nHdmiInResolution == stiHDMI_RESOLUTION_720 || m_nHdmiInResolution == stiHDMI_RESOLUTION_1080) && m_nHdmiInStatus)
	{
		*pnHdmiInStatus = 1;
	}
	else
	{
		*pnHdmiInStatus = 0;
	}

	Unlock ();

//STI_BAIL:

	return hResult;
}

stiHResult CstiMonitorTask::HdmiInResolutionGet (
	int *pnHdmiInResolution)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	*pnHdmiInResolution = m_nHdmiInResolution;

	Unlock ();

	return hResult;
}

stiHResult CstiMonitorTask::RcuLensTypeGet (
	int *lensType)

{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	*lensType = m_rcuLensType;

	return hResult;
}

stiHResult CstiMonitorTask::RcuConnectedStatusGet (
	bool *pbRcuConnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	*pbRcuConnected = m_bRcuConnected;

	Unlock ();

	return hResult;
}

stiHResult CstiMonitorTask::MfddrvdRunningGet (
	bool *pbMfddrvdRunning)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	*pbMfddrvdRunning = m_bMfddrvdRunning;

	Unlock();

	return (hResult);
}

stiHResult CstiMonitorTask::HdmiOutCapabilitiesGet (
	bool *pbConnected,
	uint32_t *pun32HdmiOutCapabilitiesBitMask,
	float * pf1080pRate,
	float * pf720pRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (pbConnected)
	{
		*pbConnected = m_bHdmiOutConnected;
	}

	if (pun32HdmiOutCapabilitiesBitMask)
	{
		*pun32HdmiOutCapabilitiesBitMask = m_un32HdmiOutCapabilitiesBitMask;
	}

	if (pf1080pRate)
	{
		*pf1080pRate = m_fHdmiOut1080pRate;
	}

	if (pf720pRate)
	{
		*pf720pRate = m_fHdmiOut720pRate;
	}

	Unlock ();

	return hResult;

}

stiHResult CstiMonitorTask::HdmiOutDisplayRateGet (
	EDisplayModes eDisplayMode,
	float * pfRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if (eDISPLAY_MODE_HDMI_1920_BY_1080 == eDisplayMode)
	{

		*pfRate = m_fHdmiOut1080pRate;

	}
	else if (eDISPLAY_MODE_HDMI_1280_BY_720 == eDisplayMode)
	{
		*pfRate = m_fHdmiOut720pRate;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiMonitorTask::HdmiOutDisplayRateGet: Invalid setting DisplayMode = %d\n", eDisplayMode);
	}

STI_BAIL:

	Unlock ();

	return (hResult);
}

// TODO:  Can we call this via PostEvent instead of directly?
stiHResult CstiMonitorTask::HdmiOutDisplayModeSet (
	EDisplayModes eDisplayMode,
	float fRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int nSystemStatus = 0;

	char szSystemCommand[256];

	Lock ();

	stiTESTCOND (eDisplayMode & m_un32HdmiOutCapabilitiesBitMask, stiRESULT_ERROR);

	// The setting is among the capabilities
	if (eDISPLAY_MODE_HDMI_1920_BY_1080 == eDisplayMode)
	{
		if (fRate > 0)
		{
			m_fHdmiOut1080pRate = fRate;
		}

		stiTESTCOND (m_fHdmiOut1080pRate >= 30.0, stiRESULT_ERROR);

		stiDEBUG_TOOL (g_stiDisplayDebug,
			stiTrace ("CstiMonitorTask::HdmiOutDisplayModeSet: setting 1080p @ %0.1f\n", m_fHdmiOut1080pRate);
		);

		sprintf (szSystemCommand, "xrandr --verbose --output HDMI-1 --mode 1920x1080 --rate %0.1f\n", m_fHdmiOut1080pRate);

		nSystemStatus = system (szSystemCommand);
		stiTESTCONDMSG (nSystemStatus == 0, stiRESULT_ERROR, "System error from xrandr: szSystemCommand = %s, nSystemStatus = %d\n", szSystemCommand, nSystemStatus);
	}
	else if (eDISPLAY_MODE_HDMI_1280_BY_720 == eDisplayMode)
	{
		if (fRate > 0)
		{
			m_fHdmiOut720pRate = fRate;
		}

		stiTESTCOND (m_fHdmiOut720pRate >= 30.0, stiRESULT_ERROR);

		stiDEBUG_TOOL (g_stiDisplayDebug,
			stiTrace ("CstiMonitorTask::HdmiOutDisplayModeSet: setting 720p @ %0.1f\n", m_fHdmiOut720pRate);
		);

		sprintf (szSystemCommand, "xrandr --verbose --output HDMI-1 --mode 1280x720 --rate %0.1f\n", m_fHdmiOut720pRate);

		nSystemStatus = system (szSystemCommand);
		stiTESTCONDMSG (nSystemStatus == 0, stiRESULT_ERROR, "System error from xrandr: szSystemCommand = %s, nSystemStatus = %d\n", szSystemCommand, nSystemStatus);
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR, "CstiMonitorTask::HdmiOutDisplayModeSet: Invalid setting DisplayMode = %d\n", eDisplayMode);
	}

STI_BAIL:

	Unlock ();

	return (hResult);
}


/* \brief Check the state of the hdmi hot plug pin
 *
 * this is seperate from the hdmi state, because we can be hot plugged
 * but still not have hdmi connectivity.
 *
 * \return none
 */
void CstiMonitorTask::HdmiOutHotPlugCheck ()
{
	auto plugState = readState ("/sys/devices/virtual/switch/hdmi/plug_state");

	bool bDetected = plugState == "1";
	if (m_bHotPlugDetected != bDetected)
	{
		m_bHotPlugDetected = bDetected;
		hdmiHotPlugStateChangedSignal.Emit ();
	}
}

/* \brief get the status of the hdmi hot plug pin
 *
 * \return true if the hot plug pin is high, else false
 */
bool CstiMonitorTask::HdmiHotPlugGet()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return(m_bHotPlugDetected);
}


static bool HDMIOutConnected ()
{
	auto state = readState ("/sys/devices/virtual/switch/hdmi/state");

	return (state == "1");
}


stiHResult CstiMonitorTask::HdmiOutCapabilitiesChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bCapabilitiesChanged = false;

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiOutCapabilitiesChanged\n");
	);

	HdmiOutHotPlugCheck();

	hResult = HdmiOutCapabilitiesUpdate (&bCapabilitiesChanged);
	stiTESTRESULT ();

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiOutCapabilitiesChanged: bCapabilitiesChanged = %d\n", bCapabilitiesChanged);
	);

	if (bCapabilitiesChanged)
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::HdmiOutCapabilitiesChanged: emitting hdmiOutStatusChangedSignal\n");
		);

		hdmiOutStatusChangedSignal.Emit ();
	}

STI_BAIL:

	return hResult;
}

stiHResult HeightWidthBestFrameRateParse (
	char * pBuffer,
	int * pnWidth,
	int * pnHeight,
	float * pfRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int nTokens = 0;
	char pToken[256];
	char * pCurrent = pBuffer;
	int nHeight = 0;
	int nWidth = 0;
	float fBestRate = 0.0;
	int nLineSize = 0;
	char *pEndOfLine = nullptr;
	char * pHz = nullptr;

	stiTESTCOND (pBuffer, stiRESULT_ERROR);

	nLineSize = strlen(pBuffer);

	pEndOfLine = &pBuffer[nLineSize];

	// If the line has "Hz" in it we want to ignore the whole line
	pHz = strstr (pBuffer, "Hz");

	if (pHz == nullptr)
	{
		// Are we at the end?
		if (*pCurrent == '\0')
		{
			stiASSERTMSG (estiFALSE, "HeightWidthBestFrameRateParse: problem with line = %s", pBuffer);
		}

		// Skip past any white space
		pCurrent = SkipWhiteSpace (pCurrent);

		// Are we at the end?
		if (*pCurrent == '\0')
		{
			stiASSERTMSG (estiFALSE, "HeightWidthBestFrameRateParse: problem with line = %s", pBuffer);
		}

		nTokens = sscanf (pCurrent, "%s", pToken);

		if (nTokens != 1)
		{
			stiASSERTMSG (estiFALSE, "HeightWidthBestFrameRateParse: problem with line = %s", pBuffer);
		}
		pCurrent += strlen(pToken) + 1;

		nTokens = sscanf (pToken, "%dx%d\n", &nWidth, &nHeight);

		if (nTokens != 2)
		{
			stiASSERTMSG (estiFALSE, "HeightWidthBestFrameRateParse: problem with line = %s", pBuffer);
		}

		for (;;)
		{
			float fRate = 0;

			// Skip any whitespace
			pCurrent = SkipWhiteSpace (pCurrent);

			// Get first frame rate
			nTokens = sscanf (pCurrent, "%s", pToken);

			if (nTokens != 1)
			{
				stiASSERTMSG (estiFALSE, "HeightWidthBestFrameRateParse: problem with line = %s", pBuffer);
			}
			pCurrent += strlen(pToken) + 1;

			nTokens = sscanf (pToken, "%f\n", &fRate);

			if (fRate > fBestRate)
			{
				fBestRate = fRate;
			}

			// skip the next two characters
			pCurrent++;
			pCurrent++;

			if (pCurrent >= pEndOfLine)
			{
				break;
			}
		}
	}

	if (pnWidth)
	{
		*pnWidth = nWidth;
	}

	if (pnHeight)
	{
		*pnHeight = nHeight;
	}

	if (pfRate)
	{
		*pfRate = fBestRate;
	}

STI_BAIL:

	return (hResult);

}

stiHResult CstiMonitorTask::HdmiOutCapabilitiesUpdate (
	bool *pbCapabilitiesChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t un32HdmiOutCapabilitiesBitMask = m_un32HdmiOutCapabilitiesBitMask;

	m_un32HdmiOutCapabilitiesBitMask = 0;

	if (HDMIOutConnected ())
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::HdmiModesGet: hdmi out connected\n");
		);

		m_bHdmiOutConnected = true;

		FILE *pFp = nullptr;
		char szBuff[256];

		if ((pFp = popen ("xrandr | awk 'begin{found = 0} {if(found == 1){ print } if($1 == \"HDMI-1\"){found = 1}}'", "r")) == nullptr)
		{
			stiTHROW (stiRESULT_ERROR);
		}

		m_un32HdmiOutCapabilitiesBitMask = 0;
		m_fHdmiOut1080pRate = 0;
		m_fHdmiOut720pRate = 0;

		while (fgets (szBuff, sizeof (szBuff), pFp))
		{
			float fFramerate = 0.0;
			int nWidth = 0;
			int nHeight = 0;

			stiDEBUG_TOOL (g_stiMonitorDebug,
				stiTrace ("CstiMonitorTask::HdmiModesGet: szBuff = %s", szBuff);
			);

			hResult = HeightWidthBestFrameRateParse (szBuff, &nWidth, &nHeight, &fFramerate);
			stiTESTRESULT ();

			stiDEBUG_TOOL (g_stiMonitorDebug,
				stiTrace ("CstiMonitorTask::HdmiModesGet: nWidth = %d, nHeight = %d, fFramerate = %0.1f\n\n", nWidth, nHeight, fFramerate);
			);

			if (fFramerate < HDMI_OUT_FRAME_RATE_MIN)
			{
				continue;
			}

			if (nWidth == 1920 && nHeight == 1080)
			{
				m_un32HdmiOutCapabilitiesBitMask |= eDISPLAY_MODE_HDMI_1920_BY_1080;
				m_fHdmiOut1080pRate = fFramerate;
				continue;
			}
			else if (nWidth == 1280 && nHeight == 720)
			{
				m_un32HdmiOutCapabilitiesBitMask |= eDISPLAY_MODE_HDMI_1280_BY_720;
				m_fHdmiOut720pRate = fFramerate;
				continue;
			}
			/*
			else if (nWidth == 720 && nHeight == 480)
			{
				m_un32HdmiOutCapabilitiesBitMask |= eDISPLAY_MODE_HDMI_720_BY_480;
				continue;
			}
			else if (nWidth == 640 && nHeight == 480)
			{
				m_un32HdmiOutCapabilitiesBitMask |= eDISPLAY_MODE_HDMI_640_BY_480;
				continue;
			}
			*/
		}

		pclose (pFp);
		pFp = nullptr;
	}
	else
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace ("CstiMonitorTask::HdmiModesGet: hdmi out not connected\n");
		);

		m_bHdmiOutConnected = false;
		m_un32HdmiOutCapabilitiesBitMask = 0;
		m_fHdmiOut1080pRate = 0.0;
		m_fHdmiOut720pRate = 0.0;

	}

	stiDEBUG_TOOL (g_stiMonitorDebug,
		stiTrace ("CstiMonitorTask::HdmiOutCapabilitiesUpdate: m_un32HdmiOutCapabilitiesBitMask: %x\n", m_un32HdmiOutCapabilitiesBitMask);
	);

	if (pbCapabilitiesChanged)
	{
		if (un32HdmiOutCapabilitiesBitMask != m_un32HdmiOutCapabilitiesBitMask)
		{
			*pbCapabilitiesChanged = true;
		}
		else
		{
			*pbCapabilitiesChanged = false;
		}
	}

STI_BAIL:

	return (hResult);

}


// Triggers when the m_bounceTimer timeout expires
void CstiMonitorTask::EventBounceTimerTimeout ()
{
	if (m_bInBounce)
	{
		HdmiInStatusChanged ();
		m_bInBounce = false;
	}
}


stiHResult CstiMonitorTask::RcuLensTypeSet ()

{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// here we talk to mfddrv daemon to get some rcu info
	//
	int nCameraControlFd = -1;
	int nResult = 0;
	std::string sLensType;
	char EpromReadBuffer[EPROM_BUFFER_LENGTH + 1];
	const int nByteNum = 0;
	size_t SizePacket;
	struct v4l2_control ctrl;
	std::size_t found;

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiMonitorTask::RcuLensTypeSet\n");
	);

	memset (EpromReadBuffer, '\0', sizeof(EpromReadBuffer));

	SizePacket = sizeof (EpromReadBuffer) - 1;

	nResult = RcuEepromRead (EpromReadBuffer, &SizePacket, LENS_TYPE_BLOCK, nByteNum);

	if (nResult != 0)
	{
		stiASSERTMSG (estiFALSE, "CstiMonitorTask::RcuLensTypeSet: RcuEepromRead failure: %d", nResult);
	}
	else
	{
		sLensType = EpromReadBuffer;

		//Trim(&sLensType);

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("CstiMonitorTask::RcuLensTypeSet: sLensType = \"%s\"\n", sLensType.c_str ());
		);
	}

	nCameraControlFd = open (g_szCameraDeviceName, O_RDWR);
	stiTESTCOND (nCameraControlFd > -1, stiRESULT_ERROR);

	ctrl.id = OV640_SET_LENS_TYPE;
	ctrl.value = OV640_LENS_TYPE_9513A_LARGAN;

	found = sLensType.find("9513A");

	if (found != std::string::npos)
	{
		ctrl.value = OV640_LENS_TYPE_9513A_LARGAN;
	}
	else
	{
		found = sLensType.find("DSL944");

		if (found != std::string::npos)
		{
			ctrl.value = OV640_LENS_TYPE_DSL944_SUNEX;
		}
		else
		{

			found = sLensType.find("T3117");

			if (found != std::string::npos)
			{
				ctrl.value = OV640_LENS_TYPE_T3117_DEMARREN;
			}
			else
			{

				found = sLensType.find("DHBO3684");

				if (found != std::string::npos)
				{
					ctrl.value = OV640_LENS_TYPE_DHBO3684_KINKO;
				}
				else
				{
					found = sLensType.find("CV0207A");

					if (found != std::string::npos)
					{
						ctrl.value = OV640_LENS_TYPE_CV0207A_DEMARREN;
					}
				}
			}
		}
	}

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CstiMonitorTask::RcuLensTypeSet: ctrl.value = %d\n", ctrl.value);
	);

	if (xioctl(nCameraControlFd, VIDIOC_S_CTRL, &ctrl) < 0)
	{
		stiTHROWMSG (stiRESULT_DRIVER_ERROR, "CstiMonitorTask::RcuLensTypeSet:: Error xoictl failed. ctrl.value = %d\n", ctrl.value);
	}

	m_rcuLensType = ctrl.value;
	
STI_BAIL:

	if (nCameraControlFd >= 0)
	{
		close (nCameraControlFd);
		nCameraControlFd = -1;
	}

	return (hResult);

}

stiHResult CstiMonitorTask::mpuSerialNotify (
	std::string mpuSerialNumber)
{
    stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	if (mpuSerialNumber.length () > 6  && mpuSerialNumber[6] >= 'D')
	{
		stiDEBUG_TOOL (g_stiMonitorDebug,
			stiTrace("CstiMonitorTask::mpuSerialNotify: Hardware has the HDMI fix\n");
		);

		m_bHdmiFixed = true;
	}

	Unlock();
  
	return (hResult);
}


bool CstiMonitorTask::headphoneConnectedGet()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_headphone;
}


bool CstiMonitorTask::microphoneConnectedGet ()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return m_headphone;
}
