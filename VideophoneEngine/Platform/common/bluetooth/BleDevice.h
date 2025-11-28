// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <functional>
#include "BleIODevice.h"
#include "CstiEventQueue.h"
#include "stiTrace.h"

class BleDevice
{
protected:
	BleDevice (std::unique_ptr<BleIODevice> device, CstiEventQueue *eventQueue);
public:
	virtual ~BleDevice ();
	BleDevice (const BleDevice &) = delete;
	BleDevice (BleDevice &&) = default;
	BleDevice &operator= (const BleDevice &) = delete;
	BleDevice &operator= (BleDevice &&) = default;
private:
	// Callbacks. These must be passed to the IODevice
	void serviceAddedCallback (const std::string &serviceUuid);
	void characteristicAddedCallback (const std::string &serviceUuid, const std::string &characUuid);
	void descriptorAddedCallback (const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid);
	void characteristicStartNotifyCallback (const std::string &serviceUuid, const std::string &characUuid, bool enabled);
	void characteristicValueChangedCallback (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value);
	void characteristicWrittenCallback (const std::string &serviceUuid, const std::string &characUuid);
	
	// The following will be implemented if device specific notification is required.
	// These are forwarded from the callbacks above on the event queue.
	virtual void didAddService (const std::string &serviceUuid) {};
	virtual void didAddCharacteristic (const std::string &serviceUuid, const std::string &characUuid) {};
	virtual void didAddDescriptor (const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid) {};
	virtual void didUpdateStartNotify (const std::string &serviceUuid, const std::string &characUuid, bool enabled) {};
	virtual void didChangeCharacteristicValue (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value) {};
	virtual void didWriteCharacteristic (const std::string &serviceUuid, const std::string &characUuid) {};
public:
	// Implemented in this class
	std::string idGet ();
	std::string nameGet ();
	std::vector<std::string> advertisedServicesGet ();
	BleIODevice* bleIODeviceGet () {return m_ioDevice.get ();};
	
	void pairedSet (bool paired);
	bool pairedGet ();
	
	void connectedSet (bool connected);
	bool connectedGet ();
	
	void servicesDiscover ();
	
	virtual bool isPulseDevice () {return false;};
	
protected:
	void characteristicStartNotifySet (
		const std::string &serviceUuid,
		const std::string &characUuid,
		bool enabled);
	
	void characteristicRead (
		const std::string &serviceUuid,
		const std::string &characUuid);
	
	void characteristicWrite (
		const std::string &serviceUuid,
		const std::string &characUuid,
		const std::vector<uint8_t> &data);
	
private:
	void characteristicWrittenCallbackEvent (const std::string &serviceUuid, const std::string &characUuid);
	
	size_t serviceAndCharacteristicHash (
		 const std::string &serviceUuid,
		 const std::string &characUuid);
	
	void characteristicWriteQueueProcess (size_t key);
	
private:
	struct writeRequest
	{
		std::string m_serviceUuid;
		std::string m_characUuid;
		std::vector<uint8_t> m_data;
		
		writeRequest (
					  const std::string& serviceUuid,
					  const std::string &characUuid,
					  const std::vector<uint8_t> &data)
		:
		m_serviceUuid (serviceUuid),
		m_characUuid (characUuid),
		m_data (data)
		{};
	};
	
	std::map<size_t,std::deque<writeRequest>> m_charWriteMap;
	
protected:
	std::unique_ptr<BleIODevice> m_ioDevice = nullptr;
	
	CstiEventQueue *m_eventQueue = nullptr;
};

