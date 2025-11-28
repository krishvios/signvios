/*!
*  \file CstiMatterStubs.cpp
*  \brief Matter Interface
*
*  Class declaration for the Matter Class.  
*   This class is responsible for communicating with Matter and smart home devices.
*   
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
*
*/
//
// Includes
//
#include "CstiMatterStubs.h"
#include "stiTools.h"


//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//

//
// Constructor
//
/*!
 * \brief  Constructor
 * 
 */
CstiMatterStubs::CstiMatterStubs ()
{

}

//
// Destructor
//
/*!
 * \brief Destructor
 */
CstiMatterStubs::~CstiMatterStubs ()
{

}

/*!
 * \brief Parses the devices QR code which is returned through a signal
 */
void CstiMatterStubs::qrCodeParse (
	const std::string qrCode)
{
	qrCodeParsedFailedSignal.Emit ();
}

	/*!
	 * \brief Commissions the device to the network. The devices assigned ID will be returned through a signal.
	 */
void CstiMatterStubs::deviceCommission (
	const uint16_t nodeId, 
	const uint16_t pinCode,
	const uint16_t discriminator,
	const std::string ssId,
	const std::string networkPassWord)
{

}

    /*!
	 * \brief Turns the light on or off based on the state of the lightOn bool
	 */
void CstiMatterStubs::lightOnSet (
	const uint16_t nodeId,
	const uint16_t endPointId,
	const bool lightOn)
{

}

	/*!
	 * \brief Sets the light's color
	 */
void CstiMatterStubs::lightColorSet (
	const uint16_t nodeId,
	const uint16_t endPointId,
	const uint16_t r,
	const uint16_t g,
  	const uint16_t b)
{

}


ISignal<IMatter::parsedQrCode>& CstiMatterStubs::qrCodeParsedSignalGet ()
{
	return qrCodeParsedSignal;
}

ISignal<>& CstiMatterStubs::qrCodeParsedFailedSignalGet ()
{
	return qrCodeParsedFailedSignal;
}

ISignal<uint16_t>& CstiMatterStubs::commissionedEndPointIdSignalGet () 
{
	return commissionedEndPointIdSignal;
}
