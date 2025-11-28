// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "BleDevice.h"
#include "BleSendProgress.h"
#include "ccrc.h"

class DfuDevice : public BleDevice
{
private:
	enum class DfuState : int
	{
		None = 0,
		SendingInitPacket,
		SendingFirmware,
		Complete,
	};
	
	// Object types
	static const uint8_t OBJECT_INIT_PACKET = 0x01;
	static const uint8_t OBJECT_FIRMWARE = 0x02;
	
	// Operation codes
	static const uint8_t OP_CODE_CREATE = 0x01;
	static const uint8_t OP_CODE_PACKET_RECEIPT_NOTIF_REQ = 0x02;
	static const uint8_t OP_CODE_CALCULATE_CHECKSUM = 0x03;
	static const uint8_t OP_CODE_EXECUTE = 0x04;
	static const uint8_t OP_CODE_SELECT_OBJECT = 0x06;
	static const uint8_t OP_CODE_RESPONSE_CODE = 0x60;
	
	static const uint16_t INIT_PACKET_PACKETS_PER_NOTIFICATION = 0;
	static const uint16_t FIRMWARE_PACKETS_PER_NOTIFICATION = 10;
	
public:
	DfuDevice (std::unique_ptr<BleIODevice> device, CstiEventQueue *eventQueue);
	~DfuDevice ();
	
	void didAddService (const std::string &serviceUuid) override;
	void didAddCharacteristic (const std::string &serviceUuid, const std::string &characUuid) override;
	void didUpdateStartNotify (const std::string &serviceUuid, const std::string &characUuid, bool enabled) override;
	void didChangeCharacteristicValue (const std::string &serviceUuid, const std::string &characUuid, const std::vector<uint8_t> &value) override;
	void didWriteCharacteristic (const std::string &serviceUuid, const std::string &characUuid) override;
	
	
	void packetReceiptNotificationsSet (uint16_t numberOfPackets);
	bool controlPointResponseSuccessCheck (const std::vector<uint8_t> &response);
	void controlPointResponseReceived (const std::vector<uint8_t> &response);
	void dataPacketWriteComplete ();
	void initPacketSend ();
	void objectSelect (uint8_t objectType);
	void objectCreate (uint8_t objectType, uint32_t size);
	void objectSizeSet (std::vector<uint8_t> data, uint32_t size);
	void opCodeWrite (const std::vector<uint8_t> &data);
	void dfuDataWrite ();
	void checksumRequest ();
	void dfuDataExecute ();
	uint32_t maxObjectSizeGet (const std::vector<uint8_t> &response);
	uint32_t objectOffsetGet (const std::vector<uint8_t> &response, uint8_t responseType);
	uint32_t objectCrc32Get (const std::vector<uint8_t> &response, uint8_t responseType);
	
private:
	bool m_receivedDfuService = false;
	bool m_receivedControlPointChar = false;
	bool m_receivedPacketChar = false;
	
	DfuState m_dfuState = DfuState::None;
	
	std::vector<uint8_t> m_initPacket;
	std::vector<uint8_t> m_firmware;
	
	uint32_t m_maxObjectSize = 0;
	uint32_t m_currentObjectOffset = 0;
	uint32_t m_currentObjectCrc32 = 0;
	
	BleSendProgress m_sendProgress;
	unsigned int m_bytesSent = 0;
	unsigned int m_percentComplete = 0;
	std::vector<uint8_t> m_byteVectorSent;
	unsigned int m_packetsToSendBeforeNotification = 0;
	unsigned int m_numPacketsSentSinceNotification = 0;
	
	CCRC m_dfuCRC;
	
	vpe::EventTimer m_timeoutTimer;
	CstiSignalConnection::Collection m_signalConnections;
};
