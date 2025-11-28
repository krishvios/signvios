// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IstiBluetooth.h"
#include "BluetoothDefs.h"
#include "BluetoothDbusInterface.h"
#include "CstiBluetoothInterface.h"
#include "CstiSignal.h"
#include "GattServiceInterface.h"
#include <chrono>

class SstiGattService;


class SstiBluezDevice : public BluetoothDbusInterface
{
public:
	SstiBluezDevice (
		DbusObject *dbusObject)
	:
		BluetoothDbusInterface (dbusObject, BLUETOOTH_DEVICE_INTERFACE)
	{
	}

	~SstiBluezDevice () override
	{
		if (m_deviceProxy)
		{
			g_object_unref (m_deviceProxy);
			m_deviceProxy = nullptr;
		}
	}

	SstiBluezDevice (const SstiBluezDevice &other) = delete;
	SstiBluezDevice (SstiBluezDevice &&other) = delete;
	SstiBluezDevice &operator= (const SstiBluezDevice &other) = delete;
	SstiBluezDevice &operator= (SstiBluezDevice &&other) = delete;

	void added (
		GVariant *properties) override;

	void changed(
		GVariant *changedProperties,
		const std::vector<std::string> &invalidatedProperties) override;

	void removed () override;

	void pair ();

	void pairCancel ();

	void connect ();

	void disconnect ();

	static CstiSignal<std::shared_ptr<SstiBluezDevice>> addedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> removedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> pairingCompleteSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> pairingFailedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> pairingCanceledSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> pairingInputTimeoutSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> connectedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> disconnectedSignal;
	static CstiSignal<> connectFailedSignal;
	static CstiSignal<> disconnectFailedSignal;
	static CstiSignal<std::shared_ptr<SstiBluezDevice>> disconnectCompletedSignal;
	static CstiSignal<> aliasChangedSignal;

	void serviceAdd (
		std::shared_ptr<SstiGattService> service)
	{
		m_services.push_back (service);
	}

	void serviceRemove (
		std::shared_ptr<SstiGattService> service)
	{
		m_services.remove (service);
	}

	void characteristicAdd (const std::string &uuid)
	{
		m_heliosAttributes.characteristicAdd (uuid);

		if ((isHelios && isHeliosReady ()) ||
			(bluetoothDevice.name == HELIOS_DFU_TARGET_NAME && isHeliosDfuTargetReady ()))
		{
			connectedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this()));
		}
	}

	void descriptorAdd (const std::string &uuid)
	{
		m_heliosAttributes.descriptorAdd (uuid);

		if ((isHelios && isHeliosReady ()) ||
			(bluetoothDevice.name == HELIOS_DFU_TARGET_NAME && isHeliosDfuTargetReady ()))
		{
			connectedSignal.Emit (std::static_pointer_cast<SstiBluezDevice>(shared_from_this()));
		}
	}

	std::shared_ptr<SstiGattCharacteristic> characteristicFind (
		const std::string &uuid)
	{
		for (auto &service : m_services)
		{
			auto characteristic = service->characteristicFind (uuid);

			// If a characteristic was found, return it.
			if (characteristic)
			{
				return characteristic;
			}
		}

		return nullptr;
	}

	inline bool isHeliosReady ()
	{
		return (m_heliosAttributes.heliosComplete () &&
			bluetoothDevice.connected &&
			bluetoothDevice.trusted &&
			servicesResolved);
	}

	inline bool isHeliosDfuTargetReady ()
	{
		return (m_heliosAttributes.heliosDfuTargetComplete () &&
			bluetoothDevice.connected &&
			servicesResolved);
	}

	void propertiesProcess (
		GVariant *changedProperties);

	bool deviceSupported();

	void rename(std::string);

	BluetoothDevice bluetoothDevice;
	bool servicesResolved = false;
	bool authenticated = false;
	bool firmwareVerified = false;
	std::string version;
	std::string serialNumber;
	std::string defaultName;
	bool m_connecting = false;
	std::chrono::system_clock::time_point authenticationTime;
	bool isSupported = false;
	bool isHelios = false;
	bool isAdvertising = false;

	class heliosAttributes
	{
	public:

		void characteristicAdd (const std::string &charUUID)
		{
			if (charUUID == PULSE_WRITE_CHARACTERISTIC_UUID)
			{
				m_havePulseWriteCharacteristic = true;
			}
			else if (charUUID == PULSE_READ_CHARACTERISTIC_UUID)
			{
				m_havePulseReadCharacteristic = true;
			}
			else if (charUUID == PULSE_AUTHENTICATE_CHARACTERISTIC_UUID)
			{
				m_havePulseAuthCharacteristic = true;
			}
			else if (charUUID == PULSE_DFU_CONTROL_POINT_CHARACTERISTIC_UUID)
			{
				m_haveDfuControlPointCharacteristic = true;
			}
			else if (charUUID == PULSE_DFU_PACKET_CHARACTERISTIC_UUID)
			{
				m_haveDfuPacketCharacteristic = true;
			}
			else if (charUUID == PULSE_DFU_BUTTONLESS_CHARACTERISTIC_UUID)
			{
				m_haveDfuButtonlessCharacteristic = true;
			}
			else if (charUUID == PULSE_GATT_CHARACTERISTIC_UUID)
			{
				m_haveGattCharacteristic = true;
			}
		}

		void descriptorAdd (const std::string &charUUID)
		{
			if (charUUID == PULSE_READ_CHARACTERISTIC_UUID)
			{
				m_havePulseReadCharDescriptor = true;
			}
			else if (charUUID == PULSE_AUTHENTICATE_CHARACTERISTIC_UUID)
			{
				m_havePulseAuthCharDescriptor = true;
			}
			else if (charUUID == PULSE_DFU_CONTROL_POINT_CHARACTERISTIC_UUID)
			{
				m_haveDfuControlPointCharDescriptor = true;
			}
			else if (charUUID == PULSE_DFU_BUTTONLESS_CHARACTERISTIC_UUID)
			{
				m_haveDfuButtonlessCharDescriptor = true;
			}
			else if (charUUID == PULSE_GATT_CHARACTERISTIC_UUID)
			{
				m_haveGattCharDescriptor = true;
			}
		}

		bool heliosComplete ()
		{
			return 	m_havePulseWriteCharacteristic &&
					m_havePulseReadCharacteristic &&
					m_havePulseReadCharDescriptor &&
					m_havePulseAuthCharacteristic &&
					m_havePulseAuthCharDescriptor &&
					m_haveDfuButtonlessCharacteristic &&
					m_haveDfuButtonlessCharDescriptor;
		}

		bool heliosDfuTargetComplete ()
		{
			return 	m_haveDfuControlPointCharacteristic &&
					m_haveDfuControlPointCharDescriptor &&
					m_haveDfuPacketCharacteristic;
		}

	private:

		bool m_havePulseWriteCharacteristic = false;
		bool m_havePulseReadCharacteristic = false;
		bool m_havePulseReadCharDescriptor = false;
		bool m_havePulseAuthCharacteristic = false;
		bool m_havePulseAuthCharDescriptor = false;
		bool m_haveDfuControlPointCharacteristic = false;
		bool m_haveDfuControlPointCharDescriptor = false;
		bool m_haveDfuPacketCharacteristic = false;
		bool m_haveDfuButtonlessCharacteristic = false;
		bool m_haveDfuButtonlessCharDescriptor = false;
		bool m_haveGattCharacteristic = false;
		bool m_haveGattCharDescriptor = false;

	};

private:

	void eventProxyNew (
		GObject *proxy,
		GAsyncResult *pRes);

	void eventPairFinish (
		GObject *proxy,
		GAsyncResult *res);

	void eventPairCancel(
		GObject *proxy,
		GAsyncResult *pRes);

	void eventConnectFinish (
		GObject *proxy,
		GAsyncResult *pRes);

	void eventDisconnectFinish (
		GObject *proxy,
		GAsyncResult *pRes);

	std::list<std::shared_ptr<SstiGattService>> m_services;

	CstiBluetoothDevice1 *m_deviceProxy = nullptr;
	heliosAttributes m_heliosAttributes;

	bool m_aliasChange = false;
};

