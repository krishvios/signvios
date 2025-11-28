////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipRegistration
//
//  File Name:  CstiSipRegistration.cpp
//
//  Abstract:
//	  This class implements the methods specific to a Registration such as the
//	  credentials and the valid and invalid addresses to manage.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiSipRegistration.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "stiSipTools.h"
#include "stiSipConstants.h"
#include "Sip/Message.h"
#include <sstream>

// Radvision
#include "RvSipRegClientTypes.h"
#include "RvSipRegClient.h"
#include "RvSipAddress.h"
#include "RvSipOtherHeader.h"
#include "RvSipViaHeader.h"
#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
#include "TransportConnection.h"
#endif
#ifdef stiTUNNEL_MANAGER
#include "rvsockettun.h"
#endif

//
// Constants
//

#define MAX_TIME_UNTIL_RE_REGISTER 21600 // in seconds (max is 6 hours)
#define FAST_REGISTER_FIXUP	1 // set irrationally fast re-registrations to STANDARD_REGISTRATION_RATE
#define TIME_TO_REGISTER_BEFORE_EXPIRES 10 // in seconds
#define STANDARD_REGISTRATION_RATE 85 // in seconds
#define MIN_REGISTRATION_RATE ((TIME_TO_REGISTER_BEFORE_EXPIRES * 2) + 1) // in seconds

//
// Typedefs
//

//
// Forward Declarations
//

// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(pThis,fmt,...) stiDEBUG_TOOL (g_stiSipRegDebug, stiTrace ("SIP Registration: %p/%p/%s: " fmt "\n", pThis, pThis->m_hRegClient, pThis->m_PhoneNumber.c_str (), ##__VA_ARGS__); )

#define TIME_TO_WAIT_ON_ERROR 15000 // How long to wait (in ms) before re-attempting a failed registration

//
// Globals
//


//
// Locals
//


//
// Class Definitions
//

//:-----------------------------------------------------------------------------
//:
//:								MEMBER FUNCTIONS
//:
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::CstiSipRegistration
//
// Description: Default constructor
//
// Abstract:
//  This method constructs the CstiSipRegistration object
//
// Returns:
//  none
//
CstiSipRegistration::CstiSipRegistration (
	const char *pszPhoneNumber,
	const std::string &SipInstanceGUID,
	int nRegistrationID,
	HRPOOL hAppPool,
	RvSipRegClientMgrHandle hRegClientMgr,
	RvSipStackHandle hStackMgr,
	std::recursive_mutex &execMutex,
	PstiSipRegistrationEventCallback pEventCallback,
	void *pCallbackParam)
	:
	m_bUnregistering (false),
	m_bTerminating (false),
	m_bQueryCompleted (false),
	m_hAppPool (hAppPool),
	m_hRegClient (nullptr),
	m_bRegistrarLoginAttempted (false),
	m_nRegistrationTimeout (300000), // 5 min
	m_wdidReRegisterTimer (nullptr),
//	m_hInvalidContactsPage (0),
	m_hValidContactsPage (nullptr),
	m_pUserData(pCallbackParam),
	m_pEventCallback (pEventCallback),
	m_PhoneNumber (pszPhoneNumber),
	m_SipInstanceGUID (SipInstanceGUID),
	m_nRegistrationID (nRegistrationID),
	m_hRegClientMgr (hRegClientMgr),
	m_hStackMgr (hStackMgr),
	m_un16RPort (0),
	m_eCurrentOperation(eOperationNone),
	m_execMutex (execMutex),
	m_un32RegistrationExpireTime (0)
{
} // end CstiSipRegistration::CstiSipRegistration


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::~CstiSipRegistration
//
// Description: CstiSipRegistration class destructor
//
// Abstract:
//  This function frees resources associated with the CstiSipRegistration object.
//
// Returns:
//  none
//
CstiSipRegistration::~CstiSipRegistration ()
{
	// Stop any registration going on
	RegisterStop ();
		
//	if (m_hInvalidContactsPage)
//	{
//		m_vInvalidContacts.clear ();
//		RPOOL_FreePage (m_hAppPool, m_hInvalidContactsPage);
//		m_hInvalidContactsPage = NULL;
//	}
//
	if (m_hValidContactsPage)
	{
		m_vValidContacts.clear ();
		RPOOL_FreePage (m_hAppPool, m_hValidContactsPage);
		m_hValidContactsPage = nullptr;
	}

} // end CstiSipRegistration::~CstiSipRegistration


/*!\brief Locks the registration object
 *
 * @return Success or failure
 */
stiHResult CstiSipRegistration::Lock ()
{
	m_execMutex.lock ();
	return stiRESULT_SUCCESS;
}


/*!\brief Unlocks the registration object
 *
 */
void CstiSipRegistration::Unlock ()
{
	m_execMutex.unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegistrarSettingsSet
//
// Description: Sets the current registrar settings
//
// Returns:
//  stiHResult
//
void CstiSipRegistration::RegistrarSettingsSet (const SstiRegistrarSettings *pstiRegistrarSettings)
{
	bool bRegistrarChanged = false;
	
	if (pstiRegistrarSettings->ProxyIpAddress != m_stiRegistrarSettings.ProxyIpAddress
	 || pstiRegistrarSettings->un16ProxyPort != m_stiRegistrarSettings.un16ProxyPort)
	{
		bRegistrarChanged = true;
	}
	
	m_stiRegistrarSettings = *pstiRegistrarSettings;
	
	// Any time we change the registrar kill the client
	// so a new one is created.
	//
	if (bRegistrarChanged)
	{
		//
		// When we change registrar we want to re-query the contacts to get our
		// reflexive address and port
		//
		m_bQueryCompleted = false;

		ContactClear ();
		RegisterStop ();
	}
	else
	{
		ContactSet ();
	}
}


/*!\brief Resets the state of the registration object back to a starting state.
 * 
 */
stiHResult CstiSipRegistration::Reset()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	DBG_MSG (this, "Reset.");

	m_ReflexiveAddress.clear ();
	m_un16RPort = 0;
	m_Queue.clear ();

	RegisterStop ();
	
	ContactClear ();
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegisterStop
//
// Description: Immediately halt the act of registration
//
// Abstract:
//
// Returns: stiHResult
//
stiHResult CstiSipRegistration::RegisterStop ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_wdidReRegisterTimer)
	{
		stiOSWdDelete (m_wdidReRegisterTimer);
		m_wdidReRegisterTimer = nullptr;
	}
	
	if (nullptr != m_hRegClient)
	{
		DBG_MSG (this, "Cancelling registration operation already in progress with operation %d", m_eCurrentOperation);

		m_bTerminating = true;
		
		// Begin termination
		RvSipRegClientTerminate (m_hRegClient);
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegClientReasonIntoString
//
// Description: Tool for decoding errors
//
// Abstract: This actually lives deep within the stack code, but they
// didn't provide an API to get at it, so I cut/pasted it out.
//
// Returns: error description string
//
static const char *RegClientReasonIntoString (
	RvSipRegClientStateChangeReason eReason)
{
	switch (eReason)
	{
		case RVSIP_REG_CLIENT_REASON_UNDEFINED:
			return "Undefined";
		case RVSIP_REG_CLIENT_REASON_USER_REQUEST:
			return "User request";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_SUCCESSFUL_RECVD:
			return "Response successful received";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_REDIRECTION_RECVD:
			return "Response redirection received";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_UNAUTHENTICATED_RECVD:
			return "Response unauthentication received";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_REQUEST_FAILURE_RECVD:
			return "Response request failure received";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_SERVER_FAILURE_RECVD:
			return "Response server failure received";
		case RVSIP_REG_CLIENT_REASON_RESPONSE_GLOBAL_FAILURE_RECVD:
			return "Response global failure received";
		case RVSIP_REG_CLIENT_REASON_LOCAL_FAILURE:
			return "Local failure";
		case RVSIP_REG_CLIENT_REASON_TRANSACTION_TIMEOUT:
			return "Transaction timeout";
		case RVSIP_REG_CLIENT_REASON_NORMAL_TERMINATION:
			return "Normal termination";
		case RVSIP_REG_CLIENT_REASON_GIVE_UP_DNS:
			return "Give Up DNS";
		case RVSIP_REG_CLIENT_REASON_NETWORK_ERROR:
			return "Network Error";
		case RVSIP_REG_CLIENT_REASON_503_RECEIVED:
			return "503 Received";
		case RVSIP_REG_CLIENT_REASON_CONTINUE_DNS:
			return "Continue DNS";
		default:
			return "<unknown>";
	}
} // end CstiSipRegistration::RegClientReasonIntoString


#define GET_CASE_STRING(s) case s: pszCaseString = #s; break;

static const char *RegClientStateIntoString (
	RvSipRegClientState eState)
{
	const char *pszCaseString = nullptr;
	
	switch (eState)
	{
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_UNDEFINED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_IDLE)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_TERMINATED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_REGISTERING)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_REDIRECTED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_UNAUTHENTICATED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_REGISTERED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_FAILED)
		GET_CASE_STRING (RVSIP_REG_CLIENT_STATE_MSG_SEND_FAILURE)
	}
	
	return pszCaseString;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::ReRegisterTimerRun
//
// Description: Runs the re-register timer once.
//
// Abstract:
//	This will set the re-register timer to run once.
//
// Returns:
//	none
//
void CstiSipRegistration::ReRegisterTimerRun ()
{
	if (nullptr != m_wdidReRegisterTimer)
	{

		DBG_MSG (this, "SIP RE-REGISTER: Timing out in %d seconds", m_nRegistrationTimeout / 1000);
		stiOSWdStart (m_wdidReRegisterTimer,
			static_cast<int>(m_nRegistrationTimeout),
			(stiFUNC_PTR) &ReRegisterTimerCB, (size_t)this);
	}
} // end CstiSipRegistration::ReRegisterTimerRun


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::StartReRegisterFor ()
//
// Description: Sets re-registration timer based on a register response message
//
// Returns: Nothing
//
void CstiSipRegistration::StartReRegisterFor (
	RvSipMsgHandle hMsg /// The message containing registration info
)
{
	RvSipHeaderListElemHandle hListElem = nullptr;

	// Extract the expires field(s) and re-register on the smallest
	unsigned int nNewTimeout = MAX_TIME_UNTIL_RE_REGISTER;
	bool bRegTimeoutChanged = true;
	RvUint32 nDeltaSec;
	RvSipExpiresHeaderHandle hExpires = nullptr;
	hExpires = (RvSipExpiresHeaderHandle)RvSipMsgGetHeaderByType (hMsg, RVSIP_HEADERTYPE_EXPIRES, RVSIP_FIRST_HEADER, &hListElem);

	if (nullptr != hExpires)
	{
		RvSipExpiresHeaderGetDeltaSeconds (hExpires, &nDeltaSec);
		if (nDeltaSec && nDeltaSec < nNewTimeout)
		{
			nNewTimeout = nDeltaSec;
			bRegTimeoutChanged = true;
		}
	}

	auto  hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hMsg, RVSIP_HEADERTYPE_CONTACT, RVSIP_FIRST_HEADER, &hListElem);
	if (!hContactHeader)
	{
		stiASSERTMSG (estiFALSE, "RE-REGISTER: No contact headers");
	}
	
	while (hContactHeader)
	{
		hExpires = RvSipContactHeaderGetExpires (hContactHeader);
		if (nullptr != hExpires)
		{
			RvSipExpiresHeaderGetDeltaSeconds (hExpires, &nDeltaSec);
			if (nDeltaSec && nDeltaSec < nNewTimeout)
			{
				nNewTimeout = nDeltaSec;
				bRegTimeoutChanged = true;
			}
		}
		else
		{
			DBG_MSG (this, "RE-REGISTER: No expires tag in contact");
		}
		
		hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hMsg, RVSIP_HEADERTYPE_CONTACT, RVSIP_NEXT_HEADER, &hListElem);
	}
	
	// Re-register slightly before it expires
	if (bRegTimeoutChanged)
	{
		if (nNewTimeout >= MIN_REGISTRATION_RATE)
		{
			SetReRegistrationTimeout ( (nNewTimeout - TIME_TO_REGISTER_BEFORE_EXPIRES) * 1000);
		}
		else
		{
			DBG_MSG (this, "WARN: We are re-registering very fast! (%d < %d)", nNewTimeout, MIN_REGISTRATION_RATE);
#ifdef FAST_REGISTER_FIXUP
			SetReRegistrationTimeout ((STANDARD_REGISTRATION_RATE - TIME_TO_REGISTER_BEFORE_EXPIRES) * 1000);				
#else // !FAST_REGISTER_FIXUP
			SetReRegistrationTimeout (nNewTimeout * 1000);
#endif // FAST_REGISTER_FIXUP
		}
		
		// Set when this registration will expire on the proxy.
		m_un32RegistrationExpireTime = stiOSTickGet () + (nNewTimeout * stiOSSysClkRateGet ());
		
		if (!m_wdidReRegisterTimer)
		{
			m_wdidReRegisterTimer = stiOSWdCreate ();
		}
		
		ReRegisterTimerRun (); // Start the re-register timer.
	}
	else
	{
		DBG_MSG (this, "No timer change.");
	}
} // end CstiSipRegistration::StartReRegisterFor ()


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegisterFailureRetry
//
// Description: Uses the timer to retry a failed registration after a period of time.
//
// Returns:
//	none
//
void CstiSipRegistration::RegisterFailureRetry ()
{
	// TODO:  May want to keep track and scale this up for subsequent failures
	SetReRegistrationTimeout (TIME_TO_WAIT_ON_ERROR);
		
	if (!m_wdidReRegisterTimer)
	{
		m_wdidReRegisterTimer = stiOSWdCreate ();
	}
	else
	{
		stiOSWdCancel (m_wdidReRegisterTimer);
	}
	
	ReRegisterTimerRun (); // Start the re-register timer.
}

RvSipTransportConnectionHandle CstiSipRegistration::ConnectionHandleGet () const
{
	RvSipTransportConnectionHandle connectionHandle = nullptr;
	RvStatus rvStatus = RV_OK;
	stiHResult hResult = stiRESULT_SUCCESS;

	rvStatus = RvSipRegClientGetConnection (m_hRegClient, &connectionHandle);
	stiTESTRVSTATUS ();

	stiUNUSED_ARG (hResult);

STI_BAIL:
	return connectionHandle;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::ReRegisterTimerCB
//
// Description: Handles the Retry timer expiration.
//
// Abstract:
//	This handler function is invoked by the re-register timer when it has
//	expired.  It posts a message to the message queue, signaling a timeout.
//
// Returns:
//	none
//
int CstiSipRegistration::ReRegisterTimerCB (
	size_t param) /// Pointer to THIS object.
{
	DBG_MSG (((CstiSipRegistration *)param), "Timeout expired. Re-registering...");
	
	((CstiSipRegistration *)param)->EventCallback (REG_EVENT_REREGISTER_NEEDED);
	
	// ((CstiSipRegistration *)param)->RegisterStart ();
	
	return (0);
} // end CstiSipRegistration::ReRegisterTimerCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::SetReRegistrationTimeout
//
// Description: Sets the timeout value for the re-register timer.
//
// Abstract:
//	This WILL NOT restart or start the timer.  To do that, call ReRegisterTimerRun ().
//
// Returns:
//	none
//
void CstiSipRegistration::SetReRegistrationTimeout (
	unsigned int nMilliseconds) /// The timeout
{
	DBG_MSG (this, "setting timeout to: %d", nMilliseconds);
	
	if (nMilliseconds > (MAX_TIME_UNTIL_RE_REGISTER * 1000))
	{
		nMilliseconds = MAX_TIME_UNTIL_RE_REGISTER * 1000;
		DBG_MSG (this, "\trequested time is too long. Setting to %d", nMilliseconds);
	}
	
	m_nRegistrationTimeout = nMilliseconds;
} // end CstiSipRegistration::SetReRegistrationTimeout


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::ValidContactsAttach
//
// Description: Attaches our current contacts which match a given filter to a message.
//	If there are none in the list, this will call ValidContactsCreate ()
//
// Abstract:
//
// Returns:
//		stiHResult
//
stiHResult CstiSipRegistration::ValidContactsAttach (
	RvSipMsgHandle hMsg, /// The message to attach contacts to
	int nMaxContacts  /// The max number of contacts to attach
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
//	RvStatus rvStatus = RV_OK;
	int n = 0;

	for (auto it = m_vValidContacts.begin ();
		n < nMaxContacts && it != m_vValidContacts.end ();
		++it)
	{
		RvSipAddressHandle hAddress = RvSipContactHeaderGetAddrSpec (*it);
		if (!hAddress)
		{
			stiASSERTMSG (estiFALSE, "Invalid address - %s", *it);
			continue;
		}

		switch (RvSipAddrUrlGetTransport (hAddress))
		{
			case RVSIP_TRANSPORT_TCP:
				// Sip TCP
				// Notice that if we are registering a UDP contact, and that's all we have room for, then
				// we'll skip adding TCP. Otherwise, TCP will be the first / preferred contact.
				if (n < nMaxContacts)
				{
					hResult = ContactHeaderAttach (hMsg, *it);
					stiTESTRESULT ();
					n++;
				}
				break;

			case RVSIP_TRANSPORT_TLS:

				hResult = ContactHeaderAttach (hMsg, *it);
				stiTESTRESULT ();
				n++;

				break;

			case RVSIP_TRANSPORT_UDP:
			case RVSIP_TRANSPORT_UNDEFINED:
				// Sip UDP
				hResult = ContactHeaderAttach (hMsg, *it);
				stiTESTRESULT ();
				n++;

				break;

			default:
				stiASSERTMSG (estiFALSE, "Unknown contact transport %d.  Skipping.", RvSipAddrUrlGetTransport (hAddress));
		};
	}
	
STI_BAIL:

	return hResult;
} // end CstiSipRegistration::ValidContactsAttach


/*!\brief Sets the expiration time for each contact to zero.
 * 
 * \param hOutboundMessage The outbound message that contains the contacts to modify.
 */
stiHResult ExpirationToZeroSet (
	RvSipMsgHandle hOutboundMessage) /// The message to attach contacts to
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Since we are unregistering, expire all the contacts
	RvSipHeaderListElemHandle hListElem = nullptr;
	auto  hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hOutboundMessage, RVSIP_HEADERTYPE_CONTACT, RVSIP_FIRST_HEADER, &hListElem);
	while (nullptr != hContactHeader)
	{
		// Set the expiration time
		RvSipExpiresHeaderHandle hExpires;
		hExpires = RvSipContactHeaderGetExpires (hContactHeader);
		if (!hExpires)
		{
			RvSipExpiresConstructInContactHeader (hContactHeader, &hExpires);
		}
		RvSipExpiresHeaderSetDeltaSeconds (hExpires, 0);
		
		// Inspect the next entry
		hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hOutboundMessage, RVSIP_HEADERTYPE_CONTACT, RVSIP_NEXT_HEADER, &hListElem);
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::InvalidContactsAttach
//
// Description: Attaches all contact needing unregistration to a given message
//
// Abstract:
//
// Returns:
//		stiHResult
//
stiHResult CstiSipRegistration::InvalidContactsAttach (
	RvSipMsgHandle hMsg /// The message to attach contacts to
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	for (auto &invalidContact : m_vInvalidContacts)
	{
		hResult = ContactHeaderAttach (hMsg, invalidContact);
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return hResult;
} // end CstiSipRegistration::InvalidContactsAttach


stiHResult CstiSipRegistration::RegistrationClientCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus;
	std::string ProxyAddress = m_stiRegistrarSettings.ProxyIpAddress;
	
	DBG_MSG (this, "Creating new registration client.");
	
	rvStatus = RvSipRegClientMgrCreateRegClient (m_hRegClientMgr, (RvSipAppRegClientHandle)this, &m_hRegClient);
	
	if (rvStatus != RV_OK)
	{
		// If RvSipRegClientMgrCreateRegClient returns an error we want to set m_hRegClient to NULL.
		// This is done to ensure that we do not call RvSipRegClientTerminate inside RegisterStop fixing bug 20696, it
		// also ensures we will attempt to re-create the register client.
		m_hRegClient = nullptr;
	}
	stiTESTRVSTATUS ();

#ifdef IPV6_ENABLED
	{
		CstiIpAddress proxyIpAddress;
		proxyIpAddress = ProxyAddress.c_str ();
		
		if (estiTYPE_IPV6 == proxyIpAddress.IpAddressTypeGet())
		{
			// If we are behind a NAT64 gateway we must convert the IP back to IPv4.
			// We tried using domain before and it breaks myPhone groups bug #23978.
			std::string ConvertedIP;
			stiHResult hResult2 = stiRESULT_SUCCESS;
			
			hResult2 = stiMapIPv6AddressToIPv4(ProxyAddress, &ConvertedIP);
			
			if (!stiIS_ERROR (hResult2) && !ConvertedIP.empty ())
			{
				ProxyAddress = ConvertedIP;
			}
		}
	}
#endif

	// Set the TO header
	{
		RvSipPartyHeaderHandle hTo;

		auto address = AddressStringCreate (
			ProxyAddress.c_str (),
			m_PhoneNumber.c_str (),
			m_stiRegistrarSettings.un16ProxyPort,
			estiTransportUnknown, "sip");

		auto toHeader = AddressHeaderStringCreate (
			"To",
			address);

		rvStatus = RvSipRegClientGetNewMsgElementHandle (m_hRegClient, RVSIP_HEADERTYPE_TO,
												RVSIP_ADDRTYPE_UNDEFINED, (void**)(void *)&hTo);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipPartyHeaderParse (hTo, RV_TRUE, const_cast<RvChar *>(toHeader.c_str ()));
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipRegClientSetToHeader (m_hRegClient, hTo);
		stiTESTRVSTATUS ();
	}

	// Set the FROM header
	{
		RvSipPartyHeaderHandle hFrom;

		// According to RFC3261 "The From header field contains the address-of-record of the person responsible for the registration.
		// 		The value is the same as the To header field unless the request is a third-party registration."
		auto address = AddressStringCreate (
			ProxyAddress.c_str (),
			m_PhoneNumber.c_str (),
			m_stiRegistrarSettings.un16ProxyPort,
			estiTransportUnknown, "sip");

		auto fromHeader = AddressHeaderStringCreate (
			"From",
			address
		);

		rvStatus = RvSipRegClientGetNewMsgElementHandle (m_hRegClient, RVSIP_HEADERTYPE_FROM,
							RVSIP_ADDRTYPE_UNDEFINED, (void**)(void *)&hFrom);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipPartyHeaderParse (hFrom, RV_FALSE, const_cast<RvChar *>(fromHeader.c_str ()));
		stiTESTRVSTATUS ();

		rvStatus = RvSipRegClientSetFromHeader (m_hRegClient, hFrom);
		stiTESTRVSTATUS ();
	}
	
	// Set the REGISTRAR header
	{
		RvSipAddressHandle hRegistrar;

		auto registrarHeader = AddressStringCreate (
			ProxyAddress.c_str (),
			nullptr,
			m_stiRegistrarSettings.un16ProxyPort,
			estiTransportUnknown, "sip"
		);
		rvStatus = RvSipRegClientGetNewMsgElementHandle (m_hRegClient, RVSIP_HEADERTYPE_UNDEFINED,
							RVSIP_ADDRTYPE_URL, (void**)(void *)&hRegistrar);
		stiTESTRVSTATUS ();

		rvStatus = RvSipAddrParse (hRegistrar, const_cast<RvChar *>(registrarHeader.c_str ()));
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipRegClientSetRegistrar (m_hRegClient, hRegistrar);
		stiTESTRVSTATUS ();
	}
	
STI_BAIL:

	stiDEBUG_TOOL (g_stiSipRegDebug,
						RvSipRegClientResources stRegClient;
						if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_REGCLIENT, &stRegClient))
						{
							stiTrace ("\n\n<CstiSipRegistration::RegistrationClientCreate> RegClient Stats Num Used = %d Max Used = %d Allocated Resources = %d\n\n",stRegClient.regClients.currNumOfUsedElements,
									stRegClient.regClients.maxUsageOfElements, stRegClient.regClients.numOfAllocatedElements);
						}
						else
						{
							stiTrace ("\n\n<CstiSipRegistration::RegistrationClientCreate> Failed to gather Registration Client resource information\n\n");
						});

	
	if (stiIS_ERROR(hResult))
	{
		RvSipRegClientResources stRegClient;
		bool bResourcesMaxed = false;
		
		// Check registration client resources.
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_REGCLIENT, &stRegClient))
		{
			if (stRegClient.regClients.currNumOfUsedElements >= stRegClient.regClients.numOfAllocatedElements)
			{
				bResourcesMaxed = true;
			}
		}
		else
		{
			stiASSERT (estiFALSE); // Failed to get stack resources.
		}
		if (bResourcesMaxed)
		{
			// There was an error creating the registration client due to a resource issue post an event for the SIP stack to handle.
			EventCallback (REG_EVENT_OUT_OF_RESOURCES);
		}
		stiASSERT (estiFALSE); // There was an error creating the registration client.
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegisterStart
//
// Description: Kicks off the act of registration
//
// Abstract:
//
// Returns: stiHResult
//
stiHResult CstiSipRegistration::RegisterStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Don't start a registration operation if the client is in the process of unregistering
	//
	if (!m_bUnregistering)
	{
		if (!m_bTerminating && m_eCurrentOperation == eOperationNone)
		{
			hResult = RegistrationExecute (eOperationRegister, nullptr);
		}
		else
		{
			DBG_MSG (this, "Queued up registration operation %d", eOperationRegister);
			m_Queue.push_back (eOperationRegister);
		}
		
		DBG_MSG (this, "Operations in queue: ");

		for (auto &registrationQueue : m_Queue)
		{
			DBG_MSG (this, "Operation: %d", registrationQueue);
		}
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::UnregisterStart
//
// Description: Kicks off the act of un-registration
//
// Abstract:
//
// Returns: stiHResult
//
stiHResult CstiSipRegistration::UnregisterStart ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Only unregister if we are not already unregistering and have valid contact information.
	// Previously we were calling RegistrationRequestCreate without a valid contact.
	// This was causing us to leak resources with Radvision. This change fixes bug 22725.
	if (!m_bUnregistering && !m_vValidContacts.empty ())
	{
		m_Queue.clear ();
		
		m_bUnregistering = true;
		
		if (!m_bTerminating && m_eCurrentOperation == eOperationNone)
		{
			hResult = RegistrationExecute (eOperationUnregister, nullptr);
		}
		else
		{
			DBG_MSG (this, "Queued up registration operation %d", eOperationUnregister);
			m_Queue.push_back (eOperationUnregister);
		}
	}
	
	return hResult;
}


stiHResult CstiSipRegistration::RegistrationNext ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bOperationStarted = false;
	
	DBG_MSG (this, "Setting operation to none");
		
	m_eCurrentOperation = eOperationNone;
	
	while (!bOperationStarted && !m_Queue.empty ())
	{
		EstiRegistrationOperation eOperation = m_Queue.front ();
		m_Queue.pop_front ();
		
		DBG_MSG (this, "Starting registration request: %d", eOperation);
		RegistrationExecute(eOperation, &bOperationStarted);
	}
	
	return hResult;
}


/*!\brief Creates a registrations request message based on the requested operation
 * 
 * \param peOperation The operation requested.  If eOperationNone is returned then the requested operation was not possible
 * \param hMsg The message in which the request is built.
 */
stiHResult CstiSipRegistration::RegistrationRequestCreate (
	EstiRegistrationOperation *peOperation,
	RvSipMsgHandle hMsg)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	vpe::sip::Message message(hMsg);

	// Re-set all CONTACT headers
	message.contactHeadersDelete ();

	switch (*peOperation)
	{
		case eOperationUnregister:
			
			if (!m_vValidContacts.empty ())
			{
				// Set all the current CONTACT headers to unregister
				hResult = ValidContactsAttach (message,
					m_stiRegistrarSettings.nMaxContacts);
				stiTESTRESULT ();
				
				hResult = ExpirationToZeroSet (message);
				stiTESTRESULT ();
			}
			else
			{
				DBG_MSG (this, "Nothing to do for operation %d", *peOperation);
				*peOperation = eOperationNone;
			}
			
			break;
			
		case eOperationUnregisterInvalid:
			
			if (!m_vInvalidContacts.empty ())
			{
				// Set all CONTACT headers to unregister
				hResult = InvalidContactsAttach (message);
				stiTESTRESULT ();
			}
			else
			{
				DBG_MSG (this, "Nothing to do for operation %d", *peOperation);
				*peOperation = eOperationNone;
			}
			
			break;

		case eOperationQuery:
			
			break;
			
		case eOperationRegister:
			
			if (!m_vValidContacts.empty ())
			{
				// Set all CONTACT headers to register
				hResult = ValidContactsAttach (message,
					m_stiRegistrarSettings.nMaxContacts);
				stiTESTRESULT ();
			}
			else
			{
				DBG_MSG (this, "No valid contact yet for operation %d; changing operation to query", *peOperation);
				*peOperation = eOperationQuery;
			}
			
			break;
			
		case eOperationNone:
			
			//
			// This case should never happen
			//
			stiASSERT (estiFALSE);
			
			break;
	}
	
STI_BAIL:

	return hResult;
}
extern bool g_bSipTunnelingEnabled;
/*!\brief Executes the operation passed in.
 * 
 * \param eOperation The operation to execute.
 * \param pbOperationStarted Set to true if the operation was started.  Otherwise, it is set to false.
 */
stiHResult CstiSipRegistration::RegistrationExecute (
	EstiRegistrationOperation eOperation,
	bool *pbOperationStarted)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszTmpAddress = nullptr;

	DBG_MSG (this, "Registration operation: %d", eOperation);
	
	// Get a handle to the outbound message
	vpe::sip::Message outboundMessage(m_hRegClient, true);

	stiTESTCOND (!m_stiRegistrarSettings.ProxyIpAddress.empty (), stiRESULT_INVALID_PARAMETER);
	
	if (!m_hRegClient)
	{
		hResult = RegistrationClientCreate ();
		stiTESTRESULT ();
	}

	if (pbOperationStarted)
	{
		*pbOperationStarted = false;
	}
	
	hResult = RegistrationRequestCreate (&eOperation, outboundMessage);
	stiTESTRESULT ();
	
	if (eOperation != eOperationNone)
	{
		//
		// Tell the registration client the IP and transport to use when registering.
		//
		RvSipTransportOutboundProxyCfg ProxyCfg;
		
		memset (&ProxyCfg, 0, sizeof (ProxyCfg));
		
		// Enclose ipv6 in brackets if it isn't already
		if (strchr (m_stiRegistrarSettings.ProxyIpAddress.c_str (), ':') && m_stiRegistrarSettings.ProxyIpAddress[0] != '[')
		{
			pszTmpAddress = new char[m_stiRegistrarSettings.ProxyIpAddress.size() + 3];
			sprintf (pszTmpAddress, "[%s]", m_stiRegistrarSettings.ProxyIpAddress.c_str ());

			ProxyCfg.strIpAddress = (RvChar *)pszTmpAddress;
		}
		else
		{
			ProxyCfg.strIpAddress = (RvChar *)m_stiRegistrarSettings.ProxyIpAddress.c_str ();
		}

		ProxyCfg.port = m_stiRegistrarSettings.un16ProxyPort;
		
		switch (m_stiRegistrarSettings.eTransport)
		{
			case estiTransportTCP:
				ProxyCfg.eTransport = RVSIP_TRANSPORT_TCP;
				
				//
				// If using TCP then set the persistency level so the stack
				// uses the connection we have already setup.
				//
				RvSipRegClientSetPersistency (m_hRegClient, RVSIP_TRANSPORT_PERSISTENCY_LEVEL_TRANSC_USER);
				break;
				
			case estiTransportTLS:
				ProxyCfg.eTransport = RVSIP_TRANSPORT_TLS;
				
				RvSipRegClientSetPersistency (m_hRegClient, RVSIP_TRANSPORT_PERSISTENCY_LEVEL_TRANSC_USER);
				break;
				
			case estiTransportUDP:
				ProxyCfg.eTransport = RVSIP_TRANSPORT_UDP;

				break;

			case estiTransportUnknown:
				stiASSERT (estiFALSE);
				ProxyCfg.eTransport = RVSIP_TRANSPORT_TCP;
				break;
			
			// default: Not implemented for type safety
		}
		
		RvSipRegClientSetOutboundDetails (m_hRegClient, &ProxyCfg, sizeof (ProxyCfg));
		
		// Now register us!
		DBG_MSG (this, "Executing registration operation %d with %s:%d", eOperation, m_stiRegistrarSettings.ProxyIpAddress.c_str (), m_stiRegistrarSettings.un16ProxyPort);
		m_bRegistrarLoginAttempted = false;
		
		m_eCurrentOperation = eOperation;
		
		rvStatus = RvSipRegClientRegister (m_hRegClient);
		stiTESTRVSTATUS ();
	
		if (pbOperationStarted)
		{
			*pbOperationStarted = true;
		}
#ifdef stiTUNNEL_MANAGER
#if APPLICATION == APP_NTOUCH_IOS
		if (!g_bSipTunnelingEnabled)
		{
			RvSipTransportConnectionHandle hConn;
			if (RV_OK == RvSipRegClientGetConnection (m_hRegClient, &hConn))
			{
				TransportConnection *pConn = (TransportConnection*)hConn;
				if (pConn)
				{
					pConn->backgroundState = TRANSPORT_BACKGROUND_DONTCARE;
					if (RV_OK != RvSipTransportConnectionSetBackground (hConn, RV_TRUE))
					{
						stiASSERT (estiFALSE);
					}
				}
			}
			else
			{
				stiASSERT (estiFALSE);
			}
		}
#endif
#endif // stiTUNNEL_MANAGER
	}

STI_BAIL:

	if (pszTmpAddress)
	{
		delete [] pszTmpAddress;
		pszTmpAddress = nullptr;
	}

	if (stiIS_ERROR (hResult))
	{
		DBG_MSG (this, "Error occurred when starting operation %d", m_eCurrentOperation);
		m_eCurrentOperation = eOperationNone;
		EventCallback (REG_EVENT_FAILED);
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::EventCallback
//
// Description: Notifies any registered event handler of an event
//
// Abstract:
//
// Returns: stiHResult
//
stiHResult CstiSipRegistration::EventCallback (EstiRegistrationEvent eEvent, size_t EventParam)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (m_pEventCallback, stiRESULT_ERROR);

	hResult = m_pEventCallback (m_pUserData, this, eEvent, EventParam);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*!\brief Clears the valid contact
 * 
 */
stiHResult CstiSipRegistration::ContactClear ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_hValidContactsPage)
	{
		m_vValidContacts.clear ();
		RPOOL_FreePage (m_hAppPool, m_hValidContactsPage);
		m_hValidContactsPage = nullptr;
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::ContactSet
//
// Description: Sets our list of valid contacts to the one passed in.
//
// Abstract:
//
// Returns: Nothing
//
stiHResult CstiSipRegistration::ContactSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	
	if (!m_bUnregistering && m_bQueryCompleted)
	{
		// Clear the list if it exists
		ContactClear ();
		
		if (!m_hValidContactsPage)
		{
			// Rip a page from the memory pool
			rvStatus = RPOOL_GetPage (m_hAppPool, 0, &m_hValidContactsPage);
			stiTESTRVSTATUS ();
		}
		
		// Create a new header on the invalid contact page and copy the current contact header into it.
		RvSipContactHeaderHandle hNewContactHeader = nullptr;
		
		//
		// If we don't have a reflexive address yet then just use the local address.
		//
		const char *pszProtocolStr = "sip";
		uint16_t un16Port;
		std::string IPAddress;
		
		if (m_ReflexiveAddress.empty ())
		{
#ifdef stiTUNNEL_MANAGER
			if (g_bSipTunnelingEnabled)
			{
				char szIPAddress[RV_ADDRESS_MAXSTRINGSIZE];
				rvStatus = RvTunGetLocalIP (nullptr, szIPAddress, sizeof(szIPAddress));
				if (rvStatus == RV_OK)
				{
					IPAddress = szIPAddress;
				}
			}
#endif

			if (IPAddress.empty ())
			{
				RvSipTransportConnectionHandle hConn;
				RvChar szLocalIpAddress[RVSIP_TRANSPORT_LEN_STRING_IP];
				RvUint16 un16Port;
				RvSipTransportAddressType eAddressType;

				rvStatus = RvSipRegClientGetConnection (m_hRegClient, &hConn);
				stiTESTRVSTATUS ();

				rvStatus = RvSipTransportConnectionGetLocalAddress (hConn, szLocalIpAddress, &un16Port, &eAddressType);
				stiTESTRVSTATUS ();

				IPAddress = szLocalIpAddress;
			}

			un16Port = m_stiRegistrarSettings.un16LocalSipPort;
		}
		else
		{
			IPAddress.assign (m_ReflexiveAddress);

			if (m_un16RPort == 0)
			{
				un16Port = m_stiRegistrarSettings.un16LocalSipPort;
			}
			else
			{
				un16Port = m_un16RPort;
			}
		}
		
		hResult = ValidContactCreate (m_PhoneNumber.c_str (), pszProtocolStr,
				m_stiRegistrarSettings.eTransport, IPAddress.c_str (), un16Port,
				1.0F, &hNewContactHeader);
		stiTESTRESULT ();

		m_vValidContacts.push_back (hNewContactHeader);
	}

STI_BAIL:
	
	if (rvStatus == RV_ERROR_OUTOFRESOURCES)
	{
		EventCallback (REG_EVENT_OUT_OF_RESOURCES);
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::ValidContactCreate
//
// Description: Create and add a contact to our valid contacts list
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult CstiSipRegistration::ValidContactCreate (
	const char *pszPhoneNumber,
	const char *pszProtocol, /// sip
	EstiTransport eTransport, /// tcp or udp
	const char *pszIpAddress, /// the ip address
	uint16_t un16Port, /// the listening port (if 0, will not create contact)
	float fQVal, /// The order of preference for ordered lists. (if 0.0, it will not be added)
	RvSipContactHeaderHandle *phContactHeader
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string address;
	std::string contactHeader;
	std::stringstream ContactHeader;
	RvSipContactHeaderHandle hNewContactHeader = nullptr;

	// Validate the parameters
	stiTESTCOND (pszPhoneNumber && *pszPhoneNumber, stiRESULT_INVALID_PARAMETER);
	stiTESTCOND (pszProtocol && *pszProtocol, stiRESULT_INVALID_PARAMETER);
	
	// Create the address string
	address = AddressStringCreate (
		pszIpAddress,
		pszPhoneNumber,
		un16Port,
		eTransport,
		pszProtocol);
	
	// Wrap the address string in a contact header
	contactHeader = AddressHeaderStringCreate (
		"Contact",
		address,
		"",
		fQVal
	);
	
	ContactHeader << contactHeader << ";reg-id=" << m_nRegistrationID << ";+sip.instance=\"<urn:uuid:" << m_SipInstanceGUID << ">\"";
	
	if (!m_hValidContactsPage)
	{
		// Rip a page from the memory pool
		rvStatus = RPOOL_GetPage (m_hAppPool, 0, &m_hValidContactsPage);
		stiTESTRVSTATUS ();
	}
	
	// Create a new header on the invalid contact page and copy the current contact header into it.
	RvSipMsgMgrHandle hMsgManager;
	rvStatus = RvSipStackGetMsgMgrHandle (m_hStackMgr, &hMsgManager);
	stiTESTRVSTATUS ();
	
	rvStatus = RvSipContactHeaderConstruct (hMsgManager, m_hAppPool, m_hValidContactsPage, &hNewContactHeader);
	stiTESTRVSTATUS ();
	
	rvStatus = RvSipContactHeaderParse (hNewContactHeader, (RvChar *)ContactHeader.str().c_str());
	stiTESTRVSTATUS ();
	
	*phContactHeader = hNewContactHeader;
	
STI_BAIL:

	return hResult;
} // end CstiSipRegistration::ValidContactCreate


/*!\brief Issues a registration request with authentication.
 *
 * \param hRegClient The registration client to use for the registration.
 */
stiHResult CstiSipRegistration::WithAuthenticationReRegister (
	RvSipRegClientHandle hRegClient)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_bRegistrarLoginAttempted)
	{
		stiASSERT (estiFALSE);
		RegisterStop ();
		
		// Pass up a login failed message to the app
		EventCallback (REG_EVENT_BAD_CREDENTIALS);
	}
	else
	{
		m_bRegistrarLoginAttempted = true;
		DBG_MSG (this, "The SIP registrar says we must authenticate, so we shall.");

		vpe::sip::Message outboundMessage(m_hRegClient, true);

		if (outboundMessage)
		{
			hResult = RegistrationRequestCreate (&m_eCurrentOperation, outboundMessage);
			stiTESTRESULT ();
			
			if (m_eCurrentOperation != eOperationNone)
			{
				RvSipRegClientAuthenticate (hRegClient);
			}
			else
			{
				DBG_MSG (this, "Registration cancelled during authentication.");
				
				RegistrationNext ();
			}
		}
	}
	
STI_BAIL:

	return hResult;
}


/*!\brief Determines the reflexive from information returned in the Via header
 * 
 * \param hMsg The handle to the response message
 * \param pbReflexiveAddressChanged Upon return, indicates if the reflexive address has changed or not
 */
stiHResult CstiSipRegistration::ReflexiveAddressDetermine (
	RvSipMsgHandle hMsg,
	bool *pbReflexiveAddressChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string reflexiveAddr;
	
	*pbReflexiveAddressChanged = false;
	
	auto viaHeader = viaHeaderGet (hMsg);
	
	// Did we find the via?
	if (!viaHeader.receivedAddr.empty ())
	{
		RvStatus rvStatus = RV_OK;
		bool validAddr = false;
		
		reflexiveAddr = viaHeader.receivedAddr;
		
		validAddr = IPAddressValidate (reflexiveAddr.c_str ());
		
		stiTESTCOND(validAddr, stiRESULT_ERROR);

		if (m_ReflexiveAddress != reflexiveAddr)
		{
			m_ReflexiveAddress.assign (reflexiveAddr);
			*pbReflexiveAddressChanged = true;
		}

		//
		// Check to see if we are a public facing endpoint by comparing the reflexive address
		// with our local address.  If they are the same the endpoint is public facing.  If the
		// endpoint is public facing then get the local port to use as the "reflexive" port. 
		// Otherwise, check to see if there is an rport and use that as the reflexive port.
		//
		RvSipTransportConnectionHandle hConn;
		RvChar szLocalIpAddress[RVSIP_TRANSPORT_LEN_STRING_IP];
		RvUint16 un16Port;
		RvSipTransportAddressType eAddressType;
		
		rvStatus = RvSipRegClientGetConnection (m_hRegClient, &hConn);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipTransportConnectionGetLocalAddress (hConn, szLocalIpAddress, &un16Port, &eAddressType);
		stiTESTRVSTATUS ();
		
		if (m_ReflexiveAddress[0] == '\0' || m_ReflexiveAddress == szLocalIpAddress)
		{
			//
			// The endpoint is public facing so get the local port and use it as the reflexive port.
			//
			RvSipTransportConnectionGetClientLocalPort (hConn, &un16Port);
				
			if (m_un16RPort != un16Port)
			{
				m_un16RPort = un16Port;
				*pbReflexiveAddressChanged = true;
			}
		}
		else if (viaHeader.rPort != UNDEFINED)
		{
			if (m_un16RPort != static_cast<uint16_t>(viaHeader.rPort))
			{
				m_un16RPort = static_cast<uint16_t>(viaHeader.rPort);
				*pbReflexiveAddressChanged = true;
			}
		}
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipRegistration::RegistrarStateChanged
//
// Description: Notifies the application of a register-client state change.
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipRegistration::RegistrarStateChangedCB (
	RvSipRegClientHandle hRegClient, /// The sip stack register-client handle
	RvSipAppRegClientHandle hAppRegClient, /// The application handle for this register-client.
	RvSipRegClientState eState, /// The new register-client state
	RvSipRegClientStateChangeReason eReason) /// The reason for the state change.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bLocked = false;
	auto pThis = (CstiSipRegistration*)hAppRegClient;
	stiTESTCOND (pThis, stiRESULT_ERROR);
	stiTESTCOND (pThis->m_hRegClient == hRegClient, stiRESULT_ERROR);
	
	pThis->Lock ();
	bLocked = true;

	DBG_MSG (pThis, "State change: %s, %s", RegClientStateIntoString(eState), RegClientReasonIntoString (eReason));
	stiDEBUG_TOOL (g_stiLoadTestLogging,
			stiTrace ("Registration state changed for phone # %s.  New state: %s, Reason: %s\n",
					pThis->PhoneNumberGet(), RegClientStateIntoString(eState), RegClientReasonIntoString (eReason));
	);

	
	switch (eState)
	{
		case RVSIP_REG_CLIENT_STATE_UNAUTHENTICATED:
			
			if (!pThis->m_bTerminating)
			{
				pThis->WithAuthenticationReRegister (hRegClient);
			}
			
			break;
			
		case RVSIP_REG_CLIENT_STATE_REDIRECTED:
			DBG_MSG (pThis, "We're being redirected to another SIP registrar.");
			if (!pThis->m_bTerminating)
			{
				pThis->m_bRegistrarLoginAttempted = false;
				// TODO: Somehow get the new handle instead??
				RvSipRegClientRegister (hRegClient);
			}
			
			break;
			
		case RVSIP_REG_CLIENT_STATE_REGISTERING:
			DBG_MSG (pThis, "Registration client is registering...");

			// Notify the app
			pThis->EventCallback (REG_EVENT_REGISTERING);
			break;
			
		case RVSIP_REG_CLIENT_STATE_REGISTERED:
		{
			if (!pThis->m_bTerminating)
			{
				if (eReason == RVSIP_REG_CLIENT_REASON_RESPONSE_UNAUTHENTICATED_RECVD)
				{
					DBG_MSG (pThis, "Registered but with a reason of unauthenticated");
					
					pThis->WithAuthenticationReRegister (hRegClient);
				}
				else
				{
					if (pThis->m_bUnregistering)
					{
						//
						// The client is to unregister.  If we completed that operation then
						// just stop the client and allow it to terminate.
						// If we haven't completed the unregister operation then
						// start the next operation in the queue (which should be the unregister
						// operation.
						//
						if (pThis->m_eCurrentOperation == eOperationUnregister)
						{
							pThis->RegisterStop();
						}
						else
						{
							pThis->RegistrationNext();
						}
					}
					else
					{
						vpe::sip::Message inboundMessage(hRegClient, false);

						RvUint32 eResponseCode = inboundMessage.statusCodeGet ();
						
						DBG_MSG (pThis, "%d Message Received from SIP registrar", eResponseCode);

						switch (pThis->m_eCurrentOperation)
						{
							case eOperationUnregisterInvalid:
								
//								if (pThis->m_hInvalidContactsPage)
//								{
//									pThis->m_vInvalidContacts.clear ();
//									RPOOL_FreePage (pThis->m_hAppPool, pThis->m_hInvalidContactsPage);
//									pThis->m_hInvalidContactsPage = NULL;
//								}
									
								break;
								
							case eOperationQuery:
							{
								DBG_MSG (pThis, "Query completed.");

								pThis->m_bQueryCompleted = true;

								bool bReflexiveAddressChanged = false;
								
								pThis->ReflexiveAddressDetermine (inboundMessage, &bReflexiveAddressChanged);
								
								pThis->ContactSet();
								
								pThis->m_Queue.push_front (eOperationRegister);
								
								break;
							}
								
							case eOperationRegister:
							{
								DBG_MSG (pThis, "Registered.");

								bool bReflexiveAddressChanged = false;
								
								// Extract the date field if given
								RvSipOtherHeaderHandle hDateHeader = RvSipMsgGetHeaderByName (inboundMessage, (RvChar*)"Date", RVSIP_FIRST_HEADER, nullptr);
								if (nullptr != hDateHeader)
								{
									char szDate[25];
									RvSipOtherHeaderGetValue (hDateHeader, szDate, sizeof (szDate), nullptr);

									DBG_MSG (pThis, "Registrar's time: %s", szDate);

									time_t ttTime;

									// Convert it to a local time_t
									stiHResult hResult = TimeConvert (szDate, &ttTime);

									if (!stiIS_ERROR (hResult))
									{
										// Pass the time on up.
										pThis->EventCallback (REG_EVENT_TIME_SET, (size_t)ttTime);
									}
								}
								
								pThis->ReflexiveAddressDetermine (inboundMessage, &bReflexiveAddressChanged);

								// We were a register event, so start the expiration timer
								pThis->StartReRegisterFor (inboundMessage);
								
								// Notify the app of our success
								pThis->EventCallback (REG_EVENT_REGISTERED);

								if (bReflexiveAddressChanged)
								{
									pThis->ContactSet();
									
									pThis->m_Queue.push_front (eOperationRegister);
								}
						
								break;
							}
							
							case eOperationNone:
							case eOperationUnregister:
								
								//
								// This case should never happen
								//
								stiASSERT (estiFALSE);
								
								break;
						}
						
						pThis->RegistrationNext ();
					}
				}
			}
			
			break;
		}

		case RVSIP_REG_CLIENT_STATE_FAILED:
		{
			RvUint32 un32ResponseCode = 0;

			vpe::sip::Message inboundMessage(hRegClient, false);

			if (inboundMessage)
			{
				un32ResponseCode = inboundMessage.statusCodeGet();
			}
			
			stiASSERTMSG (estiFALSE, "SIP: Registrar general failure because %d \"%s\" (%d)", eReason, RegClientReasonIntoString (eReason), un32ResponseCode);
			
			if (!pThis->m_bTerminating)
			{
				pThis->RegisterStop();
				
				//
				// If we were unregistering then don't tell the application layer
				// about this issue.
				//
				if (pThis->m_eCurrentOperation != eOperationUnregister)
				{
					//
					// If we experienced a network issue try looking up the
					// proxy and registrar again and reregister.
					//
					if (un32ResponseCode == SIPRESP_FORBIDDEN
					|| un32ResponseCode == SIPRESP_METHOD_NOT_ALLOWED
					|| eReason == RVSIP_REG_CLIENT_REASON_TRANSACTION_TIMEOUT
					|| eReason == RVSIP_REG_CLIENT_REASON_NETWORK_ERROR
					|| eReason == RVSIP_REG_CLIENT_REASON_LOCAL_FAILURE)
					{
						pThis->EventCallback (REG_EVENT_CONNECTION_LOST);
					}
					else
					{
						pThis->EventCallback (REG_EVENT_FAILED);
					}
				}
			}
			
			break;
		}
			
		case RVSIP_REG_CLIENT_STATE_MSG_SEND_FAILURE:
			
			if (!pThis->m_bTerminating)
			{
				pThis->RegisterStop ();
				
				//
				// If we were unregistering then don't tell the application about this issue.
				//
				if (pThis->m_eCurrentOperation != eOperationUnregister)
				{
					pThis->EventCallback (REG_EVENT_CONNECTION_LOST);
				}
			}
			
			break;
			
		case RVSIP_REG_CLIENT_STATE_TERMINATED:
			DBG_MSG (pThis, "Registration client terminated.");
			
			if (pThis->m_eCurrentOperation == eOperationUnregister)
			{
				DBG_MSG (pThis, "Unregistering Registration client terminated");
				
				if (bLocked)
				{
					pThis->Unlock ();
					bLocked = false;
				}

				delete pThis;
				pThis = nullptr;
			}
			else
			{
				DBG_MSG (pThis, "Registration client terminated for operation: %d", pThis->m_eCurrentOperation);
				
				pThis->m_hRegClient = nullptr;
				pThis->m_bTerminating = false;
				pThis->RegistrationNext ();
			}
			
			hRegClient = nullptr;
			
			break;
			
		case RVSIP_REG_CLIENT_STATE_IDLE:
			DBG_MSG (pThis, "SIP Registration client dropped into idle state.");
			break;
			
		case RVSIP_REG_CLIENT_STATE_UNDEFINED:
			stiASSERTMSG (estiFALSE, "SIP Registration client %p entered undefined state.", hRegClient);
			break;
	}
	
STI_BAIL:
	if (hResult) {} // Prevent compiler warning

	if (bLocked)
	{
		pThis->Unlock ();
	}
} // end CstiSipRegistration::RegistrarStateChanged


/*!\brief This callback is called just before the message is sent and gives us an opportunity to change the VIA header.
 * We are changing it to the reflexive address and port so that SMX will see the endpoint as not NATted.
 * 
 */
RvStatus RVCALLCONV CstiSipRegistration::FinalDestResolvedCB (
	RvSipRegClientHandle hRegClient,
	RvSipAppRegClientHandle hAppRegClient,
	RvSipTranscHandle hTransc,
	RvSipMsgHandle hMsgToSend)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvSipViaHeaderHandle hVia;
	RvSipHeaderListElemHandle hListElem = nullptr;
	auto pThis = (CstiSipRegistration*)hAppRegClient;
	
	pThis->Lock ();

	if (RvSipMsgGetMsgType(hMsgToSend) == RVSIP_MSG_REQUEST)
	{
		/* Get the top Via header. */
		hVia = (RvSipViaHeaderHandle)RvSipMsgGetHeaderByType (hMsgToSend, RVSIP_HEADERTYPE_VIA, RVSIP_FIRST_HEADER, &hListElem);
		
		// Set alias parameter to indicate that registration connection should be re-used by server
		RvSipViaHeaderSetAliasParam (hVia, RV_TRUE);

		//
		// Set our via address so that it appears we are not NATted.  This should keep SMX from
		// anchoring the media.
		//
		if (pThis->m_ReflexiveAddress[0] != '\0')
		{
			// if we are ipv6 and there are no square brackets, we will need to add them
			if (pThis->m_ReflexiveAddress[0] != '[' && std::string::npos != pThis->m_ReflexiveAddress.find (':'))
			{
				auto pszTmpBuf = new char[pThis->m_ReflexiveAddress.size () + 3];
				
				sprintf (pszTmpBuf, "[%s]", pThis->m_ReflexiveAddress.c_str ());
				rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)pszTmpBuf);
				delete [] pszTmpBuf;
				pszTmpBuf = nullptr;
				stiTESTRVSTATUS ();
			}
			else
			{
				rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)pThis->m_ReflexiveAddress.c_str ());
				stiTESTRVSTATUS ();
			}
			
			if (pThis->m_un16RPort != 0)
			{
				rvStatus = RvSipViaHeaderSetPortNum (hVia, pThis->m_un16RPort);
				stiTESTRVSTATUS ();
			}
		}

		rvStatus = RvSipViaHeaderSetOtherParams (hVia, (RvChar *)"keep");
		stiTESTRVSTATUS ();
		
		RvSipOtherHeaderHandle hHeader;
		
		RvSipOtherHeaderConstructInMsg (hMsgToSend, RV_FALSE, &hHeader);
		RvSipOtherHeaderSetName (hHeader, (RvChar*)"Supported");
		RvSipOtherHeaderSetValue (hHeader, (RvChar*)"path");
		
		RvSipOtherHeaderConstructInMsg (hMsgToSend, RV_FALSE, &hHeader);
		RvSipOtherHeaderSetName (hHeader, (RvChar*)"Supported");
		RvSipOtherHeaderSetValue (hHeader, (RvChar*)"outbound");
	}
	
STI_BAIL:

	stiUNUSED_ARG (hResult);
	
	stiASSERTMSG (rvStatus == RV_OK, "CstiSipRegistration::FinalDestResolvedCB is returning an error this may break registrations");

	pThis->Unlock ();

	return rvStatus;
}

// end file CstiSipRegistration.cpp
