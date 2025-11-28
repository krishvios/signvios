// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "IBluetoothVP.h"
#include "stiDefs.h"
#include "stiSVX.h"
#include "stiOSLinuxWd.h"
#include "stiTools.h"
#include "CstiBluetoothInterface.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiEventTimer.h"
#include "BluetoothDbusInterface.h"
#include "BluetoothDefs.h"
#include "BluetoothAudio.h"
#include <cstdint>
#include <list>
#include <mutex>
#include <condition_variable>

#include "GattDescriptorInterface.h"

class AgentManagerInterface;
class GattManagerInterface;
class PropertiesInterface;
class AdapterInterface;
class SstiBluezDevice;

#define BLUETOOTH_MANAGER_PATH      "/"

#define BLUETOOTH_PIN_CODE_MAX_SIZE	32
#define PAIRING_TIMEOUT 60000		// 60 seconds should be enough to enter here and there....

// Defines for MAC address manipulation
#define MAC_ADDR_LENGTH		17
#define MAC_ADDR_LAST_OCTET	15
#define OCTET_CHARACTERS	2
#define HEXADECIMAL			16

	
class CstiBluetooth : public IBluetoothVP, public CstiEventQueue
{
public:

	CstiBluetooth ();

	CstiBluetooth (const CstiBluetooth &other) = delete;
	CstiBluetooth (CstiBluetooth &&other) = delete;
	CstiBluetooth &operator= (const CstiBluetooth &other) = delete;
	CstiBluetooth &operator= (CstiBluetooth &&other) = delete;

	~CstiBluetooth () override;

	stiHResult Startup ();

	stiHResult Initialize ();

	stiHResult Enable (
		bool bEnable) override;

	stiHResult Scan () override;
	
	stiHResult ScanCancel () override;
	
	stiHResult Pair (
		const BluetoothDevice& BluetoothDevice) override;
	
	stiHResult PairCancel () override;
	
	stiHResult Connect (
		BluetoothDevice BluetoothDevice) override;

	stiHResult Disconnect(
		BluetoothDevice BluetoothDevice) override;

	stiHResult PairedDeviceRemove (
		BluetoothDevice BluetoothDevice) override;
	
	stiHResult PairedListGet (
		std::list<BluetoothDevice> * list) override;
	
	stiHResult ScannedListGet (
		std::list<BluetoothDevice> * list) override;

	stiHResult DeviceRename(
		const BluetoothDevice& , const std::string name) override;

	stiHResult allDevicesListGet (
		std::list<BluetoothDevice> * list) override;

	stiHResult PincodeSet (
		const char *pszPinCode) override;

	bool Powered () override;

	stiHResult PairedDevicesLog () override;

	static std::shared_ptr<SstiGattCharacteristic> characteristicFromConnectedDeviceGet(
		const std::shared_ptr<SstiBluezDevice> &device,
		const std::string &uuid);

	void agentUnregister ();

	static std::string BluetoothDefaultAdapterNameGet ();

	nonstd::observer_ptr<vpe::BluetoothAudio> AudioGet ();

public: //signal getter implementations for IstiBluetooth

	ISignal<BluetoothDevice> &deviceAddedSignalGet () override { return deviceAddedSignal; };
	ISignal<BluetoothDevice> &deviceConnectedSignalGet () override { return deviceConnectedSignal; };
	ISignal<> &deviceConnectFailedSignalGet () override { return deviceConnectFailedSignal; };
	ISignal<BluetoothDevice> &deviceDisconnectedSignalGet () override { return deviceDisconnectedSignal; };
	ISignal<> &deviceDisconnectFailedSignalGet () override { return deviceDisconnectFailedSignal; };
	ISignal<> &deviceRemovedSignalGet () override { return deviceRemovedSignal; };
	ISignal<> &scanCompleteSignalGet () override { return scanCompleteSignal; };
	ISignal<> &scanFailedSignalGet () override { return scanFailedSignal; };
	ISignal<> &pairingCompleteSignalGet () override { return pairingCompleteSignal; };
	ISignal<> &pairingFailedSignalGet () override { return pairingFailedSignal; };
	ISignal<> &pairingCanceledSignalGet () override { return pairingCanceledSignal; };
	ISignal<> &pairingInputTimeoutSignalGet () override { return pairingInputTimeoutSignal; };
	ISignal<> &pairedMaxCountExceededSignalGet () override { return pairedMaxCountExceededSignal; };
	ISignal<> &unpairCompleteSignalGet () override { return unpairCompleteSignal; };
	ISignal<> &unpairFailedSignalGet () override { return unpairFailedSignal; };
	ISignal<std::string> &displayPincodeSignalGet () override { return displayPincodeSignal; };
	ISignal<> &requestPincodeSignalGet () override { return requestPincodeSignal; };
	ISignal<> &deviceAliasChangedSignalGet () override { return deviceAliasChangedSignal; };

	CstiSignal<BluetoothDevice> deviceAddedSignal;
	CstiSignal<BluetoothDevice> deviceConnectedSignal;
	CstiSignal<> deviceConnectFailedSignal;
	CstiSignal<BluetoothDevice> deviceDisconnectedSignal;
	CstiSignal<> deviceDisconnectFailedSignal;
	CstiSignal<> deviceRemovedSignal;
	CstiSignal<> scanCompleteSignal;
	CstiSignal<> scanFailedSignal;
	CstiSignal<> pairingCompleteSignal;
	CstiSignal<> pairingFailedSignal;
	CstiSignal<> pairingCanceledSignal;
	CstiSignal<> pairingInputTimeoutSignal;
	CstiSignal<> pairedMaxCountExceededSignal;
	CstiSignal<> unpairCompleteSignal;
	CstiSignal<> unpairFailedSignal;
	CstiSignal<std::string> displayPincodeSignal;
	CstiSignal<> requestPincodeSignal;
	CstiSignal<> deviceAliasChangedSignal;

private:

	void deviceInterfaceSignalsConnect ();
	void adapterInterfaceSignalsConnect ();

	CstiSignal<std::shared_ptr<AgentManagerInterface>> m_agentManagerAddedSignal;
	CstiSignal<> m_agentRegistrationFailedSignal;

	CstiSignal<std::shared_ptr<GattManagerInterface>> m_gattManagerAddedSignal;

	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_deviceAddedSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_deviceRemovedSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_devicePairingCompleteSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_devicePairingFailedSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_devicePairingCanceledSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_devicePairingInputTimeoutSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_deviceConnectedSignal;
	CstiSignal<> m_deviceConnectFailedSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_deviceDisconnectedSignal;
	CstiSignal<> m_deviceDisconnectFailedSignal;
	CstiSignal<std::shared_ptr<SstiBluezDevice>> m_deviceDisconnectCompletedSignal;

	CstiSignal<std::shared_ptr<AdapterInterface>> m_adapterAddedSignal;
	CstiSignal<std::shared_ptr<AdapterInterface>> m_adapterRemovedSignal;
	CstiSignal<> m_adapterScanCompletedSignal;
	CstiSignal<> m_adapterScanFailedSignal;
	CstiSignal<> m_adapterRemoveDeviceCompletedSignal;
	CstiSignal<> m_adapterRemoveDeviceFailedSignal;
	CstiSignal<std::shared_ptr<AdapterInterface>> m_adapterPoweredOnSignal;

	CstiSignalConnection::Collection m_signalConnections;

	static void objectAddedCallback (
		GDBusObjectManager *objectManager,
		GDBusObject *object,
		gpointer userData);

	static void objectRemovedCallback (
		GDBusObjectManager *objectManager,
		GDBusObject *object,
		gpointer userData);

	static void interfaceAddedCallback (
		GDBusObjectManager *objectManager,
		GDBusObject *object,
		GDBusInterface *interface,
		gpointer userData);

	void interfaceAdd (
		std::shared_ptr<DbusObject> object,
		GDBusInterface *interface);

	static void interfaceRemovedCallback (
		GDBusObjectManager *objectManager,
		GDBusObject *object,
		GDBusInterface *interface,
		gpointer userData);

	static void interfacePropertiesChangedCallback(
		GDBusObjectManagerClient *object,
		GDBusObjectProxy *objectProxy,
		GDBusProxy *interfaceProxy,
		GVariant *changedProperties,
		gchar **invalidatedProperties,
		gpointer userData);
	
	static gboolean RequestPincodeMessage (
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *,
		const gchar *,
		gpointer);
	
	static gboolean RequestPasskeyMessage (
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *,
		const gchar *,
		gpointer);
	
	static gboolean RequestConfirmationMessage (
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *,
		const gchar *,
		guint,
		gpointer);

	static gboolean RequestAuthorizationMessage(
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *pInvocation,
		const gchar *ObjectPath,
		gpointer UserData);
	
	static gboolean CancelMessage(
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *,
		gpointer);
	
	static gboolean ReleaseMessage (
		CstiBluetoothAgent1 *,
		GDBusMethodInvocation *,
		gpointer);
	
	static gboolean DisplayPincodeMessage(CstiBluetoothAgent1 *,
				GDBusMethodInvocation *,
				const gchar *,
				const gchar *,
				gpointer);
	
	static gboolean DisplayPasskeyMessage( CstiBluetoothAgent1 *,
				GDBusMethodInvocation *,
				const gchar *,
				guint32,
				guint16,
				gpointer);

private:

	GDBusMethodInvocation *m_pairingInvocation = nullptr;

	enum class PairingType
	{
		Unknown = 0,
		Passkey = 1,
		PinCode = 2
	};

	PairingType m_pairingAuthenticationType = PairingType::Unknown;

	std::shared_ptr<SstiBluezDevice> m_pairingDevice;

	enum class PairingState
	{
		None = 0,
		Pairing,
	};

	PairingState m_pairingState = PairingState::None;

private:
	stiHResult BluetoothInitialize ();
	void BluetoothUninitialize ();

	stiHResult ControllerInitialize ();
	std::string execCommand(const char* cmd);

	stiHResult ScannedListAdvertisingReset ();

	/*
	 * event handlers
	 */
	void eventManagedObjectsGet ();

	void eventPair ();

	void eventObjectAdded (GDBusObject *object);
	void eventObjectRemoved (GDBusObject *object);

	void eventInterfaceAdded (GDBusObject *object, GDBusInterface *interface);
	void eventInterfaceRemoved (GDBusObject *object, GDBusInterface *interface);

	void eventAdapterInterfaceAdded (std::shared_ptr<AdapterInterface> adapterInterface);
	void eventAdapterInterfaceRemoved (std::shared_ptr<AdapterInterface> adapterInterface);
	void eventAdapterScanFailed ();

	void eventAgentManagerInterfaceAdded(std::shared_ptr<AgentManagerInterface> agentManagerInterface);
	void eventAgentRegistrationFailed ();

	void eventDeviceInterfaceAdded (std::shared_ptr<SstiBluezDevice> deviceInterface);
	void eventDeviceInterfaceRemoved (std::shared_ptr<SstiBluezDevice> deviceInterface);

	void eventDevicePairingComplete (std::shared_ptr<SstiBluezDevice> deviceInterface);
	void eventDevicePairingStatus (std::shared_ptr<SstiBluezDevice> deviceInterface);
	void eventDeviceConnected (std::shared_ptr<SstiBluezDevice> deviceInterface);
	void eventDeviceDisconnected (std::shared_ptr<SstiBluezDevice> deviceInterface);

	void eventPropertyChanged(
		std::string objectPath,
		std::string interfaceName,
		GVariant *changedProperties,
		std::vector<std::string> invalidatedProperties);

	void connectPairedDevice();

	std::shared_ptr<DbusConnection> m_dbusConnection;

	GDBusObjectManager *m_objectManagerProxy = nullptr;
	unsigned long m_objectAddedSignalHandlerId = 0;
	unsigned long m_objectRemovedSignalHandlerId = 0;
	unsigned long m_interfaceAddedSignalHandlerId = 0;
	unsigned long m_interfaceRemovedSignalHandlerId = 0;
	unsigned long m_interfacePropertyChangedId = 0;

	GDBusObjectManagerServer *m_appObjectManagerServer = nullptr;

	std::list<std::shared_ptr<SstiBluezDevice>> m_deviceList;

	std::shared_ptr<AdapterInterface> m_adapterInterface;

	std::list<BluetoothDevice> m_connectList {};

	unsigned long m_unDisplayPasskeySignalHandlerId = 0;
	unsigned long m_unDisplayPinCodeSignalHandlerId = 0;
	unsigned long m_unReleaseSignalHandlerId = 0;
	unsigned long m_unCancelSignalHandlerId = 0;
	unsigned long m_unAuthorizeSignalHandlerId = 0;
	unsigned long m_unRequsetConfirmationSignalHandlerId = 0;
	unsigned long m_unReqeuestPasskeySignalHandlerId = 0;
	unsigned long m_unRequsetPinCodeSignalHandlerId = 0;

	vpe::EventTimer m_scanTimer;
	vpe::EventTimer m_pairedListConnectTimer;

	CstiBluetoothAgent1 *m_agentProxy = nullptr;

	vpe::BluetoothAudio m_audio;

	bool m_isAudioConnected {false};

public:
	/*
	 * initiate a read of the gatt characteristic
	 */
	static stiHResult gattCharacteristicRead(const std::shared_ptr<SstiBluezDevice> &device, std::string uuid);
	/*
	 * initiate a write to the gatt characteristic
	 */
	static stiHResult gattCharacteristicWrite(const std::shared_ptr<SstiBluezDevice> &device, std::string uuid, const char * value, int len);
	/*
	 * disconnect a gatt device
	 */
	stiHResult gattDisconnect(const std::shared_ptr<SstiBluezDevice> &device);
	/*
	 * set whether or not you want notifications on a characteristic
	 */
	static stiHResult gattCharacteristicNotifySet(const std::shared_ptr<SstiBluezDevice> &pairedDevice, std::string uuid, bool value);

	stiHResult heliosDevicesGet(std::list<std::shared_ptr<SstiBluezDevice>> *list);

	std::shared_ptr<SstiBluezDevice> pairedHeliosDeviceGet (
		const std::string &address);

	stiHResult heliosPairedDevicesGet(std::list<std::shared_ptr<SstiBluezDevice>> *list);

	stiHResult deviceConnectByAddress (const std::string& deviceAddress);

	std::shared_ptr<SstiBluezDevice> deviceGetByAddress (const std::string& deviceAddress);

	void deviceConnectNewWithoutDiscovery (const std::string &deviceAddress, const std::string &addressType);

	void eventConnectPairedDevices();
	void pairedDevicesReset();
};
