// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "PulseDevice.h"
#include "stiTrace.h"
#include <openssl/aes.h>
#include <sstream>

PulseDevice::PulseDevice (std::unique_ptr<BleIODevice> ioDevice, std::function<void(BleDevice*)>callback, CstiEventQueue *eventQueue)
:
BleDevice (std::move (ioDevice), eventQueue),
m_firmwareVerifyCallback (callback)
{
}

PulseDevice::~PulseDevice ()
{
}


void PulseDevice::didAddService (const std::string &serviceUuid)
{
	if (serviceUuid == PULSE_SERVICE)
	{
		m_receivedPulseService = true;
	}
	else if (serviceUuid == PULSE_DFU_SERVICE)
	{
		m_receivedDfuService = true;
	}
	
	if (deviceReady ())
	{
		characteristicRead (PULSE_SERVICE, PULSE_AUTHENTICATE_CHARACTERISTIC);
	}
}


void PulseDevice::didAddCharacteristic (const std::string &serviceUuid, const std::string &characUuid)
{
	if (serviceUuid == PULSE_SERVICE)
	{
		if (characUuid == PULSE_WRITE_CHARACTERISTIC)
		{
			m_receivedPulseWriteChar = true;
		}
		else if (characUuid == PULSE_READ_CHARACTERISTIC)
		{
			m_receivedPulseReadChar = true;
			characteristicStartNotifySet (PULSE_SERVICE, PULSE_READ_CHARACTERISTIC, true);
		}
		else if (characUuid == PULSE_AUTHENTICATE_CHARACTERISTIC)
		{
			m_receivedPulseAuthChar = true;
			characteristicStartNotifySet (PULSE_SERVICE, PULSE_AUTHENTICATE_CHARACTERISTIC, true);
		}
	}
	else if (serviceUuid == PULSE_DFU_SERVICE)
	{
		if (characUuid == PULSE_DFU_BUTTONLESS_CHARACTERISTIC)
		{
			m_receivedDfuButtonlessChar = true;
			characteristicStartNotifySet (PULSE_DFU_SERVICE, PULSE_DFU_BUTTONLESS_CHARACTERISTIC, true);
		}
	}
	
	if (deviceReady ())
	{
		characteristicRead (PULSE_SERVICE, PULSE_AUTHENTICATE_CHARACTERISTIC);
	}
}


void PulseDevice::didUpdateStartNotify (const std::string &serviceUuid, const std::string &characUuid, bool enabled)
{
	if (serviceUuid == PULSE_SERVICE)
	{
		if (characUuid == PULSE_READ_CHARACTERISTIC)
		{
			m_notifyingPulseReadChar = enabled;
		}
		else if (characUuid == PULSE_AUTHENTICATE_CHARACTERISTIC)
		{
			m_notifyingPulseAuthChar = enabled;
		}
	}
	else if (serviceUuid == PULSE_DFU_SERVICE)
	{
		if (characUuid == PULSE_DFU_BUTTONLESS_CHARACTERISTIC)
		{
			m_notifyingDfuButtonlessChar = enabled;
		}
	}
	
	if (deviceReady ())
	{
		characteristicRead (PULSE_SERVICE, PULSE_AUTHENTICATE_CHARACTERISTIC);
	}
}


void PulseDevice::didChangeCharacteristicValue (
	const std::string &serviceUuid,
	const std::string &characUuid,
	const std::vector<uint8_t> &value)
{
	if (serviceUuid == PULSE_SERVICE)
	{
		if (PULSE_AUTHENTICATE_CHARACTERISTIC == characUuid)
		{
			authCharacValueChangedEvent (value);
		}
		else if (PULSE_READ_CHARACTERISTIC == characUuid)
		{
			readCharacValueChangedEvent (value);
		}
	}
	else if (serviceUuid == PULSE_DFU_SERVICE &&
			 characUuid == PULSE_DFU_BUTTONLESS_CHARACTERISTIC)
	{
		dfuCharacValueChangedEvent (value);
	}
}

void PulseDevice::reset ()
{
	m_receivedPulseService = false;
	m_receivedPulseWriteChar = false;
	m_receivedPulseReadChar = false;
	m_receivedPulseAuthChar = false;
	m_receivedDfuService = false;
	m_receivedDfuButtonlessChar = false;
	m_notifyingPulseReadChar = false;
	m_notifyingPulseAuthChar = false;
	m_notifyingDfuButtonlessChar = false;
	m_authenticated = false;
}

void PulseDevice::presetRingPatternStart (
										int pattern,
										uint8_t red,
										uint8_t green,
										uint8_t blue,
										uint8_t brightness)
{
	char buffer[PULSE_MAX_COMMAND_LENGTH];
	sprintf (buffer, "%d %2x %2x %2x %2x", pattern, red, green, blue, brightness);
	std::string arguments (buffer);
	
	commandSend (PULSE_PRESET_PATTERN_START_COMMAND, arguments);
}

void PulseDevice::customRingPatternStart (
										const std::vector<std::string> &frames)
{
	char buffer[PULSE_MAX_COMMAND_LENGTH];
	sprintf (buffer, "%lu", frames.size ());
	std::string arguments (buffer);
	
	commandSend (PULSE_CUSTOM_PATTERN_START_COMMAND, arguments);
	
	for (auto &frame : frames)
	{
		commandSend (PULSE_CUSTOM_PATTERN_FRAME_COMMAND, frame);
	}
}

void PulseDevice::ringStop ()
{
	std::string arguments;
	commandSend (PULSE_PATTERN_STOP_COMMAND, arguments);
}

void PulseDevice::missedSet (bool value)
{
	std::string arguments;
	std::string command = value ? PULSE_MISSED_ON_COMMAND : PULSE_MISSED_OFF_COMMAND;
	
	commandSend (command, arguments);
}

void PulseDevice::signmailSet (bool value)
{
	std::string arguments;
	std::string command = value ? PULSE_SIGNMAIL_ON_COMMAND : PULSE_SIGNMAIL_OFF_COMMAND;
	
	commandSend (command, arguments);
}

void PulseDevice::allOff ()
{
	std::string arguments;
	commandSend (PULSE_ALL_OFF, arguments);
}

void PulseDevice::commandSend (
							 const std::string &command,
							 const std::string &arguments)
{
	std::ostringstream commandString;
	
	commandString << command << " " << m_writeSequenceNumber++;
	
	if (!arguments.empty ())
	{
		commandString << " " << arguments;
	}
	
	std::string stringCommand = commandString.str();
	std::vector<uint8_t> commandToSend (stringCommand.begin (), stringCommand.end ());
	
	characteristicWrite (PULSE_SERVICE, PULSE_WRITE_CHARACTERISTIC, commandToSend);
}

bool PulseDevice::deviceReady ()
{
	return  m_receivedPulseService &&
			m_receivedPulseWriteChar &&
			m_receivedPulseReadChar &&
			m_receivedPulseAuthChar &&
			m_receivedDfuService &&
			m_receivedDfuButtonlessChar &&
			m_notifyingPulseReadChar &&
			m_notifyingPulseAuthChar &&
			m_notifyingDfuButtonlessChar;
}

void PulseDevice::infoGet ()
{
	std::string arguments;
	commandSend (PULSE_INFO_GET_COMMAND, arguments);
}

void PulseDevice::verifyFirmware ()
{
	if (m_firmwareVerifyCallback)
	{
		m_firmwareVerifyCallback (this);
	}
}

void PulseDevice::restartInBootloader (const std::string &name)
{
	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("Start DFU\n");
	);
	
	std::string bootloaderName (name);
	
	if (bootloaderName.size() > 20)
	{
		bootloaderName.resize (20);
	}
	
	std::vector<uint8_t> commandToSend = {0x02}; // Set name
	commandToSend.push_back (bootloaderName.size ()); // Length of name
	std::copy (bootloaderName.begin(), bootloaderName.end(), std::back_inserter (commandToSend)); // Name
	
	characteristicWrite (PULSE_DFU_SERVICE, PULSE_DFU_BUTTONLESS_CHARACTERISTIC, commandToSend);
}

void PulseDevice::readCharacValueChangedEvent (const std::vector<uint8_t> &value)
{
	std::string receivedResponse (value.begin (), value.end ());
	std::istringstream isstream (receivedResponse);
	std::vector<std::string> tokens;
	std::copy (std::istream_iterator<std::string>(isstream),
			   std::istream_iterator<std::string>(),
			   std::back_inserter (tokens));
	
	if (tokens[0] == PULSE_INFO_RESPONSE_COMMAND)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Received Pulse device info\n");
		);

		if (tokens.size () >= 4)
		{
			m_version = tokens[2];
			m_serialNumber = tokens[3];
			
			verifyFirmware ();
		}
	}
	else if (tokens[0] == "RESPONSE")
	{
		// Currently doing nothing...
	}
	else if (tokens[0] == "ALERT_SWITCH_HIGH")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s alert switch set to high.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "ALERT_SWITCH_MEDIUM")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s alert switch set to medium.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "ALERT_SWITCH_LOW")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s alert switch set to low.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "ALERT_SWITCH_OFF")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s alert switch set to off.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "RGB_SWITCH_HIGH")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s RGB switch set to high.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "RGB_SWITCH_MEDIUM")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s RGB switch set to medium.\n", m_serialNumber.c_str ());
		);
	}
	else if (tokens[0] == "RGB_SWITCH_LOW")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
					  stiTrace ("Pulse %s RGB switch set to low.\n", m_serialNumber.c_str ());
					  );
	}
	else if (tokens[0] == "RGB_SWITCH_OFF")
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Pulse %s RGB switch set to off.\n", m_serialNumber.c_str ());
		);
	}
	else
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Received unhandled message from Pulse %s.\n", m_serialNumber.c_str ());
		);
	}
}

void PulseDevice::authCharacValueChangedEvent (const std::vector<uint8_t> &value)
{
	const size_t nonceSize = 16;
	
	if (value == AUTHORIZED)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Received AUTHORIZED from Pulse device\n");
		);

		m_authenticated = true;
		
		infoGet ();
	}
	else if (value.size () == nonceSize)
	{
		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace ("Received nonce from Pulse device\n");
		);

		// This must be the nonce. Calculate and send response.
		std::vector<uint8_t> response;
		response.resize (nonceSize);
		AES_KEY aesKey;
		AES_set_encrypt_key ((const unsigned char*)AUTHENTICATE_KEY.data (), 128, &aesKey);
		AES_ecb_encrypt (value.data (), response.data (), &aesKey, AES_ENCRYPT);
		
		characteristicWrite (PULSE_SERVICE, PULSE_AUTHENTICATE_CHARACTERISTIC, response);
	}
	else
	{
		stiTrace ("Unexpected value from authentication characteristic\n");
	}
}

void PulseDevice::dfuCharacValueChangedEvent (const std::vector<uint8_t> &value)
{
	if (value.size () != 3)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("Invalid respone from DFU characteristic value change\n");
		);
	}
	else if (value[0] == 0x20) // Response
	{
		if (value[1] == 0x02 && // Set name command was sent
			value[2] == 0x01)	// and it was successful
		{
			// Restart the remove device in DFU mode
			std::vector<uint8_t> commandToSend = {0x01};
			characteristicWrite (PULSE_DFU_SERVICE, PULSE_DFU_BUTTONLESS_CHARACTERISTIC, commandToSend);
		}
	}
}

