// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IstiBluetooth.h"

class IBluetoothVP : public IstiBluetooth
{
public:

	IBluetoothVP (const IBluetoothVP &other) = delete;
	IBluetoothVP (IBluetoothVP &&other) = delete;
	IBluetoothVP &operator= (const IBluetoothVP &other) = delete;
	IBluetoothVP &operator= (IBluetoothVP &&other) = delete;

	virtual stiHResult allDevicesListGet (std::list<BluetoothDevice> *deviceList) = 0;
	
protected:

	IBluetoothVP () = default;
	~IBluetoothVP () override = default;

};
