// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2018 Sorenson Communications, Inc. -- All rights reserved
//
// The entirety of the dfu process is designed to run on the CstiHelios
// event queue. All timers and signal handlers are to be implemented on
// that thread.

#pragma once

#include "stiDefs.h"
#include "CstiBluetooth.h"
#include "DeviceInterface.h"
#include "BleSendProgress.h"
#include "ccrc.h"
#include "CstiEventTimer.h"

#include <string>
#include <cstdint>
#include <list>
#include <vector>

#define DFU_WITH_PASSIVE_SCAN


class NordicDfu
{
public:
	
	NordicDfu (CstiEventQueue *eventQueue);

	NordicDfu (const NordicDfu &other) = delete;
	NordicDfu (NordicDfu &&other) = delete;
	NordicDfu &operator= (const NordicDfu &other) = delete;
	NordicDfu &operator= (NordicDfu &&other) = delete;

	virtual ~NordicDfu () = default;

	stiHResult initiate (std::shared_ptr<SstiBluezDevice> device, const std::string& zipFilePath, CstiBluetooth *cstiBluetooth);

private:
	
	// Firmware files
	const static std::string INIT_FILE;
	const static std::string FIRMWARE_FILE;

	// Object types
	const static uint8_t OBJECT_INIT_PACKET = 0x01;
	const static uint8_t OBJECT_FIRMWARE = 0x02;

	// Operation codes
	const static uint8_t OP_CODE_CREATE = 0x01;
	const static uint8_t OP_CODE_PACKET_RECEIPT_NOTIF_REQ = 0x02;
	const static uint8_t OP_CODE_CALCULATE_CHECKSUM = 0x03;
	const static uint8_t OP_CODE_EXECUTE = 0x04;
	const static uint8_t OP_CODE_SELECT_OBJECT = 0x06;
	const static uint8_t OP_CODE_RESPONSE_CODE = 0x60;

	// DFU Target Attributes
	const static uint8_t DFU_PACKET_CHAR = 0x01;
	const static uint8_t DFU_CONTROL_POINT_CHAR = 0x02;
	const static uint8_t DFU_CONTROL_POINT_CCCD = 0x04;

//	constexpr const static char* const BUTTONLESS_DFU_SERVICE_UUID = "0000fe59-0000-1000-8000-00805f9b34fb";
	const static std::string BUTTONLESS_DFU_CHAR_UUID;// = "8ec90003-f315-4f60-9fb8-838830daea50";

//	constexpr const static char* const DFU_SERVICE_UUID = "0000fe59-0000-1000-8000-00805f9b34fb";
	const static std::string DFU_CONTROL_POINT_CHAR_UUID;// = "8ec90001-f315-4f60-9fb8-838830daea50";
	const static std::string DFU_PACKET_CHAR_UUID;// = "8ec90002-f315-4f60-9fb8-838830daea50";

	const static uint16_t INIT_PACKET_PACKETS_PER_NOTIFICATION = 0;
	// With the following value set to 20, bluetooth buffer overflows
	// have occurred when connected with more than one Helios device
	const static uint16_t FIRMWARE_PACKETS_PER_NOTIFICATION = 10;

	enum class DfuState
	{
		None = 0,
		RestartingRemoteDevice,
		ScanningForDfuTarget,
		SendingInitPacket,
		SendingFirmware,
		Complete,
	};

	stiHResult firmwareGet (const std::string& zipFilePath);

	void firmwareClear ();

	stiHResult dfuAddressesDetermine ();

	void dfuAddressesClear ();

	void restartRemoteDeviceInDfuMode ();

	void characteristicStartNotifyComplete (const std::string& deviceAddress, const std::string& characteristicUuid);

	void packetReceiptNotificationsSet (uint16_t numberOfPackets);
	
	bool controlPointResponseSuccessCheck (
	std::vector<uint8_t> response);

	void controlPointResponseReceived (
		const std::vector<uint8_t> &response);

	void dataPacketWriteComplete ();

	void initPacketSend ();

	void objectSelect (uint8_t objectType);

	void objectCreate (uint8_t objectType, uint32_t size);

	void objectSizeSet (std::vector<uint8_t>& data, uint32_t size);

	void opCodeWrite (const std::vector<uint8_t>& data);

	void dfuDataWrite ();

	void checksumRequest ();
	
	void dfuDataExecute ();

	static uint32_t maxObjectSizeGet (const std::vector<uint8_t>& response);
	
	static uint32_t objectOffsetGet (const std::vector<uint8_t>& response, uint8_t responseType);
	
	static uint32_t objectCrc32Get (const std::vector<uint8_t>& response, uint8_t responseType);

	void connectSignals ();

	void disconnectSignals ();

	void eventDeviceAdded (BluetoothDevice device);

	void eventDeviceConnected (BluetoothDevice device);

	void eventDeviceDisconnected (BluetoothDevice device);

	void eventGattValueChanged (
		const std::string& deviceAddress,
		const std::string& characteristicUuid,
		const std::vector<uint8_t> &response);

	void eventGattReadWriteCompleted (
		std::shared_ptr<SstiBluezDevice> device,
		const std::string& characteristicUuid);

	void eventGattReadWriteFailed ();

	void eventGattStartNotifyCompleted (
		const std::string& deviceAddress,
		const std::string& characteristicUuid);

	void eventGattStartNotifyFailed ();

	void cleanUp ();

public:
	CstiSignal<stiHResult, std::string> dfuResultSignal;

/*************************************************************************************************/

private:

	CstiSignalConnection::Collection m_signalConnections;

	CstiEventQueue *m_pEventQueue = nullptr;
	CstiBluetooth *m_pCstiBluetooth = nullptr;

	static const std::string m_initFile;
	static const std::string m_firmwareFile;

	std::shared_ptr<SstiBluezDevice> m_pButtonlessDfuDevice = nullptr;
	std::shared_ptr<SstiBluezDevice> m_pDfuTargetDevice = nullptr;
	std::string m_buttonlessDfuAddress;
	std::string m_dfuTargetAddress;

	DfuState m_dfuState = DfuState::None;

	uint32_t m_maxObjectSize = 0;

	std::vector<uint8_t> m_initPacket;
	std::vector<uint8_t> m_firmware;
	
	BleSendProgress m_sendProgress;
	unsigned int m_bytesSent = 0;
	unsigned int m_percentComplete = 0;
	std::vector<uint8_t> m_byteVectorSent;
	unsigned int m_packetsToSendBeforeNotification = 0;
	unsigned int m_numPacketsSentSinceNotification = 0;

	CCRC m_dfuCRC;

	CstiSignalConnection deviceAddedSignalConnection;
	CstiSignalConnection deviceConnectedSignalConnection;
	CstiSignalConnection deviceDisconnectedSignalConnection;
	CstiSignalConnection gattAlertSignalConnection;
	CstiSignalConnection gattValueChangedSignalConnection;
	CstiSignalConnection gattReadWriteCompletedSignalConnection;
	CstiSignalConnection gattReadWriteFailedSignalConnection;
	CstiSignalConnection gattStartNotifyCompletedSignalConnection;
	CstiSignalConnection gattStartNotifyFailedSignalConnection;
#ifdef DFU_WITH_PASSIVE_SCAN
	CstiSignalConnection connectDeviceSucceededSignalConnection;
	CstiSignalConnection connectDeviceFailedSignalConnection;
#endif

	vpe::EventTimer m_connectTimer;
	vpe::EventTimer m_timeoutTimer;
#ifdef DFU_WITH_PASSIVE_SCAN
	vpe::EventTimer m_connectNewWithoutDiscoveryTimer;

	unsigned int m_connectNewWithoutDiscoveryAttempts = 0;
#endif
	unsigned int m_connectAttempts = 0;
};

