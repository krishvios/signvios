// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiSignal.h"
#include "IstiBluetooth.h"
#include "BluetoothDefs.h"
#include "CstiEventTimer.h"

// Forward declarations
struct PlatformCallStateChange;

namespace vpe
{

/*!\brief manage bluetooth audio state for audio devices
 * coming and going, as well as some utility functions
 */
class BluetoothAudioBase
{
public:

	BluetoothAudioBase(const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface);

	void currentDeviceBroadcast ();

	stiHResult RingIndicate (
		bool bIndicator);

	virtual void callStateUpdate (const PlatformCallStateChange &callStateChange);

	CstiSignal<BluetoothAudioDevice> deviceUpdatedSignal;

	std::string BluetoothAudioDeviceGet (const BluetoothAudioDevice &device) const;

	virtual stiHResult HCIAudioEnable ();

	virtual void devicesUpdate();

protected:
	virtual void eventHeadsetsRing ();
	bool isNewDevice (const BluetoothAudioDevice &newDevice);

	BluetoothAudioDevice m_currentDevice;
	CstiSignalConnection::Collection m_signalConnections;
	std::string m_interfaceName;
	nonstd::observer_ptr<CstiEventQueue> m_eventQueue;
	vpe::EventTimer m_ringTimer;
};

} //namespace
