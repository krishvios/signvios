// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2018 Sorenson Communications, Inc. -- All rights reserved

#include "DfuDevice.h"
#include "stiTrace.h"

#define DFU_TIMEOUT 10000 // ten seconds

// Taken from Nordic device code. File: sdk/components/libraries/bootloader/dfu/nrf_dfu_handling_error.h
/*!
 *\	@brief DFU request result codes
 *
 *	@details The DFU transport layer creates request events of type @ref nrf_dfu_req_op_t.
 *			 Such events return one of these result codes.
 */
typedef enum
{
	NRF_DFU_RES_CODE_INVALID				 = 0x00,	/**< Invalid opcode. */
	NRF_DFU_RES_CODE_SUCCESS				 = 0x01,	/**< Operation successful. */
	NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED	 = 0x02,	/**< Opcode not supported. */
	NRF_DFU_RES_CODE_INVALID_PARAMETER		 = 0x03,	/**< Missing or invalid parameter value. */
	NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES	 = 0x04,	/**< Not enough memory for the data object. */
	NRF_DFU_RES_CODE_INVALID_OBJECT			 = 0x05,	/**< Data object does not match the firmware and hardware requirements,
														 	 the signature is wrong, or parsing the command failed. */
	NRF_DFU_RES_CODE_UNSUPPORTED_TYPE		 = 0x07,	/**< Not a valid object type for a create request. */
	NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED = 0x08,	/**< The state of the DFU process does not allow this operation. */
	NRF_DFU_RES_CODE_OPERATION_FAILED		 = 0x0A,	/**< Operation failed. */
	NRF_DFU_RES_CODE_EXT_ERROR				 = 0x0B,	/**< Extended error. The next byte of the response contains the error
														 	 code of the extended error (see @ref nrf_dfu_ext_error_code_t). */
} nrf_dfu_res_code_t;

/*!
 *\	@brief DFU request extended result codes
 *
 *	@details When an event return @ref NRF_DFU_RES_CODE_EXT_ERROR, it also stores an extended error code.
 *			 The transport layer can then send the extended error code together with the error code to give
 *			 the controller additional information about the cause of the error.
 */
typedef enum
{
	NRF_DFU_EXT_ERROR_NO_ERROR				= 0x00,		/**< No extended error code has been set. This error indicates an implementation problem. */
	NRF_DFU_EXT_ERROR_INVALID_ERROR_CODE	= 0x01,		/**< Invalid error code. This error code should never be used outside of development. */
	NRF_DFU_EXT_ERROR_WRONG_COMMAND_FORMAT	= 0x02,		/**< The format of the command was incorrect. This error code is not used in the
														 	 current implementation, because @ref NTF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED
														 	 and @ref NRF_DFU_RES_CODE_INVALID_PARAMETER cover all possible format errors. */
	NRF_DFU_EXT_ERROR_UNKNOWN_COMMAND		= 0x03,		/**< The command was successfully parsed, but it is not supported or unknown. */
	NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID	= 0x04,		/**< The init command is invalid. The init packet either has an invalid update type or
														 	 it is missing required fields for the update type (for example, the init packet for a
														 	 SoftDevice size field). */
	NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE	= 0x05,		/**< The firmware version is too low. For an application, the version must be greater than
														 	 the current application. For a bootloader, it must be greater than or queal to the
														 	 current version. This requirement prevents downgrade attacks. */
	NRF_DFU_EXT_ERROR_HW_VERSION_FAILURE	= 0x06,		/**< The hardware version of the device does not match the required hardware version for the udpate. */
	NRF_DFU_EXT_ERROR_SD_VERSION_FAILURE	= 0x07,		/**< The array of supported SoftDevices for the update does not contain the FWID of the
														 	 current SoftDevice. */
	NRF_DFU_EXT_ERROR_SIGNATURE_MISSING		= 0x08,		/**< The init packet does not contain a signaure. This error code is not used int the current
														 	 implementation, because init packets without a signature are regarded as invalid. */
	NRF_DFU_EXT_ERROR_WRONG_HASH_TYPE		= 0x09,		/**< The hash type that is specified by the init packet is not supported by the DFU bootloader. */
	NRF_DFU_EXT_ERROR_HASH_FAILED			= 0x0A,		/**< The hash of the firmware image cannot be calculated. */
	NRF_DFU_EXT_ERROR_WRONG_SIGNATURE_TYPE	= 0x0B,		/**< The type of the signature is unknown or not supported by the DFU bootloader. */
	NRF_DFU_EXT_ERROR_VERIFICATION_FAILED	= 0x0C,		/**< The has of the received firmware image does not match the hash in the init packet. */
	NRF_DFU_EXT_ERROR_INSUFFICIENT_SPACE	= 0x0D,		/**< The available space on the device is insufficient to hold the firmware. */
} nrf_dfu_ext_error_code_t;

DfuDevice::DfuDevice (std::unique_ptr<BleIODevice> ioDevice, CstiEventQueue *eventQueue)
:
BleDevice (std::move (ioDevice), eventQueue),
m_timeoutTimer (DFU_TIMEOUT, eventQueue)
{
	m_signalConnections.push_back (m_timeoutTimer.timeoutSignal.Connect (
		[this]{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: DFU timeout fired. DFU failed???\n");
			);
			
			// TODO: How to handle this???
			// Callback to CstiBluetooth so that it can reconnect to Pulse device?
		}));
	
	m_timeoutTimer.restart ();
}

DfuDevice::~DfuDevice ()
{
}

void DfuDevice::didAddService (const std::string &serviceUuid)
{
	if (serviceUuid == PULSE_DFU_SERVICE)
	{
		m_receivedDfuService = true;
	}
	
	if (m_receivedDfuService &&
		m_receivedControlPointChar &&
		m_receivedPacketChar)
	{
		// Ready to start DFU if we have firmware
		characteristicStartNotifySet (PULSE_DFU_SERVICE, PULSE_DFU_CONTROL_POINT_CHARACTERISTIC, true);
	}
}

void DfuDevice::didAddCharacteristic (
	const std::string &serviceUuid,
	const std::string &characUuid)
{
	if (serviceUuid == PULSE_DFU_SERVICE)
	{
		if (characUuid == PULSE_DFU_CONTROL_POINT_CHARACTERISTIC)
		{
			m_receivedControlPointChar = true;
		}
		else if (characUuid == PULSE_DFU_PACKET_CHARACTERISTIC)
		{
			m_receivedPacketChar = true;
		}
	}
	
	if (m_receivedDfuService &&
		m_receivedControlPointChar &&
		m_receivedPacketChar)
	{
		// Ready to start DFU if we have firmware
		characteristicStartNotifySet (PULSE_DFU_SERVICE, PULSE_DFU_CONTROL_POINT_CHARACTERISTIC, true);
	}
}


void DfuDevice::didUpdateStartNotify (
	const std::string &serviceUuid,
	const std::string &characUuid,
	bool enabled)
{
	if (serviceUuid == PULSE_DFU_SERVICE &&
		characUuid == PULSE_DFU_CONTROL_POINT_CHARACTERISTIC &&
		enabled == true)
	{
		// DFU has progressed. Restart timeout timer.
		m_timeoutTimer.restart ();
		
		initPacketSend ();
	}
}

void DfuDevice::didChangeCharacteristicValue (
	const std::string &serviceUuid,
	const std::string &characUuid,
	const std::vector<uint8_t> &value)
{
	if (serviceUuid == PULSE_DFU_SERVICE)
	{
		if (characUuid == PULSE_DFU_CONTROL_POINT_CHARACTERISTIC)
		{
			// DFU has progressed. Restart timeout timer.
			m_timeoutTimer.restart ();
			
			controlPointResponseReceived (value);
		}
	}
}


void DfuDevice::didWriteCharacteristic (
	const std::string &serviceUuid,
	const std::string &characUuid)
{
	if (serviceUuid == PULSE_DFU_SERVICE &&
		characUuid == PULSE_DFU_PACKET_CHARACTERISTIC)
	{
		dataPacketWriteComplete ();
	}
}

void DfuDevice::packetReceiptNotificationsSet (uint16_t numberOfPackets)
{
	m_packetsToSendBeforeNotification = numberOfPackets;
	
	std::vector<uint8_t> writeData = {OP_CODE_PACKET_RECEIPT_NOTIF_REQ};
	writeData.push_back(numberOfPackets & 0xff);
	writeData.push_back((numberOfPackets >> 8) & 0xff);
	opCodeWrite(writeData);
}

bool DfuDevice::controlPointResponseSuccessCheck (const std::vector<uint8_t> &response)
{
	if (response[2] == NRF_DFU_RES_CODE_SUCCESS)
	{
		return true;
	}
	else if (response[2] == NRF_DFU_RES_CODE_EXT_ERROR)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DfuDevice: controlPointResponseSuccessCheck: Response extended error: 0x%2x\n", response[3]);
		);
	}
	else
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DfuDevice: controlPointResponseSuccessCheck: Response error: 0x%2x\n", response[2]);
		);
	}
	
	return false;
}

void DfuDevice::controlPointResponseReceived (const std::vector<uint8_t> &response)
{
	if (response.size () < 3)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DfuDevice: controlPointResponseReceived: Malformed response\n");
		);
		return;
	}
	
	switch (response[1])
	{
		case OP_CODE_CREATE:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: OP_CODE_CREATE\n");
			);

			if (controlPointResponseSuccessCheck (response))
			{
				dfuDataWrite ();
			}
			break;
		}
		case OP_CODE_PACKET_RECEIPT_NOTIF_REQ:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: OP_CODE_PACKET_RECEIPT_NOTIF_REQ\n");
			);

			if (controlPointResponseSuccessCheck (response))
			{
				if (m_dfuState == DfuState::SendingInitPacket)
				{
					objectSelect (OBJECT_INIT_PACKET);
				}
				else if (m_dfuState == DfuState::SendingFirmware)
				{
					objectSelect (OBJECT_FIRMWARE);
				}
			}
			break;
		}
		case OP_CODE_CALCULATE_CHECKSUM:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: OP_CODE_CALCULATE_CHECKSUM\n");
			);

			if (controlPointResponseSuccessCheck (response))
			{
				if (m_sendProgress.percentCompleteGet () > m_percentComplete)
				{
					m_percentComplete = m_sendProgress.percentCompleteGet ();
					
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DfuDevice: Percent complete: %u\n", m_percentComplete);
					);
					
					if (m_percentComplete >= 100)
					{
						m_percentComplete = 0;
					}
				}
				
				if (m_sendProgress.isObjectComplete ())
				{
					uint32_t crc = objectCrc32Get (response, OP_CODE_CALCULATE_CHECKSUM);
					if (crc == (uint32_t)m_dfuCRC.getCurrentCRC ())
					{
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DfuDevice: Local crc: %u, Remote crc: %u\n", (uint32_t)m_dfuCRC.getCurrentCRC (), crc);
						);

						dfuDataExecute ();
					}
					else
					{
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DfuDevice: Checksums are not equal! Aborting dfu!\n");
						);
						
						return;
					}
				}
				else
				{
					if (m_packetsToSendBeforeNotification &&
						m_numPacketsSentSinceNotification != m_packetsToSendBeforeNotification)
					{
						// We shouldn't get here unless there is a problem
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DfuDevice: Received checksum response when not expected.\n");
						);

						return;
					}
					
					m_numPacketsSentSinceNotification = 0;
					dfuDataWrite ();
				}
			}
			break;
		}
		case OP_CODE_EXECUTE:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: OP_CODE_EXECUTE\n");
			);

			if (controlPointResponseSuccessCheck (response))
			{
				// Reset outgoing packet counter
				m_numPacketsSentSinceNotification = 0;
				
				if (!m_sendProgress.isComplete ())
				{
					m_sendProgress.nextObject ();
					
					if (m_dfuState == DfuState::SendingInitPacket)
					{
						objectCreate (OBJECT_INIT_PACKET, m_sendProgress.currentObjectAvailableLengthGet ());
					}
					else if (m_dfuState == DfuState::SendingFirmware)
					{
						objectCreate (OBJECT_FIRMWARE, m_sendProgress.currentObjectAvailableLengthGet ());
					}
				}
				else
				{
					if (m_dfuState == DfuState::SendingInitPacket)
					{
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DfuDevice: Finished sending and executing init packet. Sending firmware.\n");
						);
						
						m_dfuState = DfuState::SendingFirmware;
						
						packetReceiptNotificationsSet (FIRMWARE_PACKETS_PER_NOTIFICATION);
					}
					else if (m_dfuState == DfuState::SendingFirmware)
					{
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DfuDevice: Completed successfully.\n");
						);
						// TODO: How to handle this???
						// Callback to CstiBluetooth so that it can reconnect to Pulse device?
						m_dfuState = DfuState::Complete;
						m_timeoutTimer.stop ();
					}
				}
			}
			break;

		}
		case OP_CODE_SELECT_OBJECT:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: OP_CODE_SELECT_OBJECT\n");
			);

			if (controlPointResponseSuccessCheck (response))
			{
				m_dfuCRC.prepareCRC ();
				m_maxObjectSize = maxObjectSizeGet (response);
				
				if (m_dfuState == DfuState::SendingInitPacket)
				{
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DfuDevice: Max init packet size = %u\n", m_maxObjectSize);
					);
					
					m_sendProgress.initialize (m_initPacket.size (), m_maxObjectSize);
					objectCreate (OBJECT_INIT_PACKET, m_sendProgress.currentObjectAvailableLengthGet ());
				}
				else if (m_dfuState == DfuState::SendingFirmware)
				{
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DfuDevice: Max firmware page size = %u\n", m_maxObjectSize);
					);
					
					m_sendProgress.initialize (m_firmware.size (), m_maxObjectSize);
					objectCreate (OBJECT_FIRMWARE, m_sendProgress.currentObjectAvailableLengthGet ());
				}
			}
			break;

		}
		default:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DfuDevice: controlPointResponseReceived: Unknown response code!!!\n");
			);
		}
	}
}

void DfuDevice::dataPacketWriteComplete ()
{
	m_sendProgress.bytesSentAdd (m_bytesSent);
	m_dfuCRC.accumulateCRC ((const char*)m_byteVectorSent.data (), m_byteVectorSent.size ());
	m_bytesSent = 0;
	m_byteVectorSent.clear ();
	
	m_numPacketsSentSinceNotification++;
	
	if (m_packetsToSendBeforeNotification &&
		m_numPacketsSentSinceNotification == m_packetsToSendBeforeNotification)
	{
		// Wait for packets received notification before sending more data
		// This notification is received as a checksum response in controlPointResponseReceived
		return;
	}
	
	if (m_sendProgress.isObjectComplete ())
	{
		checksumRequest ();
		return;
	}
	
	dfuDataWrite ();
}

void DfuDevice::initPacketSend ()
{
	m_dfuState = DfuState::SendingInitPacket;
	packetReceiptNotificationsSet (INIT_PACKET_PACKETS_PER_NOTIFICATION);
}

void DfuDevice::objectSelect (uint8_t objectType)
{
	std::vector<uint8_t> writeData = {OP_CODE_SELECT_OBJECT, objectType};
	opCodeWrite (writeData);
}

void DfuDevice::objectCreate (uint8_t objectType, uint32_t size)
{
	std::vector<uint8_t> writeData = {OP_CODE_CREATE, objectType};
	objectSizeSet (writeData, size);
	opCodeWrite (writeData);
}

void DfuDevice::objectSizeSet (std::vector<uint8_t> data, uint32_t size)
{
	data.push_back (size & 0xff);
	data.push_back ((size >> 8) & 0xff);
	data.push_back ((size >> 16) & 0xff);
	data.push_back ((size >> 24) & 0xff);
}

void DfuDevice::opCodeWrite (const std::vector<uint8_t> &data)
{
	characteristicWrite (PULSE_DFU_SERVICE, PULSE_DFU_CONTROL_POINT_CHARACTERISTIC, data);
}

void DfuDevice::dfuDataWrite ()
{
	// Get current index into data that is being send
	std::vector<uint8_t>::iterator currentIndex;
	if (m_dfuState == DfuState::SendingInitPacket)
	{
		currentIndex = m_initPacket.begin ();
	}
	else if (m_dfuState == DfuState::SendingFirmware)
	{
		currentIndex = m_firmware.begin ();
	}
	currentIndex += m_sendProgress.bytesSentGet ();
	
	// Determine how many bytes to send
	m_bytesSent = 20;
	unsigned int bytesRemaining = m_sendProgress.currentObjectAvailableLengthGet ();
	
	if (bytesRemaining < 20)
	{
		m_bytesSent = bytesRemaining;
	}
	
	// Copy data into send vector
	m_byteVectorSent.resize (m_bytesSent);
	std::copy (currentIndex, currentIndex + m_bytesSent, m_byteVectorSent.begin ());
	
	// Write data
	characteristicWrite (PULSE_DFU_SERVICE, PULSE_DFU_PACKET_CHARACTERISTIC, m_byteVectorSent);
}

void DfuDevice::checksumRequest ()
{
	std::vector<uint8_t> writeData = {OP_CODE_CALCULATE_CHECKSUM};
	opCodeWrite (writeData);
}

void DfuDevice::dfuDataExecute ()
{
	std::vector<uint8_t> writeData = {OP_CODE_EXECUTE};
	opCodeWrite (writeData);
}

uint32_t DfuDevice::maxObjectSizeGet (const std::vector<uint8_t> &response)
{
	return  response[3] |
			(response[4] << 8) |
			(response[5] << 16) |
			(response[6] << 24);
}

uint32_t DfuDevice::objectOffsetGet (const std::vector<uint8_t> &response, uint8_t responseType)
{
	uint32_t value = 0;
	
	if (responseType == OP_CODE_CALCULATE_CHECKSUM)
	{
		value = response[3] |
				(response[4] << 8) |
				(response[5] << 16) |
				(response[6] << 24);
	}
	else if (responseType == OP_CODE_SELECT_OBJECT)
	{
		value = response[7] |
				(response[8] << 8) |
				(response[9] << 16) |
				(response[10] << 24);
	}
	
	return value;
}

uint32_t DfuDevice::objectCrc32Get (const std::vector<uint8_t> &response, uint8_t responseType)
{
	uint32_t value = 0;
	
	if (responseType == OP_CODE_CALCULATE_CHECKSUM)
	{
		value = response[7] |
				(response[8] << 8) |
				(response[9] << 16) |
				(response[10] << 24);
	}
	else if (responseType == OP_CODE_SELECT_OBJECT)
	{
		value = response[11] |
				(response[12] << 8) |
				(response[13] << 16) |
				(response[14] << 24);
	}
	
	return value;
}

