// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "BleDevice.h"

BleDevice::BleDevice (std::unique_ptr<BleIODevice> ioDevice, CstiEventQueue *eventQueue)
:
m_ioDevice (std::move (ioDevice)),
m_eventQueue (eventQueue)
{
	if (m_ioDevice)
	{
		m_ioDevice->callbacksSet (
								  [this](const std::string &serviceUuid)
								  {
									  serviceAddedCallback (serviceUuid);
								  },
								  [this](const std::string &serviceUuid, const std::string &characUuid)
								  {
									  characteristicAddedCallback (serviceUuid, characUuid);
								  },
								  [this](const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)
								  {
									  descriptorAddedCallback (serviceUuid, characUuid, descriptorUuid);
								  },
								  [this](const std::string &serviceUuid, const std::string &characUuid, bool enabled)
								  {
									  characteristicStartNotifyCallback (serviceUuid, characUuid, enabled);
								  },
								  [this](const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)
								  {
									  characteristicValueChangedCallback (serviceUuid, characUuid, value);
								  },
								  [this](const std::string &serviceUuid, const std::string &characUuid)
								  {
									  characteristicWrittenCallback (serviceUuid, characUuid);
								  });
	}
}

BleDevice::~BleDevice ()
{
}

std::string BleDevice::idGet ()
{
	return m_ioDevice->idGet ();
}

std::string BleDevice::nameGet ()
{
	return m_ioDevice->nameGet ();
}

std::vector<std::string> BleDevice::advertisedServicesGet ()
{
	return m_ioDevice->advertisedServicesGet ();
}

void BleDevice::pairedSet (bool paired)
{
	m_ioDevice->pairedSet (paired);
}

bool BleDevice::pairedGet ()
{
	return m_ioDevice->pairedGet ();
}

void BleDevice::connectedSet (bool connected)
{
	m_ioDevice->connectedSet (connected);
}

bool BleDevice::connectedGet ()
{
	return m_ioDevice->connectedGet ();
}

void BleDevice::servicesDiscover ()
{
	m_ioDevice->servicesDiscover ();
}

void BleDevice::characteristicStartNotifySet (
								   const std::string &serviceUuid,
								   const std::string &characUuid,
								   bool enabled)
{
	m_ioDevice->characteristicNotifySet(serviceUuid, characUuid, enabled);
}

void BleDevice::characteristicRead (
						 const std::string &serviceUuid,
						 const std::string &characUuid)
{
	m_ioDevice->characteristicRead(serviceUuid, characUuid);
}

void BleDevice::characteristicWrite (
						  const std::string &serviceUuid,
						  const std::string &characUuid,
						  const std::vector<uint8_t> &data)
{
	writeRequest request (serviceUuid, characUuid, data);
	
	auto key = serviceAndCharacteristicHash (serviceUuid, characUuid);
	auto writeQueueIter = m_charWriteMap.find (key);
	if (writeQueueIter != m_charWriteMap.end ())
	{
		writeQueueIter->second.push_back (request);
		
		if (writeQueueIter->second.size() == 1)
		{
			characteristicWriteQueueProcess (key);
		}
	}
	else
	{
		std::deque<writeRequest> queue {request};
		m_charWriteMap.emplace (key, queue);
		
		characteristicWriteQueueProcess (key);
	}
}

void BleDevice::serviceAddedCallback (const std::string &serviceUuid)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid]{didAddService (serviceUuid);});
	}
}


void BleDevice::characteristicAddedCallback (const std::string &serviceUuid, const std::string &characUuid)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid,characUuid]{didAddCharacteristic (serviceUuid, characUuid);});
	}
}


void BleDevice::descriptorAddedCallback (const std::string &serviceUuid, const std::string &characUuid, const std::string &descriptorUuid)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid,characUuid,descriptorUuid]{didAddDescriptor (serviceUuid, characUuid, descriptorUuid);});
	}
}


void BleDevice::characteristicStartNotifyCallback (const std::string &serviceUuid, const std::string &characUuid, bool enabled)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid,characUuid,enabled]{didUpdateStartNotify (serviceUuid, characUuid, enabled);});
	}
}


void BleDevice::characteristicValueChangedCallback (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid,characUuid,value]{didChangeCharacteristicValue (serviceUuid, characUuid, value);});
	}
}


void BleDevice::characteristicWrittenCallback (
	const std::string &serviceUuid,
	const std::string &characUuid)
{
	if (m_eventQueue)
	{
		m_eventQueue->PostEvent ([this,serviceUuid,characUuid]{characteristicWrittenCallbackEvent (serviceUuid, characUuid);});
	}
}


void BleDevice::characteristicWrittenCallbackEvent (
	const std::string &serviceUuid,
	const std::string &characUuid)
{
	// Call device specific implementation
	didWriteCharacteristic (serviceUuid, characUuid);
	
	// Process queue
	auto key = serviceAndCharacteristicHash (serviceUuid, characUuid);
	auto mapIter = m_charWriteMap.find (key);
	
	if (mapIter != m_charWriteMap.end () && !mapIter->second.empty ())
	{
		auto request = mapIter->second.front ();
		
		mapIter->second.pop_front ();
		
		if (!mapIter->second.empty ())
		{
			characteristicWriteQueueProcess (key);
		}
	}
}

size_t BleDevice::serviceAndCharacteristicHash (
	const std::string &serviceUuid,
	const std::string &characUuid)
{
	size_t serviceHash = std::hash<std::string>{} (serviceUuid);
	size_t charHash = std::hash<std::string>{} (characUuid);
	return serviceHash ^ (charHash << 1);
}

void BleDevice::characteristicWriteQueueProcess (size_t key)
{
	auto mapIter = m_charWriteMap.find (key);
	if (mapIter != m_charWriteMap.end ())
	{
		auto writeQueue = mapIter->second;
		if (!writeQueue.empty ())
		{
			auto request = writeQueue.front ();
			m_ioDevice->characteristicWrite (
										 request.m_serviceUuid,
										 request.m_characUuid,
										 request.m_data);
		}
	}
}

