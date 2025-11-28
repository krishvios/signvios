// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#include "BluetoothAudioBase.h"

#include "stiTrace.h"
#include "CstiEventQueue.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <sstream>

namespace vpe
{

BluetoothAudioBase::BluetoothAudioBase(const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface)
:
	m_interfaceName(interface),
	m_eventQueue(queue),
	m_ringTimer(6000, queue.get())
{
	m_signalConnections.push_back (m_ringTimer.timeoutSignal.Connect ([this]
		{
			eventHeadsetsRing();
		}));

}


/*!\brief emit signal to broadcast current device
 */
void BluetoothAudioBase::currentDeviceBroadcast ()
{
	m_eventQueue->PostEvent([this]()
		{
			stiDEBUG_TOOL(g_stiBluetoothDebug,
				stiTrace("Broadcasting current bluetooth device: connected: %d, address: %s\n", m_currentDevice.m_connected, m_currentDevice.m_address.c_str());
				);

			deviceUpdatedSignal.Emit(m_currentDevice);
		});
}

stiHResult BluetoothAudioBase::HCIAudioEnable ()
{
	return stiRESULT_SUCCESS;
}

/*!\brief get the device string for a bluetooth audio device, based on bluez-alsa
 */
std::string BluetoothAudioBase::BluetoothAudioDeviceGet (const BluetoothAudioDevice &device) const
{
	std::string ret = "INVALID_BLUETOOTH_DEVICE"; // Just to help catch errors
	std::string profile;

	// Prioritize headset, because some devices report supporting both
	if(device.m_transportType == transportType::typeSco)
	{
		profile = "sco";
	}
	else if(device.m_transportType == transportType::typeA2dp)
	{
		profile = "a2dp";
	}

	if(!profile.empty())
	{
		std::stringstream ss;

		ss << "bluealsa:HCI="
		   << m_interfaceName
		   << ",DEV="
		   << device.m_address
		   << ",PROFILE="
		   << profile
			;

		ret = ss.str();
	}

	return ret;
}


/*!\brief compare bluetooth devices (old and new) to see if device
 * should be broadcast as an update
 */
bool BluetoothAudioBase::isNewDevice (const BluetoothAudioDevice &newDevice)
{
	bool ret = false;

	// Need to handle 3 cases: connect with first device, changed device, disconnected
	if(m_currentDevice.m_connected != newDevice.m_connected
		|| m_currentDevice.m_address != newDevice.m_address)
	{
		ret = true;
	}

	return ret;
}


stiHResult BluetoothAudioBase::RingIndicate(bool bIndicator)
{
	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace ("RingIndicate, %s\n", bIndicator ? "START" : "STOP");
	);

	if (bIndicator)
	{
		m_eventQueue->PostEvent ([this]{
				eventHeadsetsRing ();
			});
	}
	else
	{
		m_eventQueue->PostEvent ([this]{
				m_ringTimer.stop ();
			});
	}

	return stiRESULT_SUCCESS;
}

void BluetoothAudioBase::callStateUpdate(const PlatformCallStateChange &stateChange)
{
}

void BluetoothAudioBase::eventHeadsetsRing ()
{
}

void BluetoothAudioBase::devicesUpdate()
{
}


} //namespace
