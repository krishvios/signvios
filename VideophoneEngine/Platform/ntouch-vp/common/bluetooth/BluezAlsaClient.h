// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiSignal.h"
#include "BluetoothDefs.h"

#include "nonstd/observer_ptr.h"

#include <string>
#include <vector>
#include <bluealsa/ctl-client.h>

// forward declarations
class CstiEventQueue;
struct PlatformCallStateChange;

namespace vpe
{


class BluezAlsaClient
{
public:

	BluezAlsaClient (const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface, const std::list<ba_pcm_type> &supportedTypes);

	void initialize ();

	BluezAlsaClient (const BluezAlsaClient &other) = delete;
	BluezAlsaClient (BluezAlsaClient &&other) = delete;
	BluezAlsaClient &operator= (const BluezAlsaClient &other) = delete;
	BluezAlsaClient &operator= (BluezAlsaClient &&other) = delete;

	~BluezAlsaClient ();

	void ringIndicateSend ();
	void callStateUpdate (const PlatformCallStateChange &callStateChange);

	std::vector<BluetoothAudioDevice> devicesGet ();
	void changesBroadcast ();

	CstiSignal<BluetoothAudioDevice> deviceAddedSignal;
	CstiSignal<BluetoothAudioDevice> deviceRemovedSignal;

private:

	void rfcommSendAll (const std::string &command);

	int eventsProcess ();

	int m_clientFd = -1;

	// Key: bluetooth address
	std::map<std::string, BluetoothAudioDevice> m_currentDevices;

	nonstd::observer_ptr<CstiEventQueue> m_bluetoothEventQueue;

	CstiSignalConnection::Collection m_signalConnections;

	int m_activeCallIndex = -1;
	std::list<ba_pcm_type> m_supportedTypes;
};


} // namespace
