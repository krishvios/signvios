// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "stiError.h"
#include "CstiSignal.h"


class ISLICRinger
{
public:

	ISLICRinger (const ISLICRinger &other) = delete;
	ISLICRinger (ISLICRinger &&other) = delete;
	ISLICRinger &operator= (const ISLICRinger &other) = delete;
	ISLICRinger &operator= (ISLICRinger &&other) = delete;
	
	// Gets instance of slic ringer
	static ISLICRinger* InstanceGet ();

	// Starts the slic ringer
	virtual stiHResult start () = 0;	

	// Stops the slic ringer
	virtual stiHResult stop () = 0;

	// Detect if slic ringer is connected
	virtual void deviceDetect () = 0;

public:
	// Event Signals
	CstiSignal<bool,int> deviceConnectedSignal;

protected:

	ISLICRinger () = default;
	virtual ~ISLICRinger () = default;
};
