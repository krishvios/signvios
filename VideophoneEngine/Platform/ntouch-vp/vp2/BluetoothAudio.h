// Sorenson Communications Inc. Confidential. -- Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BluetoothAudioBase.h"
#include "CstiSignal.h"
#include "BluezAlsaClient.h"
#include "CstiEventTimer.h"

// Forward declarations
struct PlatformCallStateChange;

namespace vpe
{

/*!\brief manage bluetooth audio state for audio devices
 * coming and going, as well as some utility functions
 */
class BluetoothAudio : public BluetoothAudioBase
{
public:

	BluetoothAudio(const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface);

	void callStateUpdate (const PlatformCallStateChange &callStateChange) override;

	void eventHeadsetsRing () override;
	void devicesUpdate() override;

	virtual stiHResult HCIAudioEnable () override;

private:

	BluezAlsaClient m_bluezAlsaClient;
};

} //namespace
