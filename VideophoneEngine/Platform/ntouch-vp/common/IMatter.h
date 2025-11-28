/*!
 * \file IMatter.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2024 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

//
// Includes
//
#include "ISignal.h"

class IMatter
{
public:

	IMatter (const IMatter &other) = delete;
	IMatter (IMatter &&other) = delete;
	IMatter &operator= (const IMatter &other) = delete;
	IMatter &operator= (IMatter &&other) = delete;


	/*!
	 * \brief Get instance 
	 */
	static IMatter *InstanceGet ();


	/*!
	 * \brief Parses the devices QR code which is returned through a signal
	 */
	virtual void qrCodeParse (const std::string qrCode) = 0;

	/*!
	 * \brief Commissions the device to the network. The devices assigned ID will be returned through a signal.
	 */
	virtual void deviceCommission (const uint16_t nodeId, 
				       const uint16_t pinCode,
				       const uint16_t discriminator,
				       const std::string ssId,
				       const std::string networkPassWord) = 0;

	/*!
	 * \brief Turns the light on or off based on the state of the lightOn bool
	 */
	virtual void lightOnSet (const uint16_t nodeId,
				 const uint16_t endPointId,
				 const bool lightOn) = 0;

	/*!
	 * \brief Sets the light's color
	 */
	virtual void lightColorSet (const uint16_t nodeId,
				    const uint16_t endPointId,
				    const uint16_t r,
				    const uint16_t g,
  				    const uint16_t b) = 0;

	struct parsedQrCode
	{
		uint8_t version = 0;
		uint16_t vendorId = 0;
		uint16_t productId = 0;

		uint8_t commissionFlow = 0;	  // Stander = 0
														// User Action Required = 1
														// Custom = 2
		bool shortDiscriminator = false;
		uint16_t discriminator = 0;
		
		uint32_t setupPinCode = 0;
	};

public:
	virtual ISignal<IMatter::parsedQrCode>& qrCodeParsedSignalGet () = 0;
	virtual ISignal<>& qrCodeParsedFailedSignalGet () = 0;
	virtual ISignal<uint16_t>& commissionedEndPointIdSignalGet () = 0; 

protected:

	IMatter () = default;
	virtual ~IMatter () = default;

private:

};
