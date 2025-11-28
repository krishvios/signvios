// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2018-2020 Sorenson Communications, Inc. -- All rights reserved
//
// The entirety of the dfu process is designed to run on the CstiHelios
// event queue. All timers and signal handlers are to be implemented on
// that thread.

#include "NordicDfu.h"
#include "stiTrace.h"
#include "GattCharacteristicInterface.h"
#ifdef DFU_WITH_PASSIVE_SCAN
	#include "AdapterInterface.h"
#endif
#include <cstdio>
#include <fstream>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#include <fcntl.h>

const std::string NordicDfu::INIT_FILE = "helios_app.dat";
const std::string NordicDfu::FIRMWARE_FILE = "helios_app.bin";

const std::string NordicDfu::BUTTONLESS_DFU_CHAR_UUID = "8ec90003-f315-4f60-9fb8-838830daea50";
const std::string NordicDfu::DFU_CONTROL_POINT_CHAR_UUID = "8ec90001-f315-4f60-9fb8-838830daea50";
const std::string NordicDfu::DFU_PACKET_CHAR_UUID = "8ec90002-f315-4f60-9fb8-838830daea50";

#define CONNECT_DELAY 2000 // two seconds
#define DFU_TIMEOUT  10000 // ten seconds
#define MAX_CONNECT_TO_TARGET_ATTEMPTS 3
#define MAX_RECONNECT_TO_TARGET_ATTEMPTS 5

/*!
 * \brief create a new dfu object
 */
NordicDfu::NordicDfu (CstiEventQueue *eventQueue)
:
	m_pEventQueue (eventQueue),
	m_connectTimer (CONNECT_DELAY, eventQueue),
	m_timeoutTimer (DFU_TIMEOUT, eventQueue)
#ifdef DFU_WITH_PASSIVE_SCAN
	,
	m_connectNewWithoutDiscoveryTimer (CONNECT_DELAY, eventQueue)
#endif
{
	m_signalConnections.push_back (m_connectTimer.timeoutSignal.Connect (
		[this]{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Connecting to device: %s\n", m_dfuTargetAddress.c_str ());
			);

			m_connectAttempts++;
			m_pCstiBluetooth->deviceConnectByAddress (m_dfuTargetAddress);
		}));

	m_signalConnections.push_back (m_timeoutTimer.timeoutSignal.Connect (
		[this]{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Timeout error on device: %s\n", m_dfuTargetAddress.c_str ());
			);

			cleanUp ();
		}));
#ifdef DFU_WITH_PASSIVE_SCAN
	m_signalConnections.push_back (m_connectNewWithoutDiscoveryTimer.timeoutSignal.Connect (
		[this]{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Target connect timer: %s\n", m_dfuTargetAddress.c_str ());
			);

			m_connectNewWithoutDiscoveryAttempts++;
			m_pCstiBluetooth->deviceConnectNewWithoutDiscovery (m_dfuTargetAddress, "random");
		}));
#endif
}


stiHResult NordicDfu::initiate (std::shared_ptr<SstiBluezDevice> device, const std::string& zipFilePath, CstiBluetooth *pCstiBluetooth)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_dfuState != DfuState::None)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DFU: Unable to start dfu. Dfu is in progress. State: %d\n", m_dfuState);
		);

		return stiRESULT_DFU_IN_PROGRESS;
	}

	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("DFU: Starting dfu, device: %s, file: %s\n", device->bluetoothDevice.address.c_str (), zipFilePath.c_str ());
	);

	m_pButtonlessDfuDevice = device;
	m_pCstiBluetooth = pCstiBluetooth;

	connectSignals ();

	hResult = dfuAddressesDetermine ();
	stiTESTRESULT ();
	
	hResult = firmwareGet (zipFilePath);
	stiTESTRESULT ();

	restartRemoteDeviceInDfuMode ();

	// Start timeout timer
	m_timeoutTimer.restart ();

STI_BAIL:

	if (hResult != stiRESULT_SUCCESS)
	{
		cleanUp ();
	}

	return hResult;
}


stiHResult NordicDfu::firmwareGet (const std::string& zipFile)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string unzipCommand;

	auto tempPath = (boost::filesystem::temp_directory_path () / boost::filesystem::unique_path ()).native ();

	if (mkdir (tempPath.c_str(), S_IRWXU) == -1)
	{
		stiTHROWMSG (stiRESULT_ERROR, "DFU: Could not create unzip directory at : %s\n", tempPath.c_str());
	}

	unzipCommand = "unzip -qq " + zipFile + " -d " + tempPath + " > /dev/null 2>&1";
	if (system (unzipCommand.c_str ()))
	{
		stiTHROWMSG (stiRESULT_ERROR, "DFU: Failed to unzip helios firmware.\n");
	}

	{
		std::ifstream initFile (tempPath + '/' + INIT_FILE, std::ios::binary | std::ios::ate);
		if (initFile)
		{
			std::streamsize fileSize = initFile.tellg ();
			initFile.seekg (0, std::ios::beg);

			m_initPacket.resize (fileSize);
			initFile.read (reinterpret_cast<char *>(m_initPacket.data ()), fileSize); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

			if (!initFile)
			{
				stiTHROWMSG (stiRESULT_ERROR, "DFU: There was an error reading the init file.\n");
			}
		}
		else
		{
			stiTHROWMSG (stiRESULT_ERROR, "DFU: Init packet, %s, is not available.\n", INIT_FILE.c_str ());
		}

		std::ifstream firmwareFile (tempPath + '/' + FIRMWARE_FILE, std::ios::binary | std::ios::ate);
		if (firmwareFile)
		{
			std::streamsize fileSize = firmwareFile.tellg ();
			firmwareFile.seekg (0, std::ios::beg);

			m_firmware.resize (fileSize);
			firmwareFile.read (reinterpret_cast<char *>(m_firmware.data ()), fileSize); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

			if (!firmwareFile)
			{
				stiTHROWMSG (stiRESULT_ERROR, "DFU: There was an error reading the firmware file.\n");
			}
		}
		else
		{
			stiTHROWMSG (stiRESULT_ERROR, "DFU: Firmware, %s, is not available.\n", FIRMWARE_FILE.c_str ());
		}
	}

STI_BAIL:

	boost::filesystem::remove_all (tempPath);

	return hResult;
}


void NordicDfu::firmwareClear ()
{
	m_initPacket.clear ();
	m_firmware.clear ();
}


stiHResult NordicDfu::dfuAddressesDetermine ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// The dfu target address is the same as the buttonless dfu
	// address except that the last octet is one greater.
	// Calculate the dfu target address and save both.
	m_buttonlessDfuAddress = m_pButtonlessDfuDevice->bluetoothDevice.address;
	m_dfuTargetAddress = m_buttonlessDfuAddress;

	stiTESTCOND (m_dfuTargetAddress.length () == MAC_ADDR_LENGTH, stiRESULT_ERROR);

	{
		// Get last octet and add 1
		uint8_t lastOctet = (uint8_t)std::stoi (m_dfuTargetAddress.substr (MAC_ADDR_LAST_OCTET, OCTET_CHARACTERS), nullptr, HEXADECIMAL) + 1;
		char lastOctetString[3];
		sprintf (lastOctetString, "%02X", lastOctet);

		// Replace the last octet with the newly calculated one
		m_dfuTargetAddress.replace (MAC_ADDR_LAST_OCTET, OCTET_CHARACTERS, lastOctetString);

		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Helios address: %s, DfuPulse address: %s\n",
						m_buttonlessDfuAddress.c_str (), m_dfuTargetAddress.c_str ());
		);
	}

STI_BAIL:

	return hResult;
}


void NordicDfu::dfuAddressesClear ()
{
	m_buttonlessDfuAddress.clear ();
	m_dfuTargetAddress.clear ();
}


void NordicDfu::restartRemoteDeviceInDfuMode ()
{
	m_dfuState = DfuState::RestartingRemoteDevice;

	// Start notifications on the characteristic
	m_pCstiBluetooth->gattCharacteristicNotifySet (m_pButtonlessDfuDevice, BUTTONLESS_DFU_CHAR_UUID, true);
}


void NordicDfu::characteristicStartNotifyComplete (
		const std::string& deviceAddress,
		const std::string& characteristicUuid)
{
	if (m_dfuState == DfuState::RestartingRemoteDevice &&
		deviceAddress == m_buttonlessDfuAddress &&
		characteristicUuid == BUTTONLESS_DFU_CHAR_UUID)
	{
		// Dfu has progressed. Restart timeout timer
		m_timeoutTimer.restart ();

		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DFU: Requesting remote device to restart in dfu mode\n");
		);

		// Write 1 to the characteristic to restart the device
		const char writeValue[1] = {1};
		m_pCstiBluetooth->gattCharacteristicWrite (
											m_pButtonlessDfuDevice,
											BUTTONLESS_DFU_CHAR_UUID,
											writeValue,
											1);
	}
	else if (m_dfuState == DfuState::SendingInitPacket &&
			 deviceAddress == m_dfuTargetAddress &&
			 characteristicUuid == DFU_CONTROL_POINT_CHAR_UUID)
	{
		// Dfu has progressed. Restart timeout timer
		m_timeoutTimer.restart ();

		stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
			stiTrace ("DFU: Writing to control point characteristic to set packet receipt notifications\n");
		);

		packetReceiptNotificationsSet (INIT_PACKET_PACKETS_PER_NOTIFICATION);
	}
}


void NordicDfu::packetReceiptNotificationsSet (uint16_t numberOfPackets)
{
	m_packetsToSendBeforeNotification = numberOfPackets;

	std::vector<uint8_t> writeData = {OP_CODE_PACKET_RECEIPT_NOTIF_REQ};
	writeData.push_back (numberOfPackets & 0xff);
	writeData.push_back ((numberOfPackets >> 8) & 0xff);
	opCodeWrite (writeData);
}

// Taken from Nordic device code. FIle : sdk/components/libraries/bootloader/dfu/nfr_dfu_handling_error.h
/**@brief DFU request result codes.
 *
 * @details The DFU transport layer creates request events of type @ref nrf_dfu_req_op_t. Such events return one of these result codes.
 */
typedef enum
{
    NRF_DFU_RES_CODE_INVALID                 = 0x00,     /**< Invalid opcode. */
    NRF_DFU_RES_CODE_SUCCESS                 = 0x01,     /**< Operation successful. */
    NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED   = 0x02,     /**< Opcode not supported. */
    NRF_DFU_RES_CODE_INVALID_PARAMETER       = 0x03,     /**< Missing or invalid parameter value. */
    NRF_DFU_RES_CODE_INSUFFICIENT_RESOURCES  = 0x04,     /**< Not enough memory for the data object. */
    NRF_DFU_RES_CODE_INVALID_OBJECT          = 0x05,     /**< Data object does not match the firmware and hardware requirements, the signature is wrong, or parsing the command failed. */
    NRF_DFU_RES_CODE_UNSUPPORTED_TYPE        = 0x07,     /**< Not a valid object type for a Create request. */
    NRF_DFU_RES_CODE_OPERATION_NOT_PERMITTED = 0x08,     /**< The state of the DFU process does not allow this operation. */
    NRF_DFU_RES_CODE_OPERATION_FAILED        = 0x0A,     /**< Operation failed. */
    NRF_DFU_RES_CODE_EXT_ERROR               = 0x0B,     /**< Extended error. The next byte of the response contains the error code of the extended error (see @ref nrf_dfu_ext_error_code_t). */
} nrf_dfu_res_code_t;

/**@brief DFU request extended result codes.
 *
 * @details When an event returns @ref NRF_DFU_RES_CODE_EXT_ERROR, it also stores an extended error code.
 *          The transport layer can then send the extended error code together with the error code to give
 *          the controller additional information about the cause of the error.
 */
typedef enum
{
    NRF_DFU_EXT_ERROR_NO_ERROR                  = 0x00, /**< No extended error code has been set. This error indicates an implementation problem. */
    NRF_DFU_EXT_ERROR_INVALID_ERROR_CODE        = 0x01, /**< Invalid error code. This error code should never be used outside of development. */
    NRF_DFU_EXT_ERROR_WRONG_COMMAND_FORMAT      = 0x02, /**< The format of the command was incorrect. This error code is not used in the
                                                             current implementation, because @ref NRF_DFU_RES_CODE_OP_CODE_NOT_SUPPORTED
                                                             and @ref NRF_DFU_RES_CODE_INVALID_PARAMETER cover all
                                                             possible format errors. */
    NRF_DFU_EXT_ERROR_UNKNOWN_COMMAND           = 0x03, /**< The command was successfully parsed, but it is not supported or unknown. */
    NRF_DFU_EXT_ERROR_INIT_COMMAND_INVALID      = 0x04, /**< The init command is invalid. The init packet either has
                                                             an invalid update type or it is missing required fields for the update type
                                                             (for example, the init packet for a SoftDevice update is missing the SoftDevice size field). */
    NRF_DFU_EXT_ERROR_FW_VERSION_FAILURE        = 0x05, /**< The firmware version is too low. For an application, the version must be greater than
                                                             the current application. For a bootloader, it must be greater than or equal
                                                             to the current version. This requirement prevents downgrade attacks.*/
    NRF_DFU_EXT_ERROR_HW_VERSION_FAILURE        = 0x06, /**< The hardware version of the device does not match the required
                                                             hardware version for the update. */
    NRF_DFU_EXT_ERROR_SD_VERSION_FAILURE        = 0x07, /**< The array of supported SoftDevices for the update does not contain
                                                             the FWID of the current SoftDevice. */
    NRF_DFU_EXT_ERROR_SIGNATURE_MISSING         = 0x08, /**< The init packet does not contain a signature. This error code is not used in the
                                                             current implementation, because init packets without a signature
                                                             are regarded as invalid. */
    NRF_DFU_EXT_ERROR_WRONG_HASH_TYPE           = 0x09, /**< The hash type that is specified by the init packet is not supported by the DFU bootloader. */
    NRF_DFU_EXT_ERROR_HASH_FAILED               = 0x0A, /**< The hash of the firmware image cannot be calculated. */
    NRF_DFU_EXT_ERROR_WRONG_SIGNATURE_TYPE      = 0x0B, /**< The type of the signature is unknown or not supported by the DFU bootloader. */
    NRF_DFU_EXT_ERROR_VERIFICATION_FAILED       = 0x0C, /**< The hash of the received firmware image does not match the hash in the init packet. */
    NRF_DFU_EXT_ERROR_INSUFFICIENT_SPACE        = 0x0D, /**< The available space on the device is insufficient to hold the firmware. */
} nrf_dfu_ext_error_code_t;

bool NordicDfu::controlPointResponseSuccessCheck (
	std::vector<uint8_t> response)
{
	bool retValue = false;
	
	if (response[2] == NRF_DFU_RES_CODE_SUCCESS)
	{
		retValue = true;
	}
	else
	{
		if (response[2] == NRF_DFU_RES_CODE_EXT_ERROR)
		{
			// An error occurred
			stiASSERTMSG (false, "controlPointResponseSuccessCheck: Response extended error: 0x%02x\n", response[3]);
		}
		else
		{
			// An error occurred
			stiASSERTMSG (false, "controlPointResponseSuccessCheck: Response error: 0x%02x\n", response[2]);
		}
	}
	
	return retValue;
}

void NordicDfu::controlPointResponseReceived (
	const std::vector<uint8_t> &response)
{
	if (response.size () < 3)
	{
		// Malformed response
		stiASSERTMSG (false, "DFU: Malformed response\n");
		return;
	}

	switch (response[1])
	{
		case OP_CODE_CREATE:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("controlPointResponseReceived: OP_CODE_CREATE\n");
			);
			
			if (controlPointResponseSuccessCheck (response))
			{
				stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("DFU: Create request completed successfully\n");
				);

				dfuDataWrite ();
			}
		}
		break;
		
		case OP_CODE_PACKET_RECEIPT_NOTIF_REQ:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("controlPointResponseReceived: OP_CODE_PACKET_RECEIPT_NOTIF_REQ\n");
			);
			
			if (controlPointResponseSuccessCheck (response))
			{
				stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("DFU: Packet receipt notification request completed successfully\n");
				);

				if (m_dfuState == DfuState::SendingInitPacket)
				{
					objectSelect (OBJECT_INIT_PACKET);
				}
				else if (m_dfuState == DfuState::SendingFirmware)
				{
					objectSelect (OBJECT_FIRMWARE);
				}
			}
		}
		break;
		
		case OP_CODE_CALCULATE_CHECKSUM:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
				stiTrace ("controlPointResponseReceived: OP_CODE_CALCULATE_CHECKSUM\n");
			);
		
			if (controlPointResponseSuccessCheck (response))
			{
				stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("DFU: Checksum request completed successfully\n");
				);

				if (m_sendProgress.percentCompleteGet () > m_percentComplete)
				{
					m_percentComplete = m_sendProgress.percentCompleteGet ();
					
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DFU: Percent complete: %u\n", m_percentComplete);
					);

					if (m_percentComplete >= 100)
					{
						// Reset counter
						m_percentComplete = 0;
					}
				}


				if (m_sendProgress.isObjectComplete ())
				{
					// This response is from a checksum request before executing the object
					uint32_t crc = objectCrc32Get (response, OP_CODE_CALCULATE_CHECKSUM);
					if (crc == (uint32_t)m_dfuCRC.getCurrentCRC ())
					{
						stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
							stiTrace ("DFU: Local crc: %u, Remote crc: %u\n", (uint32_t)m_dfuCRC.getCurrentCRC (), crc);
						);

						dfuDataExecute ();
					}
					else
					{
						stiASSERTMSG (false, "DFU: Checksums are not equal! Aborting dfu!\n");

						cleanUp ();
						return;
					}
				}
				else
				{
					// This is a notification of packets received
					if (m_packetsToSendBeforeNotification &&
							m_numPacketsSentSinceNotification != m_packetsToSendBeforeNotification)
					{
						// We shouldn't get here unless there is a problem
						stiDEBUG_TOOL(g_stiHeliosDfuDebug,
							stiTrace ("DFU: Received checksum response when not expected!!!\n");
						);

						cleanUp ();
						return;
					}

					m_numPacketsSentSinceNotification = 0;
					dfuDataWrite ();
				}
			}
		}
		break;

		case OP_CODE_EXECUTE:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
				stiTrace ("controlPointResponseReceived: OP_CODE_EXECUTE\n");
			);
		
			if (controlPointResponseSuccessCheck (response))
			{
				stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
					stiTrace ("DFU: Execute request completed successfully\n");
				);

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
							stiTrace ("DFU: Finished sending and executing init packet! Sending firmware.\n");
						);

						m_dfuState = DfuState::SendingFirmware;

						packetReceiptNotificationsSet (FIRMWARE_PACKETS_PER_NOTIFICATION);
					}
					else if (m_dfuState == DfuState::SendingFirmware)
					{
						stiTrace ("DFU: Completed successfully.\n");

						m_dfuState = DfuState::Complete;
						
						cleanUp ();
					}

				}
			}
		}
		break;

		case OP_CODE_SELECT_OBJECT:
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("controlPointResponseReceived: OP_CODE_EXECUTE\n");
			);
		
			if (controlPointResponseSuccessCheck (response))
			{
				stiDEBUG_TOOL(g_stiHeliosDfuDebug,
					stiTrace ("DFU: Select object completed successfully\n");
				);

				m_dfuCRC.prepareCRC ();
				m_maxObjectSize = maxObjectSizeGet (response);

				if (m_dfuState == DfuState::SendingInitPacket)
				{
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DFU: Max init packet size = %u\n", m_maxObjectSize);
					);

					m_sendProgress.initialize (m_initPacket.size (), m_maxObjectSize);
					objectCreate (OBJECT_INIT_PACKET, m_sendProgress.currentObjectAvailableLengthGet ());
				}
				else if (m_dfuState == DfuState::SendingFirmware)
				{
					stiDEBUG_TOOL(g_stiHeliosDfuDebug,
						stiTrace ("DFU: Max firmware page size = %u\n", m_maxObjectSize);
					);

					m_sendProgress.initialize (m_firmware.size (), m_maxObjectSize);
					objectCreate (OBJECT_FIRMWARE, m_sendProgress.currentObjectAvailableLengthGet ());
				}
			}
		}
		break;

		default:
			stiASSERTMSG (false, "DFU: Unknown response code!!!\n");
			return;
	}
}


void NordicDfu::dataPacketWriteComplete ()
{
	m_sendProgress.bytesSentAdd (m_bytesSent);
	m_dfuCRC.accumulateCRC (reinterpret_cast<const char*>(m_byteVectorSent.data ()), m_byteVectorSent.size ()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	m_bytesSent = 0;
	m_byteVectorSent.clear ();

	m_numPacketsSentSinceNotification++;

	if (m_packetsToSendBeforeNotification &&
	    m_numPacketsSentSinceNotification == m_packetsToSendBeforeNotification)
	{
		// Wait for packets received notification before sending more data
		// This notification is received as a checksum response in InterfacePropertyChangedCallback
		return;
	}

	if (m_sendProgress.isObjectComplete ())
	{
		checksumRequest ();
		return;
	}

	dfuDataWrite ();
}


void NordicDfu::initPacketSend ()
{
	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("DFU: Sending init packet\n");
	);

	m_dfuState = DfuState::SendingInitPacket;

	// Start notifications on the characteristic
	m_pCstiBluetooth->gattCharacteristicNotifySet (m_pDfuTargetDevice, DFU_CONTROL_POINT_CHAR_UUID, true);
}


void NordicDfu::objectSelect (uint8_t objectType)
{
	std::vector<uint8_t> writeData = {OP_CODE_SELECT_OBJECT, objectType};
	opCodeWrite (writeData);
}


void NordicDfu::objectCreate (uint8_t objectType, uint32_t size)
{
	std::vector<uint8_t> writeData = {OP_CODE_CREATE, objectType};
	objectSizeSet (writeData, size);
	opCodeWrite (writeData);
}


void NordicDfu::objectSizeSet (std::vector<uint8_t>& data, uint32_t size)
{
	data.push_back (size & 0xff);
	data.push_back ((size >>  8) & 0xff);
	data.push_back ((size >> 16) & 0xff);
	data.push_back ((size >> 24) & 0xff);
}


void NordicDfu::opCodeWrite (const std::vector<uint8_t>& data)
{
	m_pCstiBluetooth->gattCharacteristicWrite (
										m_pDfuTargetDevice,
										DFU_CONTROL_POINT_CHAR_UUID,
										reinterpret_cast<const char*>(data.data ()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
										data.size ());
}


void NordicDfu::dfuDataWrite ()
{
	// Get current index into data that is being sent
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

	// Send data
	m_pCstiBluetooth->gattCharacteristicWrite (
										m_pDfuTargetDevice,
										DFU_PACKET_CHAR_UUID,
										reinterpret_cast<const char*>(m_byteVectorSent.data ()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
										m_byteVectorSent.size ());
}


void NordicDfu::checksumRequest ()
{
	std::vector<uint8_t> writeData = {OP_CODE_CALCULATE_CHECKSUM};
	opCodeWrite (writeData); 
}


void NordicDfu::dfuDataExecute ()
{
	std::vector<uint8_t> writeData = {OP_CODE_EXECUTE};
	opCodeWrite (writeData); 
}


uint32_t NordicDfu::maxObjectSizeGet (const std::vector<uint8_t> &response)
{
	return	response[3] |
		(response[4]<<8) |
		(response[5]<<16) |
		(response[6]<<24);
}


uint32_t NordicDfu::objectOffsetGet (const std::vector<uint8_t> &response, uint8_t responseType)
{
	uint32_t value = 0;

	if (responseType == OP_CODE_CALCULATE_CHECKSUM)
	{
		value =	response[3] | 
			(response[4] << 8) |
			(response[5] << 16) |
			(response[6] << 24);
	}

	if (responseType == OP_CODE_SELECT_OBJECT)
	{
		value =	response[7] | 
			(response[8] << 8) |
			(response[9] << 16) |
			(response[10] << 24);
	}

	return value;
}


uint32_t NordicDfu::objectCrc32Get (const std::vector<uint8_t> &response, uint8_t responseType)
{
	uint32_t value = 0;

	if (responseType == OP_CODE_CALCULATE_CHECKSUM)
	{
		value =	response[7] | 
			(response[8] << 8) |
			(response[9] << 16) |
			(response[10] << 24);
	}

	if (responseType == OP_CODE_SELECT_OBJECT)
	{
		value =	response[11] | 
			(response[12] << 8) |
			(response[13] << 16) |
			(response[14] << 24);
	}

	return value;
}


void NordicDfu::connectSignals ()
{
	if (!deviceAddedSignalConnection)
	{
		deviceAddedSignalConnection = m_pCstiBluetooth->deviceAddedSignal.Connect (
			[this](BluetoothDevice device){
				m_pEventQueue->PostEvent ([this, device]{
						eventDeviceAdded (device);
					});
			});
	}

	if (!deviceConnectedSignalConnection)
	{
		deviceConnectedSignalConnection = m_pCstiBluetooth->deviceConnectedSignal.Connect (
			[this](BluetoothDevice device){
				m_pEventQueue->PostEvent ([this, device]{
						eventDeviceConnected (device);
					});
			});
	}

	if (!deviceDisconnectedSignalConnection)
	{
		deviceDisconnectedSignalConnection = m_pCstiBluetooth->deviceDisconnectedSignal.Connect (
			[this](BluetoothDevice device){
				m_pEventQueue->PostEvent ([this, device]{
						eventDeviceDisconnected (device);
					});
			});
	}

	if (!gattValueChangedSignalConnection)
	{
		gattValueChangedSignalConnection = SstiGattCharacteristic::valueChangedSignal.Connect (
			[this](std::shared_ptr<SstiBluezDevice> device, std::string uuid, const std::vector<uint8_t> &value)
			{
				m_pEventQueue->PostEvent ([this, device, uuid, value]{
												eventGattValueChanged (device->bluetoothDevice.address, uuid, value);
											});
		});
	}

	if (!gattReadWriteCompletedSignalConnection)
	{
		gattReadWriteCompletedSignalConnection = SstiGattCharacteristic::readWriteCompletedSignal.Connect (
			[this](std::shared_ptr<SstiBluezDevice> device, std::string uuid)
			{
				m_pEventQueue->PostEvent ([this, device, uuid]{
												eventGattReadWriteCompleted (device, uuid);
											});
			});
	}

	if (!gattReadWriteFailedSignalConnection)
	{
		gattReadWriteFailedSignalConnection = SstiGattCharacteristic::readWriteFailedSignal.Connect (
			[this](std::string uuid)
			{
				m_pEventQueue->PostEvent ([this]{
												eventGattReadWriteFailed ();
											});
			});
	}

	if (!gattStartNotifyCompletedSignalConnection)
	{
		gattStartNotifyCompletedSignalConnection = SstiGattCharacteristic::startNotifyCompletedSignal.Connect (
			[this](std::string deviceAddress, std::string uuid)
			{
				m_pEventQueue->PostEvent ([this, deviceAddress, uuid]{
												eventGattStartNotifyCompleted (deviceAddress, uuid);
											});
			});
	}

	if (!gattStartNotifyFailedSignalConnection)
	{
		gattStartNotifyFailedSignalConnection = SstiGattCharacteristic::startNotifyFailedSignal.Connect (
			[this]()
			{
				m_pEventQueue->PostEvent ([this]{
												eventGattStartNotifyFailed ();
											});
			});
	}
#ifdef DFU_WITH_PASSIVE_SCAN
	if (!connectDeviceSucceededSignalConnection)
	{
		connectDeviceSucceededSignalConnection = AdapterInterface::connectDeviceSucceededSignal.Connect (
			[this](std::string deviceAddress)
			{
				m_pEventQueue->PostEvent ([this, deviceAddress]{
												if (deviceAddress == m_dfuTargetAddress)
												{
													stiDEBUG_TOOL(g_stiHeliosDfuDebug,
														stiTrace ("DFU: Received signal that device %s connected\n", deviceAddress.c_str ());
													);

													// Dfu has progressed. Restart timeout timer
													m_timeoutTimer.restart ();
												}
											});
			});
	}

	if (!connectDeviceFailedSignalConnection)
	{
		connectDeviceFailedSignalConnection = AdapterInterface::connectDeviceFailedSignal.Connect (
			[this](std::string deviceAddress)
			{
				m_pEventQueue->PostEvent ([this, deviceAddress]{
												if (deviceAddress == m_dfuTargetAddress &&
													m_connectNewWithoutDiscoveryAttempts < MAX_CONNECT_TO_TARGET_ATTEMPTS)
												{
													stiDEBUG_TOOL(g_stiHeliosDfuDebug,
														stiTrace ("DFU: Received signal that device %s failed to connect\n", deviceAddress.c_str ());
													);

													// Dfu has progressed. Restart timeout timer
													m_timeoutTimer.restart ();
													m_connectNewWithoutDiscoveryTimer.restart ();
												}
											});
			});
	}
#endif
}


void NordicDfu::disconnectSignals ()
{
	if (m_pCstiBluetooth)
	{
		if (deviceAddedSignalConnection)
		{
			deviceAddedSignalConnection.Disconnect ();
		}

		if (deviceConnectedSignalConnection)
		{
			deviceConnectedSignalConnection.Disconnect ();
		}

		if (deviceDisconnectedSignalConnection)
		{
			deviceDisconnectedSignalConnection.Disconnect ();
		}

		if (gattValueChangedSignalConnection)
		{
			gattValueChangedSignalConnection.Disconnect ();
		}

		if (gattReadWriteCompletedSignalConnection)
		{
			gattReadWriteCompletedSignalConnection.Disconnect ();
		}

		if (gattReadWriteFailedSignalConnection)
		{
			gattReadWriteFailedSignalConnection.Disconnect ();
		}

		if (gattStartNotifyCompletedSignalConnection)
		{
			gattStartNotifyCompletedSignalConnection.Disconnect ();
		}

		if (gattStartNotifyFailedSignalConnection)
		{
			gattStartNotifyFailedSignalConnection.Disconnect ();
		}
#ifdef DFU_WITH_PASSIVE_SCAN
		if (connectDeviceSucceededSignalConnection)
		{
			connectDeviceSucceededSignalConnection.Disconnect ();
		}

		if (connectDeviceFailedSignalConnection)
		{
			connectDeviceFailedSignalConnection.Disconnect ();
		}
#endif
	}
}


void NordicDfu::eventDeviceAdded (BluetoothDevice device)
{
	auto deviceAddress = device.address;

	if (m_dfuState == DfuState::ScanningForDfuTarget &&
		deviceAddress == m_dfuTargetAddress)
		{
			// Dfu has progressed. Restart timeout timer
			m_timeoutTimer.restart ();
#ifndef DFU_WITH_PASSIVE_SCAN
			m_pCstiBluetooth->ScanCancel();

			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Starting connect timer\n");
			);

			m_connectTimer.restart ();
#endif
		}
}


void NordicDfu::eventDeviceConnected (BluetoothDevice device)
{
	auto deviceAddress = device.address;

	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("DFU: Handling device connected signal: %s\n", deviceAddress.c_str());
	);

	if ((m_dfuState == DfuState::RestartingRemoteDevice ||
		 m_dfuState == DfuState::ScanningForDfuTarget) &&
		 deviceAddress == m_dfuTargetAddress)
	{
		// Dfu has progressed. Restart timeout timer
		m_timeoutTimer.restart ();

		m_pDfuTargetDevice = nullptr;
		m_pDfuTargetDevice = m_pCstiBluetooth->deviceGetByAddress (m_dfuTargetAddress);

		if (m_pDfuTargetDevice)
		{
			initPacketSend ();
		}
	}
}


void NordicDfu::eventDeviceDisconnected (BluetoothDevice device)
{
	auto deviceAddress = device.address;

	stiDEBUG_TOOL(g_stiHeliosDfuDebug,
		stiTrace ("DFU: Handling device disconnected signal: %s\n", deviceAddress.c_str());
	);

	if (m_dfuState == DfuState::RestartingRemoteDevice &&
		deviceAddress == m_buttonlessDfuAddress)
	{
		// Dfu has progressed. Restart timeout timer
		m_timeoutTimer.restart ();

		m_pDfuTargetDevice = nullptr;
		m_pDfuTargetDevice = m_pCstiBluetooth->deviceGetByAddress (m_dfuTargetAddress);

		if (m_pDfuTargetDevice)
		{
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Starting connect timer\n");
			);

			m_connectTimer.restart ();
		}
		else
		{
			m_dfuState = DfuState::ScanningForDfuTarget;

#ifdef DFU_WITH_PASSIVE_SCAN
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Starting connect new without discovery timer\n");
			);

			m_connectNewWithoutDiscoveryTimer.restart ();
#else
			stiDEBUG_TOOL(g_stiHeliosDfuDebug,
				stiTrace ("DFU: Starting scan\n");
			);

			m_pCstiBluetooth->Scan ();
#endif
		}
	}
	else if (deviceAddress == m_dfuTargetAddress &&
			 m_dfuState != DfuState::Complete)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug,
			stiTrace ("DFU: Target device disconnected before dfu was complete!!\n");
		);

		if (m_dfuState == DfuState::RestartingRemoteDevice ||
			m_dfuState == DfuState::ScanningForDfuTarget)
		{
			if (m_connectAttempts < MAX_RECONNECT_TO_TARGET_ATTEMPTS)
			{
				// Dfu has progressed. Restart timeout timer
				m_timeoutTimer.restart ();

				stiDEBUG_TOOL(g_stiHeliosDfuDebug,
					stiTrace ("DFU: Starting connect timer\n");
				);

				m_connectTimer.restart ();

				// return, we don't want to terminate DFU by calling cleanUp
				return;
			}
		}

		cleanUp ();
	}
}


void NordicDfu::eventGattValueChanged (
	const std::string& deviceAddress,
	const std::string& characteristicUuid,
	const std::vector<uint8_t> &value)
{
	if (deviceAddress == m_dfuTargetAddress &&
		characteristicUuid == DFU_CONTROL_POINT_CHAR_UUID)
	{
		// Dfu has progressed. Restart timeout timer
		m_timeoutTimer.restart ();

		controlPointResponseReceived (value);
	}
}


void NordicDfu::eventGattReadWriteCompleted (
	std::shared_ptr<SstiBluezDevice> device,
	const std::string &characteristicUuid)
{
	if (device->bluetoothDevice.address == m_dfuTargetAddress &&
			characteristicUuid == DFU_PACKET_CHAR_UUID)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
			stiTrace ("DFU: Successfully wrote to dfu packet characteristic\n");
		);

		dataPacketWriteComplete ();
	}
	else if (device->bluetoothDevice.address == m_buttonlessDfuAddress &&
			characteristicUuid == BUTTONLESS_DFU_CHAR_UUID)
	{
		stiDEBUG_TOOL(g_stiHeliosDfuDebug > 1,
			stiTrace ("DFU: Successfully wrote to buttonless dfu characteristic\n");
		);
	}
}


void NordicDfu::eventGattReadWriteFailed ()
{
	stiASSERTMSG (false, "DFU: Gatt write failed. Aborting dfu!\n");

	cleanUp ();
}


void NordicDfu::eventGattStartNotifyCompleted (
	const std::string &deviceAddress,
	const std::string &characteristicUuid)
{
	characteristicStartNotifyComplete (deviceAddress, characteristicUuid);
}


void NordicDfu::eventGattStartNotifyFailed ()
{
	stiASSERTMSG (false, "DFU: Start notify failed! Aborting dfu!\n");

	cleanUp ();
}


void NordicDfu::cleanUp ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Stop timeout timer
	m_timeoutTimer.stop ();

	auto deviceAddress = m_buttonlessDfuAddress;
	
	if (m_dfuState != DfuState::Complete)
	{
		hResult = stiRESULT_ERROR;
	}

	// Remove the DfuPulse device from bluez
	if (m_pDfuTargetDevice)
	{
		m_pCstiBluetooth->PairedDeviceRemove (m_pDfuTargetDevice->bluetoothDevice);
	}
	
	m_dfuState = DfuState::None;
	m_pButtonlessDfuDevice = nullptr;
	m_pDfuTargetDevice = nullptr;
	disconnectSignals ();
	dfuAddressesClear ();
	firmwareClear ();
	m_percentComplete = 0;
#ifdef DFU_WITH_PASSIVE_SCAN
	m_connectNewWithoutDiscoveryAttempts = 0;
#endif
	m_connectAttempts = 0;
	
	dfuResultSignal.Emit (hResult, deviceAddress);
	
}
