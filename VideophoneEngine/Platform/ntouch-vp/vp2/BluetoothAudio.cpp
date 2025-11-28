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
	m_bluezAlsaClient(queue, interface, std::list<ba_pcm_type>{BA_PCM_TYPE_SCO})
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

/*!\brief enable SCO audio to go over the HCI bus
 *
 * by default, our TI chip is configured to send SCO audio over the PCM bus
 */
stiHResult BluetoothAudio::HCIAudioEnable ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	int dev_id {-1};
	int device_descriptor {-1};
	int ret {-1};

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("Configuring bluetooth hardware for SCO over HCI interface\n");
	);

	// This is using the bluez management interface.
	// hci.h seems deprecated, as well as hcitool
	// Not sure if I can send these types of commands over dbus?

	// Also not sure if this can be done over the management interface?

	// I think the hci api is deprecated?  But, not sure what else to use...

	// TI firmware command: Send_HCI_VS_Write_SCO_Configuration 0xFE10, 0x01, 120, 511, 0xFF
	// Here's the hcitool command that works: hcitool cmd 0x3f 0x210 0x01 120 511 0xff
	// This information is found in several TI forums
	// TODO: some mention setting flow control as well?

	// TI command: 0xFE10, split into 6 and 10 bytes: 0x3F, 0x210
	uint8_t ogf = 0x3F;
	uint16_t ocf = 0x210;

	// TODO: these values were gotten from the hcitool output
	// Not sure how the above values correspond to these...
	std::vector<uint8_t> buffer = {0x01, 0x20, 0x11, 0xFF};

	dev_id = hci_devid(m_interfaceName.c_str());
	stiTESTCONDMSG (dev_id > -1, stiRESULT_ERROR, "%s: Unable to find bluetooth adapter\n", __FUNCTION__);

	device_descriptor = hci_open_dev(dev_id);
	stiTESTCONDMSG (device_descriptor > -1, stiRESULT_ERROR, "%s: Unable to open device descriptor\n", __FUNCTION__);

	stiDEBUG_TOOL(g_stiBluetoothDebug,
		stiTrace("%s: dev_id: %d, dd: %d\n", __FUNCTION__, dev_id, device_descriptor);
	);

	ret = hci_send_cmd(device_descriptor, ogf, ocf, buffer.size(), &buffer[0]);
	stiTESTCONDMSG (ret == 0, stiRESULT_ERROR, "%s: error setting TI chip in HCI/SCO mode, return code: %d\n", __FUNCTION__, ret);

STI_BAIL:

	hci_close_dev(device_descriptor);

	return hResult;
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
