// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#include "BluezAlsaClient.h"

#include "IstiPlatform.h"
#include "CstiEventQueue.h"
#include "stiTrace.h"
#include "CstiBluetooth.h"

#include <sstream>

namespace
{

enum class EventIndicator
{
	CALL = 1,
	CALLSETUP = 2,
};

/*! \brief generate the indicator command to avoid dulpicating strings
 */
std::string indicatorEventCommandGenerate(EventIndicator indicator, int value)
{
	std::stringstream ss;

	ss << "\r\n+CIEV: " << static_cast<int>(indicator) << "," << value << "\r\n";

	return ss.str();
}

std::string RingIndicate("\r\nRING\r\n");

}

namespace vpe
{

/*!\brief Constructor for BluezAlsaClient
 *
 * \param interface interface string, for example, hci0
 */
BluezAlsaClient::BluezAlsaClient (const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface, const std::list<ba_pcm_type> &supportedTypes)
:
	m_bluetoothEventQueue(queue),
	m_supportedTypes(supportedTypes)
{
	m_clientFd = bluealsa_open(interface.c_str());

	// Listen to signal to set headset state when connecting headset during call
	m_signalConnections.push_back (deviceAddedSignal.Connect([this](BluetoothAudioDevice)
		{
			// If in a call
			if(-1 != m_activeCallIndex)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace("BluezAlsaClient: device connected while in a call\n");
				);

				PlatformCallStateChange callStateChange;

				// Craft a state update and send it through the update mechanism
				callStateChange.callIndex = m_activeCallIndex;
				callStateChange.newState = esmiCS_CONNECTED;
				callStateChange.newSubStates = estiSUBSTATE_CONFERENCING;

				callStateUpdate(callStateChange);
			}

		}));
}


/*!\brief initialize bluealsa client
 *
 * Have separate function to allow connecting to signals after object instantiation
 */
void BluezAlsaClient::initialize ()
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s\n", __FUNCTION__);
	);

	// Broadcast current devices
	changesBroadcast();

	enum ba_event subscription = static_cast<enum ba_event>(BA_EVENT_TRANSPORT_ADDED | BA_EVENT_TRANSPORT_REMOVED);

	int returnCode = bluealsa_subscribe(m_clientFd, subscription);
	stiASSERTMSG(returnCode > -1, "BluezAlsaClient:%s: unable to subscribe to bluealsa events: %d\n", __FUNCTION__, returnCode);

	if (!m_bluetoothEventQueue->FileDescriptorAttach(m_clientFd, [this]()
		{
			eventsProcess ();
			changesBroadcast();
		}))
	{
		stiASSERTMSG (estiFALSE, "%s: Can't attach clientFd %d\n", __func__, m_clientFd);
	}
}

BluezAlsaClient::~BluezAlsaClient ()
{
	close(m_clientFd);
}

/*!\brief call this after the file descriptor gets an event to process
 * all pending events
 */
int BluezAlsaClient::eventsProcess ()
{
	int ret = -1;
	struct ba_msg_event event;
	int num_events = 0;

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s: processing events\n", __FUNCTION__);
	);

	while (true)
	{
		// Possible results:
		//
		// * sizeof(event)
		// * -1 and EINTR (should try again)
		// * -1 and EAGAIN or EWOULDBLOCK (no more messages)
		// Non blocking
		ret = recv(m_clientFd, &event, sizeof(event), MSG_DONTWAIT);

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("BluezAlsaClient:%s: recv return: %d\n", __FUNCTION__, ret);
		);

		if (-1 == ret)
		{
			if(EINTR == errno)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace("BluezAlsaClient:%s: will try to read again\n", __FUNCTION__);
				);
				continue;
			}
			else if(EAGAIN == errno || EWOULDBLOCK == errno)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace("BluezAlsaClient:%s: no more messages\n", __FUNCTION__);
				);
				break;
			}
		}
		else if(ret == sizeof(event))
		{
			++num_events;

			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace("BluezAlsaClient:%s: event: 0x%x\n", __FUNCTION__, static_cast<uint8_t>(event.mask));
			);
		}
		else if (ret == 0)		// zero indicates that the peer has performed an orderly shutdown (man recv)
		{
			break;
		}
	}

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s: processed events: %d\n", __FUNCTION__, num_events);
	);

	return num_events;
}


std::vector<BluetoothAudioDevice> BluezAlsaClient::devicesGet ()
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s\n", __FUNCTION__);
	);

	std::vector<BluetoothAudioDevice> ret;
	struct ba_msg_transport *transports;

	int num_transports = bluealsa_get_transports(m_clientFd, &transports);

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s: number of transports: %d\n", __FUNCTION__, num_transports);
	);

	for(int i = 0; num_transports > -1 && i < num_transports; ++i)
	{
		BluetoothAudioDevice device;

		// TODO: any other fields that need to be set?
		device.m_connected = true;
		device.m_address.resize(MAC_ADDR_LENGTH); // don't need to include null char...
		ba2str(&transports[i].addr, &device.m_address[0]);

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("BluezAlsaClient:%s: addr: %s, type: %d, stream: %d, codec: %d, channels: %d, sampling: %d\n",
				__FUNCTION__,
				device.m_address.c_str(),
				transports[i].type,
				transports[i].stream,
				transports[i].codec,
				transports[i].channels,
				transports[i].sampling
				);
		);

		// TODO: it may be that this information isn't set
		// may need to just key off of address?
		auto it = std::find(m_supportedTypes.begin(), m_supportedTypes.end(), transports[i].type);
		if (it != m_supportedTypes.end())
		{
			if (BA_PCM_TYPE_A2DP == transports[i].type)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace("BluezAlsaClient:%s: audio device type: a2dp\n", __FUNCTION__);
				);

				device.m_transportType = transportType::typeA2dp;
			}
			else if(BA_PCM_TYPE_SCO == transports[i].type)
			{
				stiDEBUG_TOOL(g_stiBluetoothDebug,
					stiTrace("BluezAlsaClient:%s: audio device type: sco\n", __FUNCTION__);
				);

				device.m_transportType = transportType::typeSco;
			}

			auto deviceIter = std::find_if (ret.begin (), ret.end (),
							[device](BluetoothAudioDevice check)
								{return device.m_address == check.m_address;});


			// we want to have only one entry per device address
			if (deviceIter != ret.end())
			{
				// SCO takes precedence on type and sample rate.
				if ((*deviceIter).m_transportType == transportType::typeA2dp && device.m_transportType == transportType::typeSco)
				{
					(*deviceIter).m_transportType = transportType::typeSco;
					device.m_sampleRate = transports[i].sampling;
				}
			}
			else
			{
				device.m_sampleRate = transports[i].sampling;
				ret.push_back(device);
			}
		}
	}

	return ret;
}


void BluezAlsaClient::changesBroadcast ()
{
	// TODO: would be nice to get these events from bluealsa instead of tracking this state

	auto devices = devicesGet();

	// the event from bluealsa would have to get done after the
	// transports were set up (in their implementation)

	// Cases:

	// 1. device removed
	// NOTE: broadcast these first, so that added comes after
	for(auto it = m_currentDevices.begin(); it != m_currentDevices.end() ;)
	{
		bool found = false;

		for(const auto &device : devices)
		{
			if(it->first == device.m_address)
			{
				found = true;
				break;
			}
		}

		if(!found)
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace("BluezAlsaClient:%s: removing device: %s\n", __FUNCTION__, it->first.c_str());
			);

			// Mark as disconnected
			it->second.m_connected = false;
			deviceRemovedSignal.Emit(it->second);
			it = m_currentDevices.erase(it);
		}
		else
		{
			++it;
		}
	}
	// 2. device added
	for(const auto &device : devices)
	{
		if(m_currentDevices.count(device.m_address) == 0 ||
			(m_currentDevices[device.m_address].m_transportType == transportType::typeA2dp &&
				device.m_transportType == transportType::typeSco))
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace("BluezAlsaClient:%s: adding new device: %s\n", __FUNCTION__, device.m_address.c_str());
			);

			// Be sure to add device to map before emitting the signal
			// (our own signal handler refers to the map)
			m_currentDevices[device.m_address] = device;
			deviceAddedSignal.Emit(device);
		}
	}

	// other cases?

}

void BluezAlsaClient::rfcommSendAll (const std::string &command)
{
	for(const auto &keyValue: m_currentDevices)
	{
		// NOTE: this list is of connected devices, so shouldn't need
		// to check if connected or not

		stiDEBUG_TOOL(g_stiBluetoothDebug,
			stiTrace("BluezAlsaClient:%s: sending command to %s: %s\n", __FUNCTION__, keyValue.first.c_str (), command.c_str () );
		);

		bdaddr_t address;
		str2ba(keyValue.first.c_str (), &address);
		bluealsa_send_rfcomm_command (m_clientFd, address, command.c_str());
	}

}

void  BluezAlsaClient::ringIndicateSend ()
{
	rfcommSendAll(RingIndicate);
}

void BluezAlsaClient::callStateUpdate (const PlatformCallStateChange &callStateChange)
{
	// NOTE: just want to log that we got here
	// There's other debug modules that print out a descriptive call state change
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("BluezAlsaClient:%s: got call state change: %d -> %d, active call index: %d\n", __FUNCTION__, callStateChange.prevState, callStateChange.newState, m_activeCallIndex );
	);

	// Bluetooth indicators documented here: https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=292287

	// Corner cases:
	// * local has active and held call, remote hangs up, but active call is still alive
	// * phone with headset gets transfered to another phone
	// * incoming call while on a call (or hold?)
	// * about reconnecting a headset in the middle of a call (Probably need to hang on to the last call state?)
	// * probably more...

	// TODO:
	// * this could use a more robust state machine
	// * handle callheld indicator states?
	// * use enums for indicator values?
	// * query headset state?
	// * incorporate this into the bluez-alsa client api?

	switch(callStateChange.newState)
	{
	case esmiCS_CONNECTING:
	{
		// Only update headset states if not in a call
		if(-1 == m_activeCallIndex)
		{
			if(callStateChange.newSubStates & estiSUBSTATE_CALLING)
			{
				rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALLSETUP, 2));
			}
			// NOTE: I assuming that this would be SUBSTATE_ANSWERING... never got that state?
			else if(callStateChange.newSubStates & estiSUBSTATE_WAITING_FOR_USER_RESP)
			{
				rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALLSETUP, 1));
			}
			else if(callStateChange.newSubStates & estiSUBSTATE_WAITING_FOR_REMOTE_RESP)
			{
				rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALLSETUP, 3));
			}
		}
		break;
	}
	case esmiCS_CONNECTED:
	{
		if(callStateChange.newSubStates & estiSUBSTATE_CONFERENCING)
		{
			// Does this order matter?
			rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALL, 1));
			rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALLSETUP, 0));

			m_activeCallIndex = callStateChange.callIndex;
		}
		break;
	}
	case esmiCS_DISCONNECTED:
	{
		// Only process disconnected for current call
		if(m_activeCallIndex == callStateChange.callIndex)
		{
			rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALL, 0));
			m_activeCallIndex = -1;
		}
		break;
	}
	case esmiCS_DISCONNECTING:
	{
		if(-1 == m_activeCallIndex)
		{
			// If we cancel an outgoing call, be sure to exit callsetup
			if(esmiCS_CONNECTING == callStateChange.prevState)
			{
				rfcommSendAll(indicatorEventCommandGenerate(EventIndicator::CALLSETUP, 0));
			}
		}
	}

	// TODO: do these need to be handled?
	case esmiCS_UNKNOWN:
	case esmiCS_IDLE:
	case esmiCS_HOLD_LCL:
	case esmiCS_HOLD_RMT:
	case esmiCS_HOLD_BOTH:
	case esmiCS_CRITICAL_ERROR:
	case esmiCS_INIT_TRANSFER:
	case esmiCS_TRANSFERRING:
		break;
	}

}

} // namespace

