#pragma once

#include "BluetoothAudioBase.h"

namespace vpe
{

class BluetoothAudio : public BluetoothAudioBase
{
public:
	BluetoothAudio(const nonstd::observer_ptr<CstiEventQueue> &queue, const std::string &interface) :
			BluetoothAudioBase(queue, interface) {};

	~BluetoothAudio() = default;

	BluetoothAudio (const BluetoothAudio &other) = delete;
	BluetoothAudio (BluetoothAudio &&other) = delete;
	BluetoothAudio &operator= (const BluetoothAudio &other) = delete;
	BluetoothAudio &operator= (BluetoothAudio &&other) = delete;
};

}	// end of namespace
