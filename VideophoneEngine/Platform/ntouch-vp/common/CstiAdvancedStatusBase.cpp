// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2025 Sorenson Communications, Inc. -- All rights reserved

#include "CstiAdvancedStatusBase.h"

#include "stiTools.h"

#include <string>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h> 

// Function prototypes
static bool readFileValue (const std::string& path, char* buffer, size_t size);
static std::string execCommand (const std::string& command);
static std::string resolveIEEEProtocol (float frequency, int bandwidth, int dataRate);

template<typename T>
T readAndConvert (const std::string& path, T defaultValue = T())
{
    std::string buffer (100, '\0');
    if (readFileValue (path, &buffer[0], buffer.size ()))
	{
        buffer.erase (buffer.find_first_of ("\r\n"));
        try
		{
            if (std::is_same<T, int>::value)
			{
                return static_cast<T>(std::stoi (buffer));
			}
            else if (std::is_same<T, float>::value)
			{
                return static_cast<T>(std::stof (buffer));
			}
            else if (std::is_same<T, double>::value)
			{
                return static_cast<T>(std::stod (buffer));
			}
        }
		catch (...) {}
    }
    return defaultValue;
}

/*
 * do the initialization
 */
stiHResult CstiAdvancedStatusBase::initialize (
	CstiMonitorTask *monitorTask,
	CstiAudioHandler *audioHandler,
	CstiAudioInput *audioInput,
	CstiVideoInput *videoInput,
	CstiVideoOutput *videoOutput,
	CstiBluetooth *bluetooth)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (monitorTask != nullptr, stiRESULT_ERROR);
	stiTESTCOND (audioHandler != nullptr, stiRESULT_ERROR);
	stiTESTCOND (audioInput != nullptr, stiRESULT_ERROR);
	stiTESTCOND (videoInput != nullptr, stiRESULT_ERROR);
	stiTESTCOND (videoOutput != nullptr, stiRESULT_ERROR);
	stiTESTCOND (bluetooth != nullptr, stiRESULT_ERROR);
	
	m_monitorTask = monitorTask;
	m_audioHandler = audioHandler;
	m_audioInput = audioInput;
	m_videoInput = videoInput;
	m_bluetooth = bluetooth;
	m_videoOutput = videoOutput;

	hResult = mpuSerialSet ();
	stiTESTRESULT ();

	m_monitorTask->mpuSerialNotify (m_advancedStatus.mpuSerialNumber);

	staticStatusGet ();

STI_BAIL:

	return hResult;
}


/*!
 * \brief return the collected status
 * \arg- the destination for the status
 * \returns success
 */
stiHResult CstiAdvancedStatusBase::advancedStatusGet (
	SstiAdvancedStatus &advancedStatus)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	{
		std::lock_guard<std::recursive_mutex> lock (m_statusMutex);

		avAndSystemStatusUpdate ();
		wifiStatusUpdate ();
		dynamicStatusGet ();

		advancedStatus = m_advancedStatus;
	}

	return hResult;
}


std::string CstiAdvancedStatusBase::mpuSerialGet ()
{
	std::string sn;
	{
		std::lock_guard<std::recursive_mutex> lock (m_statusMutex);
		sn = m_advancedStatus.mpuSerialNumber;
	}
	return sn;
}


std::string CstiAdvancedStatusBase::rcuSerialGet ()
{
	std::string sn;
	{
		std::lock_guard<std::recursive_mutex> lock (m_statusMutex);
		sn = m_advancedStatus.rcuSerialNumber;
	}
	return sn;
}


void CstiAdvancedStatusBase::avAndSystemStatusUpdate ()
{
	m_advancedStatus.inputAudioLevel = m_audioInput->AudioLevelGet ();
	m_advancedStatus.focusPosition = m_videoInput->FocusPositionGet ();
	m_advancedStatus.brightness = m_videoInput->BrightnessGet ();
	m_advancedStatus.saturation = m_videoInput->SaturationGet ();

	m_advancedStatus.cpuTemp = readAndConvert<int>(
		"/sys/class/thermal/thermal_zone0/temp", 0) / 1000;

	m_advancedStatus.upTime = readAndConvert<int>("/proc/uptime", 0);

	std::string loadAvg (100, '\0');
	if (readFileValue ("/proc/loadavg", &loadAvg[0], loadAvg.size ()))
	{
		loadAvg.erase (loadAvg.find_first_of ("\r\n"));
		m_advancedStatus.loadAvg = loadAvg;
	}

	std::string ethernetName = execCommand (
		"ifconfig -a | grep -oE '^[a-zA-Z0-9]+' | grep -E '^e(n|th)[a-zA-Z0-9]*' | head -n1");
	if (!ethernetName.empty() && ethernetName.back() == '\n')
	{
		ethernetName.pop_back();
	}

	std::string ifconfigOutput = execCommand ("ifconfig " + ethernetName);

	m_advancedStatus.wiredConnected = (ifconfigOutput.find (" inet ") != std::string::npos);

	if (m_advancedStatus.wiredConnected)
	{
		m_advancedStatus.ethernetSpeed = readAndConvert<int>(
			"/sys/class/net/" + ethernetName + "/speed", 0);
	}
	else
	{
		m_advancedStatus.ethernetSpeed = 0;
	}

	std::list<BluetoothDevice> pairedList;
	m_bluetooth->PairedListGet (&pairedList);

	m_advancedStatus.bluetoothList.clear ();

	for (auto const &item : pairedList)
	{
		SstiBluetoothStatus pairedDevice;
		pairedDevice.Name = item.name;
		pairedDevice.bConnected = item.connected;
		m_advancedStatus.bluetoothList.push_back (pairedDevice);
	}

	std::string lsusbOutput = execCommand ("lsusb");
	std::istringstream iss (lsusbOutput);
	std::string line;
	m_advancedStatus.usbList.clear ();
	while (std::getline (iss, line))
	{
		if (line.find (" hub") != std::string::npos ||
			line.find ("Sorenson Vision, Inc.") != std::string::npos)
		{
			continue;
		}
		m_advancedStatus.usbList.emplace_back (line);
	}

	if (m_videoOutput->CECSupportedGet())
	{
		m_advancedStatus.cecTvVendor = m_videoOutput->tvVendorGet();
	}
	else
	{
		m_advancedStatus.cecTvVendor = "Unknown";
	}
}


void CstiAdvancedStatusBase::wifiStatusUpdate ()
{
	std::string interface = 
		execCommand ("iw dev | awk '$1==\"Interface\"{printf $2}'");
	stiASSERTMSG (!interface.empty (), "No wireless adapter was found!");

	std::string linkOutput {};
	if (!interface.empty ())
	{
		linkOutput = execCommand (std::string ("iw dev " + interface + " link"));

		stiDEBUG_TOOL (
			g_stiNetworkDebug,
			stiTrace ("iw dev %s link:\n\t%s\n",
				interface.c_str(), linkOutput.c_str());
		);
	}

	if (interface.empty () || linkOutput.find ("Not connected") != std::string::npos)
	{
		// set all information to zero, wireless IF down
		m_advancedStatus.wirelessConnected = false;
		m_advancedStatus.wirelessFrequency = 0.0;
		m_advancedStatus.wirelessDataRate = 0;
		m_advancedStatus.wirelessBandwidth = 0;
		m_advancedStatus.wirelessProtocol = "None";
		m_advancedStatus.wirelessSignalStrength = 0;
		m_advancedStatus.wirelessTransmitPower = 0;
		m_advancedStatus.wirelessChannel = 0;
		
		stiDEBUG_TOOL (
			g_stiNetworkDebug,
			stiTrace ("Wireless interface %s is not connected\n", interface.c_str());
		);
	}
	else
	{
		m_advancedStatus.wirelessConnected = true;

		std::istringstream issLink (linkOutput);
		std::string line;

		while (std::getline (issLink, line))
		{
			if (line.find ("freq:") != std::string::npos)
			{
				int freq = 0;
				std::istringstream lss (line.substr (line.find ("freq:") + 5));
				lss >> freq;
				// Return GHZ to one decimal place...
				freq /= 100;
				m_advancedStatus.wirelessFrequency = static_cast<float>(freq) / 10.0; // GHz
			}
			else if (line.find ("signal:") != std::string::npos)
			{
				int signal = 0;
				std::istringstream lss (line.substr (line.find ("signal:") + 7));
				lss >> signal;
				m_advancedStatus.wirelessSignalStrength = signal;
			}
			else if (line.find ("tx bitrate:") != std::string::npos)
			{
				double bitrate = 0.0;
				std::istringstream lss (line.substr (line.find ("tx bitrate:") + 11));
				lss >> bitrate;
				m_advancedStatus.wirelessDataRate = static_cast<int>(bitrate);
			}
		}

		std::string infoOutput =
			execCommand (std::string ("iw dev " + interface + " info"));

		stiDEBUG_TOOL (
			g_stiNetworkDebug,
			stiTrace ("iw dev %s info:\n\t%s\n",
				interface.c_str (), infoOutput.c_str ());
		);

		std::istringstream issInfo (infoOutput);

		while (std::getline (issInfo, line))
		{
			// Parse channel and width from the same line
			if (line.find ("channel") != std::string::npos &&
				line.find ("width:") != std::string::npos)
			{
				std::istringstream lss (line);
				std::string token;
				int channel = 0;
				int width = 0;
				// Look for "channel" and extract the next integer
				while (lss >> token)
				{
					if (token == "channel")
					{
						lss >> channel;
						m_advancedStatus.wirelessChannel = channel;
					}
					else if (token == "width:")
					{
						lss >> width;
						m_advancedStatus.wirelessBandwidth = width;
					}
				}
			}
			// Parse txpower (e.g., "txpower 20.00 dBm")
			else if (line.find ("txpower") != std::string::npos)
			{
				std::istringstream lss (line.substr (line.find ("txpower") + 7));
				double txpower = 0.0;
				lss >> txpower;
				m_advancedStatus.wirelessTransmitPower = static_cast<int>(txpower);
			}
		}

		m_advancedStatus.wirelessProtocol = resolveIEEEProtocol (
				m_advancedStatus.wirelessFrequency,
				m_advancedStatus.wirelessBandwidth,
				m_advancedStatus.wirelessDataRate);
	}
}

// Helper function to read a value from a file
static bool readFileValue (const std::string& path, char* buffer, size_t size)
{
    int fd = open (path.c_str (), O_RDONLY);

    if (fd < 0)
	{
		return false;
	}

    memset (buffer, 0, size);
    ssize_t n = stiOSRead (fd, buffer, size - 1);
    close (fd);

    return n > 0;
}

// Utility function to execute a system command and return the full output as a string
static std::string execCommand (const std::string& command)
{
    std::array<char, 256> buffer;
    std::string result;
	struct file_deleter
	{
		void operator()(FILE* d) const
		{
			pclose(d);
		}
	};

    // Open the command for reading
	std::unique_ptr<FILE, file_deleter> pipe (popen (command.c_str (), "r"));
	if (!pipe)
	{
        return result;
    }
    // Read the output a chunk at a time
	while (fgets (buffer.data (), buffer.size (), pipe.get ()) != nullptr)
	{
        result += buffer.data ();
    }

    return result;
}


/*!
 * \brief determine the wireless connection's IEEE 802.11 protocol
 *
 * https://en.wikipedia.org/wiki/IEEE_802.11
 * 
 * Protocol | Frequency | Bandwidth | Data Rate                     |
 *          |   (GHz)   |   (MHz)   |  (Mbit/s)                     |
 * -----------------------------------------------------------------
 * 802.11b  | 2.4       | 20        | 1, 2, 5.5, 11                 |
 * -----------------------------------------------------------------
 * 802.11g  | 2.4       | 5, 10, 20 | 6, 9, 12, 18, 24, 36, 48, 54  |
 * -----------------------------------------------------------------
 * 802.11n  | 2.4/5     | 20        | Up to 288.8                   |
 * 802.11n  | 2.4/5     | 40        | Up to 600                     |
 * -----------------------------------------------------------------
 * 802.11ac | 5         | 20        | Up to 346.8                   |
 * 802.11ac | 5         | 40        | Up to 800                     |
 * 802.11ac | 5         | 80        | Up to 1733.2                  |
 * 802.11ac | 5         | 160       | Up to 3466.8                  |
 * 
 */
static std::string resolveIEEEProtocol (
	float frequency,
	int bandwidth,
	int dataRate) 
{
	std::string protocol;

	if (frequency == float(5)) 
	{
		// both 802.11ac and 802.11n possible
		if (bandwidth > 40) 
		{
			protocol = "802.11ac (Wi-Fi 5)";
		} 
		else if (bandwidth == 40) 
		{
			// both 802.11ac and 802.11n possible
			if (dataRate > 600) 
			{
				protocol = "802.11ac (Wi-Fi 5)";
			} 
			else 
			{
				protocol = "802.11n (Wi-Fi 4)";
			}
		} 
		else 
		{
			// both 802.11ac and 802.11n possible
			if (dataRate > 289) 
			{
				protocol = "802.11ac (Wi-Fi 5)";
			} 
			else 
			{
				protocol = "802.11n (Wi-Fi 4)";
			}
		}
	} 
	else if (frequency == float(2.4)) 
	{
		// 802.11n, 802.11g, 802.11b possible
		if (bandwidth == 40) 
		{
			protocol = "802.11n (Wi-Fi 4)";
		} 
		else 
		{
			if (dataRate > 54) 
			{
				protocol = "802.11n (Wi-Fi 4)";
			} 
			else if (dataRate > 11) 
			{
				protocol = "802.11g";
			} 
			else 
			{
				protocol = "802.11b";
			}
		}
	} 
	else 
	{
		// non-wireless connection
		protocol = "Unknown";
	}
	
	return protocol;
}

