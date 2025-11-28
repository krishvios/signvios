#pragma once
#include "stiSVX.h"

/*!
* \file IVRCLCall.h
* \brief Methods needed to support a VRCL Server call
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/

/*!
 * \brief Methods needed to support a VRCL Server call
 * 
 */
class IVRCLCall
{
public:

	/*!
	 * \brief Get the call state
	 *
	 * \return The current state of the call.
	 */
	virtual EsmiCallState StateGet () const = 0;
	/*!
	 * \brief Call direction
	 *
	 * \return The direction of the call: inbound or outbound
	*/
	virtual EstiDirection DirectionGet () const = 0;
	/*!
	 * \brief Get the call result
	 *
	 * \return The call result or why the call was disconnected
	*/
	virtual EstiCallResult ResultGet () const = 0;
	/*!
	* \brief Get the remote DialString
	*
	* \param remoteDialString The DialString of the remote endpoint.
	*/
	virtual void RemoteDialStringGet (std::string* remoteDialString) const = 0;

	/*!
	 * \brief Get the remote name
	 *
	 * \param remoteName The remote name.  This will be the name from the contact list,
	 * the remote user name or the name from the call history list if the call was dialed from the call history list.
	 */
	virtual void RemoteNameGet (std::string* remoteName) const = 0;

	/*!
	* \brief Get the call Substate
	*
	* \param returns the Substate of the Call Object
	*/
	virtual uint32_t SubstateGet () const = 0;
	/*!
	 * \brief Get the call Statistics
	 *
	 * \param pStats The statistics of the call.
	 */
	virtual void StatisticsGet (
		SstiCallStatistics* pStats) const = 0;
	/*!
	 * \brief What protocol is being used for this call?
	 *
	 * Is this call using SIP or SIPS
	 *
	 * \return The conferening protocol (SIPS or SIP)
	 */
	virtual EstiProtocol ProtocolGet () const = 0;
	/*!
	* \brief Get the remote Product Name
	*
	* \param pRemoteProductName The ProductName/UserAgent of the remote endpoint.
	*/
	virtual void RemoteProductNameGet (std::string* pRemoteProductName) const = 0;
	/*!
	 * \brief Get the remote IP Address
	 */
	virtual std::string RemoteIpAddressGet () const = 0;
	/*!
	 * \brief Get the remote call info
	 *
	 * \return Additional call information.
	 */
	virtual ICallInfo* RemoteCallInfoGet () = 0;
	/*!
	 * \brief Is the call able to be transferred
	 *
	 * \return True if the call is transferable.
	 */
	virtual EstiBool IsTransferableGet () const = 0;
	/*!
	 * \brief Is sending text enabled for this call
	 *
	 * \ return estiTRUE if the remote endpoint has the ability
	 *          to receive text.
	 */
	virtual EstiBool TextSupportedGet () const = 0;
	/*!
	 * \brief Hangs up call
	 *
	 * \return Success or failure result
	 */
	virtual stiHResult HangUp () = 0;
	/*!
	 * \brief Places call on Hold
	 *
	 * \return Success or failure result
	*/
	virtual stiHResult Hold () = 0;
	/*!
	 * \brief Is sending DTMF enabled for this call
	 *
	 * \ return estiTRUE if the remote endpoint has the ability
	 *          to receive DTMF.
	*/
	virtual EstiBool DtmfSupportedGet () const = 0;

	/*!
	 * \brief Send DTMF Tone
	 *
	*/
	virtual void DtmfToneSend (EstiDTMFDigit eDTMFDigit) = 0;
	/*!
	 * \brief Send text to the remote endpoint
	 * \param pun16Text The text to be sent (a null-terminated Unicode character string).
	 * \param sharedTextSource the source of the shared text string.
	 */
	virtual stiHResult TextSend (const uint16_t* pun16Text, EstiSharedTextSource sharedTextSource) = 0;

	/*!
	 * \brief Resumes call from hold
	 *
	 * \return Success or failure result
	*/
	virtual stiHResult Resume () = 0;

	virtual stiHResult BridgeCreate (const std::string& uri) = 0;
	virtual stiHResult BridgeDisconnect () = 0;
	virtual stiHResult BridgeStateGet (EsmiCallState* peCallState) = 0;
	virtual void BridgeCompletionNotifiedSet (bool completed) = 0;
	virtual bool BridgeCompletionNotifiedGet () = 0;

	virtual stiHResult inline TransferToAddress (const std::string& address) = 0;
	virtual const char* RemoteMacAddressGet () = 0;
	virtual const char* RemoteVcoCallbackGet () = 0;
	virtual stiHResult BandwidthDetectionBegin () = 0;
	virtual void RemoteProductVersionGet (std::string* pRemoteProductVersion) = 0;
	virtual EstiBool VerifyAddressGet () const = 0;
	virtual EstiVcoType RemoteVcoTypeGet () = 0;
	virtual EstiBool RemoteIsVcoActiveGet () = 0;

	virtual std::string TrsUserIPGet () const = 0;
	/*!
	 * \brief Returns whether the SignMailBox is full or not.
	 *
	 * \return true if SignMailBox is full otherwise false
	 */
	virtual bool signMailBoxFullGet () const = 0;

	/*!
	 * \brief Retrieves whether or not SignMail has been initiated.
	 *
	 * \return True if SignMail has been initiated.  Otherwise, false.
	 */
	virtual bool SignMailInitiatedGet () const = 0;
	/*!
	 * \return Are we leaving a message
	*/
	virtual bool LeavingMessageGet () const = 0;
	/*!
	 * \return Has the Greeting Started
	*/
	virtual bool GreetingStartingGet () const = 0;

	/*!
 * \return Has the SignMail Completed
*/
	virtual bool SignMailCompletedGet () const = 0;

};

using VRCLCallSP = std::shared_ptr<IVRCLCall>;