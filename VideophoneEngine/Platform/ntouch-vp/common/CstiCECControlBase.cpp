// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2017 Sorenson Communications, Inc. -- All rights reserved

#include "stiTrace.h"
#include "CstiCECControlBase.h"

#include <iostream>

namespace cec
{
	using std::cout;
	using std::endl;

#include <libcec/cecloader.h>
}

/*! \brief Constructor
 *
 * This is the default constructor for the CstiCECControlBase class.
 *
 * \return None
 */
CstiCECControlBase::CstiCECControlBase() : CstiEventQueue("CstiCECControl")
{
}

/*! \brief Initialize the task
 *
 *  \retval estiOK if success
 *  \retval estiERROR otherwise
 */
stiHResult CstiCECControlBase::Initialize(CstiMonitorTask *monitorTask, const std::string &device, const std::string &name)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_monitorTask = monitorTask;
	m_hdmiOutHotPlugSignalConnection = m_monitorTask->hdmiHotPlugStateChangedSignal.Connect (
		[this]() {
			PostEvent (
				std::bind(&CstiCECControlBase::eventHdmiHotPlugStatusChanged, this)
			);
		}
	);

	PostEvent ([this, device, name] () { cecConnect(device, name); });

	return(hResult);
}

/*! \brief Destructor
 *
 * This is the default destructor for the CstiMonitorTask class.
 *
 * \return None
 */
CstiCECControlBase::~CstiCECControlBase()
{
	CstiEventQueue::StopEventLoop();
	if (m_adapter)
	{
		m_adapter->Close();
		cec::UnloadLibCec(m_adapter);
		m_adapter = nullptr;
	}
}

/*
 * connect to the cec bus
 */
stiHResult CstiCECControlBase::cecConnect(const std::string &device, const std::string &name)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_cecConfig = sci::make_unique<CEC::libcec_configuration>();

	m_cecConfig->Clear();
	memset(m_cecConfig->strDeviceName, '\0', sizeof(m_cecConfig->strDeviceName));
	strncpy(m_cecConfig->strDeviceName, name.c_str(), sizeof(m_cecConfig->strDeviceName) - 1);

	m_cecConfig->clientVersion = CEC::LIBCEC_VERSION_CURRENT;
	m_cecConfig->bActivateSource = 0;
	m_cecConfig->deviceTypes.Add(CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE);

	m_cecConfig->callbacks = &m_callbacks;
	m_cecConfig->callbackParam = this;

	m_adapter = cec::LibCecInitialise(m_cecConfig.get());
	if (!m_adapter)
	{
		stiTHROWMSG(stiRESULT_ERROR, "CstiCECControlBase::cecConnect: Can't initialize libCEC\n");
	}

	// init video on targets that need this
	m_adapter->InitVideoStandalone();

	if (!m_adapter->Open(device.c_str()))
	{
		stiTHROWMSG(stiRESULT_ERROR, "CstiCECControlBase::cecConnect: Can't open the %s port on the CEC bus\n", device.c_str());
	}

	m_hdmiConnected = m_monitorTask->HdmiHotPlugGet();

	if (m_hdmiConnected)
	{
		if (m_logicalAddress == CEC::CECDEVICE_UNREGISTERED)
		{
			CEC::cec_logical_addresses logicalAddresses = m_adapter->GetLogicalAddresses();
			m_logicalAddress = logicalAddresses.primary;
			m_physicalAddress = m_adapter->GetDevicePhysicalAddress(m_logicalAddress);
		}

		powerCheck();
		m_activeSource = m_adapter->GetActiveSource();
		if (m_activeSource == CEC::CECDEVICE_UNREGISTERED || m_activeSource == CEC::CECDEVICE_UNKNOWN)
		{
			eventActiveSet();
		}
	}
	else
	{
		stiDEBUG_TOOL(g_stiCECControlDebug,
			stiTrace("not activating television because hdmi is not connected\n");
		);
	}

STI_BAIL:
	return(hResult);
}

/*! \brief Start the event loop
 *
 *  \retval success
 */
stiHResult CstiCECControlBase::Startup()
{
	CstiEventQueue::StartEventLoop();
	return(stiRESULT_SUCCESS);
}

/*! \brief schedule the event to turn on the tv
 *
 *  \retval  success
 */
stiHResult CstiCECControlBase::displayPowerSet(bool on)
{
	PostEvent(std::bind(&CstiCECControlBase::eventDisplayPowerSet, this, on));
	return(stiRESULT_SUCCESS);
}

/*! \brief event to turn on the tv
 *
 *  \return success unless we couldn't transmit the cec command
 */
stiHResult CstiCECControlBase::eventDisplayPowerSet(bool on)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CEC::cec_command command;
	CEC::cec_opcode opCode = CEC::CEC_OPCODE_NONE;

	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("CstiCECControlBase::eventDisplayPowerSet enter, on = %s, current = %s\n", torf(on), torf(m_tvPowered));
	);

	if (on && !m_tvPowered)
	{
		opCode = CEC::CEC_OPCODE_IMAGE_VIEW_ON;
	}
	else if (!on && m_tvPowered)
	{
		opCode = CEC::CEC_OPCODE_STANDBY;
	}

	if (opCode != CEC::CEC_OPCODE_NONE)
	{
		command.Format(command, m_logicalAddress, CEC::CECDEVICE_TV, opCode);
		if (!m_adapter->Transmit(command))
		{
			stiTHROWMSG(stiRESULT_ERROR, "CstiCECControlBase::eventDisplayPowerSet: Failed to transmit command %s\n", torf(on));
		}
	}

STI_BAIL:
	return(hResult);
}

/* \brief push a uint16_t onto the argument vector for a command
 *
 * \return none
 */
void CstiCECControlBase::uint16_tPush(CEC::cec_command &command, uint16_t &val)
{
	auto ptr = reinterpret_cast<char *>(&val);
	command.parameters.PushBack(ptr[1]);
	command.parameters.PushBack(ptr[0]);
}

/* \brief schedule the event to set ourself as the active source
 *
 * \return success
 */
stiHResult CstiCECControlBase::activeSet()
{
	PostEvent (std::bind(&CstiCECControlBase::eventActiveSet, this));
	return(stiRESULT_SUCCESS);
}


/* \brief event to set ourself as the active source
 *
 * \return success unless we couldn't transmit a cec command
 */
stiHResult CstiCECControlBase::eventActiveSet()
{

	stiHResult hResult = stiRESULT_SUCCESS;

	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("CstiCECControlBase::eventActiveSet - we are currently %sactive, power = %s\n",
					m_activeSource == m_logicalAddress ? "" : "not ",
					m_tvPowered ? "on" : "off");
	);

	CEC::cec_command command;

	if (m_tvPowered)
	{
		command.Clear();
		command.Format(command, m_logicalAddress, CEC::CECDEVICE_BROADCAST, CEC::CEC_OPCODE_ACTIVE_SOURCE);
		uint16_tPush(command, m_physicalAddress);
		if (!m_adapter->Transmit(command))
		{
			stiTHROWMSG(stiRESULT_ERROR, "CstiCECControlBase::eventActiveSet: Failed to transmit ACTIVE_SOURCE\n");
		}

		command.Clear();
		command.Format(command, m_logicalAddress, CEC::CECDEVICE_TV, CEC::CEC_OPCODE_MENU_STATUS);
		command.parameters.PushBack((char)CEC::CEC_MENU_STATE_ACTIVATED);
		if (!m_adapter->Transmit(command))
		{
			stiASSERTMSG(estiFALSE, "Can't transmit menu_state_activated\n");
		}
	}

	m_activeSource = m_logicalAddress;

	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("CstiCECControlBase::eventActiveSet - exit\n");
	);

STI_BAIL:
	return(hResult);
}

/* \brief return if the connected tv supports cec commands
 *
 * \return true if it does, else false
 */
bool CstiCECControlBase::cecSupported() const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return(m_cecSupported);
}

/* \brief return if the connected tv is powered on or onot
 *
 * \return true if it is, else false
 */
bool CstiCECControlBase::tvPowered() const
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	return(m_tvPowered);
}

std::string CstiCECControlBase::tvVendorGet()
{
	std::lock_guard<std::recursive_mutex> lock (m_execMutex);
	std::string vendorName;

	vendorName = m_adapter->ToString(m_vendorId);
	return vendorName;
}

/* \brief event handler for when the hdmi hot plug status changes
 * 
 * \returns none
 */
void CstiCECControlBase::eventHdmiHotPlugStatusChanged()
{
	auto hdmiConnected = m_monitorTask->HdmiHotPlugGet();
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("Enter eventHdmiHotPlugStatusChanged m_hdmiConnected = %s, hdmiConnected = %s\n",
						torf(m_hdmiConnected), torf(hdmiConnected));
	);

	if (hdmiConnected != m_hdmiConnected)
	{
		m_hdmiConnected = hdmiConnected;
		stiDEBUG_TOOL(g_stiCECControlDebug,
			stiTrace("Changing connection ");
		);
		if (!m_hdmiConnected)
		{
			stiDEBUG_TOOL(g_stiCECControlDebug,
				stiTrace("to not connected\n");
			);
			m_tvPowered = false;
			m_cecSupported = false;
		}
		else
		{
			stiDEBUG_TOOL(g_stiCECControlDebug,
				stiTrace("to connected\n");
			);
			CEC::cec_command command;
			command.Clear();
			command.Format(command, m_logicalAddress, CEC::CECDEVICE_BROADCAST, CEC::CEC_OPCODE_REPORT_PHYSICAL_ADDRESS);
			uint16_tPush(command, m_physicalAddress);
			command.parameters.PushBack((char)CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
			if (m_adapter->Transmit(command))
			{
				powerCheck();
			}
			else
			{
				stiASSERTMSG(estiFALSE, "Can't transmit our physical address\n");
			}
		}
	}
}

/* \brief check to see if the television supports cec commands, and if it is 'on'
 *
 * \return none
 */
void CstiCECControlBase::powerCheck()
{

	bool cecSupported = false;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	CEC::cec_power_status powerStatus = m_adapter->GetDevicePowerStatus(CEC::CECDEVICE_TV);
	switch(powerStatus)
	{
		case CEC::CEC_POWER_STATUS_ON:
		case CEC::CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON:
			m_tvPowered = true;
			cecSupported = true;
			m_hdmiConnected = true;
		break;

		case CEC::CEC_POWER_STATUS_STANDBY:
		case CEC::CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY:
			m_tvPowered = false;
			cecSupported = true;
			m_hdmiConnected = true;
			break;

		case CEC::CEC_POWER_STATUS_UNKNOWN:
			m_tvPowered = false;
			cecSupported = false;
			break;
	}
	if (cecSupported != m_cecSupported)
	{
		m_cecSupported = cecSupported;
		cecSupportedSignal.Emit(m_cecSupported);
	}
}

/* \brief callback when a device on the cec bus asserts ActiveSource
 *
 * \return none
 */
void CstiCECControlBase::cecSourceActivated(void *ptr, const CEC::cec_logical_address addr, const uint8_t state)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got CecSourceActivated callback\n");
	);
	auto self = static_cast<CstiCECControlBase *>(ptr);
	self->PostEvent(std::bind(&CstiCECControlBase::eventCECSourceActivated, self, addr, state));
}

/* \brief event to actually handle the source activated callback
 *
 * \return none
 */
void CstiCECControlBase::eventCECSourceActivated(const CEC::cec_logical_address &addr, const uint8_t &state)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("enter eventSourceActivated with logical address %d\n", addr);
	);

	CEC::cec_command command;
	CEC::cec_opcode opcode {CEC::CEC_OPCODE_NONE};
	CEC::cec_menu_state menuState {CEC::CEC_MENU_STATE_ACTIVATED};

	if (m_activeSource != m_logicalAddress && addr == m_logicalAddress)
	{
		opcode = CEC::CEC_OPCODE_MENU_STATUS;
	}
	else if (m_activeSource == m_logicalAddress && addr != m_logicalAddress)
	{
		opcode = CEC::CEC_OPCODE_MENU_STATUS;
		menuState = CEC::CEC_MENU_STATE_DEACTIVATED;
	}

	m_activeSource = addr;		// could be us, could be someone else
	if (addr == CEC::CECDEVICE_TV)
	{
		m_cecSupported = true;
		m_tvPowered = true;
	}

	if (m_tvPowered && opcode != CEC::CEC_OPCODE_NONE)
	{
		command.Clear();
		command.Format(command, m_logicalAddress, CEC::CECDEVICE_TV, opcode);
		command.parameters.PushBack((char)menuState);
		m_adapter->Transmit(command);
	}
}

/* \brief callback for when a menustate has chanded (currently not handled)
 *
 * \return true (handled successfully)
 */
int CstiCECControlBase::cecMenuStateChanged(void *self, const CEC::cec_menu_state item)
{
	
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got menu callback %d\n", item);
	);
	return(1);
}

/* \brief callback when we get an alert.  (not sure what this is)
 *
 * \return true
 */
int CstiCECControlBase::cecAlert(void *self, const CEC::libcec_alert alert, const CEC::libcec_parameter p)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got an alert callback %d, %d", alert, p.paramType);

		// if the type isn't string, it is unknown, so we ignore it if it's not a string
		if (p.paramType == CEC::CEC_PARAMETER_TYPE_STRING)
		{
			stiTrace(", \"%s\"", reinterpret_cast<char *>(p.paramData));
		}
		stiTrace("\n");
	);
	return(1);
}

/* \brief callback for when our configuration changed.
 *
 * this happens when we move the hdmi connector and we get a new physical address
 *
 * \return true
 */
int CstiCECControlBase::cecConfigurationChanged(void *ptr, const CEC::libcec_configuration cecConfig)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got CecConfigurationChanged callback\n");
	);
	auto self = static_cast<CstiCECControlBase *>(ptr);
	self->PostEvent(std::bind(&CstiCECControlBase::eventCECConfigurationChanged, self, cecConfig));
	return(1);
}

/* \brief
 * the event that actually handles the configuration changed callback
 */
void CstiCECControlBase::eventCECConfigurationChanged(const CEC::libcec_configuration &cecConfig)
{

	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("enter eventCecConfigurationChanged callback\n");
	);
	m_logicalAddress = cecConfig.logicalAddresses.primary;
	m_cecVersion = static_cast<uint8_t>(cecConfig.cecVersion);
	m_physicalAddress = m_adapter->GetDevicePhysicalAddress(m_logicalAddress);

	CEC::cec_command command;
	command.Clear();
	command.Format(command, m_logicalAddress, CEC::CECDEVICE_BROADCAST, CEC::CEC_OPCODE_REPORT_PHYSICAL_ADDRESS);
	uint16_tPush(command,m_physicalAddress);
	command.parameters.PushBack((char)CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE);
	if (m_adapter->Transmit(command))
	{
		*m_cecConfig = cecConfig;
	}
	else
	{
		stiASSERTMSG(estiFALSE, "Can't transmit address change\n");
	}
}

/* \brief callback for us to handle various commands on the cec bus
 *
 * \return true
 */
int CstiCECControlBase::cecCommand(void *ptr, const CEC::cec_command command)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got cecCommand callback\n");
	);
	auto self = static_cast<CstiCECControlBase *>(ptr);
	self->PostEvent(std::bind(&CstiCECControlBase::eventCECCommand, self, command));
	return(1);
}

/* \brief
 * the event that actually handles the cec command callback
 */
void CstiCECControlBase::eventCECCommand(const CEC::cec_command &command)
{
	CEC::cec_command newCommand;

	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("CstiCECControlBase::eventCECCommand got command %1.1x%1.1x:%2.2x", command.initiator, command.destination, command.opcode);
		if (command.parameters.size)
		{
			for (int i = 0; i < command.parameters.size; i++)
			{
				stiTrace(":%2.2x", command.parameters.data[i]);
			}
		}
		stiTrace("\n");
	);

	if (!m_adapter)		// make sure we don't handle a command before the Open is complete
	{
		stiDEBUG_TOOL(g_stiCECControlDebug,
			stiTrace("have no adapter yet\n");
		);
	}
	else
	{
		if (command.destination != CEC::CECDEVICE_BROADCAST && command.destination != m_logicalAddress)
		{
			stiDEBUG_TOOL(g_stiCECControlDebug,
				stiTrace("command wasn't to us (%d)\n", command.destination);
			);
		}
		else
		{
			if (!m_hdmiConnected)
			{
				m_hdmiConnected = true;
				if (command.initiator == CEC::CECDEVICE_TV)
				{
					m_cecSupported = true;
				}
			}
	
			/*
	 		* let's think about which command codes we will support
	 		*/
			switch(command.opcode)
			{
				case CEC::CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
					newCommand.Clear();
					newCommand.Format(newCommand, m_logicalAddress, command.initiator, CEC::CEC_OPCODE_REPORT_POWER_STATUS);
					newCommand.parameters.PushBack(CEC::CEC_POWER_STATUS_ON);
					if (!m_adapter->Transmit(newCommand))
					{
						stiASSERTMSG(estiFALSE, "failed to transmit power status\n");
					}
					break;
				case CEC::CEC_OPCODE_GET_CEC_VERSION:
					newCommand.Clear();
					newCommand.Format(newCommand, m_logicalAddress, command.initiator, CEC::CEC_OPCODE_CEC_VERSION);
					newCommand.parameters.PushBack(m_cecVersion);
					if (!m_adapter->Transmit(newCommand))
					{
						stiASSERTMSG(estiFALSE, "failed to transmit CEC_VERSION\n");
					}
					break;
				case CEC::CEC_OPCODE_STANDBY:
					if (command.initiator == CEC::CECDEVICE_TV)
					{
						m_tvPowered = false;
					}
					break;
				case CEC::CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
					if (command.initiator == CEC::CECDEVICE_TV)
					{
						m_tvPowered = true;
					}
					if (m_activeSource == m_logicalAddress)
					{
						newCommand.Clear();
						newCommand.Format(newCommand, m_logicalAddress, command.initiator, CEC::CEC_OPCODE_ACTIVE_SOURCE);
						uint16_tPush(newCommand, m_physicalAddress);
						if (!m_adapter->Transmit(newCommand))
						{
							stiASSERTMSG(estiFALSE, "failed to transmit ACTIVE_SOURCE\n");
						}
					}
					break;
				case CEC::CEC_OPCODE_ACTIVE_SOURCE:
				case CEC::CEC_OPCODE_IMAGE_VIEW_ON:
					m_activeSource = command.initiator;
					if (command.initiator == CEC::CECDEVICE_TV)
					{
						m_tvPowered = true;
					}
					break;
				case CEC::CEC_OPCODE_REPORT_POWER_STATUS:
					if (command.initiator == CEC::CECDEVICE_TV)
					{
						if (command.parameters.data[0] == CEC::CEC_POWER_STATUS_ON ||
							command.parameters.data[0] == CEC::CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON)
						{
							m_tvPowered = true;
							if (m_activeSource == m_logicalAddress)
							{
								eventActiveSet();
							}
						}
						else
						{
							m_tvPowered = false;
						}
					}
					break;
				case CEC::CEC_OPCODE_DEVICE_VENDOR_ID:
					if (command.initiator == CEC::CECDEVICE_TV)
					{
						uint32_t tmp = command.parameters.data[2] | (command.parameters.data[1] << 8) | (command.parameters.data[0] << 16);
						m_vendorId = static_cast<CEC::cec_vendor_id>(tmp);
					}
					break;
				case CEC::CEC_OPCODE_GIVE_DECK_STATUS:
					newCommand.Clear();
					newCommand.Format(newCommand, m_logicalAddress, command.initiator, CEC::CEC_OPCODE_DECK_STATUS);
					newCommand.parameters.PushBack(CEC::CEC_DECK_INFO_OTHER_STATUS);
					if (!m_adapter->Transmit(newCommand))
					{
						stiASSERTMSG(estiFALSE, "failed to transmit DECK_STATUS\n");
					}
					break;
				case CEC::CEC_OPCODE_ROUTING_CHANGE:
					{
					uint16_t originalAddress{};
					uint16_t newAddress{};
					CEC::cec_opcode opcode {CEC::CEC_OPCODE_NONE};

					originalAddress = command.parameters.data[1] | (command.parameters.data[0] << 8);
					newAddress = command.parameters.data[3] | (command.parameters.data[2] << 8);

					if (originalAddress == m_physicalAddress && newAddress != m_physicalAddress)
					{
						opcode = CEC::CEC_OPCODE_INACTIVE_SOURCE;
					}
					else if (originalAddress != m_physicalAddress && newAddress == m_physicalAddress)
					{
						opcode = CEC::CEC_OPCODE_ACTIVE_SOURCE;
					}

					if (opcode != CEC::CEC_OPCODE_NONE)
					{
						newCommand.Clear();
						newCommand.Format(newCommand, m_logicalAddress, CEC::CECDEVICE_BROADCAST, opcode);
						uint16_tPush(newCommand, m_physicalAddress);
						if (!m_adapter->Transmit(newCommand))
						{
							stiASSERTMSG(estiFALSE, "failed to set proper status with ROUTING_CHANGE\n");
						}
						else
						{
							if (opcode == CEC::CEC_OPCODE_ACTIVE_SOURCE)
							{
								m_activeSource = m_logicalAddress;
							}
							else
							{
								m_activeSource = CEC::CECDEVICE_UNKNOWN;
							}
						}
					}
					}
					break;
				case CEC::CEC_OPCODE_SET_MENU_LANGUAGE:
					if (command.initiator == CEC::CECDEVICE_TV) {
						if (!m_tvPowered)
						{
							m_tvPowered = true;
							if (m_activeSource == m_logicalAddress)
							{
								eventActiveSet();
							}
						}
					}
					break;
				
				/*
				 * currently unhandled commands
				 */
				case CEC::CEC_OPCODE_TEXT_VIEW_ON:
				case CEC::CEC_OPCODE_INACTIVE_SOURCE:
				case CEC::CEC_OPCODE_ROUTING_INFORMATION:
				case CEC::CEC_OPCODE_SET_STREAM_PATH:
				case CEC::CEC_OPCODE_RECORD_OFF:
				case CEC::CEC_OPCODE_RECORD_ON:
				case CEC::CEC_OPCODE_RECORD_STATUS:
				case CEC::CEC_OPCODE_RECORD_TV_SCREEN:
				case CEC::CEC_OPCODE_CLEAR_ANALOGUE_TIMER:
				case CEC::CEC_OPCODE_CLEAR_DIGITAL_TIMER:
				case CEC::CEC_OPCODE_CLEAR_EXTERNAL_TIMER:
				case CEC::CEC_OPCODE_SET_ANALOGUE_TIMER:
				case CEC::CEC_OPCODE_SET_DIGITAL_TIMER:
				case CEC::CEC_OPCODE_SET_EXTERNAL_TIMER:
				case CEC::CEC_OPCODE_SET_TIMER_PROGRAM_TITLE:
				case CEC::CEC_OPCODE_TIMER_CLEARED_STATUS:
				case CEC::CEC_OPCODE_TIMER_STATUS:
				case CEC::CEC_OPCODE_CEC_VERSION:
				case CEC::CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
				case CEC::CEC_OPCODE_GET_MENU_LANGUAGE:
				case CEC::CEC_OPCODE_REPORT_PHYSICAL_ADDRESS:
				case CEC::CEC_OPCODE_DECK_CONTROL:
				case CEC::CEC_OPCODE_DECK_STATUS:
				case CEC::CEC_OPCODE_PLAY:
				case CEC::CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS:
				case CEC::CEC_OPCODE_SELECT_ANALOGUE_SERVICE:
				case CEC::CEC_OPCODE_SELECT_DIGITAL_SERVICE:
				case CEC::CEC_OPCODE_TUNER_DEVICE_STATUS:
				case CEC::CEC_OPCODE_TUNER_STEP_DECREMENT:
				case CEC::CEC_OPCODE_TUNER_STEP_INCREMENT:
				case CEC::CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
				case CEC::CEC_OPCODE_VENDOR_COMMAND:
				case CEC::CEC_OPCODE_VENDOR_COMMAND_WITH_ID:
				case CEC::CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN:
				case CEC::CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP:
				case CEC::CEC_OPCODE_SET_OSD_STRING:
				case CEC::CEC_OPCODE_GIVE_OSD_NAME:
				case CEC::CEC_OPCODE_SET_OSD_NAME:
				case CEC::CEC_OPCODE_MENU_REQUEST:
				case CEC::CEC_OPCODE_MENU_STATUS:
				case CEC::CEC_OPCODE_USER_CONTROL_PRESSED:
				case CEC::CEC_OPCODE_USER_CONTROL_RELEASE:
				case CEC::CEC_OPCODE_FEATURE_ABORT:
				case CEC::CEC_OPCODE_ABORT:
				case CEC::CEC_OPCODE_GIVE_AUDIO_STATUS:
				case CEC::CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
				case CEC::CEC_OPCODE_REPORT_AUDIO_STATUS:
				case CEC::CEC_OPCODE_SET_SYSTEM_AUDIO_MODE:
				case CEC::CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
				case CEC::CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:
				case CEC::CEC_OPCODE_SET_AUDIO_RATE:
				case CEC::CEC_OPCODE_START_ARC:
				case CEC::CEC_OPCODE_REPORT_ARC_STARTED:
				case CEC::CEC_OPCODE_REPORT_ARC_ENDED:
				case CEC::CEC_OPCODE_REQUEST_ARC_START:
				case CEC::CEC_OPCODE_REQUEST_ARC_END:
				case CEC::CEC_OPCODE_END_ARC:
				case CEC::CEC_OPCODE_CDC:
#if DEVICE == DEV_NTOUCH_VP4
				case CEC::CEC_OPCODE_REPORT_SHORT_AUDIO_DESCRIPTORS:
				case CEC::CEC_OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTORS:
#endif
				case CEC::CEC_OPCODE_NONE:
					break;
			}
		}
	}
}

/* \brief callback to handle keypress events.
 *
 * we will receive these when we are the active source (stub for when we will want to handle them)
 *
 * \return true
 */
int CstiCECControlBase::cecKeyPress(void *ptr, const CEC::cec_keypress key)
{
	stiDEBUG_TOOL(g_stiCECControlDebug,
		stiTrace("got keypress callback code - %d duration- %d\n", key.keycode, key.duration);
	);
	
	auto self = static_cast<CstiCECControlBase *>(ptr);
	self->cecKeyPressSignal.Emit(key);
	return(1);
}

/* \brief callback for logging and debugging
 *
 * \return true
 */
int CstiCECControlBase::cecLogMessage(void *self, const CEC::cec_log_message message)
{
	stiDEBUG_TOOL( (int)g_stiCECControlDebug == message.level,
		stiTrace("got log message callback - %s, level %d, time %lld\n", message.message, message.level, message.time);
	);
	return(1);
}
