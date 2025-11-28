// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BasePlatform.h"
#include <functional>
#include <tuple>
#include <boost/optional.hpp>

class IPlatformVP : public BasePlatform
{
public:

	IPlatformVP (const IPlatformVP &other) = delete;
	IPlatformVP (IPlatformVP &&other) = delete;
	IPlatformVP &operator= (const IPlatformVP &other) = delete;
	IPlatformVP &operator= (IPlatformVP &&other) = delete;

	virtual bool webServiceIsRunning () = 0;
	virtual void webServiceStart () = 0;

	virtual void tofDistanceGet (const std::function<void(int)> &onSuccess) = 0;

	// BIOS interface functions
	virtual std::string serialNumberGet () const = 0;
	virtual stiHResult serialNumberSet (const std::string &serialNumber) = 0;
	virtual std::tuple<int,boost::optional<int>> hdccGet () const = 0;
	virtual std::tuple<stiHResult, stiHResult> hdccSet (boost::optional<int> hdcc, boost::optional<int> hdccOverride) = 0;
	virtual void hdccOverrideDelete () = 0;
	virtual void systemSleep () override = 0;
	virtual void systemWake () override = 0;

public:

	CstiSignal<bool> headphoneConnectedSignal;
	CstiSignal<bool> microphoneConnectedSignal;
	CstiSignal<bool> testModeSetSignal;
	CstiSignal<std::vector<float>&, std::vector<float>&> micMagnitudeSignal;
	
protected:

	IPlatformVP () = default;
	~IPlatformVP () override = default;

};
