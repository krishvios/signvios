////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential - Do not distribute
// Copyright 2000-2020 by Sorenson Communications, Inc. All rights reserved.
//
//  Class Name: CstiCall
//
//  File Name:  CstiCall.cpp
//
//  Owner:	  Eugene R. Christensen
//
//  Abstract:
//	  This class implements the methods specific to a Call such as the actual
//	  commands to accept a call, start a call, etc. with the RADVision stack.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiCall.h"
#include "stiTrace.h"
#include "stiTools.h"
#include <cstring>
#include "CstiProtocolManager.h"
#include "CstiSipCall.h"
#include "CstiCallStorage.h"
#include "stiRemoteLogEvent.h"
#include "CstiEventQueue.h"
#include "IstiNetwork.h"
#include "EndpointMonitor.h"

//
// Constants
//
std::string CstiCall::g_szDEFAULT_RELAY_LANGUAGE = RELAY_LANGUAGE_ENGLISH;
const int g_nDEFAULT_RELAY_LANGUAGE = 1;
static const std::string TRS_SIP_PREFIX = "<sip:";
static const std::string TRS_PURPOSE = "purpose=trs-user-ip";

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//
int CstiCall::m_nCallCount = 0;
std::mutex CstiCall::m_callCountMutex{};
int CstiCall::m_nNextCallIndex = 0;
static std::mutex g_CallIndexMutex{};


#ifdef stiHOLDSERVER
int CstiCall::m_nMaxHoldServerCalls = MAX_HOLDSERVER_CALLS;
#endif
//
// Locals
//

//
// Class Definitions
//

//:-----------------------------------------------------------------------------
//:
//:     MEMBER FUNCTIONS
//:
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiCall::CstiCall
//
// Description: Default constructor
//
// Abstract:
//  This method constructs the CstiCall object
//
// Returns:
//  none
//
CstiCall::CstiCall (
	CstiCallStorage *poCallStorage,
	const SstiUserPhoneNumbers &UserPhoneNumbers,
	const std::string &userId,
	const std::string &groupUserId,
	const std::string &localDisplayName,
	const std::string &localAlternateName,
	const std::string &vrsCallId,
	const SstiConferenceParams *pstConferenceParams)
:
	m_stConferenceParams (*pstConferenceParams),
	m_poCallStorage (poCallStorage),
	m_localDisplayName (localDisplayName),
	m_localAlternateName (localAlternateName),
	m_vrsCallId (vrsCallId),
	m_nRemotePreferredLanguage (g_nDEFAULT_RELAY_LANGUAGE),
	m_bLocalCallerIdBlocked (pstConferenceParams->bBlockCallerID)
{
	stiLOG_ENTRY_NAME (CstiCall::CstiCall);

	m_dhvConnectingTimer.TimeoutSet (stiDHV_CONNECTING_TIMEOUT_DEFAULT * MSEC_PER_SEC);

	//
	// Store the call index into this call object
	// and increment it for the next call.
	//
	{
		std::unique_lock<std::mutex> lock(g_CallIndexMutex);
		m_nCallIndex = ++m_nNextCallIndex; 
	}

	// Initialize all the Remote values
	m_szCalledName[0] = '\0';
	m_szRemoteContactName[0] = '\0';
	m_szOriginalRemoteName[0] = '\0';
#ifdef stiHOLDSERVER
	m_AssertedIdentity[0] = '\0';
#endif

	// Initialize local call info
	m_oLocalCallInfo.UserPhoneNumbersSet (&UserPhoneNumbers);
	m_oLocalCallInfo.UserIDSet (userId);
	m_oLocalCallInfo.GroupUserIDSet (groupUserId);

	// Reset the remote Call Info object
	SstiUserPhoneNumbers sUserPhoneNumbers;
	memset (&sUserPhoneNumbers, 0, sizeof (sUserPhoneNumbers));
	m_oRemoteCallInfo.UserPhoneNumbersSet (&sUserPhoneNumbers);

	LocalPreferredLanguageSet (g_szDEFAULT_RELAY_LANGUAGE, g_nDEFAULT_RELAY_LANGUAGE);
#ifdef stiHOLDSERVER
	RemotePreferredLanguageSet (g_szDEFAULT_RELAY_LANGUAGE);
	m_eVcoType = estiVCO_NONE;
	m_bIsVcoActive = estiFALSE;
	m_bIsCallH323Interworked = estiFALSE;
#endif

	{
		std::lock_guard<std::mutex> lock (m_callCountMutex);
		m_nCallCount++;
	}

#ifdef stiHOLDSERVER
	stiASSERT (m_nCallCount <= m_nMaxHoldServerCalls);
#endif //#ifdef stiHOLDSERVER

	time(&m_timeCallStart);
	
	m_signalConnections.push_back (pleaseWaitTimer.timeoutEvent.Connect (
	    [this]{
			CstiProtocolManager *protocolManager = ProtocolManagerGet ();
			std::string address = UriGet ();
			
			// Fall back to the remote IP which we know works if for whatever reason we dont have a uri value.
			if (address.empty ())
			{
				address = RemoteIpAddressGet ();
			}
			
			if (protocolManager)
			{
				stiRemoteLogEventSend ("EventType=VRSFailover Reason=PleaseWaitTimerFired Address=%s DialMethod=%d", address.c_str(), DialMethodGet());
				protocolManager->pleaseWaitSignal.Emit (); // estiMSG_PLEASE_WAIT
			}
			else
			{
				stiASSERTMSG (estiFALSE, "CstiCall please wait timer failed to obtain protocol manager.");
			}
	    }));
	
	m_signalConnections.push_back (vrsFailOverTimer.timeoutEvent.Connect(
	      [this]{
			  ForceVRSFailover ();
	      }));

	vpe::EndpointMonitor::instanceGet ()->callCreated (m_nCallIndex);

	collectTrace (
		formattedString ("Call instance created: SipInstanceGUID %s, eNatTraversalSIP %d, eSIPDefaultTransport %d, bIPv6Enabled %d, eSecureCallMode %d, useIceLiteMode %d",
			pstConferenceParams->SipInstanceGUID.c_str (),
			pstConferenceParams->eNatTraversalSIP,
			pstConferenceParams->eSIPDefaultTransport,
			pstConferenceParams->bIPv6Enabled,
			pstConferenceParams->eSecureCallMode,
			pstConferenceParams->useIceLiteMode));
} // end CstiCall::CstiCall


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::~CstiCall
//
// Description: CstiCall class destructor
//
// Abstract:
//  This function frees resources associated with the CstiCall object.
//
// Returns:
//  none
//
CstiCall::~CstiCall ()
{
	pleaseWaitTimer.Stop ();
	vrsFailOverTimer.Stop ();

	// Delete SstiMessageInfo
	if (m_pstMessageInfo)
	{
		delete m_pstMessageInfo;
		m_pstMessageInfo = nullptr;
	}

	{
		std::lock_guard<std::mutex> lock (m_callCountMutex);
		m_nCallCount--;
	}

#ifdef stiREFERENCE_DEBUGGING
	stiTrace ("CstiCall::~CstiCall Calls still alive (m_nCallCount): %d\n", m_nCallCount);
#endif

	vpe::EndpointMonitor::instanceGet ()->callDestroyed (m_nCallIndex);
	collectTrace ("Call instance destroyed");
	sendTrace ();
} // end CstiCall::~CstiCall


/*!\brief Get the remote call list name
*
*/
void CstiCall::RemoteCallListNameGet (
	std::string *remoteCallListName) const
{
	Lock ();

	*remoteCallListName = m_remoteCallListName;

	Unlock ();
}


/*!\brief Set the remote call list name
*
*/
void CstiCall::RemoteCallListNameSet (
	const std::string &remoteCallListName)
{
	Lock ();

	m_remoteCallListName = remoteCallListName;

	Unlock ();
}


/*!\brief Get the remote contact name
*
*/
void CstiCall::RemoteContactNameGet (
	std::string *pRemoteContactName) const
{
	Lock ();

	pRemoteContactName->assign (m_szRemoteContactName);

	Unlock ();
}


/*!\brief Set the remote contact name
*
*/
void CstiCall::RemoteContactNameSet (
	const char *pszRemoteContactName)
{
	callTraceRecordMessageGDPR ("%s", pszRemoteContactName);

	Lock ();

	if (pszRemoteContactName)
	{
		strncpy (m_szRemoteContactName, pszRemoteContactName, sizeof (m_szRemoteContactName) - 1);
		m_szRemoteContactName[sizeof (m_szRemoteContactName) - 1] = '\0';
	}
	else
	{
		m_szRemoteContactName[0] = '\0';
	}

	Unlock ();
}


/*! \brief Completes a call to a remote endpoint.
 *
 *  \return  estiOK if dial message is sent successfully, estiERROR otherwise.
 *
 */
stiHResult CstiCall::ContinueDial ()
{
	callTraceRecord;

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	if(StateGet () == esmiCS_INIT_TRANSFER || 
	   StateGet () == esmiCS_TRANSFERRING)
	{
		if(StateGet() == esmiCS_TRANSFERRING)
		{
			m_eTransferLogType = estiTRANSFER_LOG_TYPE_TRANSFEREE;
			RemoteDialStringGet (&m_transferFromDialString);
		}
		else
		{
			m_eTransferLogType = estiTRANSFER_LOG_TYPE_TRANSFERER;
			auto poLocalCallInfo = (CstiCallInfo *)LocalCallInfoGet ();
			const SstiUserPhoneNumbers *psLocalPhoneNumbers = poLocalCallInfo->UserPhoneNumbersGet ();
			m_transferFromDialString = psLocalPhoneNumbers->szPreferredPhoneNumber;
		}

		// Attempt to complete transfer to URI
		hResult = Transfer (m_RoutingAddress.originalStringGet());
		stiTESTRESULT ();
	}
	else
	{
		{
			//
			// The call is not a VRS call.
			//

			//
			// If the call has a redirected number (a new number)
			// then set it as the remote dial string and original
			// dail string.
			//
			std::string newDialString;

			NewDialStringGet (&newDialString);

			if (!newDialString.empty())
			{
				RemoteDialStringSet (newDialString);
				OriginalDialStringSet (DialMethodGet(), newDialString);
			}
		}
		
		CstiProtocolManager *poProtocolManager;
		poProtocolManager = ProtocolManagerGet ();
		stiTESTCOND (poProtocolManager != nullptr, stiRESULT_ERROR);
		
		if (poProtocolManager)
		{
			hResult = poProtocolManager->DSNameResolved (shared_from_this());
			stiTESTRESULT ();
		}
	}

STI_BAIL:

	Unlock ();

	callTraceRecordMessage ("result %d", hResult);

	return (hResult);
} // end CstiCall::Complete


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::Lock
//
// Description: Locks a mutex to lock the class
//
// Abstract:
//  Locks a mutex to lock access to certain data members of the class
//
// Returns:
//  None
//
stiHResult CstiCall::Lock() const
{

	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_poCallStorage)
	{
		m_poCallStorage->lock ();
	}

	if (!stiIS_ERROR (hResult))
	{
		m_LockMutex.lock();
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::Unlock
//
// Description: Unlocks a mutex to lock the class
//
// Abstract:
//  Unlocks a mutex to unlock access to certain data members of the class
//
// Returns:
//  None
//
void CstiCall::Unlock() const
{
	m_LockMutex.unlock();
	if (m_poCallStorage)
	{
		m_poCallStorage->unlock ();
	}
}


/*!
 * \brief Sets the next state on the call object
 *
 *  Sets the next state on the call object and makes a state change callback to notify upper
 *  layers of the change of state.
 *
 *  Warning: this method currently does not do any validation of the state transition.
 *
 * \retval stiHResult with success or failure results
 */
stiHResult CstiCall::NextStateSet (
	EsmiCallState eState,
	uint32_t un32Substate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SstiStateChange StateChange;

	Lock ();

	StateChange.call = shared_from_this();
	StateChange.ePrevState = m_eCallState;
	StateChange.un32PrevSubStates = m_un32Substate;
	StateChange.eNewState = eState;
	StateChange.un32NewSubStates = un32Substate;

	StateSet (eState, un32Substate);

	// Notify the app that we are making this state change. This allows conference manager
	// and CCI to handle the message appropriately.
	ProtocolCallGet ()->ProtocolManagerGet ()->callStateChangedSignal.Emit (StateChange);

	Unlock ();

//STI_BAIL:

	return hResult;
}


/*!
 * \brief Hangs up call after first checking with CCI to determine if it is OK to do so.
 *
 *  This function will fail if the state is not CONNECTING or CONNECTED.  Use
 *  FeatureCheck (eCALL_HANGUP) to determine whether or not this function can
 *  be called successfully.
 *
 * \retval estiOK Successfully hung up the call.
 * \retval other Failed to hang up the call.
 */
stiHResult CstiCall::HangUp (EstiBool bUseResult)
{
	callTraceRecord;

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	// Make sure the state is valid for this function
	if (StateGet() & (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL
	 | esmiCS_HOLD_RMT |  esmiCS_HOLD_BOTH | esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING))
	{
		if (m_protocolCall)
		{
			hResult = m_protocolCall->HangUp ();

			if (StateValidate (esmiCS_CONNECTING) && estiRESULT_UNKNOWN == ResultGet ())
			{
				ResultSet (estiRESULT_LOCAL_HANGUP_BEFORE_ANSWER);
			}
		}
		else
		{
			stiASSERT (estiFALSE);
		}
	} // end if
	
	// Check to see if we were viewing a greeting or recording a message.
	else if (StateValidate (esmiCS_DISCONNECTED) && 
			 SubstateValidate (estiSUBSTATE_LEAVE_MESSAGE | estiSUBSTATE_MESSAGE_COMPLETE))
	{
		if (m_protocolCall)
		{
			hResult = m_protocolCall->HangUp ();
		}
		else
		{
			stiASSERT (estiFALSE);
		}
	}
	else if (!StateValidate (esmiCS_DISCONNECTING))
	{
		stiASSERT (estiFALSE);
	} // end else

	Unlock ();

	callTraceRecordMessage ("result %d", hResult);

	return hResult;
}


/*
 *!\brief Place a call on hold
 *
 */
stiHResult CstiCall::Hold ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	callTraceRecord;

	Lock ();
	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();
	stiTESTCOND (poProtocolManager != nullptr, stiRESULT_ERROR);
	
	hResult = poProtocolManager->CallHold (shared_from_this());
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	callTraceRecordMessage ("result %d", hResult);

	return (hResult);
} // end CstiCall::CallHold


/*!\brief Retreive a held call
 *
 */
stiHResult CstiCall::Resume ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	callTraceRecord;

	Lock ();
	
	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();
	stiTESTCOND (poProtocolManager != nullptr, stiRESULT_ERROR);
	
	hResult = poProtocolManager->CallResume (shared_from_this());
	stiTESTRESULT ();
	
STI_BAIL:

	Unlock ();

	callTraceRecordMessage ("result %d", hResult);

	return (hResult);
} // end CstiCall::CallResume


///\brief Flashes the remote light ring.
///
stiHResult CstiCall::RemoteLightRingFlash ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	
	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();
	stiTESTCOND (poProtocolManager != nullptr, stiRESULT_ERROR);
	
	hResult = poProtocolManager->RemoteLightRingFlash (shared_from_this());
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!\brief Answer an incoming call
 *
 *  This function will fail if the state is not CONNECTING and the sub-state
 *  is not WAITING_FOR_USER_RESP.  Use FeatureCheck (eCALL_ANSWER) or
 *  FeatureCheck (eCALL_RESPONSE_FEATURES) to determine whether or not this
 *  function can be called successfully.
 *
 *  \retval estiOK If connecting a call and the call answer message is successfully
 *      sent.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCall::Answer ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	callTraceRecord;

	Lock ();
	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();

	// Make sure the state is valid for this function
	if ((StateGet() & (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL
	 | esmiCS_HOLD_RMT |  esmiCS_HOLD_BOTH))
	  && poProtocolManager)
	{
		hResult = poProtocolManager->CallAnswer (shared_from_this());
		stiTESTRESULT ();
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	Unlock ();
	
	callTraceRecordMessage ("result %d", hResult);

	return (hResult);
} // end CstiCall::CallAnswer


/*!\brief Transfer a call
 *
 *  This function will fail if the state is not CONNECTING and the sub-state
 *  is not WAITING_FOR_USER_RESP.  Use FeatureCheck (eCALL_ANSWER) or
 *  FeatureCheck (eCALL_RESPONSE_FEATURES) to determine whether or not this
 *  function can be called successfully.
 *
 *  \retval estiOK If connecting a call and the call answer message is successfully
 *      sent.
 *  \retval estiERROR Otherwise.
 */
stiHResult CstiCall::Transfer (
    const std::string &dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiProtocolManager *poProtocolManager = nullptr;

	callTraceRecord;

	Lock ();

	// Get the  call phone numbers to check against.
	const SstiUserPhoneNumbers *phoneNumbers = m_oRemoteCallInfo.UserPhoneNumbersGet();
	if (m_eRemoteDialMethod == estiDM_BY_DS_PHONE_NUMBER &&
	    (phoneNumbers->szLocalPhoneNumber == dialString ||
	     phoneNumbers->szTollFreePhoneNumber == dialString))
	{
		stiTHROW (stiRESULT_ERROR);
	}
	
	// Disallow transfer to 911
	if ("911" == dialString)
	{
		stiTHROW (stiRESULT_UNABLE_TO_TRANSFER_TO_911);
	}

	poProtocolManager = ProtocolManagerGet ();
	stiTESTCOND (poProtocolManager, stiRESULT_ERROR);

	if ((StateValidate (esmiCS_CONNECTED | esmiCS_INIT_TRANSFER) &&
		(estiINCOMING == DirectionGet () || IsTransferableGet ())))
	{
		// Reset the CalledName string
		CalledNameSet ("");

		hResult = poProtocolManager->CallTransfer (shared_from_this(), dialString);
		stiTESTRESULT ();
	}
	else if (StateValidate (esmiCS_TRANSFERRING))
	{
		hResult = poProtocolManager->CallTransfer (shared_from_this(), dialString);
		stiTESTRESULT ();
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	Unlock ();

	callTraceRecordMessage ("result %d", hResult);

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::Accept
//
// Description: This function is used to accept an incoming call.
//
// Abstract:
//  This function is used to accept an incoming call.
//
// Returns:
//  This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiCall::Accept ()
{
	callTraceRecord;

	DirectionSet (estiINCOMING);
	DELEGATE_RET_CODE(m_protocolCall, stiHResult, Accept);
}


///\brief Rejects the call using the provided reason.
///
stiHResult CstiCall::Reject (
	EstiCallResult eCallResult) //!< The reason for rejection
{
	stiLOG_ENTRY_NAME (CstiCall::Reject);
	callTraceRecordMessage ("%d", eCallResult);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	ResultSet (eCallResult);

	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();
	stiTESTCOND (poProtocolManager != nullptr, stiRESULT_ERROR);
	
	hResult = poProtocolManager->CallReject (shared_from_this());
	stiTESTRESULT ();

STI_BAIL:

	Unlock ();
	
	return hResult;
} // end CstiCall::Reject


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::ProtocolCallSet
//
// Description: This function sets the ProtocolCall associated with this call.
//
void CstiCall::ProtocolCallSet (
	CstiProtocolCallSP protocolCall)
{
	callTraceRecord;

	Lock ();

	if (protocolCall)
	{
		protocolCall->CstiCallSet(shared_from_this());
	}

	m_protocolCall = protocolCall;

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::ResultNameGet
//
// Description: This function returns a string representation of the call result.
//
std::string CstiCall::ResultNameGet () const
{
	std::string resultName;

	switch (ResultGet ())
	{
		case estiRESULT_UNKNOWN:
			resultName = "estiRESULT_UNKNOWN";
			break;
		case estiRESULT_CALL_SUCCESSFUL:
			resultName = "estiRESULT_CALL_SUCCESSFUL";
			break;
		case estiRESULT_LOCAL_SYSTEM_REJECTED:
			resultName = "estiRESULT_LOCAL_SYSTEM_REJECTED";
			break;
		case estiRESULT_LOCAL_SYSTEM_BUSY:
			resultName = "estiRESULT_LOCAL_SYSTEM_BUSY";
			break;
		case estiRESULT_NOT_FOUND_IN_DIRECTORY:
			resultName = "estiRESULT_NOT_FOUND_IN_DIRECTORY";
			break;
		case estiRESULT_DIRECTORY_FIND_FAILED:
			resultName = "estiRESULT_DIRECTORY_FIND_FAILED";
			break;
		case estiRESULT_REMOTE_SYSTEM_REJECTED:
			resultName = "estiRESULT_REMOTE_SYSTEM_REJECTED";
			break;
		case estiRESULT_REMOTE_SYSTEM_BUSY:
			resultName = "estiRESULT_REMOTE_SYSTEM_BUSY";
			break;
		case estiRESULT_REMOTE_SYSTEM_UNREACHABLE:
			resultName = "estiRESULT_REMOTE_SYSTEM_UNREACHABLE";
			break;
		case estiRESULT_REMOTE_SYSTEM_UNREGISTERED:
			resultName = "estiRESULT_REMOTE_SYSTEM_UNREGISTERED";
			break;
		case estiRESULT_REMOTE_SYSTEM_BLOCKED:
			resultName = "estiRESULT_REMOTE_SYSTEM_BLOCKED";
			break;
		case estiRESULT_DIALING_SELF:
			resultName = "estiRESULT_DIALING_SELF";
			break;
		case estiRESULT_LOST_CONNECTION:
			resultName = "estiRESULT_LOST_CONNECTION";
			break;
		case estiRESULT_NO_ASSOCIATED_PHONE:
			resultName = "estiRESULT_NO_ASSOCIATED_PHONE";
			break;
		default:
			resultName = "???";
			break;
	}

	return resultName;
}


/*!\brief Get the remote dial string
 *
 */
void CstiCall::RemoteDialStringGet (
	std::string *pRemoteDialString) const
{
	Lock ();
	*pRemoteDialString = m_remoteDialString;
	Unlock ();
};


/*!\brief Get the remote dial method
 *
 */
EstiDialMethod CstiCall::RemoteDialMethodGet () const
{
	Lock ();
	EstiDialMethod eDialMethod = m_eRemoteDialMethod;

	if (eDialMethod == estiDM_UNKNOWN)
	{
		if (RemoteInterfaceModeGet () == estiINTERPRETER_MODE)
		{
			eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
		}
		else
		{
			eDialMethod = estiDM_BY_DIAL_STRING;
		}
	}
	Unlock ();

	return eDialMethod;
}


/*!\brief Set the remote dial method
 *
 */
void CstiCall::RemoteDialMethodSet (
	EstiDialMethod eMethod) //<! The dial method
{
	callTraceRecordMessage ("%d", eMethod);

	Lock ();
	m_eRemoteDialMethod = eMethod;
	Unlock ();
}


/*!\brief Set the remote dial string
 *
 */
void CstiCall::RemoteDialStringSet (
    std::string dialString)
{
	callTraceRecordMessageGDPR ("%s", dialString.c_str ());

	Lock ();
	m_remoteDialString = dialString;
	Unlock ();
}


/*!
 * \brief Sets an override for the from name when an outgoing call is placed
 *        This ensures that the remote endpoint sees this name instead of the
 *        name that would normally be sent.  OptString is used to distinguish
 *        between the empty string and an unset string.
 */
void CstiCall::fromNameOverrideSet (const OptString &fromNameOverride)
{
	callTraceRecordMessageGDPR ("%s", fromNameOverride.value_or ("").c_str ());

	m_fromNameOverride = fromNameOverride;
}


OptString CstiCall::fromNameOverrideGet () const
{
	return m_fromNameOverride;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::localAlternateNameGet
//
// Description: Get the Local Alternate Name
//
// Abstract:
//
// Returns:
//  None.
//
std::string CstiCall::localAlternateNameGet () const
{
	Lock();

	auto name = m_localAlternateName;

	Unlock();

	return name;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::localAlternateNameSet
//
// Description: Set the Local Alternate Name
//
// Abstract:
//
// Returns:
//  None.
//
void CstiCall::localAlternateNameSet (
	const std::string &alternateName)
{
	Lock();
	m_localAlternateName = alternateName;
	Unlock();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::vrsCallIdGet
//
// Description: Get the VRS Call ID
//
// Abstract:
//
// Returns:
//  VRS Call ID
//
std::string CstiCall::vrsCallIdGet () const
{
	Lock();

	auto callId = m_vrsCallId;

	Unlock();

	return callId;
}

 
////////////////////////////////////////////////////////////////////////////////
//; CstiCall::vrsCallIdSet
//
// Description: Set the VRS Call ID
//
// Abstract:
//
// Returns:
//  None.
//
void CstiCall::vrsCallIdSet (
	const std::string &vrsCallId)
{
	callTraceRecordMessageGDPR ("%d", vrsCallId.c_str ());

	Lock();
	m_vrsCallId = vrsCallId;
	Unlock();
}

std::string CstiCall::vrsAgentIdGet () const
{
	return m_vrsAgentId;
}


void CstiCall::vrsAgentIdSet (const std::string &agentId)
{
	callTraceRecordMessageGDPR ("%s", agentId.c_str ());

	Lock ();
	m_vrsAgentId = agentId;
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalDisplayNameGet
//
// Description: Get the local display name
//
// Returns:
//
std::string CstiCall::LocalDisplayNameGet () const
{
	Lock ();
	auto displayName = m_localDisplayName;
	Unlock ();

	return displayName;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalDisplayNameSet
//
// Description: Set LocalDisplayNameSet
//
// Abstract: Will create a copy of the passed data for internal use.
//	The original string remains the caller's responsibility.
//
// Returns:
//
void CstiCall::LocalDisplayNameSet (
	const std::string &displayName)
{
	callTraceRecordMessageGDPR ("%s", displayName.c_str ());

	Lock ();
	m_localDisplayName = displayName;
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalPreferredLanguageGet
//
// Description: Get the local preferred language to be used for interpretive
// services.
//
// Returns:
//
std::string CstiCall::LocalPreferredLanguageGet () const
{
	Lock();
	std::string localPreferredLanguage = m_localPreferredLanguage.get_value_or("");
	Unlock();
	return localPreferredLanguage;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalPreferredLanguageGet
//
// Description: Get the local preferred language to be used for interpretive
// services.
//
// Returns:
//
void CstiCall::LocalPreferredLanguageGet (OptString *preferredLanguage) const
{
	Lock();

	*preferredLanguage = m_localPreferredLanguage;

	Unlock();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalPreferredLanguageSet
//
// Description: Set the local preferred language to be used for interpretive
// services.
//
// Returns:
//
void CstiCall::LocalPreferredLanguageSet (const OptString &preferredLanguage, int nLang)
{
	Lock ();

	m_nLocalPreferredLanguage = nLang;

	m_localPreferredLanguage = preferredLanguage;

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalReturnCallDialStringGet
//
// Description: Get the local return dial string.  This is the string we will
// send telling the called videophone how to dial us back.
//
// Returns:
//
void CstiCall::LocalReturnCallDialStringGet (
	std::string *returnDialString) const
{
	Lock();

	if (returnDialString)
	{
		*returnDialString = m_localReturnCallDialString;
	}
	else
	{
		stiASSERT(false);
	}

	Unlock();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::LocalReturnCallDialMethodGet
//
// Description: Get the local return dial method.  This is the method we will
// send telling the called videophone how to dial us back.
//
// Returns:
//
EstiDialMethod CstiCall::LocalReturnCallDialMethodGet () const
{
	Lock ();

	EstiDialMethod eMethod = m_eLocalReturnCallDialMethod;

	Unlock ();

	return (eMethod);
}


/*! \brief Validate the current state against the passed in states.
 *
 * Check the current state to see if it is one of the states that was passed
 * into this function.
 *
 * \retval estiTRUE If the current state is one of those passed in.
 * \retval estiFALSE Otherwise.
 */
EstiBool CstiCall::StateValidate (
	IN const uint32_t un32States) const //!< The state (or states ORed together) to check
{
	stiLOG_ENTRY_NAME (CstiCall::StateValidate);
	EstiBool bResult = estiFALSE;

	Lock ();
	// Is the current state one of the states that was passed in?
	if (0 < (m_eCallState & un32States))
	{
		bResult = estiTRUE;
	} // end if

	Unlock ();
	return (bResult);
} // end CstiCall::StateValidate


/*! \brief Validate the current sub-state against the passed in sub-states.
 *
 * Check the current sub-state to see if it is one of the sub-states that was
 * passed into this function.
 *
 * \retval estiTRUE if the current sub-state is one of those passed in.
 * \retval estiFALSE Otherwise.
 */
EstiBool CstiCall::SubstateValidate (
	IN const uint32_t un32Substates) const //!< The substate (or substates ORed together) to check
{
	stiLOG_ENTRY_NAME (CstiCall::SubstateValidate);
	EstiBool bResult = estiFALSE;

	Lock ();
	// Is the current sub-state one of the sub-states that was passed in?
	if (0 < (m_un32Substate & un32Substates))
	{
		bResult = estiTRUE;
	} // end if
	Unlock ();

	return (bResult);
} // end CstiCall::SubstateValidate


/*! \brief Notifies us that we should refresh due to a change of call information.
 *
 * Specifically, a change to light ring, phone numbers, or remote name should
 * call this to make sure everything stays up-to-date.
 *
 * This will only notify the app if the call is active.
 *
 */
void CstiCall::CallInformationChanged ()
{
	callTraceRecord;

	Lock ();
	CstiProtocolManager *poProtocolManager = ProtocolManagerGet ();
	if (poProtocolManager)
	{
		switch (m_eCallState)
		{
			case esmiCS_CONNECTED:
			case esmiCS_CONNECTING:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			    poProtocolManager->callInformationChangedSignal.Emit (shared_from_this());
				break;
			default:
				break;
		}
	}
	Unlock ();
} // end CstiCall::CallInformationChanged


/*!\brief Get the dial method
 *
 */
EstiDialMethod CstiCall::DialMethodGet () const
{
	Lock ();
	EstiDialMethod eDialMethod = m_eDialMethod;

	//
	// If we have been transferred always show the dial method of the first call.
	//
	if (TransferredGet())
	{
		if (DirectionGet() == estiINCOMING)
		{
			eDialMethod = m_eOriginalRemoteDialMethod;
		}
		else
		{
			eDialMethod = m_eOriginalDialMethod;
		}
	}
	else
	{
		if (DirectionGet() == estiINCOMING)
		{
			eDialMethod = RemoteDialMethodGet ();

			if (eDialMethod == estiDM_UNKNOWN)
			{
				if (estiINTERPRETER_MODE == RemoteInterfaceModeGet ())
				{
					eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
				}
				else if (estiTECH_SUPPORT_MODE == RemoteInterfaceModeGet ())
				{
					eDialMethod = estiDM_BY_DS_PHONE_NUMBER;
				}
				else
				{
					eDialMethod = estiDM_BY_DIAL_STRING;
				}
			}
		}
	}
	
	Unlock ();

	return eDialMethod;
}

/*!\brief Set the dial method
 *
 */
void CstiCall::DialMethodSet (
	EstiDialMethod eMethod) //<! The dial method
{
	callTraceRecordMessage ("%d", eMethod);

	Lock ();
	m_eDialMethod = eMethod;
	Unlock ();
}


/*!\brief Get the original dial string
 *
 */
void CstiCall::OriginalDialStringGet (
    std::string *originalDialString) const
{
	Lock ();
	*originalDialString = m_originalDialString;
	Unlock ();
}


/*!\brief Set the original dial string
 *
 */
void CstiCall::OriginalDialStringSet (
    EstiDialMethod dialMethod,
    std::string dialString)
{
	callTraceRecordMessageGDPR ("%d, %s", dialMethod, dialString.c_str ());

	Lock ();
	m_originalDialString = dialString;
	m_eOriginalDialMethod = dialMethod;
	Unlock ();
}


void CstiCall::OriginalDialMethodSet (
    EstiDialMethod dialMethod)
{
	callTraceRecordMessage ("%d", dialMethod);

	Lock ();
	m_eOriginalDialMethod = dialMethod;
	Unlock ();
}


/*!\brief Set the intended dial string
 *
 */
void CstiCall::IntendedDialStringSet (
    std::string dialString)
{
	callTraceRecordMessageGDPR ("%s", dialString.c_str ());

	Lock ();
	m_intendedDialString = dialString;
	Unlock ();
}


/*!\brief Get the intended dial string
 *
 */
void CstiCall::IntendedDialStringGet (
    std::string *intendedDialString) const
{
	Lock ();
	*intendedDialString = m_intendedDialString;
	Unlock ();
}


/*!\brief Get the IP Address
 *
 */
std::string CstiCall::RemoteIpAddressGet () const
{
	Lock ();

	auto remoteIpAddress = m_RoutingAddress.ipAddressStringGet ();

	Unlock ();

	return remoteIpAddress;
}


/*!\brief Set the IP Address
 *
 */
void CstiCall::RemoteIpAddressSet (
	const std::string &remoteIpAddress)
{
	callTraceRecordMessageGDPR ("%s", remoteIpAddress.c_str ());

	Lock ();

	m_RoutingAddress.ipAddressSet (remoteIpAddress);

	Unlock ();
};


/*!\brief Set whether or not the called party is in the contacts list
 *
 */
void CstiCall::IsInContactsSet (
	EstiInContacts eIsInContacts)
{
	Lock ();
	m_eIsInContacts = eIsInContacts;
	Unlock ();
}


/*!\brief Get whether or not the called party is in the contacts list
 *
 */
EstiInContacts CstiCall::IsInContactsGet () const
{
	Lock ();
	EstiInContacts eRet = m_eIsInContacts;
	Unlock ();
	return eRet;
}

CstiItemId CstiCall::ContactIdGet() const
{
	return m_ContactId;
}

void CstiCall::ContactIdSet(const CstiItemId &Id)
{
	callTraceRecord;

	m_ContactId = Id;
}

CstiItemId CstiCall::ContactPhoneNumberIdGet() const
{
	return m_ContactPhoneNumberId;
}

void CstiCall::ContactPhoneNumberIdSet(const CstiItemId &Id)
{
	callTraceRecord;

	m_ContactPhoneNumberId = Id;
}

stiHResult CstiCall::BridgeCreate (
	const std::string &uri)
{
	callTraceRecordMessageGDPR ("%s", uri.c_str ());

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Only support bridged calls with SIP
	//
	stiTESTCOND(estiSIP == ProtocolGet(), stiRESULT_ERROR);

	{
		auto sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
		stiTESTCOND(sipCall, stiRESULT_ERROR);

		hResult = sipCall->SipManagerGet ()->BridgeCreate (uri, shared_from_this());
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiCall::BridgeDisconnect()
{
	callTraceRecord;

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiSipCallSP sipCall = nullptr;

	stiTESTCOND(estiSIP == ProtocolGet(), stiRESULT_ERROR);

	sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	stiTESTCOND(sipCall, stiRESULT_ERROR);

	hResult = sipCall->BridgeDisconnect();
	
STI_BAIL:

	return hResult;
}


stiHResult CstiCall::BridgeStateGet (EsmiCallState *peCallState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiSipCallSP sipCall = nullptr;

	stiTESTCOND(estiSIP == ProtocolGet(), stiRESULT_ERROR);

	sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	stiTESTCOND(sipCall, stiRESULT_ERROR);

	hResult = sipCall->BridgeStateGet (peCallState);

STI_BAIL:

	return hResult;
}



/*!\brief Creates a backgorund conference call leg to the specified URI.
 *
 *\note The purpose of this type of call is to allow for the conference to be connected to and established without impacting an existing call.
 *  The platform is not made aware of this call leg so failures to connect or delays getting members in the room will not interrupt an existing call.
 *
 */
stiHResult CstiCall::backgroundGroupCallCreate (const std::string &uri, const std::string &conferenceId)
{
	callTraceRecordMessageGDPR ("%s %s", uri.c_str (), conferenceId.c_str ());

	stiHResult hResult = stiRESULT_SUCCESS;

	auto sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	stiTESTCOND(sipCall, stiRESULT_ERROR);
	
	sipCall->SipManagerGet ()->dhviCreate (uri, conferenceId, shared_from_this(), true);
	
	dhvConnectingTimerStart (sipCall->SipManagerGet ());

STI_BAIL:

	return hResult;
}


///\brief Sets the return info for callback purposes
///
/// This function is used to set the information needed by the remote system
/// to call back to the desired location (not always the originating system).
///  This data will be passed in SInfo to the remote system.
///
/// \returns An stiHResult value containing success or failure codes
///
stiHResult CstiCall::LocalReturnCallInfoSet (
    EstiDialMethod eMethod,
    std::string dialString)
{
	stiLOG_ENTRY_NAME (v::LocalReturnCallInfoSet);
	callTraceRecordMessageGDPR ("%d, %s", eMethod, dialString.c_str ());

	Lock ();

	stiHResult hResult = stiRESULT_SUCCESS;

	m_eLocalReturnCallDialMethod =  eMethod;
	m_localReturnCallDialString = dialString;

	Unlock ();

	return hResult;
}

///\brief Sets the call info for leaving a video message
///
/// This function is used to set the information needed to determine if a
/// video message can be left and what bitrate to record it at.
///
/// \returns An stiHResult value containing success or failure codes
///
stiHResult CstiCall::MessageInfoSet(SstiMessageInfo *pstMessageInfo)
{
	stiLOG_ENTRY_NAME (v::LocalReturnCallInfoSet);
	callTraceRecord;

	Lock ();

	stiHResult hResult = stiRESULT_SUCCESS;
	
	stiTESTCOND (pstMessageInfo, stiRESULT_ERROR);

	// If we don't have a MessageInfo structure yet, allocate one now.
	if (!m_pstMessageInfo)
	{
		m_pstMessageInfo = new SstiMessageInfo ();
		stiTESTCOND (m_pstMessageInfo, stiRESULT_MEMORY_ALLOC_ERROR);
	}

	//Switching to use HTTPS instead of HTTP
	protocolSchemeChange (pstMessageInfo->GreetingURL, "https");

	// Update the class copy.
	*m_pstMessageInfo = *pstMessageInfo;
	
STI_BAIL:

	Unlock ();

	return hResult;
}

///\brief Gets a copy of the call info for leaving a video message
///
/// This function is used to get the information needed to determine if a
/// video message can be left and what bitrate to record it at.
///
/// \returns An stiHResult value containing success or failure codes
///
stiHResult CstiCall::MessageInfoGet (SstiMessageInfo *pstMessageInfo)
{
	Lock ();
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pstMessageInfo, stiRESULT_ERROR);
	
	// If we don't already have this sturcutre, 
	// allocate it so that it now.
	if (!m_pstMessageInfo)
	{
		m_pstMessageInfo = new SstiMessageInfo ();
		stiTESTCOND (m_pstMessageInfo, stiRESULT_MEMORY_ALLOC_ERROR);
	}
	
	// Copy the structure
	*pstMessageInfo = *m_pstMessageInfo;
	
STI_BAIL:

	Unlock ();
	
	return hResult;
}


/*!
 * \brief Retrieves whether or not SignMail is available to be sent.
 *
 * \return True if SignMail is available.  Otherwise, false.
 */
bool CstiCall::IsSignMailAvailableGet () const
{
	bool signMailAvailable = false;

	Lock();
	signMailAvailable = m_pstMessageInfo ? m_pstMessageInfo->bP2PMessageSupported == estiTRUE : false;
	Unlock();

	return signMailAvailable;
}


/*!
 * \brief Skips from dialing and goes directly to recording a SignMail.
 *
 */
stiHResult CstiCall::SignMailSkipToRecord ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_protocolCall)
	{
		hResult = m_protocolCall->SignMailSkipToRecord();
	}

	return hResult;
}


/*! \brief Returns the Greeting Type
 *
 * If there is a message info it returns the greeting type
 *
 * \retval eSignMailGreetingType
 */
eSignMailGreetingType CstiCall::MessageGreetingTypeGet () const
{
	return m_pstMessageInfo ? m_pstMessageInfo->eGreetingType : eGREETING_DEFAULT;
}

/*! \brief Returns the Greeting URL.
 *
 * If there is a message info it returns the greeting URL
 *
 * \retval const char* to the Greeting URL.
 */
std::string CstiCall::MessageGreetingURLGet() const
{
	return m_pstMessageInfo ? m_pstMessageInfo->GreetingURL : std::string ();
}

/*! \brief Returns the Greeting Text
 *
 * If there is a message info it returns the greeting text
 *
 * \retval const char* to the Greeting Text.
 */
std::string CstiCall::MessageGreetingTextGet() const
{
	return m_pstMessageInfo ? m_pstMessageInfo->GreetingText : std::string ();
}

/*! \brief Returns the message record time in seconds.
 *
 * If there is a message info it returns the message
 * record time in seconds.
 *
 * \retval Record time in seconds.
 */
int CstiCall::MessageRecordTimeGet()
{
	int nRecordTime = 0;

	Lock ();
	if (m_pstMessageInfo)
	{
		nRecordTime = m_pstMessageInfo->nMaxRecordSeconds;
	}
	Unlock ();

	return nRecordTime;
}

/*! \brief Returns whether or not the countdown greeting is embedded in the greeting.
 *
 * Checks the message info to see if the countdown video is embedded. 
 *
 * \retval estiTRUE if the countdown video is embedded
 * 		   estiFALSE if the countdown video is not embedded.
 */
EstiBool CstiCall::MessageCountdownEmbeddedGet()
{
	EstiBool bEmbedded = estiFALSE;

	Lock ();
	if (m_pstMessageInfo)
	{
		bEmbedded = m_pstMessageInfo->bCountdownEmbedded;
	}
	Unlock ();

	return bEmbedded;
}

/*! \brief Returns the message record time in seconds.
 *
 * If there is a message info it returns the download speed
 * of the remote phone.
 *
 * \retval Speed in bits per second.
 */
int CstiCall::RemoteDownloadSpeedGet () const
{
	int nDownloadSpeed = 0;
	Lock ();
	if (m_pstMessageInfo)
	{
		nDownloadSpeed = m_pstMessageInfo->nRemoteDownloadSpeed;
	}
	Unlock ();

	return nDownloadSpeed;
}

/*!
* \brief Places the call object in a state that indicates that a transfer was at least started.
*
*/
void CstiCall::TransferredSet ()
{
	callTraceRecord;

	Lock ();
	
	//
	// When a call is transferred we will continue to return the remote name and
	// dial string from the first call.  This is to get around issues in the relay system
	// where the next interpreter's station does not self identify as the hearing user.
	// If that is corrected then this code should be revisited.
	//
	if (!m_bTransferred)
	{
		std::string RemoteName;
	
		if (m_protocolCall)
		{
			m_protocolCall->RemoteNameGet (&RemoteName);
		}
		else
		{
			// Check to see if they have a contact name for the callee
			RemoteContactNameGet (&RemoteName);
			if (RemoteName.empty())
			{
				RemoteCallListNameGet (&RemoteName);
			}
		}
		
		if (!RemoteName.empty ())
		{
			strncpy (m_szOriginalRemoteName, RemoteName.c_str(), sizeof (m_szOriginalRemoteName) - 1);
			m_szOriginalRemoteName[sizeof (m_szOriginalRemoteName) - 1] = '\0';
		}

		m_eOriginalRemoteDialMethod = m_eRemoteDialMethod;
		m_originalRemoteDialString = m_remoteDialString;
	
		m_bTransferred = estiTRUE;
	}
	
	Unlock ();
};
	

/*!
* \brief Gets information about whether or not the call has been transferred
*
* \retval true The call has been transferred
* \retval false The call has not been transferred
*/
EstiBool CstiCall::TransferredGet () const
{
	Lock ();
	EstiBool bRet = m_bTransferred;
	Unlock ();
	return (bRet);
}

/*!\brief Set the transfer dial string
 *
 */
void CstiCall::TransferDialStringSet (
    std::string dialString)
{
	callTraceRecordMessageGDPR ("%s", dialString.c_str ());

	Lock ();
	m_transferDialString = dialString;
	Unlock ();
}

/*!\brief Get the transfer dial string
 *
 */
void CstiCall::TransferDialStringGet (
    std::string *transferDialString) const
{
	Lock ();
	*transferDialString = m_transferDialString;
	Unlock ();
}


///\brief Gets the address verification boolean.
///\retval estiTRUE The address needs to be verified
///\retval estiFALSE the address does not need to be verified
///
EstiBool CstiCall::VerifyAddressGet () const
{
	Lock ();
	EstiBool bRet = m_bVerifyAddress;
	Unlock ();
	return bRet;
}


///\brief Sets the address verification boolean.
///\param bVerifyAddress If true, the address needs to be verified otherwise it does not.
///
void CstiCall::VerifyAddressSet (
	EstiBool bVerifyAddress)
{
	callTraceRecordMessage ("%d", bVerifyAddress);

	Lock ();
	m_bVerifyAddress = bVerifyAddress;
	Unlock ();
}


///\brief Gets the dial string
///
/// If the there is an original dial string then return this.  If there
/// isn't a original dial string then return the remote dial string
/// If there isn't a dial string and the remote endpoint is not
/// an interpreter then return the ip address (we don't want
/// to expose the ip address of an interpreter vp).
///
stiHResult CstiCall::DialStringGet (
	EstiDialMethod *peDialMethod,
    std::string *pDialString) const
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	*peDialMethod = DialMethodGet ();

	if (TransferredGet ())
	{
		//
		// If we have transferred we always want to get the original dial string.
		// If the call is an incoming call then always get the remote dial string.
		// If the call was an outgoing call always get the original outgoing dial string.
		//
		if (estiINCOMING == DirectionGet ())
		{
			*pDialString = m_originalRemoteDialString;
		}
		else
		{
			OriginalDialStringGet (pDialString);
		}
	}
	else
	{
		OriginalDialStringGet (pDialString);

		// Is the dial string empty?
		// AND we didn't dial by VRS?
		//
		// The original dial string is empty always on an incoming call.
		// It is empty on an outgoing call only if the application layer
		// called SVRS without a number.
		//
		// We also need to call RemoteDialStringGet for incoming VRS calls.
		//
		if ((pDialString->empty () &&
			estiDM_BY_VRS_PHONE_NUMBER != *peDialMethod && estiDM_BY_VRS_WITH_VCO != *peDialMethod) ||
		   
			estiINCOMING == DirectionGet ())
		{
			// Find something else to load it with.  For VRS calls (with no hearing number supplied),
			// don't show the IP address.
			
			RemoteDialStringGet (pDialString);
			
#if 0 // With the way SIP is working today, the remote dial string should contain a phone number, a URI or be empty.  It should be
	  // empty if the call was anonymous and therefore we don't want to get the IP address of the remote endpoint.

			//
			// If we are not talking to an interpreter and
			// there is no remote dial string then
			// set the IP address of the remote device.
			//
			if (pDialString->empty ()
			 && RemoteInterfaceModeGet () != estiINTERPRETER_MODE && RemoteInterfaceModeGet () != estiTECH_SUPPORT_MODE)
			{
				RemoteIpAddressGet (pDialString);
			}
#endif
		}
	}
	
	Unlock ();

	return hResult;
}


///\brief Gets the new dial string
///
void CstiCall::NewDialStringGet (
	std::string *pDialString) const
{
	Lock ();
	*pDialString = m_newDialString;
	Unlock ();
}


/*!\brief Set the new dial string
 *
 */
void CstiCall::NewDialStringSet (
    std::string dialString)
{
	callTraceRecordMessageGDPR ("%s", dialString.c_str ());

	Lock ();
	m_newDialString = dialString;
	Unlock ();
}


void CstiCall::StateSet (
	EsmiCallState eState,
	uint32_t un32Substate)
{
	Lock ();
	if(eState != esmiCS_UNKNOWN &&
			eState != esmiCS_IDLE &&
		eState != esmiCS_CONNECTING &&
		eState != esmiCS_CONNECTED &&
		eState != esmiCS_HOLD_LCL &&
		eState != esmiCS_HOLD_RMT  &&
		eState != esmiCS_HOLD_BOTH  &&
		eState != esmiCS_DISCONNECTING &&
		eState != esmiCS_DISCONNECTED &&
		eState != esmiCS_CRITICAL_ERROR &&
		eState != esmiCS_INIT_TRANSFER &&
		eState != esmiCS_TRANSFERRING
		)
	{
		stiTrace("Setting invalid state on call.\n");
	}
	m_eCallState = eState;
	m_un32Substate = un32Substate;
	m_logs.collectNewStateTrace (eState, un32Substate);

	switch (m_eCallState)
	{
		case esmiCS_CONNECTING:
		{
			if (DirectionGet() == estiOUTGOING)
			{
				//
				// Start the connecting time when we reach the 
				// "calling" substate.
				//
				if (estiSUBSTATE_CALLING == m_un32Substate)
				{
					if (m_protocolCall)
					{
						m_protocolCall->connectingTimeStart();
					}
					else
					{
						stiASSERTMSG(false, "Entered Calling Substate without a protocolCall.");
					}
				}
				
				//
				// If we reached the "waiting for remote response" substate
				// then set the connecting stop time.
				//
				if (estiSUBSTATE_WAITING_FOR_REMOTE_RESP == m_un32Substate)
				{
					if (m_protocolCall)
					{
						m_protocolCall->connectingTimeStop ();
					}
					else
					{
						stiASSERTMSG(false, "Entered Waiting For Remote Response Substate without a protocolCall.");
					}
				}
			}
			
			break;
		}
		
		case esmiCS_CONNECTED:
		{
			//
			// If we have reached the connected state but have not yet
			// set the connecting stop time then set it now
			//
			if (DirectionGet() == estiOUTGOING)
			{
				if (m_protocolCall)
				{
					m_protocolCall->connectingTimeStop ();
				}
				else
				{
					stiASSERTMSG(false, "Attempting to stop connecting time with no protocol call.");
				}
			}
			
			if (estiSUBSTATE_CONFERENCING == m_un32Substate)
			{
				if (m_protocolCall)
				{
					m_protocolCall->connectedTimeStart ();
				}
				else
				{
					stiASSERTMSG(false, "Attempting to start connected time with no protocol call.");
				}
				
				if (!m_hearingCapabilityCheckNumber.empty ())
				{
					dhviCapabilityCheck (m_hearingCapabilityCheckNumber);
				}
			}
			
			break;
		}
		
		case esmiCS_DISCONNECTING:
		case esmiCS_DISCONNECTED:

			// We should not be going from disconnected or disconnecting to connecting.
			stiASSERT (esmiCS_CONNECTING != eState);

			//
			// Fall through
			//

		case esmiCS_CRITICAL_ERROR:
		{
			//
			// If we have not yet set the connecting stop time then set it now
			//
			if (DirectionGet() == estiOUTGOING)
			{
				if (m_protocolCall)
				{
					m_protocolCall->connectingTimeStop();
				}
				else
				{
					stiASSERTMSG(false, "Attempting to stop connecting time due to error with no protocol call.");
				}
			}
			
			FailureTimersStop ();
			
			//
			// Set the call stop time if we have not set it yet.
			//			
			if (m_protocolCall)
			{
				m_protocolCall->connectedTimeStop ();
			}
			else
			{
				stiASSERTMSG(false, "Attempting to stop connected time due to error with no protocol call.");
			}
			
			break;
		}
			
		case esmiCS_UNKNOWN:
		case esmiCS_IDLE:
		case esmiCS_HOLD_LCL:
		case esmiCS_HOLD_RMT:
		case esmiCS_HOLD_BOTH:
		case esmiCS_INIT_TRANSFER:
		case esmiCS_TRANSFERRING:
			
			break;
	}

	Unlock ();
}


#ifdef stiHOLDSERVER
////////////////////////////////////////////////////////////////////////////////
//; CstiCall::HSActiveStatusSet
//
// Description: Sets whether the call is active
//
// Abstract:
//	The call is marked active if the call was received when the server was
//	open. If the server was closed when the call was recieved, the call is
//	marked inactive.
//
// Returns:
//	None
//
void CstiCall::HSActiveStatusSet (EstiBool bHSActiveStatus)
{
	stiLOG_ENTRY_NAME (CstiCall::HSActiveStatusSet);
	Lock ();
	m_bHSActiveStatus = bHSActiveStatus;
	Unlock ();
} // end CstiCall::HSActiveStatusSet

#endif // stiHOLDSERVER


/*!\brief Retrieves the number of seconds it to took to connect to the remote endpoint
 * 
 * The value is only valid for outgoing calls.  The time is measured between the
 * "calling" substate and the "waiting for remote response" substate.  If the
 * method is called prior to reaching the "waiting for remote response" susbstate
 * then the current time is used as the "end" time.
 * 
 * \returns The time to take to connect to the remote endpoint in milliseconds.=
 */
std::chrono::milliseconds CstiCall::ConnectingDurationGet ()
{
	std::chrono::milliseconds connectingDuration = {};
	
	Lock ();
	
	if (m_protocolCall)
	{
		connectingDuration = m_protocolCall->connectingDurationGet ();
	}
	else
	{
		stiASSERTMSG(false, "Attempting to get connecting duration with no protocol call");
	}
	
	Unlock ();
	
	return connectingDuration;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::CallDurationGet
//
// Description: Returns the number of seconds that this call has been connected.
//
// Abstract:
//
// Returns:
//	double - number of seconds this call has been connected.
//
double CstiCall::CallDurationGet ()
{
	stiLOG_ENTRY_NAME (CstiCall::CallDurationGet);

	double callTime = 0;

	Lock ();
	
	if (m_protocolCall)
	{
		auto duration = m_protocolCall->connectedDurationGet ();
		
		callTime = duration.count() / 1000.0;
	}
	else
	{
		stiASSERTMSG(false, "Attempting to get call duration with no protocol call.");
	}
	
	Unlock ();
	return callTime;
} // end CstiCall::CallDurationGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::secondsSinceCallEndGet
//
// Description: Returns the number of seconds since the call terminated.
//
// Abstract:
//
// Returns:
//	double - number of seconds this call has been disconnected.
//
double CstiCall::secondsSinceCallEndGet ()
{
	stiLOG_ENTRY_NAME (CstiCall::secondsSinceCallEndGet);

	double timeSinceEnded = 0;

	Lock ();

	if (m_protocolCall)
	{
		timeSinceEnded = static_cast<double>(m_protocolCall->secondsSinceCallEndGet ().count());
	}
	else
	{
		stiASSERTMSG(false, "Attempting to get time since call ended without a protocol call.");
	}

	Unlock ();

	return timeSinceEnded;
} // end CstiCall::secondsSinceCallEndGet


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::StatsCollect
//
// Description: Cause statistics to be collected for this call object.
//
// Abstract:
//
// Returns:
//	none
//
void CstiCall::StatsCollect ()
{
	// Lock the call object during the stats collection
	Lock ();

	//
	// Is this call object in a Connected or Hold state?
	//
	if (StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
	{
		auto protocolCall = ProtocolCallGet ();

		if (protocolCall)
		{
			protocolCall->StatsCollect ();
			
			// Collect stats for any associated DHV call.
			auto sipCall = std::static_pointer_cast<CstiSipCall> (protocolCall);
			if (sipCall->dhvCstiCallGet () &&
				sipCall->dhvCstiCallGet ()->ProtocolCallGet ())
			{
				sipCall->dhvCstiCallGet ()->ProtocolCallGet ()->StatsCollect ();
			}
		}
	}

	// Release the lock
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::StatsClear
//
// Description: Cause statistics to be cleared for this call object.
//
// Abstract:
//
// Returns:
//	none
//
void CstiCall::StatsClear ()
{
	Lock ();

	auto protocolCall = ProtocolCallGet ();

	if (protocolCall)
	{
		protocolCall->StatsClear ();
	}

	Unlock ();
}


EstiBool CstiCall::AllowHangUpGet () const
{
	EstiBool bAllowHangup;

	Lock ();

	bAllowHangup = m_protocolCall ? m_protocolCall->AllowHangUpGet () : estiTRUE;

	Unlock ();

	return bAllowHangup;
}


void CstiCall::AllowHangupSet (EstiBool bAllow)
{
	callTraceRecordMessage ("%d", bAllow);

	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->AllowHangupSet (bAllow);
	}

	Unlock ();
}


EstiBool CstiCall::IsHoldableGet () const
{
	EstiBool bIsHoldable;

	Lock ();

	bIsHoldable = m_protocolCall ? m_protocolCall->IsHoldableGet () : estiFALSE;

	Unlock ();

	return bIsHoldable;
}


EstiBool CstiCall::IsTransferableGet () const
{
	EstiBool bIsTransferable;

	Lock ();

	bIsTransferable = m_protocolCall ? m_protocolCall->IsTransferableGet () : estiFALSE;

	// If bIsTransferable is true ...
	if (bIsTransferable &&

		// check to make sure we aren't connected with an MCU.
		// If we wish to test against only a GVC MCU, change the parameter passed in
		// to estiMCU_GVC.
		(ConnectedWithMcuGet (estiMCU_ANY) ||

		// Is the remote device type a hold server?
		RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) ||

		// Is the remote interface mode either Interpreter or Tech Support mode?
		RemoteInterfaceModeGet () == estiINTERPRETER_MODE ||
		RemoteInterfaceModeGet () == estiTECH_SUPPORT_MODE))
	{
		bIsTransferable = estiFALSE;
	}

	Unlock ();

	return bIsTransferable;
}


void CstiCall::RemoteNameGet (std::string *pName) const
{
	Lock ();

	//
	// If we have been transferred then always show the 
	// name from the first call.
	//
	if (TransferredGet())
	{
		pName->assign (m_szOriginalRemoteName);
	}
	else
	{
		if (m_protocolCall)
		{
			m_protocolCall->RemoteNameGet (pName);
		}
		else
		{
			// Check to see if they have a contact name for the callee
			RemoteContactNameGet (pName);
			if (pName->empty())
			{
				RemoteCallListNameGet (pName);
			}
		}
	}

	Unlock ();
}


void CstiCall::RemoteAlternateNameGet (std::string *pName) const
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->RemoteAlternateNameGet (pName);
	}
	else
	{
		pName->clear ();
	}

	Unlock ();
}


EstiInterfaceMode CstiCall::RemoteInterfaceModeGet () const
{
	EstiInterfaceMode eInterfaceMode;

	Lock ();

	eInterfaceMode = m_protocolCall ? m_protocolCall->RemoteInterfaceModeGet () : estiSTANDARD_MODE;

	Unlock ();

	return eInterfaceMode;
}


void CstiCall::VideoPlaybackSizeGet (uint32_t *pW, uint32_t *pH) const
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->VideoPlaybackSizeGet (pW, pH);
	}
	else
	{
		*pW = 1; *pH = 1;
	}

	Unlock ();
}


EstiSwitch CstiCall::VideoPlaybackMuteGet ()
{
	if (m_protocolCall)
	{
		return m_protocolCall->VideoPlaybackMuteGet ();
	}

	return estiOFF;
}



void CstiCall::VideoRecordSizeGet (uint32_t *pW, uint32_t *pH) const
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->VideoRecordSizeGet (pW, pH);
	}
	else
	{
		*pW = 1;
		*pH = 1;
	}

	Unlock ();
}

/*!
 * \brief TextSend
 *
 * Add text share text to the existing call. 
 * TODO this class should be enhanced to clear the text sharing
 * if the call is put on hold and re-add it later.  That way text sharing
 * can be call independent in the case where there is call waiting.
 * All it needs to do is save the smart ptr to the text share and then
 * apply the share again when the call comes out of the hold state.
 *
 * \param pwszText The string to be sent (unicode characters).
 */
stiHResult CstiCall::TextSend (const uint16_t *pwszText, EstiSharedTextSource sharedTextSource)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	
	// Make sure the state is valid for this function
	if (StateGet() == esmiCS_CONNECTED)
	{
		if (TextSupportedGet())
		{
			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(m_protocolCall);
			sipCall->TextSend (pwszText, sharedTextSource);
		}
		else
		{
			stiTrace("CstiCall::TextSend-- Text not supported\n\n");
		}
	}
	else
	{
		stiTrace("CstiCall::TextSend-- ERROR!  Wrong call state for sending text.\n\n");
	}

	Unlock ();

	return hResult;
}


void CstiCall::TextShareRemoveRemote()
{
	callTraceRecord;

	if (m_protocolCall)
	{
		m_protocolCall->TextShareRemoveRemote ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::RemotePreferredLanguageGet
//
// Description: Get the remote preferred language to be used for interpretive
// services.
//
// Returns:
//
std::string CstiCall::RemotePreferredLanguageGet () const
{
	Lock();
	std::string remotePreferredLanguage = m_remotePreferredLanguage;
	Unlock();
	return remotePreferredLanguage;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiCall::RemotePreferredLanguageSet
//
// Description: Set the local preferred language to be used for interpretive
// services.
//
// Returns:
//
void CstiCall::RemotePreferredLanguageSet (const std::string &preferredLanguage)
{
	Lock();
	m_remotePreferredLanguage = preferredLanguage;
	Unlock();
}


int CstiCall::RemotePtzCapsGet () const
{
	int nPtzCaps;

	Lock ();

	nPtzCaps = m_protocolCall ? m_protocolCall->RemotePtzCapsGet () : 0;

	Unlock ();

	return nPtzCaps;
}


void CstiCall::StatisticsGet (SstiCallStatistics *pStats) const
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->StatisticsGet (pStats);
	}
	else
	{
		*pStats = {};
	}

	Unlock ();
}


EstiBool CstiCall::RemoteDeviceTypeIs (int nType) const
{
	EstiBool bResult;

	Lock ();

	bResult = m_protocolCall ? m_protocolCall->RemoteDeviceTypeIs (nType) : estiFALSE;

	Unlock ();

	return bResult;
}

EstiBool CstiCall::TextSupportedGet () const
{
	Lock ();
	
	EstiBool bRet = m_protocolCall ? m_protocolCall->TextSupportedGet () : estiFALSE;
	
	Unlock ();
	
	return (bRet);
};

EstiBool CstiCall::DtmfSupportedGet () const
{
	Lock ();
	
	EstiBool bRet = m_protocolCall ? m_protocolCall->DtmfSupportedGet () : estiFALSE;
	
	Unlock ();
	
	return (bRet);
};

void CstiCall::DtmfToneSend (
	EstiDTMFDigit eDTMFDigit)
{
	if (m_protocolCall)
	{
		m_protocolCall->DtmfToneSend(eDTMFDigit);
	}
}


EstiProtocol CstiCall::ProtocolGet () const
{
	if (m_protocolCall)
	{
		return m_protocolCall->ProtocolGet();
	}
	return estiPROT_UNKNOWN;
}


void CstiCall::CalledNameGet (std::string *pCalledName) const
{
	Lock ();
	pCalledName->assign (m_szCalledName);
	Unlock ();
}


void CstiCall::CalledNameSet (const char *szName)
{
	callTraceRecordMessageGDPR ("%s", szName);

	Lock ();
	if (nullptr != szName)
	{
		strncpy (m_szCalledName, szName, sizeof (m_szCalledName) - 1);
		m_szCalledName[sizeof (m_szCalledName) - 1] = '\0';
	} // end if
	Unlock ();
};

/*
 * Gets the phone number provided by the Video Mail Server.
 * If this is set the call most likely was between two Sorenson endpoints and ended up being routed to Video Mail Server.
 * We can use this number to perform a Directory resolve to leave a SignMail without involving the Video Mail Server.
 */
void CstiCall::VideoMailServerNumberGet(std::string *pVideoMailNumber) const
{
	Lock();
	*pVideoMailNumber = m_VideoMailServerNumber;
	Unlock();
};

/*
 * Sets the phone number received from the Video Mail Server.
 */
void CstiCall::VideoMailServerNumberSet(const std::string *pVideoMailNumber)
{
	Lock();
	m_VideoMailServerNumber = *pVideoMailNumber;
	Unlock();
};

/*
* Gets if the call is a Direct SignMail.
*/
bool CstiCall::directSignMailGet () const
{
	return m_directSignMail;
};

/*
* Sets if the call is a Direct SignMail.
*/
void CstiCall::directSignMailSet (bool directSignMail)
{
	Lock ();
	m_directSignMail = directSignMail;
	Unlock ();
};

/**
 * Gets the CallId from the CstiProtocolCall object
 */
void CstiCall::CallIdGet (std::string *pCallId)
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->CallIdGet (pCallId);
	}

	Unlock ();
}

void CstiCall::OriginalCallIDGet (std::string *pCallID)
{
	Lock ();

	if (m_protocolCall)
	{
		m_protocolCall->OriginalCallIDGet (pCallID);
	}

	Unlock ();
}

void CstiCall::RemoteProductNameGet (std::string *pRemoteProductName) const
{
	Lock ();
	if (m_protocolCall)
	{
		m_protocolCall->RemoteProductNameGet(pRemoteProductName);
	}
	else
	{
		*pRemoteProductName = "";
	}
	Unlock ();
}

bool CstiCall::ProtocolModifiedGet ()
{
	if (m_protocolCall)
	{
		return m_protocolCall->ProtocolModifiedGet();
	}
	return false;
}

stiHResult CstiCall::ContactShare (const SstiSharedContact &contact)
{
	auto sipCall = std::static_pointer_cast<CstiSipCall>(m_protocolCall);
	return sipCall->ContactShare (contact);
}
	

bool CstiCall::ShareContactSupportedGet ()
{
	auto sipCall = std::static_pointer_cast<CstiSipCall>(m_protocolCall);
	return sipCall->ShareContactSupportedGet ();
}


/*!\brief Determine if the current connection is with an MCU of the type(s)
* 		passed in.
*
* \param unType - Can be any of the EstiMcuType enum values or the ORed product of them.
* 		Pass in estiMCU_ANY if you simply wish to know that you are connected with any type of MCU.
* \return estiTRUE if any one of the enum's passed in is the current type we are connected with,
* \return estiFALSE otherwise
*/
EstiBool CstiCall::ConnectedWithMcuGet (unsigned int unType) const
{
	EstiBool bRet = estiFALSE;

	Lock ();

	bRet = m_eConnectedWithMcuType & unType ? estiTRUE : estiFALSE;

	Unlock ();

	return bRet;
}


/*!\brief Set the MCU type we are connected with
*
* \param eType - the type of MCU we are connected with.
* 		estiMCU_NONE - Not connected with an MCU
* 		estiMCU_GENERIC - Connected with an MCU but not through Group Video Chat
* 		estiMCU_GVC - Connecte with an MCU for Group Video Chat
* \return an stiHResult signifying success or failure
*/
stiHResult CstiCall::ConnectedWithMcuSet (EstiMcuType eType)
{
	callTraceRecordMessage ("%d", eType);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	switch (eType)
	{
		case estiMCU_NONE:
		case estiMCU_GVC:
			m_eConnectedWithMcuType = eType;
			break;

		case estiMCU_GENERIC:
			// If we have already set this to estiMCU_GVC, don't change it.
			if (estiMCU_GVC != m_eConnectedWithMcuType)
			{
				m_eConnectedWithMcuType = eType;
			}
			break;

		case estiMCU_ANY:
			stiTHROW (stiRESULT_ERROR);
			break;
	}

STI_BAIL:

	Unlock ();

	return hResult;
}


/*!\brief Get the Group Video Chat conference room id
*
* \param pConferenceId - the conference room id will be returned in this parameter
* \return an stiHResult signifying success or failure
*/
stiHResult CstiCall::ConferencePublicIdGet (std::string *pConferenceId) const
{
	Lock ();

	*pConferenceId = m_stConferenceRoomStats.ConferencePublicId;

	Unlock ();

	return stiRESULT_SUCCESS;
}


/*!\brief Set the Group Video Chat conference room id
*
* \param pConferenceId - the conference room id
* \return an stiHResult signifying success or failure
*/
stiHResult CstiCall::ConferencePublicIdSet (const std::string *pConferenceId)
{
	callTraceRecordMessageGDPR ("%s", pConferenceId->c_str ());

	Lock ();

	m_stConferenceRoomStats.ConferencePublicId = *pConferenceId;

	Unlock ();

	return stiRESULT_SUCCESS;
}


/*!\brief Update the conference room stats
*
* \param bAddAllowed - A boolean indicating that additional participants are allowed in the room
* \param nRoomActive - the number of current participants in this room
* \param nRoomAllowed - the number of participants allowed in this room
* \return an stiHResult signifying success or failure
*/
stiHResult CstiCall::ConferenceRoomStatsSet (
		bool bAddAllowed,
		int nRoomActive,
		int nRoomAllowed,
		const std::list<SstiGvcParticipant> *pParticipantList)
{
	Lock ();

	m_stConferenceRoomStats.bAddAllowed = bAddAllowed;
	m_stConferenceRoomStats.nRoomActive = nRoomActive;
	m_stConferenceRoomStats.nRoomAllowed = nRoomAllowed;
	m_stConferenceRoomStats.ParticipantList = *pParticipantList;
	
	stiHResult hResult = stiRESULT_SUCCESS;

	if (nRoomActive > m_stConferenceRoomStats.peakParticipants)
	{
		m_stConferenceRoomStats.peakParticipants = nRoomActive;
	}

	Unlock ();

	return hResult;
}

/*!\brief Can a participant be added to the conference room
*
* This considers the number of active vs the number allowed unless pbLackOfResources is passed in.
* \param pbLackOfResources - if passed in and the function is returning false, it is set to inform
*				the calling function if the reason is due to resources.
* \return true if allowed, false otherwise
*/
bool CstiCall::ConferenceRoomParticipantAddAllowed (
	bool *pbLackOfResources)
{
	Lock ();

	// Set the return value to true if the number of active is less than allowed.
	bool bRet = m_stConferenceRoomStats.nRoomActive < m_stConferenceRoomStats.nRoomAllowed;

	// Did we get a return parameter passed in?
	if (pbLackOfResources)
	{
		// Yes, need to take into account if it is due to lack of resources on the MCU.

		// If not allowed, is it because of resources?
		if (!m_stConferenceRoomStats.bAddAllowed)
		{
			// If add is not allowed, it is because we lack resources.
			*pbLackOfResources = true;
			bRet = false;
		}
		else
		{
			*pbLackOfResources = false;
		}
	}

	Unlock ();

	return bRet;
}


/*!\brief Returns the peak number of participants during a group call
 *
 */
int CstiCall::ConferenceRoomPeakParticipantsGet () const
{
	Lock ();

	auto peakParticipants = m_stConferenceRoomStats.peakParticipants;

	Unlock ();

	return peakParticipants;
}


void CstiCall::HearingStatusSend (EstiHearingCallStatus eHearingStatus)
{
	callTraceRecordMessage ("%d", eHearingStatus);

	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	if (sipCall)
	{
		sipCall->HearingStatusSend (eHearingStatus);
	}
}

void CstiCall::NewCallReadyStatusSend (bool bNewCallReadyStatus)
{
	callTraceRecordMessage ("%d", bNewCallReadyStatus);

	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	if (sipCall)
	{
		sipCall->NewCallReadyStatusSend (bNewCallReadyStatus);
	}
}


bool CstiCall::CanSpawnNewRelayCall ()
{
	bool bSpawnNewCall = false;
	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	if (sipCall)
	{
		bSpawnNewCall = sipCall->NewCallReadyStatusGet();
	}
	return bSpawnNewCall;
}


EstiHearingCallStatus CstiCall::HearingCallStatusGet ()
{
	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	
	EstiHearingCallStatus hearingStatus = estiHearingCallDisconnected;
	
	if (sipCall)
	{
		hearingStatus = sipCall->HearingStatusGet();
	}
	return hearingStatus;
}


void CstiCall::SpawnCall(const SstiSharedContact &CallInfo)
{
	callTraceRecord;

	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	if (sipCall)
	{
		sipCall->SpawnCallSend(CallInfo);
	}
}

/*
 * Gets if the call needs geolocation.
 */
bool CstiCall::geolocationRequestedGet () const
{
	Lock ();
	bool geolocationRequested = m_geolocationRequested;
	Unlock ();
	
	return geolocationRequested;
}

/*
 * Sets if the call requested geolocation.
 */
void CstiCall::geolocationRequestedSet (bool geolocationRequested)
{
	callTraceRecordMessage ("%d", geolocationRequested);

	Lock ();
	m_geolocationRequested = geolocationRequested;
	Unlock ();
}

/*
 * Gets if geolocation is available for the call.
 */
GeolocationAvailable CstiCall::geolocationAvailableGet () const
{
	Lock ();
	GeolocationAvailable geolocationAvailable = m_geolocationAvailable;
	Unlock ();
	
	return geolocationAvailable;
}

/*
 * Sets if geolocation is available for the call.
 */
void CstiCall::geolocationAvailableSet (GeolocationAvailable geolocationAvailable)
{
	callTraceRecordMessage ("%d", geolocationAvailable);

	Lock ();
	m_geolocationAvailable = geolocationAvailable;
	Unlock ();
}


/*!\brief Save the devices initial orientation for logging
 *
 * Gets the preferred display size and stores if the call is starting
 * in landscape or portrait orientation. This will be used in call stats logging.
 */
void CstiCall::InitialPreferredDisplaySizeLog ()
{
	if (m_initialOrientation == DisplayOrientation::Unknown)
	{
		unsigned int displayWidth = 0;
		unsigned int displayHeight = 0;
		IstiVideoOutput::InstanceGet ()->PreferredDisplaySizeGet(&displayWidth, &displayHeight);
		
		if (displayWidth < displayHeight)
		{
			m_initialOrientation = DisplayOrientation::Portrait;
		}
		else
		{
			m_initialOrientation = DisplayOrientation::Landscape;
		}
		
		m_currentOrientation = m_initialOrientation;
	}
}


/*!\brief Save changes in preferred display size for logging.
 *
 * Gets the preferred display size and stores how long the call was in the
 * previous orientation. Also saves number of changes in a call. Values will be 
 * logged with call stats at end of call.
 */
void CstiCall::PreferredDisplaySizeChangeLog ()
{
	unsigned int displayWidth = 0;
	unsigned int displayHeight = 0;
	
	IstiVideoOutput::InstanceGet ()->PreferredDisplaySizeGet(&displayWidth, &displayHeight);
	
	if (displayWidth < displayHeight)
	{
		m_DurationInLandscape = CallDurationGet () - m_DurationInPortrait;
		m_currentOrientation = DisplayOrientation::Portrait;
	}
	else
	{
		m_DurationInPortrait = CallDurationGet () - m_DurationInLandscape;
		m_currentOrientation = DisplayOrientation::Landscape;
	}
	
	m_NumberRotations++;
}


/*!\brief Gets the preferred display size stats to be logged
 *
 * Calculates the preferred orientation of the call and final duration in each orientation.
 * Then constructs stats string to be logged.
 *
 * \param[out] preferredDisplaySizeStats - String to contain preferred display stats.
 */
void CstiCall::PreferredDisplaySizeStatsGet (std::string *preferredDisplaySizeStats)
{
	std::stringstream displaySizeStats;
	std::string initialOrientation = (m_initialOrientation == DisplayOrientation::Portrait ? "Portrait" : "Landscape");
	std::string preferredOrientation;
	
	// We need to determine the final duration in the current orientation since the last change.
	if (m_currentOrientation == DisplayOrientation::Portrait)
	{
		m_DurationInPortrait = CallDurationGet () - m_DurationInLandscape;
	}
	else
	{
		m_DurationInLandscape = CallDurationGet () - m_DurationInPortrait;
	}
	
	preferredOrientation = m_DurationInPortrait > m_DurationInLandscape ? "Portrait" : "Landscape";
	
	displaySizeStats << "\nPreferred Display Size Stats:\n";
	displaySizeStats << "\tInitialOrientation = " << initialOrientation << "\n";
	displaySizeStats << "\tPreferredOrientation = " << preferredOrientation<< "\n";
	displaySizeStats << "\tNumbeOfRotations = " << m_NumberRotations << "\n";
	
	*preferredDisplaySizeStats = displaySizeStats.str ();
}


/*!\brief Stops the pleaseWaitTimer and vrsFailOverTimer.
 *
 *  \return none.
 */
void CstiCall::FailureTimersStop ()
{
	callTraceRecord;

	if (vrsFailOverTimer.IsActive ())
	{
		vrsFailOverTimer.Stop ();
	}
	
	if (pleaseWaitTimer.IsActive ())
	{
		pleaseWaitTimer.Stop ();
	}
}

vpe::EncryptionState CstiCall::encryptionStateGet () const
{
	if (m_protocolCall == nullptr)
	{
		return vpe::EncryptionState::NONE;
	}

	auto sipCall = std::static_pointer_cast<CstiSipCall>(m_protocolCall);

	return sipCall->encryptionStateGet ();
}

/*!\brief Causes event to be posted to use VRS failover route.
 *
 * Ensures that the disconnect of the SIP call is handled by the SipManager thread.
 *
 *  \return none.
 */
void CstiCall::ForceVRSFailover ()
{
	callTraceRecord;

	CstiProtocolManager *protocolManager = ProtocolManagerGet ();
		
	if (protocolManager)
	{
		CstiProtocolManager *protocolManager = ProtocolManagerGet ();
		protocolManager->VRSFailoverCallDisconnect (shared_from_this());
		
		m_useVRSFailover = true;
		m_forcedVRSFailover = true;
	}
	else
	{
		stiASSERTMSG (estiFALSE, "CstiCall vrs failover timer failed to obtain protocol manager.");
	}
}


/*!\brief Parses Call-Info for trs-user-ip.
 *
 * We want to return a valid IP address if one can be found. Otherwise we will
 * provide the entire Call-Info string in case there is a parsing error
 * that a manual inspection of the string might still allow for the call to billed.
 *
 *  \return String with valid trs-user-ip if found otherwise full Call-Info string.
 */
std::string CstiCall::TrsUserIPGet () const
{
	std::string sipCallInfo = SIPCallInfoGet ();
	std::string trsUserIP;
	
	if (std::string::npos != sipCallInfo.find (TRS_PURPOSE) &&
		std::string::npos != sipCallInfo.find (TRS_SIP_PREFIX))
	{
		auto ipStart = sipCallInfo.find (TRS_SIP_PREFIX) + TRS_SIP_PREFIX.length ();
		sipCallInfo.erase (sipCallInfo.find (">"));
		sipCallInfo = sipCallInfo.substr (ipStart);
		
		if (IPAddressValidateEx (sipCallInfo.c_str ()))
		{
			trsUserIP = sipCallInfo;
		}
		else
		{
			stiASSERTMSG(estiFALSE, "Call-Info does not contain valid address IP=%s", sipCallInfo.c_str ());
		}
	}
	else
	{
		stiASSERTMSG(estiFALSE, "Call-Info=%s is not formated properly to parse trs-user-ip", sipCallInfo.c_str ());
	}
	
	return trsUserIP;
}


/*!\brief Returns true if the call state is such that media is allowed to be sent
 *
 */
bool CstiCall::mediaSendAllowed () const
{
	return ((StateValidate (esmiCS_CONNECTED)
	 && !SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD | estiSUBSTATE_NEGOTIATING_RMT_HOLD | estiSUBSTATE_ESTABLISHING))
	 || SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME | estiSUBSTATE_NEGOTIATING_RMT_RESUME));
}


/*!\brief Sets if the call is placed using the unregistered domain.
 *
 */
void CstiCall::unregisteredCallSet (bool unregistered)
{
	callTraceRecordMessage ("%d", unregistered);

	m_unregisteredCall = unregistered;
}


/*!\brief Returns true if the call was placed using the unregistered domain.
 *
 */
bool CstiCall::unregisteredCallGet () const
{
	return m_unregisteredCall;
}


/*!\brief Sets the remote address we are sending SIP messages to. Converts to IPv4 if passed an IPv6 address.
 *
 */
void CstiCall::remoteDestSet (const std::string &remoteAddr)
{
	callTraceRecordMessageGDPR ("%s", remoteAddr.c_str ());

	std::string mappedAddr = remoteAddr;
	
	if (IPv6AddressValidateEx (remoteAddr.c_str ()))
	{
		stiMapIPv6AddressToIPv4 (remoteAddr, &mappedAddr);
	}
	
	m_remoteDest = mappedAddr;
}


/*!\brief Returns the remote address we are sending INVITES to.
 *
 */
std::string CstiCall::remoteDestGet () const
{
	return m_remoteDest;
}


std::string CstiCall::localIpAddressGet() const
{
	auto routingAddress = RoutingAddressGet();
	auto routingIpAddress = routingAddress.ipAddressGet();
	std::string result = routingIpAddress.OriginalAddressGet();

	if (result.empty())
	{
		auto call = std::static_pointer_cast<CstiSipCall>(m_protocolCall);
		if (!call->IsBridgedCall())
		{
			result = m_remoteDest;
		}
		else
		{
			stiASSERTMSG(false, "Unable to determine remote IP address for call");
		}
	}

	return IstiNetwork::InstanceGet()->localIpAddressGet(estiTYPE_IPV4, result);
}

/*!\brief Set if a callLeg was ever created for this call object.
 *
 */
void CstiCall::callLegCreatedSet (bool created)
{
	callTraceRecordMessage ("%d", created);

	m_CallLegCreated = created;
}


/*!\brief Returns if a callLeg was ever created for this call object.
 *
 */
bool CstiCall::callLegCreatedGet () const
{
	return m_CallLegCreated;
}


/*!\brief Set if we want to log this call.
 *
 */
void CstiCall::logCallSet (bool logCall)
{
	m_logCall = logCall;
}


/*!\brief Returns if a call can be logged. Default is true.
 *
 */
bool CstiCall::logCallGet () const
{
	return m_logCall;
}

ISignal<IstiCall::DhviState>& CstiCall::dhviStateChangeSignalGet ()
{
	return dhviStateChangeSignal;
}


void CstiCall::dhviStateSet (IstiCall::DhviState dhviState)
{
	callTraceRecordMessage ("%d", dhviState);

	Lock ();
	m_dhviState = dhviState;
	Unlock ();
	
	dhviStateChangeSignal.Emit (dhviState);
	
	stiDEBUG_TOOL (g_stiDhvStateChange,
		stiTrace("Changing DHV State to %d\n", dhviState);
	);
}


IstiCall::DhviState CstiCall::dhviStateGet ()
{
	Lock ();
	auto dhviState = m_dhviState;
	Unlock ();
	
	return dhviState;
}


void CstiCall::dhviCapabilityCheck (const std::string &phoneNumber)
{
	m_hearingCapabilityCheckNumber = phoneNumber;
	
	if (SubstateGet () == estiSUBSTATE_CONFERENCING && RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		auto sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
		if (sipCall)
		{
			sipCall->sendDhviCapabilityCheckMsg (phoneNumber);
		}
		
		m_hearingCapabilityCheckNumber.clear ();
	}
}


/*!\brief Disconnects DHV call using SipManager thread.
 *
 * \note This needs to be called if attempting to disconnect the DHV call from a  thread other than SipManager. Resolves bug#30510.
*/
stiHResult CstiCall::dhviMcuDisconnect ()
{
	callTraceRecord;

	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(ProtocolCallGet());
	
	stiTESTCONDMSG (sipCall, stiRESULT_VALUE_NOT_FOUND , "dhviMcuDisconnect failed to find CstiSipCall");
	
	sipCall->SipManagerGet ()->dhvSipMgrMcuDisconnectCall (shared_from_this());
	
STI_BAIL:
	
	return hResult;
}


void CstiCall::dhvHearingNumberSet (const std::string &hearingNumber)
{
	callTraceRecordMessageGDPR ("%s", hearingNumber.c_str ());
	m_dhvHearingNumber = hearingNumber;
}


std::string CstiCall::dhvHearingNumberGet () const
{
	return m_dhvHearingNumber;
}

void CstiCall::dhvConnectingTimerStart (CstiEventQueue* eventQueue)
{
	callTraceRecord;
	m_dhvConnectingTimerSignalConnection = m_dhvConnectingTimer.timeoutEvent.Connect (
		[this, eventQueue]() {
			eventQueue->PostEvent (
				[this]() {
					std::string callID;
					CallIdGet (&callID);
					stiRemoteLogEventSend ("EventType=DHVI Reason=NoAnswer CallID=%s", callID.c_str ());
					dhviMcuDisconnect ();
				}
			);
		}
	);
	m_dhvConnectingTimer.Start ();
}

void CstiCall::dhvConnectingTimerStop ()
{
	callTraceRecord;
	m_dhvConnectingTimer.Stop ();
	m_dhvConnectingTimerSignalConnection.Disconnect ();
}

const std::vector<SstiSipHeader>& CstiCall::additionalHeadersGet() const
{
	return m_additionalHeaders;
}

void CstiCall::additionalHeadersSet(const std::vector<SstiSipHeader>& additionalHeaders)
{
	m_additionalHeaders = additionalHeaders;
}

void CstiCall::additionalHeadersSet(const std::vector<SstiSipHeader>&& additionalHeaders)
{
	m_additionalHeaders = std::move(additionalHeaders);
}

void CstiCall::collectTrace (std::string message)
{
	m_logs.collectGeneralTrace (message);
}

void CstiCall::sendTrace ()
{
	m_logs.sendTrace ();
}

// end file CstiCall.cpp
