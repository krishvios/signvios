// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#include "BluetoothAudio.h"

#include "stiTrace.h"
#include "CstiEventQueue.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluealsa/ctl-proto.h>

#include <sstream>

namespace vpe
{

BluetoothAudio::BluetoothAudio(const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface)
:
	BluetoothAudioBase(queue, interface),
	m_bluezAlsaClient(queue, interface, std::list<ba_pcm_type>{BA_PCM_TYPE_A2DP, BA_PCM_TYPE_SCO})
{
	m_signalConnections.push_back (m_bluezAlsaClient.deviceAddedSignal.Connect([this](const vpe::BluetoothAudioDevice &device)
		{
			if(isNewDevice(device))
			{
				m_currentDevice = device;
				deviceUpdatedSignal.Emit(m_currentDevice);
			}
		}));

	m_signalConnections.push_back (m_bluezAlsaClient.deviceRemovedSignal.Connect([this](const vpe::BluetoothAudioDevice &device)
		{
			if(isNewDevice(device))
			{
				m_currentDevice = device;
				deviceUpdatedSignal.Emit(m_currentDevice);
			}
		}));

	m_bluezAlsaClient.initialize();

}

void BluetoothAudio::callStateUpdate(const PlatformCallStateChange &stateChange)
{
	m_bluezAlsaClient.callStateUpdate(stateChange);
}

void BluetoothAudio::eventHeadsetsRing ()
{
	m_bluezAlsaClient.ringIndicateSend ();
	m_ringTimer.restart ();
}

void BluetoothAudio::devicesUpdate()
{
	m_bluezAlsaClient.changesBroadcast();
}


} //namespace
