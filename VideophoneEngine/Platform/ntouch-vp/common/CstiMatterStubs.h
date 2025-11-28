/*!
*  \file CstiMatterStubs.h
*  \brief The Matter Stubs Interface
*
*  Class declaration for the Matter Stubs Interface Class.  
*   This class is responsible for communicating with Smart Home devices
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
*
*/
#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "IMatter.h"
#include "CstiSignal.h"
#include "CstiEventQueue.h"

//
// Constants
//

//
// Forward Declarations
//

//
// Globals
//

/*!
 * \brief 
 *  
 *  Class declaration for the MatterStubs Class.  
 *	This class is just stubs to be used on platforms that don't support matter.
 *  
 */
class CstiMatterStubs : public IMatter 
{
public:
	
	/*!
	 * \brief CstiMatterStubs constructor
	 */
	CstiMatterStubs ();
	
	CstiMatterStubs (const CstiMatterStubs &other) = delete;
	CstiMatterStubs (CstiMatterStubs &&other) = delete;
	CstiMatterStubs &operator= (const CstiMatterStubs &other) = delete;
	CstiMatterStubs &operator= (CstiMatterStubs &&other) = delete;

	/*!
	 * \brief destructor
	 */
	~CstiMatterStubs () override;

	/*!
	 * \brief Get instance 
	 */
	static IMatter *InstanceGet ();

	/*!
	 * \brief Parses the devices QR code not implemented and signals an error
	 */
	void qrCodeParse (const std::string qrCode) override;

	/*!
	 * \brief Commissions the device to the network, not implemented and signals an error  
	 */
	void deviceCommission (
		const uint16_t nodeId, 
		const uint16_t pinCode,
		const uint16_t discriminator,
		const std::string ssId,
		const std::string networkPassWord) override;

	/*!
	 * \brief Turns the light on or off based on the state of the lightOn bool 
	 */
	void lightOnSet (
		const uint16_t nodeId,
		const uint16_t endPointId,
		const bool lightOn) override;

	/*!
	 * \brief Sets the light's color
	 */
	void lightColorSet (
		const uint16_t nodeId,
		const uint16_t endPointId,
		const uint16_t r,
		const uint16_t g,
  		const uint16_t b) override;


public:
	ISignal<IMatter::parsedQrCode>& qrCodeParsedSignalGet () override;
	ISignal<>& qrCodeParsedFailedSignalGet () override;
	ISignal<uint16_t>& commissionedEndPointIdSignalGet () override; 

	CstiSignal<IMatter::parsedQrCode> qrCodeParsedSignal;
	CstiSignal<> qrCodeParsedFailedSignal;
	CstiSignal<uint16_t> commissionedEndPointIdSignal;

private:

};

