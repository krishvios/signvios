////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiSipManager
//
//  File Name:	CstiSipManager.cpp
//
//	Abstract:
//		The Conference Manager is responsible for interfacing with the SIP
//		engine (the stack), the data objects, and the UI/Controlling application
//		 (thru the Conference Control Interface).  It maintains the current state
//		of the system and provides the ability to receive and initiate calls.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "stiSipConstants.h"
#include "CstiSipManager.h"
#include "CstiSipCall.h"
#include "Sip/Message.h"
#include "CstiVideoRecord.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "stiOS.h" // OS Abstraction definitions
#include "openssl/md5.h"
#include <cmath> // For floating point info functions
#include "CstiCallStorage.h"
#include "stiSipTools.h"
#include "IstiBlockListManager2.h"
#include "CstiHostNameResolver.h"
#include "CstiSipResolver.h"
#include "stiRemoteLogEvent.h"
#include "CstiSdp.h"
#include "IstiPlatform.h"
#include <algorithm>
#include "CstiPhoneNumberValidator.h"
#include "stiTaskInfo.h"
#include "IstiNetwork.h"
#include "EndpointMonitor.h"
#include "RadvisionLogging.h"

#ifdef stiHOLDSERVER
#include <Log.h>
#endif

// Sip stack
#include "RvSipStack.h"
#include "RV_SIP_DEF.h"
#include "RvSipRegClientTypes.h"
#include "RvSipRegClient.h"
#include "RvSipPartyHeader.h"
#include "RvSipExpiresHeader.h"
#include "RvSipContactHeader.h"
#include "RvSipAddress.h"
#include "RvSipBody.h"
#include "RvSipContentTypeHeader.h"
#include "RvSipMid.h"
#include "RvSipCallLeg.h"
#include "RvSipOtherHeader.h"
#include "RvSipCSeqHeader.h"
#include "RvSipAllowHeader.h"
#include "RvSipAuthenticationHeader.h"
#include "RvSipAuthorizationHeader.h"
#include "RvSipDateHeader.h"
#include "RvSipViaHeader.h"
#include "RvSipReferToHeader.h"
#include "RvSipSubscription.h"
#include "RvSipTransmitter.h"
#include "RvSipResolver.h"
#include "rtp.h"
#include "rtcp.h"
#include "CstiIceManager.h"
#include "rvrtplogger.h"
#include "rvloglistener.h"

#include <sstream>
#include <functional>
#include <boost/lexical_cast.hpp>

#if defined(stiMVRS_APP)
#include <openssl/evp.h>
#endif
#ifdef stiENABLE_TLS
#include <openssl/ssl.h>
#endif
#ifdef stiTUNNEL_MANAGER
#include "rvsockettun.h"
#include "rvrtpseli.h"
#include "rvares.h"
#endif


//
// Constants
//
#define stiLOG_SLP_CHANGES
#define ID_BUF_LEN 200

static const char g_szAnonymousUser[] = "anonymous";
static const char g_szAnonymousDomain[] = "anonymous.invalid";
static const char g_szUnregisteredDomain[] = "unregistered.sorenson.com";
static const char g_szGvcRegex[] =  //!< Regular expression for GVC URI validation.
        "^sip:SVRS-GC-" // Begins with "sip:SVRS-GC-"
        "[0-9a-fA-F]{8,8}-"  // 8 hex followed by '-'
        "[0-9a-fA-F]{4,4}-" // 4 hex followed by -
        "[0-9a-fA-F]{4,4}-" // 4 hex followed by -
        "[0-9a-fA-F]{4,4}-" // 4 hex followed by -
        "[0-9a-fA-F]{12,12}"  // 12 hex
        "@\\.*";  // ends with the '@' followed by either a domain or ip address

//
// These are the old strings used to signal anonymous calling.  They are here for backwards compatibility.
// At some point we can get rid of them.
//
static const char g_szPrivateUser[] = "private";
static const char g_szPrivateDomain[] = "private.svrs.com";

//
// Private SIP Headers
//
static const char g_szSVRSHeartbeatRequest[] = "P-SVRS-RQ";	// Used to initiate a heartbeat (and possibly other requests)
static const char g_szSVRSH323InterworkedPrivateHeader[] = "P-SVRS-H323-Interworked"; // Used to detect that H.323 has been interworked by the SBC
static const char g_szSVRSTrsUserIpPrivateHeader[] = "P-SVRS-TRS-User-IP"; // Used to detect that the deaf ip address has been sent by the SBC


#ifdef stiHOLDSERVER
static const char g_szSVRSAssertedIdentityHeader[] = "P-Asserted-Identity"; // Used to obtain the identity when caller Id is blocked
#endif
static const char g_szSVRSCallInfoHeader[] = "Call-Info"; // Used by VRS vendors to provide original IP Address value
static const char g_szSVRSIVVR[] = "P-SVRS-IVVR"; // Signals that the endpoint is behind an IVVR
static const char g_szSVRSGeolocation[] = "P-SVRS-Geolocation"; // Header for geolocation string per RFC 5870
static const char g_szSVRSVriUserInfo[] = "P-SVRS-VRI-USER-INFO";
static const char g_szSVRSInterpreterSkills[] = "P-SVRS-INTERPRETER-SKILLS";

static const int stiOPTIONS_REQUEST_ALLOWED_ELAPSED_TIME = 30; // Ignore sequence after this number of seconds
static const unsigned int stiOPTIONS_REQUEST_HEARTBEAT = 1;

const uint32_t un32STACK_RESOURCE_CHECK_WAIT = 300000; // Time to wait (5 minutes expressed in milliseconds).

// Settings
//#define UNKNOWN_CALL_NAME_ALLOWED 1 // Enable URI's for unknown users, like "sip:unknownperson@ip"
#define MULTIRING_NOT_PORTED_REGISTER 1 // Enable this to test the multiring registration (disables ported mode registration)
#ifdef MULTIRING_NOT_PORTED_REGISTER
	#ifndef PHONE_NUMBER_IN_INVITE
		#define PHONE_NUMBER_IN_INVITE 1 // required for multiring
	#endif
#endif
#ifndef MAX_CHANNELS_PER_CALL
	#define MAX_CHANNELS_PER_CALL 3 // Audio+text+video
#endif
#define MAX_MEDIA (MAX_CALLS * MAX_CHANNELS_PER_CALL)

static constexpr int PROXY_WATCHDOG_TIMEOUT_MS = 30000;  // 30 seconds
#define INVITE_RESPONSE_TIMEOUT 180000 // This is how many ms of inactivity before an INVITE connection is abandoned
#define USER_NOT_FOUND_DELAY 5000 // How many ms to delay until reporting a misdirected call
#define DEFAULT_PROXY_KEEPALIVE_TIMEOUT 45000 // Usually between 45-85 seconds.
#define RESTART_PROXY_REGISTRAR_LOOKUP_TIMEOUT 5000 // Time between not being able to connect and trying again
#define MAX_TIME_TO_WAIT_FOR_RECONNECT 300000 // Maximum time to wait before attempting to reconnect (300 seconds)

#define LOOKUP_CALL_ID "LOOKUP"

#define RADVISION_ALL_ADAPTER_ADDRESSES "[0:0:0:0:0:0:0:0]" // this constant tells the stack to listen on all available adapters and addresses

#define MAX_TCP_PROXY_CONNECTION_SYN_RETRIES 2	// Number of SYN retries to allow for persistent TCP connection to proxy

#define stiMAX_DNS_SERVERS 10 // Matches the maximum number of servers used by iOS.

// Uncomment the next line for more debug info concerning SIP locking.
//#define DEBUG_SIP_LOCKING

// Uncomment the next line for more debug info concerning the mid-level stack.
//#define DEBUG_SIP_MID_LEVEL

// Uncomment the next line to log to SipLog.txt instead of to STDOUT.
// (Radvision support prefers this type of log.)
//#define DEBUG_SIP_LOG_TO_FILE

// Uncomment the next line to log Radvision memory pool allocation. (May cause stack shutdown inconsistancies.)
//#define DEBUG_SIP_LOG_MEMORY_POOL

// Uncomment the next line for more debug info concerning the stack state machine.
//#define DEBUG_SIP_STACK_STATE

#define REGID_NONE									0
#define REGID_LOCAL_PHONE_NUMBER					1
#define REGID_TOLL_FREE_PHONE_NUMBER				2
#define REGID_RING_GROUP_LOCAL_PHONE_NUMBER			3
#define REGID_RING_GROUP_TOLL_FREE_PHONE_NUMBER 	4
#define REGID_SORENSON_PHONE_NUMBER					5

#define INTERPRETER_FROM_DOMAIN "dial.svrs.com"
#define TECH_SUPPORT_FROM_DOMAIN INTERPRETER_FROM_DOMAIN
#define VRI_FROM_DOMAIN INTERPRETER_FROM_DOMAIN

//
// Typedefs
//
struct SstiTransmitterInfo
{
	CstiSipCallSP sipCall;
	CstiSipManager *pSipManager{nullptr};
};

//
// Forward Declarations
//
#ifdef stiLOG_SLP_CHANGES
static void Test (RvSipTransportMgrHandle hTransportMgr);
#endif //#ifdef stiLOG_SLP_CHANGES


// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiSipMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )
#define DBG_MSG_DNS(fmt,...) stiDEBUG_TOOL (g_stiDNSDebug, stiTrace ("SIP DNS: " fmt "\n", ##__VA_ARGS__); )

#define stiCONST_TO_STRING(s) #s
#define UNHANDLED_CASE(s) case s: stiDEBUG_TOOL (g_stiSipMgrDebug, stiTrace ("Unhandled case %s\n", #s); ); break;
#define GET_CASE_STRING(s) case s: caseString=#s; break;

//
// Globals
//

// See header for detailed explaination
std::map<size_t,CstiSipCallLegWP> CstiSipManager::m_callLegMap;
std::recursive_mutex CstiSipManager::m_callLegMapMutex;

//
// Locals
//

////////////////////////////////////////////////////////////////////////////////
//; UserIsPhoneNumber
//
// Description: Determine if the string is a phone number.
//
// Abstract:
//  Numbers that are considered phone numbers are:
//		x11 (where x cannot be the digit 0
//		10-digit (e.g. 8012879400)  (the first character cannot be a '0' or '1')
//		11-digit (e.g. 18012879400) where the first digit must be a '1'
//			(the second character cannot be a '0' or '1')
//		12-digit (e.g. +18012879400) where the first two digits must be "+1"
//		International, determined by the first three digits being 011 and followed
//			by 1 or more digits.
//
// Returns:
//  true - if the passed in number meets one of the above descriptions
//	false - otherwise
//
static bool UserIsPhoneNumber (const std::string &User)
{
	bool bRet = false;
	int nPosToStartIsDigitCheck = 0;

	// We need to have a non-empty string
	if (!User.empty ())
	{

		// If it is only 3-digits, validate that it is x11 where x cannot be a 0
		if (User.length () == 3)
		{
			if (isdigit (User[0]) &&
				User[0] != '0' &&
				User[1] == '1' &&
				User[2] == '1')
			{
				bRet = true;
				nPosToStartIsDigitCheck = 3;
			}
		}

		// Is it an international number (begins with 011 and is at least 4-digits)?
		// with 011 (international numbers).
		else if (0 == User[0] &&
				 1 == User[1] &&
				 1 == User[2] &&
				 User.length () >= 4)
		{
			// The rest of the characters must all be digits
			bRet = true;
			nPosToStartIsDigitCheck = 3;
		}

		else
		{
			switch (User.length ())
			{
				case 10:
					// If it is a 10-digit number, it is valid as long as the first character is not a '1' or a '0'
					if (User[0] != '0' &&
						User[0] != '1')
					{
						bRet = true;
						nPosToStartIsDigitCheck = 0;
					}
					break;

				case 11:
					// Is the first character a '1'?
					if (User[0] == '1' &&

						// The second character cannot be a '0' or '1'.
						User[1] != '0' &&
						User[1] != '1')
					{
							bRet = true;
							nPosToStartIsDigitCheck = 1;
					}
					break;

				case 12:
					// Is the first character a '+' and the second a '1'?
					if (User[0] == '+' &&
						User[1] == '1' &&

						// The third character cannot be a '0' or '1'.
						User[2] != '0' &&
						User[2] != '1')
					{
							bRet = true;
							nPosToStartIsDigitCheck = 2;
					}
					break;

				default:
					break;
			}
		}

		// If it is valid so far, check that the remainder of the characters are digits.
		if (bRet)
		{
			for (size_t i = nPosToStartIsDigitCheck; i < User.length (); i++)
			{
				if (!isdigit (User[i]))
				{
					bRet = false;
					break;
				}
			}
		}
	}

	return bRet;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::GetCseq
//
// Description: Friendlier way than Radvision's to get the cseq of a call leg.
//
// Abstract:
//  Returns a numeric value representing the most recent INVITE number.
//
// Returns:
//  cseq
//
inline unsigned int GetCseq (RvSipCallLegHandle hCallLeg)
{
	RvInt32 cseq = 0;
	RvSipCallLegGetCSeq (hCallLeg, &cseq);
	return cseq;
}


void RVCALLCONV CstiSipManager::NonceCountIncrementor (
	RvSipAuthenticatorHandle        hAuthenticator,
	RvSipAppAuthenticatorHandle     hAppAuthenticator,
	void*                           hObject,
	void*                           peObjectType,
	RvSipAuthenticationHeaderHandle hAuthenticationHeader,
	RvInt32*                        pNonceCount)
{
	auto pThis = (CstiSipManager *)hAppAuthenticator;
	
	pThis->Lock ();

	pThis->m_unLastNonceCount++;
	(*pNonceCount) = pThis->m_unLastNonceCount;

	pThis->Unlock ();
}


//
// Class Definitions
//


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CstiSipManager
//
// Description: Constructor
//
// Abstract:
//  This is the default constructor
//
// Returns:
//  None
//
CstiSipManager::CstiSipManager (
	CstiCallStorage *pCallStorage, 	///< The call container.
	IstiBlockListManager2 *pBlockListManager)
:
	CstiProtocolManager (
		pCallStorage,
		pBlockListManager,
		stiSIP_MGR_TASK_NAME),
    m_proxyWatchdogTimer (PROXY_WATCHDOG_TIMEOUT_MS),
    m_proxyConnectionKeepaliveTimer (DEFAULT_PROXY_KEEPALIVE_TIMEOUT),
    m_stackStartupRetryTimer (un16STACK_STARTUP_RETRY_WAIT),
    m_resourceCheckTimer (un32STACK_RESOURCE_CHECK_WAIT),
    m_restartProxyRegistrarLookupTimer (0),
    m_proxyRegisterRetryTimer (0),
	m_isAvailable (true)
{
	m_stConferencePorts.nListenPort = nDEFAULT_SIP_LISTEN_PORT;

	
	m_TunneledIpAddr.clear ();

	m_GvcUri = new CstiRegEx (g_szGvcRegex);

	m_signalConnections.push_back (m_proxyWatchdogTimer.timeoutEvent.Connect (
			[this]{ PostEvent([this]{ eventProxyWatchdog(); });}));

	m_signalConnections.push_back (m_proxyConnectionKeepaliveTimer.timeoutEvent.Connect (
			[this]{ PostEvent([this]{ ConnectionKeepalive(); });}));

	m_signalConnections.push_back (m_resourceCheckTimer.timeoutEvent.Connect (
			[this]{ PostEvent([this]{ eventStackResourcesCheck(); });}));

	m_signalConnections.push_back (m_stackStartupRetryTimer.timeoutEvent.Connect (
			[this]{ PostEvent([this]{ eventStackStartup(); });}));

	m_signalConnections.push_back (m_restartProxyRegistrarLookupTimer.timeoutEvent.Connect (
	    [this]{
		    PostEvent([this]{
				// Cancel the m_proxyRegisterRetryTimer since we don't want both
				// firing while handling this timer too.
				m_proxyRegisterRetryTimer.Stop ();

				// Send the event up through the CstiConferenceManager class so that
				// we can see if there are any active calls.
				sipRegisterNeededSignal.Emit ();

				// Resolve our VRS failover domain now as well.
				vrsServerDomainResolveSignal.Emit ();
			});
	    }));

	m_signalConnections.push_back (m_proxyRegisterRetryTimer.timeoutEvent.Connect (
		[this]{
		    PostEvent([this]{
				// Cancel the m_restartProxyRegistrarLookupTimer since we don't want both
				// firing while handling this timer too.  It will be restarted when we
				// successfully register again.
				m_restartProxyRegistrarLookupTimer.Stop ();

				// Delete the current resolver, if any, to force the DNS proxy lookup process
				// to start from the beginning.
				ProxyRegistrationLookupRestart ();
			});
		}));

	m_preferredDisplaySizeListener =
		IstiVideoOutput::InstanceGet ()->preferredDisplaySizeChangedSignalGet ().Connect (
		[this]{
			PostEvent (
			    std::bind(&CstiSipManager::eventPreferredDisplaySizeChanged, this));
		});

	m_geolocationChangedListener =
		IstiPlatform::InstanceGet ()->geolocationChangedSignalGet ().Connect (
		[this](const std::string& geolocation){
			PostEvent ([this, geolocation]{eventGeolocationChanged (geolocation);});
		});
	
	m_geolocationClearListener =
		IstiPlatform::InstanceGet ()->geolocationClearSignalGet ().Connect (
		[this]{
			PostEvent ([this]{eventGeolocationClear ();});
		});

} // end CstiSipManager::CstiSipManager


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::~CstiSipManager
//
// Description: Destructor
//
// Abstract:
//  This is the default destructor
//
// Returns:
//  None
//
CstiSipManager::~CstiSipManager ()
{
	// Make sure stack is shut down.
	StackShutdown ();

	Lock ();

	RegistrationsDelete ();

	delete m_GvcUri;

	m_preferredDisplaySizeListener.Disconnect ();

	if (m_geolocationChangedListener)
	{
		m_geolocationChangedListener.Disconnect ();
	}
	
	if (m_geolocationClearListener)
	{
		m_geolocationClearListener.Disconnect ();
	}

	Unlock ();
} // end CstiSipManager::~CstiSipManager

bool g_bSipTunnelingEnabled = false;
#ifdef stiUSE_GOOGLE_DNS
bool g_bUseGoogleDNS = true;
#endif

////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//
// Returns:
//
stiHResult CstiSipManager::Initialize (
	ProductType productType,
	const std::string &version,
	const SstiSipConferencePorts *pConferencePorts)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	m_bShutdown = estiFALSE;

	productSet (productType, version);

	m_stConferencePorts = *pConferencePorts;

#ifdef stiENABLE_ICE
	if (!g_stiDisableIce)
	{
		m_iceManager = sci::make_unique<CstiIceManager> ();
		m_iceManager->useLiteModeSet (m_stConferenceParams.useIceLiteMode);
		m_iceManager->rejectHostOnlyRequestsSet (m_stConferenceParams.rejectHostOnlyIceRequests);

		m_iceConnections.push_back (m_iceManager->iceGatheringCompleteSignal.Connect (
				[this] (size_t callLegKey)
				{
					PostEvent (
						[callLegKey] ()
						{
							auto callLeg = callLegLookup (callLegKey);

							if (callLeg)
							{
								callLeg->IceGatheringComplete ();
							}
						});

				}));

		m_iceConnections.push_back (m_iceManager->iceGatheringTryingAnotherSignal.Connect (
				[this] (size_t callLegKey)
				{
					PostEvent (
						[this, callLegKey] ()
						{
							// NOTE: this function is too long to put inline
							eventIceGatheringTryingAnother (callLegKey);
						});
				}));

		m_iceConnections.push_back (m_iceManager->iceNominationsCompleteSignal.Connect (
				[this] (size_t callLegKey, bool success)
				{
					PostEvent (
						[callLegKey, success] ()
						{
							auto callLeg = callLegLookup (callLegKey);

							if (callLeg)
							{
								// Handles the "ICE Nominations Completed" message by updating the SDP and sending the re-invite.
								callLeg->IceNominationsComplete (success);
							}
						});
				}));
	}
#endif

	Unlock ();

	return (hResult);

} // end CstiSipManager::Initialize


/*!
 * \brief Custom event loop implementation to interface with Radvision's SIP
 *        select/callback mechanism.  This will handle events from the SIP
 *        stack, as well as events that have been posted with PostEvent.
 *
 * \note This is not the ideal way to use the event queue, but we're
 *       forced into it to accommodate Radvision's limited interface.
 */
//#define REGISTER_SELECT_THREAD 1 // Radvision recommended doing this, but it appears to cause shutdown issues.
void CstiSipManager::eventLoop ()
{
	try
	{

	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	RvStatus rvStatus = RV_OK;

	// Initiate the mid layer before starting the stack.
	RvSipMidCfg sMidCfg;
	sMidCfg.maxUserFd = 10;
	sMidCfg.maxUserTimers = 0;
	sMidCfg.hLog = nullptr;

	rvStatus = RvSipMidInit ();
	stiTESTRVSTATUS ();

	rvStatus = RvSipMidConstruct (sizeof (sMidCfg), &sMidCfg, &m_hMidMgr);
	stiTESTRVSTATUS ();

#ifdef REGISTER_SELECT_THREAD
	rvStatus = RvSipMidAttachThread (m_hMidMgr, (RvChar*)"SipManager");
	stiTESTRVSTATUS ();
#endif

	// Set up the task to do a "Select" on the task message pipe as
	// well as those set up by the stack.
	rvStatus = RvSipMidSelectCallOn (
		m_hMidMgr,
		queueFileDescriptorGet (),
		RVSIP_MID_SELECT_READ,
		CstiSipManager::SipMgrPipeReadCB,
		this);
	stiTESTRVSTATUS ();

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	DebugTools::instanceGet()->DebugOutput("vpEngine::CstiSipManager::eventLoop() - Calling SIP Startup");
#endif
	
#ifdef stiTUNNEL_MANAGER
	RvTunSetPolicy(RvTun_DISABLED);
#endif

	// Start the stack, synchronously
	// (though async should work, we probably want it ready)
	// We are not checking the result of StackStartupHandle due to code that will attempt to start
	// the stack again if it fails. Checking the status here can prevent this code from running if it fails.
	eventStackStartup ();

	//
	// Loop continually reading incoming events until the boolean indicating
	// we should shutdown has been set.
	//
#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	DebugTools::instanceGet()->DebugOutput("vpEngine::CstiSipManager::eventLoop() - ...SIP Startup OK()");
#endif

	while (!m_bShutdown)
	{
		// Process any pending sip or sip manager events.
		RvSipStackSelect ();
	}  // end while

	// Synchronize shutdown.
	Lock ();
	// Shut down the mid-level
	if (m_hMidMgr)
	{
#ifdef REGISTER_SELECT_THREAD
		RvSipMidDetachThread (m_hMidMgr);
#endif
		RvSipMidDestruct (m_hMidMgr);
		RvSipMidEnd ();
		m_hMidMgr = nullptr;
	}
	StackShutdown ();
	Unlock ();

	}
	catch (const std::exception &e)
	{
		stiTrace ("CstiSipManager::eventLoop: ***UNHANDLED EXCEPTION*** (Thread: %s) - %s\n", m_taskName.c_str(), e.what());
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}
	catch (...)
	{
		stiTrace ("CstiSipManager::eventLoop: ***UNHANDLED EXCEPTION*** (Thread: %s) - Unknown\n");
		IstiPlatform::InstanceGet()->RestartSystem(IstiPlatform::estiRESTART_REASON_UNHANDLED_EXCEPTION);
	}

STI_BAIL:
	return;
}
#ifdef REGISTER_SELECT_THREAD
	#undef REGISTER_SELECT_THREAD
#endif

#ifdef stiENABLE_TLS
#define STRING_SIZE 2048
/*!\brief Notifies that a connection has reached the state where TLS Sequence has started.
 *           This is the place for the application to exchange handles with
 *           the TLS connection. If an AppHandle was previously set to the connection
 *           it will be in phAppConn, that way the application can keep track
 *           if the connection was created by the application
 * \param hConn A connection that has started the TLS sequence
 * \param phAppConn The handle given by the application
 */
void RVCALLCONV CstiSipManager::TransportConnectionTlsSequenceStartedCB (
	IN  RvSipTransportConnectionHandle hConn,
	INOUT RvSipTransportConnectionAppHandle* phAppConn)
{
	RV_UNUSED_ARG(hConn);
	RV_UNUSED_ARG(phAppConn);
//	CstiSipManager *pThis = (CstiSipManager*)phAppConn;	
}

/* !\brief:  This function notifies the application whenever a certificate
 *           needs to be processed. If you wish to leave the Stack decision
 *           regarding a certificate, return the prevError parameter as the
 *           return value.
 * \param prevError - The error previously detected by the Stack. A positive
 * \param number indicates that the certificate is OK.
 * \param Certificate - The certificate for which the callback is called. This
 *            certificate can be encoded using RvSipTransportTlsEncodeCert().
 */
RvInt32 CstiSipManager::TransportVerifyCertificateEvCB (
	IN    RvInt32 prevError,
	IN    RvSipTransportTlsCertificate  certificate)
{
	RvChar szCert[STRING_SIZE] = {'\0'};
	RvChar szLogData[STRING_SIZE] = {'\0'};
	RvChar szTmpData[STRING_SIZE] = {'\0'};
	X509 *pCert = nullptr;
	RvInt certLen = STRING_SIZE;
	RvStatus rv = RV_OK;
	auto pszCert = (RvChar*)&szCert;
	rv = RvSipTransportTlsEncodeCert (certificate, &certLen, szCert);
	if (RV_OK != rv)
	{
		DBG_MSG("failed to get certificate\n");
	}
	pCert = d2i_X509(nullptr, (const unsigned char**) &pszCert, (int)certLen);
	X509_NAME_oneline (X509_get_issuer_name(pCert), szTmpData, STRING_SIZE);
	sprintf(szLogData, "Cert Analysis - issuer:");
	strcat (szLogData, szTmpData);
	X509_NAME_oneline (X509_get_subject_name(pCert), szTmpData, STRING_SIZE);
	strcat (szLogData, ", subject name: ");
	strcat (szLogData, szTmpData);
	X509_free (pCert);
	DBG_MSG ("%s\n", szLogData);
	
//    return prevError; // Does not change the SSL pass/fail decision.
	return 1; // positive value indicates valid certificate
}

/*!\brief This callback is used to override the default post connection
 *           assertion of the Stack. Once a connection has completed the handshake, it is
 *           necessary to make sure that the certificate presented was indeed issued
 *           for the address to which the connection was made. This assertion is
 *           automatically performed by the Stack. If, for some reason, the application wishes
 *           to override a failed assertion, it can implement this callback.
 *           For example, this callback can be used to compare the host name against
 *           a predefined list of outgoing proxies.
 * \param hConnection   - The handle of the connection that changed TLS states.
 * \param hAppConnection - The application handle for the connection.
 * \param strHostName   - A NULL terminated string, indicating the host name
 *                          (IP/FQDN) to which the connection was meant to connect.
 * \param hMsg          - The message if the connection was asserted against a message.
 * \param pbAsserted     - RV_TRUE if the connection was asserted successfully.
 *                          RV_FALSE if the assertion failed. In this case, the connection
 *                          will be terminated automatically.
 */
void RVCALLCONV CstiSipManager::TransportConnectionTlsPostConnectionAssertionEvCB (
	IN    RvSipTransportConnectionHandle hConnection,
	IN    RvSipTransportConnectionAppHandle hAppConnection,
	IN    RvChar* strHostName,
	IN    RvSipMsgHandle hMsg,
	OUT   RvBool* pbAsserted)
{
	RV_UNUSED_ARG(hConnection);
	RV_UNUSED_ARG(hAppConnection);
	RV_UNUSED_ARG(hMsg);
	DBG_MSG("TLS - Accepting certificate to host %s", strHostName);
	*pbAsserted = RV_TRUE;
}

/*!\brief This callback is used to notify the application on TLS
 * \param hConnection The handle of the connection that changed TLS state
 * \param eState The connection state
 * \param eReason The reason for the state change
 */
RvStatus RVCALLCONV CstiSipManager::TransportConnectionTlsStateChangedCB (
	IN    RvSipTransportConnectionHandle hConnection,
	IN    RvSipTransportConnectionAppHandle hAppConnection,
	IN    RvSipTransportConnectionTlsState eState,
	IN    RvSipTransportConnectionStateChangedReason eReason)
{
	RvStatus rv = RV_OK;
	auto pThis = (CstiSipManager*)hAppConnection;
	
	if (pThis)
	{
		pThis->Lock ();
	}

	switch (eState)
	{
		case RVSIP_TRANSPORT_CONN_TLS_STATE_HANDSHAKE_READY:
		{
			DBG_MSG("connection %p - TLS State changed to handshake ready\n", hConnection);
			/* deciding what engine to use.
			 1. for clients we use the client engine.
			 2. for servers we use the server engine.
			 */
			if (RVSIP_TRANSPORT_CONN_REASON_CLIENT_CONNECTED == eReason)
			{
				/* Starting the handshake. Since this is the client we associate the client
				 engine to the connection and specify the default verification call back*/
				if (pThis)
				{
					rv = RvSipTransportConnectionTlsHandshake(hConnection,
															  pThis->m_hTlsClientEngine,
															  RVSIP_TRANSPORT_TLS_HANDSHAKE_SIDE_DEFAULT,
															  CstiSipManager::TransportVerifyCertificateEvCB);
					stiASSERTMSG(rv == RV_OK, "Connection %p - TLS error starting handshake.", hConnection);
				}
			}
			else if (RVSIP_TRANSPORT_CONN_REASON_SERVER_CONNECTED == eReason)
			{
				/* Starting the handshake. Since this is the server we associate the server
				 engine to the connection and do not specify a certificate verification call
				 back. another option is to specify a call back and than client certificate
				 will be required*/
				if (pThis)
				{
					rv = RvSipTransportConnectionTlsHandshake(hConnection,
															  pThis->m_hTlsServerEngine,
															  RVSIP_TRANSPORT_TLS_HANDSHAKE_SIDE_DEFAULT,
															  CstiSipManager::TransportVerifyCertificateEvCB);
				}
			}
			else if (RVSIP_TRANSPORT_CONN_REASON_ERROR == eReason)
			{
				stiASSERTMSG(estiFALSE, "Connection %p - TLS Transport connection error.", hConnection);
			}
			break;
		}
		case RVSIP_TRANSPORT_CONN_TLS_STATE_CONNECTED:
			DBG_MSG("connection %p - TLS State changed to TLS Connected\n", hConnection);
			break;
		case RVSIP_TRANSPORT_CONN_TLS_STATE_HANDSHAKE_COMPLETED:
			DBG_MSG("connection %p - TLS State changed to handshake completed\n", hConnection);
			break;
		case RVSIP_TRANSPORT_CONN_TLS_STATE_HANDSHAKE_STARTED:
			DBG_MSG("connection %p - TLS State changed to handshake started\n", hConnection);
			break;
		case RVSIP_TRANSPORT_CONN_TLS_STATE_HANDSHAKE_FAILED:
			stiASSERTMSG(estiFALSE, "connection %p - TLS State changed to handshake failed.", hConnection);
			break;
		case RVSIP_TRANSPORT_CONN_TLS_STATE_TERMINATED:
			DBG_MSG("connection %p - TLS State changed to terminated\n", hConnection);
			break;
		case RVSIP_TRANSPORT_CONN_TLS_STATE_CLOSE_SEQUENSE_STARTED:
			DBG_MSG("connection %p - TLS State changed to close sequence started\n", hConnection);
			break;
		default:
			break;
	}

	if (pThis)
	{
		pThis->Unlock ();
	}

	return rv;
}

/*!\brief Initializing Tls security: in this function we use openSSL to load
 * the certificates and key, therefore the files are built in a format openSSL
 * recognizes. Keys and certificates can be read with different function
 * and than be passed to the engine configuration struct.
 * Constructs two TLS engines a client engine and a server engine.
 * The Server engine is the server that holds a private key and a matching certificate.
 * The client engine holds a certificate of a trusted entity (certificate authority).
 */
#define CA_CERT_FILE "CORP_CA.cer.pem"
#define MAX_FILE_PATH 511
stiHResult CstiSipManager::InitTlsSecurity ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	BIO *pCert = nullptr;
	RvSipTransportTlsEngineCfg TlsEngineCfg;
	RvChar cert[STRING_SIZE] = {'\0'};
	
	if (RV_FALSE == RvSipStackMgrIsTlsFeatureEnabled ())
	{
		stiASSERTMSG (estiFALSE, "Sip stack must be compiled with TLS support to enable TLS!");
	}

	// constructing the client engine
	memset (&TlsEngineCfg, 0, sizeof (TlsEngineCfg));

	// The client engine only verifies certificates so it only needs to load TLS
	// methods and the depth of the certificate chain it will allow to certify
	TlsEngineCfg.eTlsMethod = RVSIP_TRANSPORT_TLS_METHOD_SSL_V23;
	TlsEngineCfg.certDepth = 5;

	// Using the initialized configuration to build the client TLS engine
	rvStatus = RvSipTransportTlsEngineConstruct(m_hTransportMgr,
										  &TlsEngineCfg,
										  sizeof (TlsEngineCfg),
										  &m_hTlsClientEngine);
	stiTESTRVSTATUS ();

	// Adding the approving CA to the list of certificates
	memset (cert, 0, sizeof (cert));

	// Loading certificate
	pCert = BIO_new(BIO_s_file());

	if (pCert)
	{
		std::stringstream CertificateFile;

		std::string StaticDataFolder;

		stiOSStaticDataFolderGet (&StaticDataFolder);
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4 || APPLICATION == APP_REGISTRAR_LOAD_TEST_TOOL
		CertificateFile << StaticDataFolder << "certs/" << CA_CERT_FILE;
#else
		CertificateFile << StaticDataFolder << CA_CERT_FILE;
#endif

		if (BIO_read_filename (pCert, CertificateFile.str ().c_str ()) > 0)
		{
			auto certEnd = (unsigned char*)cert;
			X509 *x509 = PEM_read_bio_X509 (pCert, nullptr, nullptr, nullptr);
			stiTESTCOND (x509, stiRESULT_ERROR);

			RvInt certLen = i2d_X509 (x509, &certEnd);

			// Adding a trusted certificate to the engine.
			// This should be the CA that signed the certificate for the server engine
			rvStatus = RvSipTransportTlsEngineAddTrustedCA (m_hTransportMgr,
													 m_hTlsClientEngine,
													 cert,
													 certLen);
			stiTESTRVSTATUS ();
		}
	}
	
STI_BAIL:

	if (pCert)
	{
		BIO_free(pCert);
	}

	return hResult;
}  // end CstiSipManager::InitTlsSecurity
#undef MAX_FILE_PATH
#undef CA_CERT_FILE
#endif // stiENABLE_TLS


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StackShutdown
//
// Description: Synchronously shuts down the Sip stack (if running)
//
// Abstract:
//
// Returns: Nothing
//
void CstiSipManager::StackShutdown ()
{
	Lock ();
	
	if (m_bRunning)
	{
		for (auto &registration: m_Registrations)
		{
			delete registration.second;
		}
		m_Registrations.clear ();
		
		// Cancel the resource check timer.  It will be restarted when the stack is restarted.
		m_resourceCheckTimer.Stop ();

		TCPConnectionDestroy ();
		
		if (m_iceManager)
		{
			m_iceManager->IceStackUninitialize ();
		}
		
		//
		// Delete all public domain resolvers currently in the list.
		//
		m_PublicDomainResolvers.clear ();

		//
		// Delete all private domain resolvers currently in the list.
		//
		if (m_proxyDomainResolver)
		{
			stiASSERT (!m_proxyDomainResolver->m_bResolving);

			m_proxyDomainResolver = nullptr;
		}

#ifdef stiENABLE_TLS
		if (m_hTlsClientEngine)
		{
			if (m_hTransportMgr)
			{
				RvSipTransportTlsEngineDestruct (m_hTransportMgr, m_hTlsClientEngine);
			}
			m_hTlsClientEngine = nullptr;
		}
#endif
		
		// Shut down the registration system
		RegistrationsDelete ();
#if !defined(stiTUNNEL_MANAGER) || APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
		// This can cause a crash if rvSdpDestruct has already been called
		// Shut down the SDP manager
		rvSdpDestruct ();
#endif

		// Shut down the stack
		if (m_hStackMgr)
		{
			RvSipStackStopEventProcessingExt (m_hStackMgr);
			RvSipStackDestruct (m_hStackMgr);
			
			// Set our member pointers to the handles to NULL, they were destructed by RvSipStackDestruct
			m_hStackMgr = nullptr;
			m_hRegClientMgr = nullptr;
			m_hLog = nullptr;
			m_hCallLegMgr = nullptr;
			m_hAuthenticatorMgr = nullptr;
			m_hTransportMgr = nullptr;
			m_hSubscriptionMgr = nullptr;
			m_hTransactionMgr = nullptr;
			m_hResolverMgr = nullptr;
			m_hPersistentProxyConnection = nullptr;
			m_hKeepAliveTransmitter = nullptr;
		}

		// delete the app memory pool
		if (m_hAppPool)
		{
			// Free all the memory
			RPOOL_Destruct (m_hAppPool);
			m_hAppPool = nullptr;
		}

		m_pCallStorage->removeAll ();

		// Shutdown complete
		m_bRunning = estiFALSE;
	}
	Unlock ();
}


/*!\brief Converts the transport type to a string
 */
static std::string getTransportString (RvSipTransport transportType)
{
	switch (transportType)
	{
		case RVSIP_TRANSPORT_UNDEFINED:
			return "Unknown";

		case RVSIP_TRANSPORT_UDP:
			return "UDP";

		case RVSIP_TRANSPORT_TCP:
			return "TCP";

		case RVSIP_TRANSPORT_SCTP:
			return "SCTP";

		case RVSIP_TRANSPORT_TLS:
			return "TLS";

		case RVSIP_TRANSPORT_WEBSOCKET:
			return "WS";

		case RVSIP_TRANSPORT_WSS:
			return "WSS";

		case RVSIP_TRANSPORT_OTHER:
			return "Other";
	}

	return "Unknown";
}


/*!\brief Prepares a SIP message for transmission to the endpoint monitor
 */
void logSipMessage (
	RvChar *buffer,
	RvUint32 buffLen,
	const std::string &direction,
	RvSipTransportConnectionHandle hConn,
	RvSipTransportLocalAddrHandle hLocalAddr,
	RvSipTransportAddr *remoteAddrDetails)
{
	int64_t now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

	std::string message;
	message.assign(buffer, buffLen);

	RvSipTransportAddr localAddrDetails;

	if (hConn)
	{
		RvSipTransportConnectionGetActualLocalAddress (hConn, &localAddrDetails);
	}
	else
	{
		RvSipTransportMgrLocalAddressGetDetails (hLocalAddr, sizeof (localAddrDetails), &localAddrDetails);
	}

	vpe::EndpointMonitor::instanceGet ()->sipMessage (
		message,
		direction,
		remoteAddrDetails->strIP,
		remoteAddrDetails->port,
		localAddrDetails.strIP,
		localAddrDetails.port,
		getTransportString(remoteAddrDetails->eTransportType),
		now);
}


/*!\brief Grabs any incoming SIP message for transmission to the endpoint monitor
 */
void RVCALLCONV TransportConnectionBufferRecvdCB(
	RvSipTransportMgrHandle             hTransportMgr,
	RvSipAppTransportMgrHandle          hAppTransportMgr,
	RvSipTransportLocalAddrHandle       hLocalAddr,
	RvSipTransportAddr                  *pSenderAddrDetails,
	RvSipTransportConnectionHandle      hConn,
	RvSipTransportConnectionAppHandle   hAppConn,
	RvChar                              *buffer,
	RvUint32                            buffLen,
	RvBool                              *pbDiscardBuffer)
{
	logSipMessage(buffer, buffLen, "rcvd", hConn, hLocalAddr, pSenderAddrDetails);

	*pbDiscardBuffer = RV_FALSE;
}



/*!\brief Grabs any outgoing SIP message for transmission to the endpoint monitor
 */
void RVCALLCONV TransportConnectionBufferToSendCB(
	RvSipTransportMgrHandle             hTransportMgr,
	RvSipAppTransportMgrHandle          hAppTransportMgr,
	RvSipTransportLocalAddrHandle       hLocalAddr,
	RvSipTransportAddr                  *pDestAddrDetails,
	RvSipTransportConnectionHandle      hConn,
	RvSipTransportConnectionAppHandle   hAppConn,
	RvChar                              *buffer,
	RvUint32                            buffLen,
	RvBool                              *pbDiscardBuffer)
{
	logSipMessage(buffer, buffLen, "sent", hConn, hLocalAddr, pDestAddrDetails);

	*pbDiscardBuffer = RV_FALSE;
}


// Disallow all inbound connections. Endpoints that are configured to
// register with a proxy should establish a connection to
// the proxy and all calls to the endpoint should utilize that connection.
// Connections from the endpoint to any entities other than the proxy
// should be initiated by the endpoint itself.
void RVCALLCONV CstiSipManager::TransportConnectionCreatedCB(
	RvSipTransportMgrHandle hTransportMgr,
	RvSipAppTransportMgrHandle hAppTransportMgr,
	RvSipTransportConnectionHandle hConn,
	RvSipTransportConnectionAppHandle *phAppConn,
	RvBool *pbDrop)
{
	auto self = reinterpret_cast<const CstiSipManager *>(hAppTransportMgr);

	std::lock_guard<std::recursive_mutex> lock (self->m_execMutex);

	*pbDrop = RV_FALSE;

	// If this endpoint is configured to use an inbound proxy then
	// deny all incoming connections.
	if (self->UseInboundProxy ())
	{
		*pbDrop = RV_TRUE;
	}
}


/*!
 * \brief Starts up (or restart) the sip stack
 */
void CstiSipManager::eventStackStartup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

#ifdef stiHOLDSERVER
	LOG_DEBUG << "Message=\"SIP stack is starting up.\"" << std::flush;
#endif

	// If the stack is already running, shut it down first
	StackShutdown ();

	Lock ();

	std::string LocalIpAddress;
	std::string LocalIpv6Address;
	RvSipStackCfg sStackConfig;

#ifdef stiUSE_ZEROS_FOR_IP
	if (m_stConferenceParams.bIPv6Enabled)
	{
		LocalIpAddress.assign (RADVISION_ALL_ADAPTER_ADDRESSES);
	}
	else
	{
		// Using 0's for IP address. This resolves tsc_bind errors endpoints see when tunneling
		// causing the SIP stack to fail to startup.
		LocalIpAddress.assign ("0.0.0.0");
	}
#else // not stiUSE_ZEROS_FOR_IP
	hResult = stiGetLocalIp (&LocalIpAddress, estiTYPE_IPV4);
	stiTESTRESULT ();

	if (m_stConferenceParams.bIPv6Enabled)
	{
		hResult = stiGetLocalIp (&LocalIpv6Address, estiTYPE_IPV6);
		stiTESTRESULT ();
	}
#endif // stiUSE_ZEROS_FOR_IP
	
	if (RV_TRUE != RvSipStackMgrIsEnhancedDnsFeatureEnabled ())
	{
		stiASSERTMSG (estiFALSE, "Sip stack must be compiled with enhanced dns support to work properly with Sorenson proxies!");
	}

	// Set the stack settings.
	RvSipStackInitCfg (sizeof (RvSipStackCfg), &sStackConfig);

#ifdef VPE_RV_LOGGING
	sStackConfig.defaultLogFilters = vpe::RadvisionLogging::sipLogFilterGet();

#ifndef DEBUG_SIP_LOG_TO_FILE
	// Log to console, not to file.
	sStackConfig.pfnPrintLogEntryEvHandler = CstiSipManager::StackLogCB;
#endif // DEBUG_SIP_LOG_TO_FILE
#endif // VPE_RV_LOGGING

	sStackConfig.supportedExtensionList = (RvChar*)"100rel"; // We need to support reliable for UPDATE to work during INVITE.
	sStackConfig.manualPrack = RV_TRUE;
	sStackConfig.manualAckOn2xx = RV_TRUE;
	sStackConfig.maxRegClients = 8; // two each for: local, toll free, multiring local, and multiring toll free numbers
#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER)
	sStackConfig.maxCallLegs = 25; // This represents all call legs, not just established calls. The reason it is high is to account for call legs being dropped/rejected.
	sStackConfig.maxSubscriptions = 25;
	sStackConfig.maxConnections = sStackConfig.maxCallLegs + 1;
	sStackConfig.bUseRportParamInVia = RV_TRUE;
#else
	// the HOLDServer needs the following settings
	sStackConfig.maxCallLegs = m_maxCalls * 2; // This represents all call legs, not just established calls. The reason it is high is to account for call legs being dropped/rejected.
	sStackConfig.maxSubscriptions = m_maxCalls;
	sStackConfig.maxConnections = m_maxCalls + 1;
	sStackConfig.retransmissionT1 = 1000;
	//sStackConfig.numberOfProcessingThreads = -1; // 0, 1 or -1 == single threaded. >1 == multi-threaded
#endif
	sStackConfig.maxTransactions = (sStackConfig.maxCallLegs + sStackConfig.maxSubscriptions) * 2;
	sStackConfig.enableInviteProceedingTimeoutState = RV_TRUE; // This way we'll get notified if the call times out
	sStackConfig.provisionalTimer = INVITE_RESPONSE_TIMEOUT;
	sStackConfig.bDLAEnabled = RV_TRUE;  // Enable the ability to dynamically change the IP address
	sStackConfig.maxNumOfLocalAddresses = 12; // 
	sStackConfig.bEnableForking = RV_TRUE; // Enable the ability for calls to fork when different message tags are detected

	sStackConfig.maxDnsServers = 3; // The maximum number of DNS servers that can be used at the same time
	sStackConfig.maxElementsInSingleDnsList = 15;

	sStackConfig.sendReceiveBufferSize = MAX_SIP_MESSAGE_SIZE;  // Some devices (e.g. Cisco MCU) send such long messages that our buffer needs to be larger to handle them

#if APPLICATION == APP_NTOUCH // We are only changing this for ntouch VP as the other endpoints have plenty of memory.
	// This is half of the default size to keep memory low.
	// The highest value returned by some phones in the field is 10.
	sStackConfig.numOfReadBuffers = 49;
#endif

	//
	// Setup IPv4 UDP Address and port
	//
	strcpy (sStackConfig.localUdpAddress, LocalIpAddress.c_str ());
#ifndef stiINTEROP_EVENT
	if (m_stConferencePorts.nListenPort == STANDARD_SIP_PORT)
	{
		sStackConfig.localUdpPort = 50060;
	}
	else
	{
		sStackConfig.localUdpPort = m_stConferencePorts.nListenPort;
	}
#else
	sStackConfig.localUdpPort = m_stConferencePorts.nListenPort;
#endif

	//
	// Setup IPv6 UDP Address and port
	//
	if (!LocalIpv6Address.empty ())
	{
		sStackConfig.numOfExtraUdpAddresses = 1;
		sStackConfig.localUdpAddresses = (RvChar **)RvSipMidMemAlloc (sStackConfig.numOfExtraUdpAddresses * sizeof (RvChar*));
		sStackConfig.localUdpAddresses[0] = (RvChar *)RvSipMidMemAlloc (LocalIpv6Address.size () + 1);

		strcpy (sStackConfig.localUdpAddresses[0], LocalIpv6Address.c_str ());

		sStackConfig.localUdpPorts = (RvUint16 *)RvSipMidMemAlloc (sStackConfig.numOfExtraUdpAddresses * sizeof(RvUint16));
#ifndef stiINTEROP_EVENT
		sStackConfig.localUdpPorts[0] = 50060;
#else
		sStackConfig.localUdpPorts[0] = m_stConferencePorts.nListenPort;
#endif
	}

	//
	// Setup IPv4 TCP address and port
	//
	sStackConfig.tcpEnabled = RV_TRUE;

	strcpy (sStackConfig.localTcpAddress, LocalIpAddress.c_str ());
	sStackConfig.localTcpPort = m_stConferencePorts.nListenPort;

	//
	// Setup IPv6 TCP address and port
	//
	if (!LocalIpv6Address.empty ())
	{
		sStackConfig.numOfExtraTcpAddresses = 1;
		sStackConfig.localTcpAddresses = (RvChar **)RvSipMidMemAlloc (sStackConfig.numOfExtraTcpAddresses * sizeof (RvChar*));
		sStackConfig.localTcpAddresses[0] = (RvChar *)RvSipMidMemAlloc (LocalIpv6Address.size () + 1);

		strcpy (sStackConfig.localTcpAddresses[0], LocalIpv6Address.c_str ());

		sStackConfig.localTcpPorts = (RvUint16 *)RvSipMidMemAlloc (sStackConfig.numOfExtraTcpAddresses * sizeof(RvUint16));
		sStackConfig.localTcpPorts[0] = m_stConferencePorts.nListenPort;
	}

#ifdef stiENABLE_TLS
	if (LocalIpv6Address.empty())
	{
		sStackConfig.numOfTlsAddresses = 1;
	}
	else
	{
		sStackConfig.numOfTlsAddresses = 2;
	}
	sStackConfig.localTlsAddresses = (RvChar **) RvSipMidMemAlloc (sStackConfig.numOfTlsAddresses * sizeof(RvChar*));
	stiTESTCOND (sStackConfig.localTlsAddresses, stiRESULT_ERROR);

	memset (sStackConfig.localTlsAddresses, 0, sStackConfig.numOfTlsAddresses * sizeof(RvChar*));

	sStackConfig.localTlsPorts = (RvUint16*) RvSipMidMemAlloc (sStackConfig.numOfTlsAddresses * sizeof(RvUint16));
	stiTESTCOND (sStackConfig.localTlsPorts, stiRESULT_ERROR);

	memset (sStackConfig.localTlsPorts, 0, sStackConfig.numOfTlsAddresses * sizeof(RvUint16));

	//
	// Add IPv4 address for TLS
	//
	sStackConfig.localTlsAddresses[0] = (RvChar*)RvSipMidMemAlloc (RV_ADDRESS_MAXSTRINGSIZE + 1);
	strcpy (sStackConfig.localTlsAddresses[0], LocalIpAddress.c_str ());
	sStackConfig.localTlsPorts[0] = m_stConferencePorts.nTLSListenPort;
	
	//
	// Add IPv6 address for TLS
	//
	if (!LocalIpv6Address.empty ())
	{
		sStackConfig.localTlsAddresses[1] = (RvChar*)RvSipMidMemAlloc (RV_ADDRESS_MAXSTRINGSIZE + 1);
		strcpy (sStackConfig.localTlsAddresses[1], LocalIpv6Address.c_str ());
		sStackConfig.localTlsPorts[1] = m_stConferencePorts.nTLSListenPort;
	}

	sStackConfig.numOfTlsEngines  = 2;
	sStackConfig.maxTlsSessions   = 10;
#endif
#ifdef stiUSE_GOOGLE_DNS
	if (g_bUseGoogleDNS)
	{
		sStackConfig.numOfDnsServers  = 1;
		sStackConfig.pDnsServers = (RvSipTransportAddr*)RvSipMidMemAlloc (sizeof(RvSipTransportAddr));
		memset (sStackConfig.pDnsServers, 0, sizeof(RvSipTransportAddr));
		sStackConfig.pDnsServers[0].eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
		sStackConfig.pDnsServers[0].eTransportType = RVSIP_TRANSPORT_TCP;
		sStackConfig.pDnsServers[0].port = 53;
		strcpy(sStackConfig.pDnsServers[0].strIP, "8.8.8.8");
	}
#endif
#if defined(stiMVRS_APP)
	// Resolve issue on iOS with connections that don't get cleaned up
	sStackConfig.maxTlsSessions = 1000;
	sStackConfig.maxConnections = 100;
#endif
//#ifdef stiTUNNEL_MANAGER
//	sStackConfig.pDnsBindAddr = (RvSipTransportAddr*)RvSipMidMemAlloc (sizeof(RvSipTransportAddr));
//	sStackConfig.pDnsBindAddr->eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
//	strcpy(sStackConfig.pDnsBindAddr->strIP, szLocalIpAddress);
//#endif
	DBG_MSG ("Starting sip stack on %s\n\tport %d UDP and port %d TCP",
		LocalIpAddress.c_str (),
		sStackConfig.localUdpPort,
		sStackConfig.localTcpPort);

	// Set up the stack
	rvStatus = RvSipStackConstruct (sizeof (RvSipStackCfg), &sStackConfig, &m_hStackMgr);
	stiTESTRVSTATUS ();

#ifdef stiENABLE_TLS
	for (int i = 0; i < sStackConfig.numOfTlsAddresses; i++)
	{
		RvSipMidMemFree (sStackConfig.localTlsAddresses[i]);
	}
	RvSipMidMemFree (sStackConfig.localTlsAddresses);
	RvSipMidMemFree (sStackConfig.localTlsPorts);
#endif
#ifdef stiUSE_GOOGLE_DNS
	if (g_bUseGoogleDNS)
	{
		RvSipMidMemFree(sStackConfig.pDnsServers);
	}
#endif
	m_bRunning = estiTRUE; // Flag it as running now!

	// Set the mid-level logger
#ifdef DEBUG_SIP_MID_LEVEL
	RV_LOG_Handle hLog;
	RvSipStackGetLogHandle (m_hStackMgr, &hLog);
	RvSipMidSetLog (m_hMidMgr, hLog);
#endif

	// Set up SDP (for media negotiation)
	RvSdpStackCfg sSdpCfg;
	memset (&sSdpCfg, 0, sizeof (sSdpCfg));

	// Disable SDP logging to prevent crash
	sSdpCfg.disableSdpLogs = RV_TRUE;

	rvStatus = rvSdpConstruct (&sSdpCfg);
	stiTESTRVSTATUS ();

	// Save some important handles we may need
	RvSipStackGetRegClientMgrHandle (m_hStackMgr, &m_hRegClientMgr);
	RvSipStackGetLogHandle (m_hStackMgr, &m_hLog);
	RvSipStackGetCallLegMgrHandle (m_hStackMgr, &m_hCallLegMgr);
	RvSipStackGetAuthenticatorHandle (m_hStackMgr, &m_hAuthenticatorMgr);
	RvSipStackGetTransportMgrHandle (m_hStackMgr, &m_hTransportMgr);
	RvSipStackGetSubsMgrHandle (m_hStackMgr, &m_hSubscriptionMgr);
	RvSipStackGetTransactionMgrHandle (m_hStackMgr, &m_hTransactionMgr);
	RvSipStackGetResolverMgrHandle (m_hStackMgr, &m_hResolverMgr);

#ifdef VPE_RV_LOGGING
	{
		RvRtpSetExternalLogManager ((RvRtpLogger*)m_hLog);
		RvRtpSetLogModuleFilter (RVRTP_GLOBAL_MASK, vpe::RadvisionLogging::rtpLogFiterGet());
		auto logFile = RvLogListenerGetLogfile ((RvLogMgr*)m_hLog);
		if (logFile != nullptr)
		{
			RvLogListenerSetMilliSecondsInLog (logFile, RV_TRUE);
		}
	}
#endif

	// Construct a pool of memory for the application
#ifdef DEBUG_SIP_LOG_MEMORY_POOL
	m_hAppPool = RPOOL_Construct (1024, 32, m_hLog, RV_FALSE, "ApplicationPool");
#else
	m_hAppPool = RPOOL_Construct (1024, 32, nullptr, RV_FALSE, "ApplicationPool");
#endif

	// Associate this class with the stack
	RvSipCallLegMgrSetAppMgrHandle (m_hCallLegMgr, this);

	// Set up registrar callbacks
	RvSipRegClientEvHandlers sRegistrarHandlers;
	memset (&sRegistrarHandlers, 0, sizeof (RvSipRegClientEvHandlers));
	sRegistrarHandlers.pfnStateChangedEvHandler = CstiSipRegistration::RegistrarStateChangedCB;
	sRegistrarHandlers.pfnFinalDestResolvedEvHandler = CstiSipRegistration::FinalDestResolvedCB;

	rvStatus = RvSipRegClientMgrSetEvHandlers (m_hRegClientMgr, &sRegistrarHandlers, sizeof (RvSipRegClientEvHandlers));
	stiTESTRVSTATUS ();

	// Set up transport callbacks
	RvSipTransportMgrEvHandlers sTransportHandlers;
	memset (&sTransportHandlers, 0, sizeof (RvSipTransportMgrEvHandlers));
	sTransportHandlers.pfnEvBadSyntaxMsg = CstiSipManager::TransportParseErrorCB;
	sTransportHandlers.pfnEvConnServerReuse = CstiSipManager::TransportConnectionServerReuseCB;
#ifdef stiENABLE_TLS
	sTransportHandlers.pfnEvTlsStateChanged = CstiSipManager::TransportConnectionTlsStateChangedCB;
	sTransportHandlers.pfnEvTlsSeqStarted = CstiSipManager::TransportConnectionTlsSequenceStartedCB;
	sTransportHandlers.pfnEvTlsPostConnectionAssertion = CstiSipManager::TransportConnectionTlsPostConnectionAssertionEvCB;
#endif
	sTransportHandlers.pfnEvConnCreated = TransportConnectionCreatedCB;
	sTransportHandlers.pfnEvBufferReceived = TransportConnectionBufferRecvdCB;
	sTransportHandlers.pfnEvBufferToSend = TransportConnectionBufferToSendCB;

	rvStatus = RvSipTransportMgrSetEvHandlers (m_hTransportMgr, reinterpret_cast<RvSipAppTransportMgrHandle>(this), &sTransportHandlers, sizeof (RvSipTransportMgrEvHandlers));
	stiTESTRVSTATUS ();

	// Set up transaction callbacks
	RvSipTransactionEvHandlers sTransactionHandlers;
	memset (&sTransactionHandlers, 0, sizeof (RvSipTransactionEvHandlers));
	sTransactionHandlers.pfnEvMsgToSend = CstiSipManager::TransactionMessageSendCB;
	sTransactionHandlers.pfnEvStateChanged = CstiSipManager::TransactionStateChangedCB;
	sTransactionHandlers.pfnEvTransactionCreated = CstiSipManager::TransactionCreatedCB;

	rvStatus = RvSipTransactionMgrSetEvHandlers (m_hTransactionMgr, this, &sTransactionHandlers, sizeof (RvSipTransactionMgrEvHandlers));
	stiTESTRVSTATUS ();

	// Set up call leg callbacks
	RvSipCallLegEvHandlers sCallLegHandlers;
	memset (&sCallLegHandlers, 0, sizeof (RvSipCallLegEvHandlers));
	sCallLegHandlers.pfnStateChangedEvHandler = CstiSipManager::CallLegStateChangeCB;
	sCallLegHandlers.pfnCallLegCreatedEvHandler = CstiSipManager::CallLegCreatedCB;
	sCallLegHandlers.pfnMsgReceivedEvHandler = CstiSipManager::CallLegMessageCB;
	sCallLegHandlers.pfnTranscCreatedEvHandler = CstiSipManager::CallLegTransactionCreatedCB;
	sCallLegHandlers.pfnTranscStateChangedEvHandler = CstiSipManager::CallLegTransactionStateChangeCB;
	sCallLegHandlers.pfnPrackStateChangedEvEvHandler = CstiSipManager::CallLegPrackStateCB;
	sCallLegHandlers.pfnProvisionalResponseRcvdEvHandler = CstiSipManager::CallLegProvisionalResponseReceivedCB;
	sCallLegHandlers.pfnReInviteStateChangedEvHandler = CstiSipManager::CallLegReInviteCB;
	sCallLegHandlers.pfnMsgToSendEvHandler = CstiSipManager::CallLegMessageSendCB;
	sCallLegHandlers.pfnCallLegCreatedDueToForkingEvHandler = CstiSipManager::CallForkDetectedCB;
	sCallLegHandlers.pfnFinalDestResolvedEvHandler = CstiSipManager::CallLegFinalDestResolvedCB;
	sCallLegHandlers.pfnNewConnInUseEvHandler = CstiSipManager::NewConnectionCB;

	rvStatus = RvSipCallLegMgrSetEvHandlers (m_hCallLegMgr, &sCallLegHandlers, sizeof (RvSipCallLegEvHandlers));
	stiTESTRVSTATUS ();

	// Set up subscription monitoring callbacks
	RvSipSubsMgrSetAppMgrHandle (m_hSubscriptionMgr, this);
	RvSipSubsEvHandlers sSubscriptionHandlers;
	memset (&sSubscriptionHandlers, 0, sizeof (RvSipSubsEvHandlers));
	sSubscriptionHandlers.pfnSubsCreatedEvHandler = CstiSipManager::SubscriptionCreatedCB;
	sSubscriptionHandlers.pfnReferNotifyReadyEvHandler = CstiSipManager::ReferNotifyReadyCB;
	sSubscriptionHandlers.pfnStateChangedEvHandler = CstiSipManager::SubscriptionStateChangeCB;
	sSubscriptionHandlers.pfnNotifyEvHandler = CstiSipManager::SubscriptionNotifyCB;

	rvStatus = RvSipSubsMgrSetEvHandlers (m_hSubscriptionMgr, &sSubscriptionHandlers, sizeof (RvSipSubsEvHandlers));
	stiTESTRVSTATUS ();

	// Set up authentication callbacks
	RvSipAuthenticatorEvHandlers sAuthenticationHandlers;
	memset (&sAuthenticationHandlers, 0, sizeof (RvSipAuthenticatorEvHandlers));
	sAuthenticationHandlers.pfnMD5AuthenticationExHandler = CstiSipManager::AuthenticationMD5CB;
	sAuthenticationHandlers.pfnGetSharedSecretAuthenticationHandler = CstiSipManager::AuthenticationLoginCB;
	sAuthenticationHandlers.pfnNonceCountUsageAuthenticationHandler = CstiSipManager::NonceCountIncrementor;
	
	// Make sure this object gets passed ot authentication callbacks
	rvStatus = RvSipAuthenticatorSetAppHandle (m_hAuthenticatorMgr, (RvSipAppAuthenticatorHandle)this);
	stiTESTRVSTATUS ();

	rvStatus = RvSipAuthenticatorSetEvHandlers (m_hAuthenticatorMgr, &sAuthenticationHandlers, sizeof (RvSipAuthenticatorEvHandlers));
	stiTESTRVSTATUS ();

#if defined (stiENABLE_TLS)
	hResult = InitTlsSecurity();
	stiTESTRESULT ();
#endif

	DBG_MSG ("stack successfully initialized.");

#ifdef stiLOG_SLP_CHANGES
	Test (m_hTransportMgr);
#endif //#ifdef stiLOG_SLP_CHANGES

	if (m_iceManager)
	{
		m_iceManager->rvLogManagerSet ((RvLogMgr*)m_hLog);

		hResult = m_iceManager->IceStackInitialize (m_stConferenceParams.maxIceSessions);
		stiTESTRESULT ();
	}

	// Start a timer that checks for problems with the stack resources
	m_resourceCheckTimer.Start ();

#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
	if (!m_stConferenceParams.bIPv6Enabled)
	{
		SIPResolverIPv4DNSServersSet ();
	}
#endif
	
	ConferenceParamsSet (&m_stConferenceParams);
	
	// Set the time to be used later in case we are failing to register and need to restart.
	m_un32StackStartupTime = stiOSTickGet ();
	
	
STI_BAIL:

	// If we failed above, shut down any parts we had done and set a timer to try again.
	if (stiIS_ERROR (hResult))
	{
		sipStackStartupFailedSignal.Emit();

		// How long to wait before retrying the stack startup.
		// The default is un16STACK_STARTUP_RETRY_WAIT (20 seconds)
		uint16_t stackStartupRetryWait = un16STACK_STARTUP_RETRY_WAIT;

#if APPLICATION == APP_HOLDSERVER
		//The hold server should not try to rebind on a different port, but should still retry
		stiASSERTMSG(estiFALSE,
			"CstiSipManager stack startup failed, with code %d. Retrying in %d seconds.",
			hResult, stackStartupRetryWait / 1000);
#else

		// Try to bind to the sockets to make sure they are available.

		auto portUsageCheck = [LocalIpAddress, &stackStartupRetryWait](uint16_t & port, bool isTCPPort)
		{
			bool portInUse;
			stiHResult portStatusResult = stiOSPortUsageCheck (port, isTCPPort,
															   LocalIpAddress.c_str (), portInUse);
			if (!stiIS_ERROR (portStatusResult) && portInUse)
			{
				port = stiOSAvailablePortGet (LocalIpAddress.c_str (), isTCPPort);
				stiASSERTMSG (estiFALSE,
							  "CstiSipManager stack startup failed, retrying with ListenPort = %d",
							  port);
				// We do not need to wait a full 20 seconds,
				// we know what the issue is let's try again in 500ms.
				stackStartupRetryWait = 500;
			}
		};

		portUsageCheck (m_stConferencePorts.nListenPort, true); //TCP Port Check
		portUsageCheck (m_stConferencePorts.nTLSListenPort, true); //TLS Port Check
		portUsageCheck (m_stConferencePorts.nListenPort, false);// UDP Port Check
#endif

		StackShutdown ();

		// Set a timer to try again
		m_stackStartupRetryTimer.TimeoutSet (stackStartupRetryWait);
		m_stackStartupRetryTimer.Restart ();
	}
	else
	{
#if APPLICATION == APP_HOLDSERVER
		LOG_DEBUG << "Message=\"SIP stack startup succeeded.\" "
			<< "Result=" << hResult << std::flush;
#endif

		sipStackStartupSucceededSignal.Emit();
	}


	Unlock ();
} // end CstiSipManager::eventStackStartup


/*!\brief Returns the local listening port.
 * 
 */
uint16_t CstiSipManager::LocalListenPortGet ()
{
	return m_stConferencePorts.nListenPort;
}


/*!\brief Sets the conference addresses for the public and private addresses
 * 
 */
void CstiSipManager::ConferenceAddressSet ()
{
	bool bConferenceAddressChanged = false;
	
	if (UseInboundProxy ())
	{
		//
		// If we have both a public address and a private address and
		// they are different from the last time we registered
		// the conference addresses then register the conference
		// addresses.
		//
		if (!m_PublicDomainIpAddress.empty () && !m_proxyDomain.empty ()
		 && (m_ConferenceAddresses.SIPPublicAddress != m_PublicDomainIpAddress
		 || m_ConferenceAddresses.nSIPPublicPort != m_un16PublicDomainPort
		 || m_ConferenceAddresses.SIPPrivateAddress != m_proxyDomain))
		{
			m_ConferenceAddresses.SIPPublicAddress = m_PublicDomainIpAddress;
			m_ConferenceAddresses.nSIPPublicPort = m_un16PublicDomainPort;
			
			m_ConferenceAddresses.SIPPrivateAddress = m_proxyDomain;
			m_ConferenceAddresses.nSIPPrivatePort = nDEFAULT_SIP_LISTEN_PORT; // The port should be resolved by the calling endpoint using SRV lookup

			bConferenceAddressChanged = true;
		}
	}
	else
	{
		int nListeningPort = LocalListenPortGet ();
		
		// The address pushed to the public database will always be IPv4
		if (m_ConferenceAddresses.SIPPublicAddress != m_stConferenceParams.PublicIPv4
			|| m_ConferenceAddresses.nSIPPublicPort != nListeningPort
			|| m_ConferenceAddresses.SIPPrivateAddress != m_stConferenceParams.PublicIPv4
			|| m_ConferenceAddresses.nSIPPrivatePort != nListeningPort)
		{
			m_ConferenceAddresses.SIPPublicAddress = m_stConferenceParams.PublicIPv4;
			m_ConferenceAddresses.nSIPPublicPort = nListeningPort;
			
			m_ConferenceAddresses.SIPPrivateAddress = m_ConferenceAddresses.SIPPublicAddress;
			m_ConferenceAddresses.nSIPPrivatePort = m_ConferenceAddresses.nSIPPublicPort;
			
			bConferenceAddressChanged = true;
		}
	}

#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER) && (APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL)
	// Our address in core may have changed/expired.
	if (bConferenceAddressChanged)
	{
		sipMessageConfAddressChangedSignal.Emit (m_ConferenceAddresses);
	}
#endif
	stiUNUSED_ARG (bConferenceAddressChanged);  // Load test tool gets a compile error if we don't have this here.

}


/*!
 * \brief Handles the Resolver Retry event
 * \param pResolver
 */
void CstiSipManager::eventResolverRetry (
	const CstiSipResolverSharedPtr &pResolver)
{
	switch (pResolver->m_eType)
	{
		case CstiSipResolver::eProxy:
		{

			// If this resolver is still the active private domain resolver
			// then try resolving again.  Otherwise, delete it.
			//
			if (pResolver == m_proxyDomainResolver)
			{
				ProxyDomainResolve ();
			}

			break;
		}

		case CstiSipResolver::ePublic:
		{
			if (!pResolver->m_bCanceled
			 && pResolver->m_PrimaryProxyAddress == m_stConferenceParams.SIPRegistrationInfo.PublicDomain
			 && pResolver->m_AltProxyAddress == m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt
			 && pResolver->m_nAllowedTransports == estiTransportTCP)
			{
				stiHResult hResult = pResolver->Resolve ();

				if (stiIS_ERROR (hResult))
				{
					pResolver->m_DnsRetryTimer.Start ();
				}
			}
			break;
		}

	}
}

/*!
 * \brief "factory" for resolver shared pointer
 *
 * Also sets up the callback connections
 */
CstiSipResolverSharedPtr CstiSipManager::ResolverAllocate(CstiSipResolver::EResolverType type)
{
	std::string primaryProxyAddress;
	std::string altProxyAddress;
	auto allowedProxyTransports = m_stConferenceParams.nAllowedProxyTransports;

	switch(type)
	{
		case CstiSipResolver::ePublic:
			primaryProxyAddress = m_stConferenceParams.SIPRegistrationInfo.PublicDomain;
			altProxyAddress = m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt;
			allowedProxyTransports = estiTransportTCP;
			break;
		case CstiSipResolver::eProxy:
			primaryProxyAddress = ProxyDomainGet ();
			altProxyAddress = ProxyDomainAltGet ();
			break;
	}

	// NOTE: can't use make_shared since we need a custom deleter
	auto resolver = CstiSipResolverSharedPtr(
		new CstiSipResolver(
			type,
			m_hResolverMgr,
			m_hTransportMgr,
			m_execMutex,
			m_hAppPool,
			primaryProxyAddress,
			altProxyAddress,
			allowedProxyTransports,
			m_stConferenceParams.bIPv6Enabled),
		std::bind(&CstiSipManager::ResolverDeleter, this, std::placeholders::_1)
		);

	// Store connection in a map to be cleaned up on delete
	// NOTE: in this case the connection could be cleaned up with the resolver object.
	//       (TODO: may be nice to address that some how)

	// NOTE: use weak_ptr to avoid circular reference (was causing crash)
	// TODO: may be nice to implement "weak_bind"... ?

	CstiSipResolverWeakPtr resolverWeak = resolver;

	m_resolverConnections[resolver.get ()].push_back (
		resolver->eventSignal.Connect ([this, resolverWeak](EstiResolverEvent event)
			{
				if (CstiSipResolverSharedPtr resolverShared = resolverWeak.lock())
				{
					PostEvent (std::bind (&CstiSipManager::eventResolverCallback, this, resolverShared, event));
				}
			}
			)
		);

	m_resolverConnections[resolver.get ()].push_back (
		resolver->m_DnsRetryTimer.timeoutEvent.Connect ([this, resolverWeak]()
			{
				if (CstiSipResolverSharedPtr resolverShared = resolverWeak.lock())
				{
					PostEvent (std::bind (&CstiSipManager::eventResolverRetry, this, resolverShared));
				}
			}
			)
		);

	return resolver;
}

/*!
 * \brief deleter for resolver shared pointer
 */
void CstiSipManager::ResolverDeleter (CstiSipResolver *resolver)
{
	m_resolverConnections.erase (resolver); // Disconnect
	delete resolver;
}

/*!
 * \brief Event handler for events from the resolver
 */
void CstiSipManager::eventResolverCallback (
	const CstiSipResolverSharedPtr &pResolver,
	EstiResolverEvent resolverEvent)
{
	// Set resolving back to false to allow for the resolver to be deleted if it was canceled.
	if (pResolver)
	{
		pResolver->m_bResolving = false;
	}

	switch (resolverEvent)
	{
		case RESOLVE_EVENT_COMPLETE:
		{
			switch (pResolver->m_eType)
			{
				case CstiSipResolver::eProxy:
				{
					if (!pResolver->m_bCanceled)
					{
						//
						// If the domain and transport to resolve hasn't changed...
						// As long as the resolved proxy address is either the primary or alternate, and the
						// resolved transport is one of the allowed transports then we are good to go.
						//
						if ((*pResolver->m_pProxyAddress == ProxyDomainGet ()
							|| *pResolver->m_pProxyAddress == ProxyDomainAltGet ())
						&& (pResolver->m_eResolvedTransport & m_stConferenceParams.nAllowedProxyTransports))
						{
							//
							// See if the proxy address or port has changed.  If either have then
							// re-register with the proxy.
							//
							if (m_proxyDomainIpAddress != pResolver->m_ResolvedIPAddress
							 || m_proxyDomainPort != pResolver->m_un16ResolvedPort)
							{
								m_proxyDomain = *pResolver->m_pProxyAddress;

								m_proxyDomainIpAddress = pResolver->m_ResolvedIPAddress;
								m_proxyDomainPort = pResolver->m_un16ResolvedPort;

								ConferenceAddressSet ();

								// Set any new registrations and/or changes
								for (const auto &credentials: m_stConferenceParams.SIPRegistrationInfo.PasswordMap)
								{
									if ((credentials.first == m_UserPhoneNumbers.szLocalPhoneNumber
									 || credentials.first == m_UserPhoneNumbers.szTollFreePhoneNumber
									 || credentials.first == m_UserPhoneNumbers.szRingGroupLocalPhoneNumber
									 || credentials.first == m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber
									 || credentials.first == m_UserPhoneNumbers.szSorensonPhoneNumber))
									{
										Register (credentials.first.c_str());
									}
								}
							}

							m_eLastNATTransport = pResolver->m_eResolvedTransport;
						}
					}
					else
					{
						//
						// Delete the resolver.
						//
						if (pResolver == m_proxyDomainResolver)
						{
							m_proxyDomainResolver = nullptr;
						}
					}

					break;
				}

				case CstiSipResolver::ePublic:
				{
					if (!pResolver->m_bCanceled)
					{
						//
						// If the domain and transport to resolve hasn't changed...
						// As long as the resolved proxy address is either the primary or alternate, and the
						// resolved transport is one of the allowed transports then we are good to go.
						//
						if ((*pResolver->m_pProxyAddress == m_stConferenceParams.SIPRegistrationInfo.PublicDomain
						 || *pResolver->m_pProxyAddress == m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt)
						&& pResolver->m_eResolvedTransport == estiTransportTCP)
						{
							if (m_PublicDomainIpAddress != pResolver->m_ResolvedIPAddress
							|| m_un16PublicDomainPort != pResolver->m_un16ResolvedPort)
							{
								m_PublicDomainIpAddress = pResolver->m_ResolvedIPAddress;
								m_un16PublicDomainPort = pResolver->m_un16ResolvedPort;

								ConferenceAddressSet();
							}
						}

						m_LastResolvedPublicDomain = *pResolver->m_pProxyAddress;
					}

					//
					// Remove the resolver from the list.
					//
					ResolverRemove (pResolver);

					break;
				}
			}

			break;
		}

		case RESOLVE_EVENT_FAILED:
		{
			if (!pResolver->m_bCanceled)
			{
				pResolver->m_DnsRetryTimer.Start ();
			}
			else
			{
				if (pResolver == m_proxyDomainResolver)
				{
					m_proxyDomainResolver = nullptr;
				}
			}

			break;
		}
		case RESOLVE_OUT_OF_RESOURCES:
		{
			std::string eventMessage = "EventType=SipStackRestart Reason=SipResolverOutOfResources";
			stiRemoteLogEventSend (&eventMessage);
			StackRestart ();
			break;
		}
	} // end switch
}

/*!\brief The resolver is no longer valid.  Remove it from any resolver lists.
 *
 * \param pResolver The resolver to clean up.
 */
stiHResult CstiSipManager::ResolverRemove (
	const CstiSipResolverSharedPtr &pResolver)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock();

	stiASSERT (!pResolver->m_bResolving); // We don't want to delete resolvers that are currently resolving.

	auto it = std::find(m_PublicDomainResolvers.begin(), m_PublicDomainResolvers.end(), pResolver);
	if (it != m_PublicDomainResolvers.end())
	{
		m_PublicDomainResolvers.erase(it);
	}

	Unlock();

	return hResult;
}

bool CstiSipManager::IsRegistered ()
{
	bool bRegistered = false;
	
	for (auto &registration: m_Registrations)
	{
		if (registration.second)
		{
			bRegistered = true;
		}
	}
	
	return bRegistered;
}


/*!\brief Returns true if a new proxy domain resolver is required
 *
 */
bool CstiSipManager::newProxyDomainResolverRequired ()
{
	return (
		//
		// There is no current resolver and the private domain information has changed...
		//
		m_proxyDomainResolver == nullptr

		//
		// There is a current resolver and it is resolving or resolved something different than what we had
		// previously.
		//
		|| (m_proxyDomainResolver->m_PrimaryProxyAddress != ProxyDomainGet ()
			|| m_proxyDomainResolver->m_nAllowedTransports != m_stConferenceParams.nAllowedProxyTransports)
	);
}


/*!
 * \brief CstiSipManager::eventNatTraversalConfigure
 */
void CstiSipManager::eventNatTraversalConfigure ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!UseInboundProxy ())
	{
		//
		// We are disabling NAT Traversal or are already disabled.
		//

		//
		// Remove the domain resolver for non-agent phones to indicate
		// we are not using a proxy domain. This will be the trigger
		// to re-resolve the domain if the endpoint is switched back
		// to use NAT Traversal.
		//
		if (!IsLocalAgentMode ())
		{
			ProxyDomainResolverDelete ();
		}

		//
		// Unregister any registration clients
		//
		for (auto &registration: m_Registrations)
		{
			if (registration.second)
			{
				Unregister(registration.first.c_str());
			}
		}

		//
		// Destroy the current TCP connection.
		//
		TCPConnectionDestroy ();

		//
		// Set up the ICE manager without the TURN and STUN servers
		//
		if (m_iceManager)
		{
			//
			// If NAT Traversal is disabled don't specify STUN or TURN.
			//
			hResult = m_iceManager->ServersSet ({}, {}, CstiIceSession::Protocol::None);
			stiTESTRESULT ();
		}

		//
		// Unregister and registration clients
		//
		Unregister(nullptr);

		//
		// Clear the proxy ip address and port information in non-agent modes
		//
		// Agent phones need this resolution information to place calls
		//
		if (!IsLocalAgentMode ())
		{
			m_proxyDomainIpAddress.clear ();
			m_proxyDomainPort = 0;

			m_eLastNATTransport = estiTransportUnknown;

			m_proxyDomain.clear ();

			m_LastResolvedPublicDomain.clear ();
			m_PublicDomainIpAddress.clear ();
			m_un16PublicDomainPort = 0;
		}

		//
		// Set the conference address
		//
		ConferenceAddressSet ();

		// Resolve the agent realm when not registered
		if (IsLocalAgentMode ())
		{
			if (newProxyDomainResolverRequired ())
			{
				//
				// Delete the current resolver, if there is one.
				//
				ProxyDomainResolverDelete ();

				m_proxyDomainResolver = ResolverAllocate(CstiSipResolver::eProxy);
			}

			ProxyDomainResolve ();
		}
	}
	else
	{
#ifdef stiTUNNEL_MANAGER
		if (m_stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling)
		{
			if (RvTunGetPolicy () == RvTun_ENABLED)
			{

				//
				// We are currently tunneling.
				// Check to see if the Tunnel IP address has changed.
				// If it has changed then call EventNetworkChangeNotify to re-setup the
				// local addresses to use the new tunnel IP
				//
				if (m_stConferenceParams.TunneledIpAddr != m_TunneledIpAddr)
				{
					//
					// The tunnel IP has changed.  Store the new address and call
					// eventNetworkChangeNotify...
					//
					m_TunneledIpAddr.assign(m_stConferenceParams.TunneledIpAddr);
					eventNetworkChangeNotify ();
				}
			}
			else
			{
				//
				// We are not currently tunneling.
				// Check to see if we have a tunnel ip address.
				// If we have one then start tunneling.
				//
				if (!m_stConferenceParams.TunneledIpAddr.empty())
				{
					m_TunneledIpAddr.assign(m_stConferenceParams.TunneledIpAddr);
					RvTunSetPolicy (RvTun_ENABLED);
					g_bSipTunnelingEnabled = true;

					eventNetworkChangeNotify ();
				}
			}
		}
		else
		{
			//
			// Tunneling should be disabled.  Check to see if we
			// are currently tunneling and, if we are, stop tunneling.
			//
			if (RvTunGetPolicy () == RvTun_ENABLED)
			{
				RvTunSetPolicy (RvTun_DISABLED);
				g_bSipTunnelingEnabled = false;

				m_TunneledIpAddr.clear();

				eventNetworkChangeNotify ();
			}
		}
#endif

		//
		// We are enabled for NAT Traversal or becoming enabled
		//

		//
		// Unregister any registration clients that have been removed from the credentials list
		// or we don't have a phonenumber for.
		//
		for (auto &registration: m_Registrations)
		{
			auto credentials = m_stConferenceParams.SIPRegistrationInfo.PasswordMap.find (registration.first);

			if (registration.second
			 && (credentials == m_stConferenceParams.SIPRegistrationInfo.PasswordMap.end()
			 || (strcmp (registration.second->PhoneNumberGet(), m_UserPhoneNumbers.szLocalPhoneNumber) != 0
			 && strcmp (registration.second->PhoneNumberGet(), m_UserPhoneNumbers.szTollFreePhoneNumber) != 0
			 && strcmp (registration.second->PhoneNumberGet(), m_UserPhoneNumbers.szRingGroupLocalPhoneNumber) != 0
			 && strcmp (registration.second->PhoneNumberGet(), m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber) != 0
			 && strcmp (registration.second->PhoneNumberGet(), m_UserPhoneNumbers.szSorensonPhoneNumber) != 0)))
			{
				Unregister(registration.first.c_str());
			}
		}

		//
		// If we have a resolver manager and the public domain is not empty...
		//
		if (m_hResolverMgr && !m_stConferenceParams.SIPRegistrationInfo.PublicDomain.empty())
		{
			CstiSipResolverSharedPtr pResolver;

			//
			// Get the newest resolver
			//
			if (!m_PublicDomainResolvers.empty ())
			{
				pResolver = m_PublicDomainResolvers.back ();
			}

			//
			// If the resolver is not for this domain and transport then
			// start the resolution process.
			//
			if (
			 //
			 // There is no current resolver.
			 //
			 (pResolver == nullptr && (m_stConferenceParams.SIPRegistrationInfo.PublicDomain != m_LastResolvedPublicDomain
			 && (m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt.empty () || m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt != m_LastResolvedPublicDomain)))
			 ||

			 //
			 // There is a current resolver
			 //
			 (pResolver && (pResolver->m_PrimaryProxyAddress != m_stConferenceParams.SIPRegistrationInfo.PublicDomain
			 || pResolver->m_AltProxyAddress != m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt
			 || pResolver->m_nAllowedTransports != estiTransportTCP)))
			{
				//
				// Cancel all resolvers currently in the list.
				//
				for (auto &resolver: m_PublicDomainResolvers)
				{
					resolver->m_bCanceled = true;
				}

				// Try the primary domain first, then if that fails we'll do the alternate
				pResolver = ResolverAllocate(CstiSipResolver::ePublic);

				m_PublicDomainResolvers.push_back (pResolver);

				// Make sure the resolve call succeeded.  If it did, but it came from
				// the cache, the resolution is already complete.  So instead of adding
				// it to the list, let's just make sure it gets cleaned up.
				stiHResult hResolveResult = pResolver->Resolve ();

				if (stiIS_ERROR (hResolveResult))
				{
					//
					// An error occured.  Start a timer to try again.
					//
					pResolver->m_DnsRetryTimer.Start ();
				}
			}
		}

		//
		// If we have a resolver manager and the private domain is not empty...
		//
		// NOTE: don't want to do this in interpreter mode
		// Maybe abstract out just the private and agent resolver stuff
		if (m_hResolverMgr && !ProxyDomainGet ().empty ())
		{
			//
			// If the private domain is different from the previous setting then do a DNS lookup
			//
			if (newProxyDomainResolverRequired ())
			{
				ProxyRegistrarLookup ();
			}
			else if (!m_proxyDomain.empty ())
			{
				//
				// The private domain information has not changed.
				//
				ConferenceAddressSet ();

				//
				// Find any new numbers and set them up for registration
				//
				std::map<std::string, std::string>::const_iterator i;

				for (i = m_stConferenceParams.SIPRegistrationInfo.PasswordMap.begin (); i != m_stConferenceParams.SIPRegistrationInfo.PasswordMap.end(); i++)
				{
					//
					// See if the number is in the previous map.  If it is not then set it up for registration
					//
					auto j = m_Registrations.find (i->first);

					if (j == m_Registrations.end()
					 && (i->first == m_UserPhoneNumbers.szLocalPhoneNumber
					 || i->first ==  m_UserPhoneNumbers.szTollFreePhoneNumber
					 || i->first ==  m_UserPhoneNumbers.szRingGroupLocalPhoneNumber
					 || i->first ==  m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber
					 || i->first ==  m_UserPhoneNumbers.szSorensonPhoneNumber))
					{
						//
						// The number wasn't found so set it up for registration
						//
						Register (i->first.c_str ());
					}
				}
			}
		}

		//
		// Set up TURN and/or STUN if NAT Traversal has been enabled
		//
		if (m_iceManager)
		{
			m_iceManager->PrioritizePrivacySet (m_stConferenceParams.prioritizePrivacy);
			
			switch (m_stConferenceParams.eNatTraversalSIP)
			{
				case estiSIPNATDisabled:
				case estiSIPNATEnabledUseTunneling:

					hResult = m_iceManager->ServersSet ({}, {}, CstiIceSession::Protocol::None);
					stiTESTRESULT ();

					break;

				case estiSIPNATEnabledMediaRelayDisabled:

					hResult = m_iceManager->ServersSet (
						m_stConferenceParams.TurnServer,		// Use the TURN servers as STUN servers
						m_stConferenceParams.TurnServerAlt,
						CstiIceSession::Protocol::Stun
						);
					stiTESTRESULT ();

					break;

				case estiSIPNATEnabledMediaRelayAllowed:

					hResult = m_iceManager->ServersSet (
						m_stConferenceParams.TurnServer,
						m_stConferenceParams.TurnServerAlt,
						CstiIceSession::Protocol::Turn
						);
					stiTESTRESULT ();

					break;
			}
		}
	}

STI_BAIL:
	return;
}


void CstiSipManager::LoggedInSet (
	bool bLoggedIn)
{
	Lock ();
	m_bLoggedIn = bLoggedIn;
	Unlock ();
}

///\brief Applies various nat settings based on the conference parameters.
///
///\param pstConferenceParams ConferenceParams to apply
///
stiHResult CstiSipManager::ConferenceParamsSet (
	const SstiConferenceParams *pstConferenceParams)
{
	Lock ();
	
	// Save all the conference params
	CstiProtocolManager::ConferenceParamsSet (pstConferenceParams);

	if (eventLoopIsRunning ())
	{
		PostEvent (
		    std::bind(&CstiSipManager::eventNatTraversalConfigure, this));
	}
	
	Unlock ();

	return stiRESULT_SUCCESS;
}


/*!\brief Post an event to re-register
 *
 */
stiHResult CstiSipManager::ClientReregister ()
{
	PostEvent (
	    std::bind(&CstiSipManager::eventClientReregister, this));

	return stiRESULT_SUCCESS;
}


/*!\brief Event handler for th client re-register event.
 *
 */
void CstiSipManager::eventClientReregister ()
{
	ProxyRegistrationLookupRestart ();
}


stiHResult CstiSipManager::RegistrationStatusCheck (int nStackRestartDelta)
{
	PostEvent (
	    std::bind(&CstiSipManager::eventRegistrationStatusCheck, this, nStackRestartDelta));

	return stiRESULT_SUCCESS;
}

// Checks the current time to the timeout on the registration for each number.
// If the time elapsed is greater than some delta then we are not registered so restart the sip stack.
void CstiSipManager::eventRegistrationStatusCheck (
	int stackRestartDelta)
{
	std::stringstream SipStats;
	unsigned int unSecondsInMinute = 60;
	
	if (UseInboundProxy () && m_bLoggedIn)
	{
		uint32_t un32CurrentTime = stiOSTickGet ();
		
		for (auto &registration: m_Registrations)
		{
			uint32_t un32RegistrationExpireTime = m_un32StackStartupTime;
			
			if (registration.second)
			{
				un32RegistrationExpireTime = registration.second->RegistrationExpireTimeGet ();
			}
			
			if (un32RegistrationExpireTime && (un32CurrentTime > un32RegistrationExpireTime))
			{
				if ((un32CurrentTime - un32RegistrationExpireTime) > (unsigned int)stackRestartDelta * unSecondsInMinute * stiOSSysClkRateGet ())
				{
					SipStats << "EventType=SipStackRestart";
					SipStats << " PhoneNumber=" << registration.second->PhoneNumberGet ();
					SipStats << " CurrentOperation=" << registration.second->CurrentOperationGet ();
					SipStats << " Transport=" << registration.second->TransportGet () << "\n";
					
					SstiRvSipStats stRvSipStats {};
					RvStatsGet (&stRvSipStats);
					
					SipStats << stRvSipStats;
					std::string EventMessage = SipStats.str ();
					stiRemoteLogEventSend (&EventMessage);
					
					StackRestart ();
					
					break;
				}
			}
		}
	}
}


/*!\brief Cause a DNS lookup and the a re-register with the proxy.
 *
 */
void CstiSipManager::ProxyRegistrationLookupRestart ()
{
	//
	// Delete the current resolver, if any, to force the DNS proxy lookup process
	// to start from the beginning.
	//
	ProxyDomainResolverDelete ();

	ProxyRegistrarLookup ();
}


/*!\brief Starts the timer to kick off the registration process.
 *
 */
void CstiSipManager::ProxyRegistrarLookupTimerStart ()
{
	int nWindow = m_stConferenceParams.SIPRegistrationInfo.nRestartProxyMaxLookupTime -
			m_stConferenceParams.SIPRegistrationInfo.nRestartProxyMinLookupTime;
	int nRetryTime = 0;
	if (nWindow != 0)
	{
		srand (stiOSTickGet());
		nRetryTime = rand () % nWindow;
	}

	auto timeoutMs =
		(m_stConferenceParams.SIPRegistrationInfo.nRestartProxyMinLookupTime + nRetryTime) * 1000;

	m_restartProxyRegistrarLookupTimer.TimeoutSet (timeoutMs);
	m_restartProxyRegistrarLookupTimer.Start ();
}

/*!\brief Gets the proxy domain based on interface mode
 *
 */
std::string CstiSipManager::ProxyDomainGet ()
{
	return IsLocalAgentMode () ?
		m_stConferenceParams.SIPRegistrationInfo.AgentDomain :
		m_stConferenceParams.SIPRegistrationInfo.PrivateDomain;
}

/*!\brief Get the proxy alternate domain based on interface mode
 *
 */
std::string CstiSipManager::ProxyDomainAltGet ()
{
	return IsLocalAgentMode () ?
		m_stConferenceParams.SIPRegistrationInfo.AgentDomainAlt :
		m_stConferenceParams.SIPRegistrationInfo.PrivateDomainAlt;
}


/*!\brief Starts the resolution process for the private domain
 *
 */
stiHResult CstiSipManager::ProxyDomainResolve ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCONDMSG(m_proxyDomainResolver.get (),
		stiRESULT_ERROR, "Proxy resolver not allocated");

	hResult = m_proxyDomainResolver->Resolve ();

	if (stiIS_ERROR (hResult))
	{
		TransportConnectionCheck (false);
		//
		// An error occured.  Start a timer to try again.
		//
		m_proxyDomainResolver->m_DnsRetryTimer.Start ();
	}
	else
	{
		//
		// Set a timer that fires periodically to force complete DNS lookup and registrations.
		//
		ProxyRegistrarLookupTimerStart ();
	}

STI_BAIL:
	return hResult;
}


/*!\brief Deletes or cancels the private domain resolver.
 *
 */
void CstiSipManager::ProxyDomainResolverDelete ()
{
	//
	// If the resolver is not currently resolving then we can go ahead and delete it.
	// Otherwise, we will have to mark it as canceled and the resolver callback
	// will delete it.
	//
	if (m_proxyDomainResolver)
	{
		if (m_proxyDomainResolver->m_bResolving)
		{
			m_proxyDomainResolver->m_bCanceled = true;
		}

		m_proxyDomainResolver = nullptr;
	}
}

/*!\brief Post an event to re-register
 * \param bActiveCalls - So SipManager knows if there are active calls before re-registering.
 */
stiHResult CstiSipManager::ReRegisterNeeded (bool bActiveCalls)
{
	PostEvent (
	    std::bind(&CstiSipManager::eventReRegisterNeeded, this, bActiveCalls));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Handle the Re-register event
 * \param bActiveCalls - indicates if there are any active calls
 */
void CstiSipManager::eventReRegisterNeeded (
	bool bActiveCalls)
{
	// If there are active calls, just restart the timer again
	if (bActiveCalls)
	{
		ProxyRegistrarLookupTimerStart ();
	}
	else
	{
		//
		// Delete the current resolver, if any, to force the DNS proxy lookup process
		// to start from the beginning.
		//
		ProxyRegistrationLookupRestart ();
	}
}


/*!\brief Performs a DNS lookup on the proxy and registrar.
 * 
 * NOTE: this is pretty specific to the private domain.
 * TODO: take resolver Type as an argument?
 * It's called all over the place...
 */
stiHResult CstiSipManager::ProxyRegistrarLookup ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	//
	// Reset all of the registration objects, if any.
	//
	for (auto &registration: m_Registrations)
	{
		if (registration.second)
		{
			registration.second->Reset ();
		}
	}

	//
	// If we have a TCP connection destroy it
	//
	TCPConnectionDestroy ();

	//
	// Clear any previously resolved proxy address information.
	//
	m_proxyDomain.clear ();
	m_proxyDomainIpAddress.clear ();
	m_proxyDomainPort = 0;

	// If registering with the proxy, or if we are an unregistered agent
	if (UseInboundProxy () || IsLocalAgentMode ())
	{
		// Get the correct domain depending on the mode
		std::string proxyDomain = ProxyDomainGet ();
		std::string proxyDomainAlt = ProxyDomainAltGet ();

		//
		// If the private domain is not empty then go ahead and attempt resolving.
		// If it is empty then this method will get called when it gets set.
		//
		if (!proxyDomain.empty ())
		{
			//
			// If we don't currently have a resolver or if the domains and/or transport has
			// changed then create a resolver.
			//
			if (m_proxyDomainResolver == nullptr
			 || m_proxyDomainResolver->m_PrimaryProxyAddress != proxyDomain
			 || m_proxyDomainResolver->m_AltProxyAddress != proxyDomainAlt
			 || (m_proxyDomainResolver->m_eResolvedTransport & m_stConferenceParams.nAllowedProxyTransports) == 0)
			{
				//
				// Delete the current resolver, if there is one.
				//
				ProxyDomainResolverDelete ();

				//
				// Start the resolution process. We will try the primary domain first, then if that fails we'll do the alternate
				//
				if (m_hResolverMgr == NULL)
				{
					hResult = stiMAKE_ERROR(stiRESULT_MEMORY_ALLOC_ERROR);
				}
				else
				{
					m_proxyDomainResolver = ResolverAllocate(CstiSipResolver::eProxy);
					
					ProxyDomainResolve ();
				}
			}
			else
			{
				hResult = m_proxyDomainResolver->Continue ();
			}
		}
	}

	if (stiIS_ERROR (hResult))
	{
		TransportConnectionCheck (true);
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::Startup
//
// Description: Start the task.
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiSipManager::Startup ()
{
	stiLOG_ENTRY_NAME (CstiSipManager::Startup);

	StartEventLoop ();

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Synchronously shuts down the SIP Manager
 */
stiHResult CstiSipManager::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiSipManager::Shutdown);

	PostEvent (
		[this]
		{
			m_bShutdown = estiTRUE;
			RvSipStackStopEventProcessingExt (m_hStackMgr);
		});

	StopEventLoop ();

	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegistrarSettingsDetermine
//
// Description: Determines which settings will be needed to register with
// 	the current registrar.
//
// Abstract:
//
// Returns:
//  hResult
//
stiHResult CstiSipManager::RegistrarSettingsDetermine (
	const char *pszPhoneNumber, ///< The phone number whose settings we shall return (if NULL, function exits)
	CstiSipRegistration::SstiRegistrarSettings *pRegSettings) ///< OUT A struct to contain all the settings
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// There is no "ported mode" in use, so try to use the registrar settings returned from core <RegistrationInfoGet>
	stiTESTCOND_NOLOG (!m_proxyDomainIpAddress.empty (), stiRESULT_ERROR);

	// If pszPhoneNumber is null, exit function
	stiTESTCOND_NOLOG (pszPhoneNumber, stiRESULT_ERROR);

	{
		auto it = m_stConferenceParams.SIPRegistrationInfo.PasswordMap.find (pszPhoneNumber);
		if (it == m_stConferenceParams.SIPRegistrationInfo.PasswordMap.end ())
		{
			DBG_MSG ("The number %s was not found in the RegistrationInfo", pszPhoneNumber);
			stiTESTCOND_NOLOG (false, stiRESULT_ERROR);
		}

		// Return the settings for the default/standard registrar
		pRegSettings->eProtocol = estiSIP;
		pRegSettings->User = it->first;
		pRegSettings->Password = it->second;

		pRegSettings->ProxyAddress = m_proxyDomain;
		pRegSettings->ProxyIpAddress = m_proxyDomainIpAddress;
		pRegSettings->un16ProxyPort = m_proxyDomainPort;

		pRegSettings->eTransport = m_eLastNATTransport;

		if (pRegSettings->eTransport == estiTransportTLS)
		{
			pRegSettings->un16LocalSipPort = m_stConferencePorts.nTLSListenPort;
		}
		else
		{
			pRegSettings->un16LocalSipPort = LocalListenPortGet ();
		}
		
		if (strcmp (pszPhoneNumber, UserPhoneNumbersGet ()->szRingGroupLocalPhoneNumber) == 0
		 || strcmp (pszPhoneNumber, UserPhoneNumbersGet ()->szRingGroupTollFreePhoneNumber) == 0)
		{
			// Multiring-specific registrar settings
			pRegSettings->nMaxContacts = 1;
		}
		else
		{
			// Regular registrar
			pRegSettings->nMaxContacts = 999;
		}
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegistrationsDelete
//
// Description: Delete all sip registrations
//
// Abstract:
//
// Returns:
//  stiHResult
//
void CstiSipManager::RegistrationsDelete ()
{
	Lock ();
	
	for (auto &registration: m_Registrations)
	{
		if (registration.second)
		{
			delete registration.second;
			registration.second = nullptr;
		}
	}
	m_Registrations.clear ();
	
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegistrationsCreate
//
// Description: Create all new sip registrations
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult CstiSipManager::RegistrationsCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	const SstiUserPhoneNumbers *psUserPhoneNumbers = UserPhoneNumbersGet ();
	
	if (psUserPhoneNumbers->szLocalPhoneNumber[0])
	{

		hResult = RegistrationCreate (psUserPhoneNumbers->szLocalPhoneNumber, REGID_LOCAL_PHONE_NUMBER);
		stiTESTRESULT ();
	}

	if (psUserPhoneNumbers->szTollFreePhoneNumber[0])
	{
		hResult = RegistrationCreate (psUserPhoneNumbers->szTollFreePhoneNumber, REGID_TOLL_FREE_PHONE_NUMBER);
		stiTESTRESULT ();
	}

	if (psUserPhoneNumbers->szRingGroupLocalPhoneNumber[0])
	{
		hResult = RegistrationCreate (psUserPhoneNumbers->szRingGroupLocalPhoneNumber, REGID_RING_GROUP_LOCAL_PHONE_NUMBER);
		stiTESTRESULT ();
	}

	if (psUserPhoneNumbers->szRingGroupTollFreePhoneNumber[0])
	{
		hResult = RegistrationCreate (psUserPhoneNumbers->szRingGroupTollFreePhoneNumber, REGID_RING_GROUP_TOLL_FREE_PHONE_NUMBER);
		stiTESTRESULT ();
	}

	if (psUserPhoneNumbers->szSorensonPhoneNumber[0])
	{
		hResult = RegistrationCreate (psUserPhoneNumbers->szSorensonPhoneNumber, REGID_SORENSON_PHONE_NUMBER);
		stiTESTRESULT ();
	}
	
STI_BAIL:
	return hResult;
}

	
////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegistrationCreate
//
// Description: Create a sip registration for the given phone number
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult CstiSipManager::RegistrationCreate (
	const char *pszNumberToRegister,
	int nRegistrationID)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipRegistration *pRegistration = nullptr;
	CstiSipRegistration::SstiRegistrarSettings stiRegistrarSettings;
	
	auto it = m_Registrations.find (pszNumberToRegister);
	
	//
	// The address data member will be empty if there isn't registration info for the number to register.
	//
	hResult = RegistrarSettingsDetermine (pszNumberToRegister, &stiRegistrarSettings);

	if (!stiIS_ERROR (hResult))
	{
		if (it == m_Registrations.end ())
		{
			pRegistration = new CstiSipRegistration (pszNumberToRegister, m_stConferenceParams.SipInstanceGUID, nRegistrationID, m_hAppPool, m_hRegClientMgr, m_hStackMgr, 
													 m_execMutex, CstiSipManager::RegistrationCB, this);
			stiTESTCOND (pRegistration, stiRESULT_MEMORY_ALLOC_ERROR);
			
			pRegistration->RegistrarSettingsSet (&stiRegistrarSettings);

			m_Registrations[pszNumberToRegister] = pRegistration;
		}
		else
		{
			it->second->RegistrarSettingsSet(&stiRegistrarSettings);
		}
	}
	else
	{
		hResult = stiRESULT_SUCCESS;
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ConnectionKeepaliveStateChangeCB
//
// Description: Handles an event from a TCP keepalive
//
// Returns:
//	nothing
void RVCALLCONV CstiSipManager::ConnectionKeepaliveStateChangeCB (
	RvSipTransmitterHandle hTrx,
	RvSipAppTransmitterHandle hAppTrx,
	RvSipTransmitterState eState,
	RvSipTransmitterReason eReason,
	RvSipMsgHandle hMsg,
	void* pExtraInfo
)
{
	//pThis->Lock (); <-- uncomment if this method ever accesses the sip manager

	switch (eState)
	{
		// Not interested in handling these at this time
		case RVSIP_TRANSMITTER_STATE_UNDEFINED:
		case RVSIP_TRANSMITTER_STATE_IDLE:
		case RVSIP_TRANSMITTER_STATE_RESOLVING_ADDR:
		case RVSIP_TRANSMITTER_STATE_READY_FOR_SENDING:
		case RVSIP_TRANSMITTER_STATE_ON_HOLD:
		case RVSIP_TRANSMITTER_STATE_FINAL_DEST_RESOLVED:
			
			break;
			
		case RVSIP_TRANSMITTER_STATE_MSG_SENT:
			
			RvSipTransmitterTerminate (hTrx);
			
			break;
			
		case RVSIP_TRANSMITTER_STATE_NEW_CONN_IN_USE:
			DBG_MSG ("sip keepalive - connection in use");
			break;
			
		case RVSIP_TRANSMITTER_STATE_TERMINATED:

			if (hAppTrx)
			{
				auto pTransmitterInfo = (SstiTransmitterInfo *)hAppTrx;

				if (pTransmitterInfo->sipCall)
				{
					//
					// If this transmitter is the one we are waiting to terminate then
					// set the data member to NULL to inidate that it has terminated.  If
					// this isn't done then we can get a subsequent request to terminate the
					// transmitter that doesn't exist after this point.
					//
					if (pTransmitterInfo->sipCall->m_hKeepAliveTransmitter == hTrx)
					{
						pTransmitterInfo->sipCall->m_hKeepAliveTransmitter = nullptr;
					}
				}
				else
				{
					//
					// If this transmitter is the one we are waiting to terminate then
					// set the data member to NULL to inidate that it has terminated.  If
					// this isn't done then we can get a subsequent request to terminate the
					// transmitter that doesn't exist after this point.
					//
					if (pTransmitterInfo->pSipManager->m_hKeepAliveTransmitter == hTrx)
					{
						pTransmitterInfo->pSipManager->m_hKeepAliveTransmitter = nullptr;
					}
				}

				delete pTransmitterInfo;
				pTransmitterInfo = nullptr;
			}
			
			break;
			
		case RVSIP_TRANSMITTER_STATE_MSG_SEND_FAILURE:
			
			switch (eReason)
			{
				case RVSIP_TRANSMITTER_REASON_UNDEFINED:
				case RVSIP_TRANSMITTER_REASON_NETWORK_ERROR:
				case RVSIP_TRANSMITTER_REASON_CONNECTION_ERROR:
				case RVSIP_TRANSMITTER_REASON_OUT_OF_RESOURCES:
					DBG_MSG ("sip keepalive error %d, State = %d", eReason, eState);
					if (hAppTrx)
					{
						auto pTransmitterInfo = (SstiTransmitterInfo *)hAppTrx;

						if (pTransmitterInfo->sipCall)
						{
							pTransmitterInfo->sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
						}
						else
						{
							pTransmitterInfo->pSipManager->ProxyRegistrarLookup ();
						}
					}
					break;
				case RVSIP_TRANSMITTER_REASON_NEW_CONN_CREATED:
				case RVSIP_TRANSMITTER_REASON_CONN_FOUND_IN_HASH:
				case RVSIP_TRANSMITTER_REASON_USER_COMMAND:
					DBG_MSG ("sip keepalive success");
					break;
				// default: commented out for enum safety
			}
			
			RvSipTransmitterTerminate (hTrx);
			
			break;
		// default: commented out for enum safety
	}

	//pThis->Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ConnectionKeepalive
//
// Description: Post a connection keepalive event
//
// Returns:
//	hResult
stiHResult CstiSipManager::ConnectionKeepalive ()
{
	PostEvent (
	    std::bind(&CstiSipManager::eventConnectionKeepalive, this));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Event handler for a connection keepalive timeout
 * \param pSipCall
 */
void CstiSipManager::eventConnectionKeepalive ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	RvSipTransmitterHandle hTransmitter = nullptr;
	RvSipTransmitterMgrHandle hTransmitterMgr;
	RvSipTransmitterEvHandlers TransmitterHandlers;
	RvSipTransportAddr DestAddr;
	RvSipTransportConnectionHandle hConnection = nullptr;
	SstiTransmitterInfo *pTransmitterInfo = nullptr;

	//
	// If we get here and the handle is not NULL then the transmitter must
	// have not completed in time.  We must terminate it at this point in order to
	// free resources that are attached to the transmitter.
	//
	if (m_hKeepAliveTransmitter)
	{
		RvSipTransmitterTerminate (m_hKeepAliveTransmitter);
		m_hKeepAliveTransmitter = nullptr;
	}

	if (UseInboundProxy () && m_useProxyKeepAlive)
	{
		hConnection = m_hPersistentProxyConnection;
	}
	
	if (hConnection)
	{
		// Create a new transmitter based on hConnection
		memset (&DestAddr, 0, sizeof (RvSipTransportAddr));
		rvStatus = RvSipTransportConnectionGetTransportType (hConnection, &DestAddr.eTransportType);
		if (rvStatus == RV_ERROR_INVALID_HANDLE) // Connection has been lost
		{
			ProxyRegistrarLookup();
		}
		else
		{
			stiTESTRVSTATUS ();
			
			rvStatus = RvSipTransportConnectionGetRemoteAddress (hConnection, DestAddr.strIP, &DestAddr.port, &DestAddr.eAddrType);
			stiTESTRVSTATUS ();
			
			rvStatus = RvSipStackGetTransmitterMgrHandle (m_hStackMgr, &hTransmitterMgr);
			stiTESTRVSTATUS ();
			
			memset (&TransmitterHandlers, 0, sizeof (TransmitterHandlers));
			TransmitterHandlers.pfnStateChangedEvHandler = CstiSipManager::ConnectionKeepaliveStateChangeCB;
			
			pTransmitterInfo = new SstiTransmitterInfo;

			pTransmitterInfo->sipCall = nullptr;
			pTransmitterInfo->pSipManager = this;

			rvStatus = RvSipTransmitterMgrCreateTransmitter (hTransmitterMgr, (RvSipAppTransmitterHandle)pTransmitterInfo, &TransmitterHandlers, sizeof (TransmitterHandlers), &hTransmitter);
			stiTESTRVSTATUS ();
			//
			// Transmitter successfully created.  This structure will be deleted when the transmitter terminates.
			//
			pTransmitterInfo = nullptr;

			rvStatus = RvSipTransmitterSetPersistency (hTransmitter, RV_TRUE);
			stiTESTRVSTATUS ();
			
			rvStatus = RvSipTransmitterSetConnection (hTransmitter, hConnection);
			stiTESTRVSTATUS ();
			
			rvStatus = RvSipTransmitterSetDestAddress (hTransmitter, &DestAddr, sizeof (DestAddr), nullptr, 0);
			stiTESTRVSTATUS ();
			
			// Send the keepalive
			rvStatus = RvSipTransmitterSendBuffer (hTransmitter, (RvChar*)"\r\n\r\n", 4);
			stiTESTRVSTATUS ();

			//
			// Assign the transmitter to the appropriate object (SIP Manager or SIP Call)
			//
			m_hKeepAliveTransmitter = hTransmitter;
			hTransmitter = nullptr;
		}
	}
	else
	{
		DBG_MSG ("No connection to keep alive!");
	}
	
STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (hTransmitter)
		{
			RvSipTransmitterTerminate (hTransmitter);
			hTransmitter = nullptr;
		}
	}
	
	if (pTransmitterInfo)
	{
		//
		// We have a pointer to a transmitter info object which means we failed to create the transmitter.
		// We need to make sure it is cleaned up before leaving this method.
		//

		delete pTransmitterInfo;
		pTransmitterInfo = nullptr;
	}

	//
	// Always restart the timer, as long as there is a connection, even on error.
	//
	if (hConnection)
	{
		// Restart the timer
		ProxyConnectionKeepaliveStart();
		
		// Assert we are not seeing long delays between keep alive attempts.
		if (m_useProxyKeepAlive)
		{
			auto now = std::chrono::steady_clock::now ();
			
			if (m_lastKeepAliveTime.time_since_epoch().count())
			{
				m_timeBetweenKeepAlive = std::chrono::duration_cast<std::chrono::seconds> (now - m_lastKeepAliveTime);
				
				stiASSERTMSG(m_timeBetweenKeepAlive.count() < (DEFAULT_PROXY_KEEPALIVE_TIMEOUT * 10), "Connection KeepAlive was delayed = %d.", m_timeBetweenKeepAlive.count());
			}
			
			m_lastKeepAliveTime = now;
		}
	}
	else if (IsRegistered ()) // Bug# 32095 If we are registered and don't have a connection we should try to re-register.
	{
		stiRemoteLogEventSend ("EventType=ConnectionKeepAlive Reason=Forcing Registration");
		ReRegisterNeeded (m_pCallStorage->activeCallCountGet() > 0);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ProxyConnectionKeepaliveStart
//
// Description: Sends out a TCP keepalive to the current proxy (if any)
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult CstiSipManager::ProxyConnectionKeepaliveStart ()
{
	if (m_useProxyKeepAlive)
	{
		m_proxyConnectionKeepaliveTimer.Start ();
	}
	
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegistrationCB
//
// Description: Called when a registration event occourrs.
//
// Abstract:
//
// Returns:
//  stiHResult
//
stiHResult CstiSipManager::RegistrationCB (
	void *pUserData,
	CstiSipRegistration *pRegistration,
	EstiRegistrationEvent eEvent,
	size_t EventParam)
{
	auto pThis = (CstiSipManager*)pUserData;
	std::string phoneNumber = std::string(pRegistration->PhoneNumberGet());
	
	pThis->PostEvent (
	    std::bind(&CstiSipManager::eventRegistrationCallback, pThis,
			phoneNumber, eEvent, EventParam));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Called when a registration event occurs
 */
void CstiSipManager::eventRegistrationCallback (
	std::string phoneNumber,
	EstiRegistrationEvent registrationEvent,
	size_t eventParam)
{
	CstiSipRegistration *pRegistration = nullptr;

	//
	// Look up the registration object by phone number to see if it still
	// exists.  If it does then process the registration event.
	//
	auto it = m_Registrations.find (phoneNumber);

	if (it != m_Registrations.end () && it->second)
	{
		pRegistration = it->second;
	}

	if (pRegistration)
	{
		// This event keeps the connection alive as well as anything, so reset the timer
		if (UseInboundProxy ())
		{
			ProxyConnectionKeepaliveStart ();
		}

		// Handle the event
		switch (registrationEvent)
		{
			case REG_EVENT_REGISTERING:
				sipRegistrationRegisteringSignal.Emit ();
				break;

			case REG_EVENT_REGISTERED:
			{
				const CstiSipRegistration::SstiRegistrarSettings *pCurrentSettings = pRegistration->RegistrarSettingsGet ();
				
				stiASSERTMSG(pCurrentSettings, "RegistrarSettingsGet returned NULL");

				m_nConnectRetryAttemptCount = 0;

				sipRegistrationConfirmedSignal.Emit (pCurrentSettings->ProxyIpAddress);
				std::string ReflexiveAddress = "0.0.0.0";
				uint16_t un16Port = 0;

				// Return the reflexive address to be used as our public ip if we are not tunneling
#ifdef stiTUNNEL_MANAGER
				if (RvTunGetPolicy() != RvTun_ENABLED)
#endif
				{
					std::string ipAddress;
					pRegistration->ReflexiveAddressGet (&ipAddress, &un16Port);
					ReflexiveAddress = ipAddress;

					if (ipAddress != m_ReflexiveAddress)
					{
						publicIpResolvedSignal.Emit (ipAddress);
					}
				}

#ifdef stiTUNNEL_MANAGER

				else if (!m_TunneledIpAddr.empty ())
				{
					ReflexiveAddress = m_TunneledIpAddr;
				}

#endif

				stiRemoteLogEventSend ("EventType = Registered  Number = %s Server = %s:%u ReflexiveIP = %s:%u PhoneNumberCount = %d", phoneNumber.c_str (), pCurrentSettings->ProxyIpAddress.c_str (), pCurrentSettings->un16ProxyPort, ReflexiveAddress.c_str (), un16Port, m_Registrations.size ());
				
				break;
			}
			
			case REG_EVENT_FAILED:
			{
				const CstiSipRegistration::SstiRegistrarSettings *pCurrentSettings = pRegistration->RegistrarSettingsGet ();

				stiASSERTMSG (estiFALSE, "Registration failed. %s@%s:%d", pRegistration->PhoneNumberGet (), pCurrentSettings->ProxyIpAddress.c_str (), pCurrentSettings->un16ProxyPort);
				
				RegisterFailureRetry (pRegistration->PhoneNumberGet ());

				break;
			}

			case REG_EVENT_CONNECTION_LOST:
			{
				const CstiSipRegistration::SstiRegistrarSettings *pCurrentSettings = pRegistration->RegistrarSettingsGet ();

				stiASSERTMSG (estiFALSE, "Connection lost. %s@%s:%d", pRegistration->PhoneNumberGet (), pCurrentSettings->ProxyIpAddress.c_str (), pCurrentSettings->un16ProxyPort);
				
				CstiSipRegistration::SstiRegistrarSettings CorrectSettings;
				RegistrarSettingsDetermine (pRegistration->PhoneNumberGet (), &CorrectSettings);

				if (pCurrentSettings->un16ProxyPort == CorrectSettings.un16ProxyPort &&
					pCurrentSettings->ProxyIpAddress == CorrectSettings.ProxyIpAddress)
				{
					//
					// If the resolver's DNS list is not empty then immediately attempt to use
					// the next entry in the list. Otherwise, wait for a period of time
					// before attempting to start the resolution process over.
					//
					if (m_proxyDomainResolver && m_proxyDomainResolver->DNSListEmpty ())
					{
						ProxyRegistrarLookup ();
					}
					else
					{
						// That's our address that was broken, so redo the DNS lookup

						// Add random exponential with full jitter to prevent DOS of SIP server
						// http://www.awsarchitectureblog.com/2015/03/backoff.html

						auto  nRetryTime = static_cast<int>(RESTART_PROXY_REGISTRAR_LOOKUP_TIMEOUT * pow (2, m_nConnectRetryAttemptCount));
						int nMaxRetry = std::min (MAX_TIME_TO_WAIT_FOR_RECONNECT, nRetryTime);
						srand (stiOSTickGet());
						int nRetryTimeWithJitter = rand () % nMaxRetry;
						m_nConnectRetryAttemptCount++;
						DBG_MSG("Failed to register. Attempt: %d - Waiting %3.2F seconds (max %d)", m_nConnectRetryAttemptCount, nRetryTimeWithJitter / 1000.0F, nMaxRetry / 1000);

						m_proxyRegisterRetryTimer.TimeoutSet (nRetryTimeWithJitter);
						m_proxyRegisterRetryTimer.Start ();
					}
				}
				else if (!CorrectSettings.ProxyIpAddress.empty ())	// If we have a resolved private domain...
				{
					// That's not the correct address, therefore set it
					pRegistration->RegistrarSettingsSet (&CorrectSettings);

					// register is required to correct the problem
					Register (pRegistration->PhoneNumberGet ());
				}

				break;
			}
			
			case REG_EVENT_REREGISTER_NEEDED:
			{
				Register (pRegistration->PhoneNumberGet ());

				break;
			}

			case REG_EVENT_BAD_CREDENTIALS:
				stiASSERTMSG (estiFALSE, "Registration -- bad credentials %s", pRegistration->PhoneNumberGet ());
				RegisterFailureRetry (pRegistration->PhoneNumberGet ());
				break;

			case REG_EVENT_TIME_SET:
				break;
				
			case REG_EVENT_OUT_OF_RESOURCES:
			{
				std::string eventMessage = "EventType=SipStackRestart Reason=SipRegistrationOutOfResources";
				stiRemoteLogEventSend (&eventMessage);
				StackRestart ();
				break;
			}
			//default:
		}
	}
}


/*!
 * \brief Handles a timeout while attempting to connect
 */
void CstiSipManager::eventProxyWatchdog ()
{
	// Redo the DNS lookup to find a better ip address
	ProxyRegistrarLookup ();
}


/*!
 * \brief Event handler to Register with a Sip registrar
 * \param phoneNumber
 */
void CstiSipManager::eventRegister (
	std::string phoneNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	const char *pszNumberToRegister = phoneNumber.c_str();
	stiTESTCOND_NOLOG (m_hStackMgr, stiRESULT_ERROR);
	
	//
	// Only register if we are doing NAT Traversal, we are doing multi-ring or we are ported.
	//
	if (UseInboundProxy ())
	{
		if (phoneNumber.length() > 0)
		{
			//
			// Only allow the registration to take place if the phone number is in the list of
			// phone numbers.
			//
			int nRegistrationID = REGID_NONE;
			
			if (strcmp (pszNumberToRegister, m_UserPhoneNumbers.szLocalPhoneNumber) == 0)
			{
				nRegistrationID = REGID_LOCAL_PHONE_NUMBER;
			}
			else if (strcmp (pszNumberToRegister, m_UserPhoneNumbers.szTollFreePhoneNumber) == 0)
			{
				nRegistrationID = REGID_TOLL_FREE_PHONE_NUMBER;
			}
			else if (strcmp (pszNumberToRegister, m_UserPhoneNumbers.szRingGroupLocalPhoneNumber) == 0)
			{
				nRegistrationID = REGID_RING_GROUP_LOCAL_PHONE_NUMBER;
			}
			else if (strcmp (pszNumberToRegister, m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber) == 0)
			{
				nRegistrationID = REGID_RING_GROUP_TOLL_FREE_PHONE_NUMBER;
			}
			else if (strcmp (pszNumberToRegister, m_UserPhoneNumbers.szSorensonPhoneNumber) == 0)
			{
				nRegistrationID = REGID_SORENSON_PHONE_NUMBER;
			}
				
			if (nRegistrationID != REGID_NONE)
			{
				//
				// Make sure registration object exists
				//
				RegistrationCreate (pszNumberToRegister, nRegistrationID);
				
				auto it = m_Registrations.find (pszNumberToRegister);

				if (it != m_Registrations.end () && it->second)
				{
					//
					// If we don't havea  persisitent TCP connection created yet then create one.
					//
					if (!m_hPersistentProxyConnection)
					{
						TCPConnectionCreate ();
					}

					hResult = it->second->RegisterStart ();
					stiTESTRESULT ();
				}
			}
		}
		else
		{
			//
			// Make sure all registration objects exist.
			//
			RegistrationsCreate();
			
			// Go through each registration and have them re-register
			for (auto &registration: m_Registrations)
			{
				if (registration.second)
				{
					//
					// If we don't have a persisitent TCP connection created yet then create one.
					//
					if (!m_hPersistentProxyConnection)
					{
						TCPConnectionCreate ();
					}

					hResult = registration.second->RegisterStart ();
					stiTESTRESULT ();
				}
			}
		}
	}
	else
	{
		//
		// We are not registering so make sure our registration client is no longer
		// and any associated timers have been killed.
		//
		if (phoneNumber.length() > 0)
		{
			auto it = m_Registrations.find (pszNumberToRegister);
			if (it != m_Registrations.end () && it->second)
			{
				hResult = it->second->RegisterStop ();
				stiTESTRESULT ();
			}
		}
		else
		{
			// Go through each registration and have them re-register
			for (auto &registration: m_Registrations)
			{
				hResult = registration.second->RegisterStop ();
				stiTESTRESULT ();
			}
		}
	}

STI_BAIL:
	
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ReinviteSend
//
// Description: Send a re-invite at our earliest opportunity
//
// Returns:
//  estiOK on success
//
void CstiSipManager::ReInviteSend (
    CstiCallSP call,
	int currentRetryCount,
	bool lostConnectionDetection)
{
	if (call)
	{
		PostEvent (
			[call, currentRetryCount, lostConnectionDetection]
			{
				auto sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
				if (!sipCall->m_bGatewayCall)
				{
					sipCall->ReInviteSend(lostConnectionDetection, currentRetryCount);
				}
			});
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RegisterFailureRetry
//
// Description: Re-attempt a failed registration
//
// Returns:
//  estiOK on success
//
stiHResult CstiSipManager::RegisterFailureRetry (
	const char *pszPhoneNumber) ///< The phone number to (re)register can be NULL to unregister everything
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string phoneNumber;
	if (pszPhoneNumber && *pszPhoneNumber)
	{
		phoneNumber = pszPhoneNumber;
	}

	PostEvent (
	    std::bind(&CstiSipManager::eventRegisterFailureRetry, this, phoneNumber));

	return (hResult);
}


/*!
 * \brief Re-attempt a failed registration
 * \param phoneNumber
 */
void CstiSipManager::eventRegisterFailureRetry (
	std::string phoneNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiTESTCOND_NOLOG (m_hStackMgr, stiRESULT_ERROR);
	
	//
	// Only register if we are doing NAT Traversal or we are ported.
	//
	if (UseInboundProxy ())
	{
		if (phoneNumber.length() > 0)
		{
			auto it = m_Registrations.find (phoneNumber);
			if (it != m_Registrations.end ())
			{
				stiTESTCOND (it->second, stiRESULT_ERROR);
				it->second->RegisterFailureRetry ();
			}
		}
		else
		{
			// Go through each registration and have them re-register
			for (auto &registration: m_Registrations)
			{
				if (registration.second)
				{
					registration.second->RegisterFailureRetry ();
				}
			}
		}
	}
	else
	{
		// We are no longer registering so do not attempt to recover
	}

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::Register
//
// Description: Register with a Sip registrar
//
// Abstract: if pszPublicIpAddress is NULL, it will re-register with
// the existing address
//
// Returns:
//  estiOK on success
//
stiHResult CstiSipManager::Register (
	const char *pszPhoneNumber) ///< The phone number to (re)register can be NULL to reregister everything
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	std::string phoneNumber;
	if (pszPhoneNumber && *pszPhoneNumber)
	{
		phoneNumber = pszPhoneNumber;
	}
	
	PostEvent (
	    std::bind(&CstiSipManager::eventRegister, this, phoneNumber));

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::Unregister
//
// Description: Unregister from a Sip registrar
//
// Returns:
//  estiOK on success
//
stiHResult CstiSipManager::Unregister (
	const char *pszPhoneNumber) ///< The phone number to (re)register can be NULL to unregister everything
{
	std::string phoneNumber;
	if (pszPhoneNumber && *pszPhoneNumber)
	{
		phoneNumber = pszPhoneNumber;
	}
	
	PostEvent (
	    std::bind(&CstiSipManager::eventUnregister, this, phoneNumber));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Event handler to unregister from a Sip registrar
 * \param phoneNumber
 */
void CstiSipManager::eventUnregister (
	std::string phoneNumber)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND_NOLOG (m_hStackMgr, stiRESULT_ERROR);

	if (phoneNumber.length() > 0)
	{
		auto it = m_Registrations.find (phoneNumber);
		if (it != m_Registrations.end ())
		{
			stiTESTCOND (it->second, stiRESULT_ERROR);
			hResult = it->second->UnregisterStart ();
			stiTESTRESULT ();
			m_Registrations.erase (it);
		}
	}
	else
	{
		// Go through each registration and have them unregister
		auto it = m_Registrations.begin ();
		while (it != m_Registrations.end ())
		{
			if (it->second)
			{
				hResult = it->second->UnregisterStart ();
				stiTESTRESULT ();
				auto toErase = it;
				++it;
				m_Registrations.erase (toErase);
			}
			else
			{
				++it;
			}
		}
	}

STI_BAIL:
	
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::AddrUrlGetPortNum ()
//
// Description: Utility to extract the port from an address
//
// Abstract:
// 	This is to override AddrUrlGetPortNum(), which does not support
//		non-SIP addresses.
//	todo: Is there a SIP stack function that does this for us?
//
// Returns:
//  the port number
//
RvUint32 CstiSipManager::AddrUrlGetPortNum(
	RvSipAddressHandle hAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);

	std::string header;
	RvUint32 nPort = 0;

	hResult = sipAddrGet (hAddress, m_hAppPool, &header);
	stiTESTRESULT ();

	{
		auto pos = header.find('@');

		if (pos != std::string::npos)
		{
			pos = header.find_first_of (";>:", pos);

			if (pos != std::string::npos && header[pos] != ';' && header[pos] != '>')
			{
				nPort = atol(header.substr(pos + 1).c_str ());
			}
		}
	}

STI_BAIL:

	if (!nPort)
	{
		nPort = nDEFAULT_SIP_LISTEN_PORT;
	}

	return nPort;
} // end CstiSipManager::AddrUrlGetPortNum ()


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::AddrUrlGetHost ()
//
// Description: Utility to extract the host name from an address
//
// Abstract:
// 	This is to override AddrUrlGetHost(), which does not support
//		non-SIP addresses.
//	todo: Is there a SIP stack function that does this for us?
//
// Returns:
//  a radvision status code
//
stiHResult CstiSipManager::AddrUrlGetHost (
	RvSipAddressHandle hAddress,
	std::string *host)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
	std::string header;

	hResult = sipAddrGet (hAddress, m_hAppPool, &header);
	stiTESTRESULT ();

	{
		auto pos = header.find ('@');

		if (pos != std::string::npos)
		{
			header = header.substr (pos + 1);

			pos = header.find_first_of (";>:", pos);

			if (pos != std::string::npos)
			{
				header.resize(pos);
			}

			*host = header;
		}
	}

STI_BAIL:

	return hResult;
} // end CstiSipManager::AddrUrlGetHost ()


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::AddSinfoToMessage
//
// Description: Attaches SInfo to a given message
//
// Abstract:
//
// Returns: estiERROR on fail, estiOK on success.
//
stiHResult CstiSipManager::AddSInfoToMessage (
	RvSipMsgHandle hOutboundMsg, ///< The outbound message to attach Sinfo to
	CstiSipCallSP sipCall) ///< A call object to attach to the new message
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string systemInfo;

	rvStatus = RvSipMsgSetContentTypeHeader (hOutboundMsg, (char*)SINFO_MIME);
	stiTESTRVSTATUS ();

	hResult = SInfoGenerate (&systemInfo, sipCall);
	stiTESTRESULT ();
	
	rvStatus = RvSipMsgSetBody (hOutboundMsg, (RvChar *)systemInfo.c_str ());
	stiTESTRVSTATUS ();
	
	// Save a copy of the SInfo in the call object
	sipCall->SInfoSet (systemInfo);
	
STI_BAIL:

	return hResult;
} // end CstiSipManager::AddSinfoToMessage


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::SInfoGenerate
//
// Description: Creates a new SInfo message
//
// Abstract:
//
// Returns: estiERROR on fail, estiOK on success.
//
stiHResult CstiSipManager::SInfoGenerate (
	std::string *systemInfo, ///< The pointer to store the Sinfo message
	CstiSipCallSP sipCall) ///< A call object of the new Sinfo message
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = SystemInfoSerialize (systemInfo, sipCall,
		m_bDefaultProviderAgreementSigned, m_eLocalInterfaceMode, m_stConferenceParams.AutoDetectedPublicIp.c_str (),
		m_stConferenceParams.ePreferredVCOType,
		m_stConferenceParams.vcoNumber.c_str (), m_stConferenceParams.bVerifyAddress,
		productNameGet().c_str(),
		m_version.c_str(),
		g_nSVRSSIPVersion,
		m_stConferenceParams.bBlockCallerID,
		m_stConferenceParams.eAutoSpeedSetting);
	stiTESTRESULT ();
	
STI_BAIL:

	return hResult;
} // end CstiSipManager::SInfoGenerate


/*!\brief Prints the subscription state
 * 
 * \param eSubscriptionState The state to printing
 */
void PrintSubscriptionState (
	RvSipSubsState eSubscriptionState)
{
	std::string caseString;
	
	switch (eSubscriptionState)
	{
		GET_CASE_STRING (RVSIP_SUBS_STATE_SUBS_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_UNSUBSCRIBE_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_TERMINATING)
		GET_CASE_STRING (RVSIP_SUBS_STATE_REFRESH_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_ACTIVATED)
		GET_CASE_STRING (RVSIP_SUBS_STATE_2XX_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_UNSUBSCRIBE_2XX_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_SUBS_SENT)
		GET_CASE_STRING (RVSIP_SUBS_STATE_REDIRECTED)
		GET_CASE_STRING (RVSIP_SUBS_STATE_UNAUTHENTICATED)
		GET_CASE_STRING (RVSIP_SUBS_STATE_NOTIFY_BEFORE_2XX_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_STATE_REFRESHING)
		GET_CASE_STRING (RVSIP_SUBS_STATE_UNSUBSCRIBING)
		GET_CASE_STRING (RVSIP_SUBS_STATE_TERMINATED)
		GET_CASE_STRING (RVSIP_SUBS_STATE_UNDEFINED)
		GET_CASE_STRING (RVSIP_SUBS_STATE_IDLE)
		GET_CASE_STRING (RVSIP_SUBS_STATE_PENDING)
		GET_CASE_STRING (RVSIP_SUBS_STATE_ACTIVE)
		GET_CASE_STRING (RVSIP_SUBS_STATE_MSG_SEND_FAILURE)
	}
	
	vpe::trace ("Subscription State: ", caseString, "\n");
}


///\brief Simply assigns the application call leg object (the CstiSipCall) as the application subscription object.
///
void RVCALLCONV CstiSipManager::SubscriptionCreatedCB (
	RvSipSubsHandle    hSubs,
	RvSipCallLegHandle hCallLeg,
	RvSipAppCallLegHandle hAppCallLeg,
	RvSipAppSubsHandle *phAppSubs)
{
	*phAppSubs = (RvSipAppSubsHandle)hAppCallLeg;
}


/*!\brief Sends a notify request indicating the subscription has been terminated.
 *
 * \param hSubscription The handle to the subscription that is being terminated
 * \param hAppSubscription The handle to the application subscription object
 */
static stiHResult NotifySubscriptionTerminate (
	RvSipSubsHandle hSubscription,
	RvSipAppSubsHandle hAppSubscription)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// create the notification object
	RvSipNotifyHandle hNotify;
	RvStatus rvStatus = RV_OK;
	
	RvSipSubscriptionSubstate eSubscriptionState = RVSIP_SUBSCRIPTION_SUBSTATE_TERMINATED;
	RvSipSubscriptionReason eNotifyReason = RVSIP_SUBSCRIPTION_REASON_NORESOURCE;
	RvInt32 nExpires = 0;
	RvInt16 nNotifyResponseCode = SIPRESP_SERVICE_UNAVAILABLE;
	
	rvStatus = RvSipSubsCreateNotify (hSubscription, (RvSipAppNotifyHandle)hAppSubscription, &hNotify);
	stiTESTRVSTATUS ();
	
	rvStatus = RvSipNotifySetSubscriptionStateParams (hNotify, eSubscriptionState, eNotifyReason, nExpires);
	stiTESTRVSTATUS ();
	
	rvStatus = RvSipNotifySetReferNotifyBody (hNotify, nNotifyResponseCode);
	stiTESTRVSTATUS ();
	
	rvStatus = RvSipNotifySend (hNotify); //send the notify request
	stiTESTRVSTATUS ();
	
STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::SubscriptionStateChangeCB
//
// Description: This callback is called when we receive an incoming REFER (Transfer)
//
// Abstract: Respond to an incoming REFER request with 202, and then connect the
//	new call with the Refer-to party
//
// Returns: Nothing
//
#define URI_MAX_SIZE 128
void RVCALLCONV CstiSipManager::SubscriptionStateChangeCB (
	RvSipSubsHandle hSubscription, ///< The SIP Stack subscription handle.
	RvSipAppSubsHandle hAppSubs, ///< The application handle for this subscription.
	RvSipSubsState eState, ///< The new subscription state.
	RvSipSubsStateChangeReason eReason ///< The reason for the state change.
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLegKey = (size_t)hAppSubs;
	CstiSipCallLegSP callLeg = callLegLookup (callLegKey);
	CstiSipCallSP sipCall = nullptr;
	CstiCallSP call = nullptr;
	CstiSipManager *pThis = nullptr;

	stiTESTCOND (nullptr != callLeg, stiRESULT_ERROR);
	sipCall = callLeg->m_sipCallWeak.lock ();
	stiTESTCOND (nullptr != sipCall, stiRESULT_ERROR);
	call = sipCall->CstiCallGet ();
	stiTESTCOND (nullptr != call, stiRESULT_ERROR);
	pThis = reinterpret_cast<CstiSipManager *> (sipCall->ProtocolManagerGet ());
	stiTESTCOND (nullptr != pThis, stiRESULT_ERROR);

	stiDEBUG_TOOL (g_stiSipMgrDebug,
		PrintSubscriptionState (eState);
	);

	pThis->Lock ();

	// Get the instance pointers
	switch (eState)
	{
		//
		// The following cases are the notifier states.
		//
		case RVSIP_SUBS_STATE_SUBS_RCVD:

			if (RVSIP_SUBS_REASON_REFER_RCVD == eReason 
				|| RVSIP_SUBS_REASON_REFER_RCVD_WITH_REPLACES == eReason)
			{
				RvSipReferToHeaderHandle hReferTo;
				RvSipSubsGetReferToHeader (hSubscription, &hReferTo);
				RvSipAddressHandle hReferToAddr = RvSipReferToHeaderGetAddrSpec (hReferTo);
				RvSipMethodType eMethod = RvSipAddrUrlGetMethod (hReferToAddr);

				if (eMethod == RVSIP_METHOD_UNDEFINED ||
					eMethod == RVSIP_METHOD_INVITE)
				{
					std::string host;

					// Get the host name
					RvSipAddressHandle hToAddress = RvSipReferToHeaderGetAddrSpec (hReferTo);
					RvUint hostLen = 16;
					RvUint bufLen = 0;

					while (bufLen < hostLen)
					{
						host.resize (hostLen, '\0');
						bufLen = hostLen;
						RvSipAddrUrlGetHost (hToAddress, (RvChar *)&host[0], bufLen, &hostLen);
					}

					// Make sure we're not dialing ourselves
					std::string LocalIPv4;
					std::string LocalIPv6;

					LocalIPv4 = sipCall->localIpAddressGet ();
#ifdef IPV6_ENABLED
					stiGetLocalIp (&LocalIPv6, estiTYPE_IPV6);
#endif
					if (host.empty ())
					{
						stiASSERTMSG (estiFALSE, "Attempt to transfer to invalid destination");
						RvSipSubsRespondReject (hSubscription, SIPRESP_BAD_REQUEST, (RvChar *)"Invalid destination.");
					}
					else if (0 == strcmp (host.c_str (), "127.0.0.1") || 
						0 == strcmp (host.c_str (), "::1") || 
						LocalIPv4 == host || 
						LocalIPv6 == host || 
						pThis->m_stConferenceParams.PublicIPv4 == host || 
						pThis->m_stConferenceParams.PublicIPv6 == host)
					{
						stiASSERTMSG (estiFALSE, "Attempt to transfer to ourselves");
						RvSipSubsRespondReject (hSubscription, SIPRESP_LOOP_DETECTED, (RvChar *)"Cannot transfer to self.");
					}
					else
					{
						stiHResult hResult = pThis->TransferDial (sipCall, hSubscription, host);

						if (stiIS_ERROR (hResult))
						{
							stiASSERTMSG (estiFALSE, "Transfer failed\n");
							RvSipSubsRespondReject (hSubscription, SIPRESP_BAD_REQUEST, (RvChar *)"Transfer failed\n");
						}
					}
				}
				else
				{
					stiASSERTMSG (estiFALSE, "Refer has a method parameter different than INVITE. Reject it");
					RvSipSubsRespondReject (hSubscription, SIPRESP_BAD_REQUEST, (RvChar *)"Unsupported Method In Refer-To header");
				}
			}

			break;

		case RVSIP_SUBS_STATE_UNSUBSCRIBE_RCVD:

			RvSipSubsRespondAccept (hSubscription, 0);
			NotifySubscriptionTerminate (hSubscription, hAppSubs);

			break;

		case RVSIP_SUBS_STATE_TERMINATING:

			DBG_MSG ("TRANSFER Subscription ended.");

			break;

		case RVSIP_SUBS_STATE_REFRESH_RCVD:
		case RVSIP_SUBS_STATE_ACTIVATED:
			break;

		//
		// The following cases are the subscriber states
		//
		case RVSIP_SUBS_STATE_UNSUBSCRIBE_2XX_RCVD: 
		{
			DBG_MSG ("TRANSFER Unsubscribed.");

			break;
		}

		case RVSIP_SUBS_STATE_2XX_RCVD:
		case RVSIP_SUBS_STATE_NOTIFY_BEFORE_2XX_RCVD:
		case RVSIP_SUBS_STATE_SUBS_SENT:
		case RVSIP_SUBS_STATE_REDIRECTED:
		case RVSIP_SUBS_STATE_REFRESHING:
		case RVSIP_SUBS_STATE_UNSUBSCRIBING:
			break;

		case RVSIP_SUBS_STATE_UNAUTHENTICATED: 
		{
			RvSipCallLegHandle hCallLeg = nullptr;
			RvSipMsgHandle hInboundMsg = nullptr;

			if (RV_OK == RvSipSubsGetDialogObj (hSubscription, &hCallLeg) && hCallLeg)
			{
				if (RV_OK != RvSipCallLegGetReceivedMsg (hCallLeg, &hInboundMsg))
				{
					hInboundMsg = nullptr;
				}
			}

			if (hInboundMsg)
			{
				if (sipCall->m_bProxyLoginAttempted)
				{
					stiASSERTMSG (estiFALSE, "Subscription proxy Username/password incorrect");
					NotifySubscriptionTerminate (hSubscription, hAppSubs);
				}
				else
				{
					sipCall->m_bProxyLoginAttempted = true;

					DBG_MSG ("The subscription proxy says we must authenticate, so we shall.");

					//
					// We currently only use subscriptions with REFERs.  In this case, the refer was challenged.
					// Calling RvSipSubsRefer again will resend the refer with authentication information.
					//
					// Note: if we ever start doing subscriptions outside of REFERs then this will need to change
					// so that the code checks to see if the the subscription is *not* associated with a REFER
					// and in that case call RvSubsSubscribe instead.
					//
					RvSipSubsRefer (hSubscription);
				}
			}
			else
			{
				NotifySubscriptionTerminate (hSubscription, hAppSubs);
			}
			break;
		}

		//
		// The following cases are for states common to both the subscriber and notifier
		//
		case RVSIP_SUBS_STATE_ACTIVE: 
		{
			DBG_MSG ("Subscription successful. %p", hAppSubs);

			//
			// If we are the transferer and the subscription is active then
			// unsubscribe from the subscription.
			//
			if (sipCall->SubscriptionGet () == hSubscription &&
				sipCall->TransferDirectionGet () == estiOUTGOING)
			{
				RvSipSubsUnsubscribe (hSubscription);
			}

			// Reset the login counter in case the proxy demands authentication for something else
			sipCall->m_bProxyLoginAttempted = false;

			break;
		}

		case RVSIP_SUBS_STATE_TERMINATED: 
		{
			//
			// If the subscription is this calls subscription then
			// set it to NULL so it isn't used again.
			//
			if (sipCall->SubscriptionGet () == hSubscription)
			{
				sipCall->SubscriptionSet (nullptr);

				//
				// If we are the transferer then hangup the call.
				//
				if (sipCall->TransferDirectionGet () == estiOUTGOING)
				{
					sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NONE);
				}
			}

			break;
		}

		case RVSIP_SUBS_STATE_MSG_SEND_FAILURE: 
		{
			// If we receive a "503 Message Send Failure" response, inform the stack to giveup.
			// The stack will terminate the subscription once we call the xxxDNSGiveUp function.
			RvStatus rvStatus = RvSipSubsDNSGiveUp (hSubscription, nullptr);
			stiASSERT (RV_OK == rvStatus);
			break;
		}

		case RVSIP_SUBS_STATE_UNDEFINED:
		case RVSIP_SUBS_STATE_IDLE:
		case RVSIP_SUBS_STATE_PENDING:

			break;
	}

	pThis->Unlock ();

STI_BAIL:
	stiASSERT (!stiIS_ERROR (hResult));
}
#undef URI_MAX_SIZE


/*!\brief Prints the notify status
 * 
 * \param eNotifyStatus The notify status to print
 */
void PrintSubsNotifyStatus (
	RvSipSubsNotifyStatus eNotifyStatus)
{
	std::string caseString;
	
	switch (eNotifyStatus)
	{
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_REQUEST_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_FINAL_RESPONSE_SENT)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_REJECT_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_TERMINATED)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_2XX_RCVD)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_REDIRECTED)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_UNAUTHENTICATED)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_UNDEFINED)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_IDLE)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_MSG_SEND_FAILURE)
		GET_CASE_STRING (RVSIP_SUBS_NOTIFY_STATUS_REQUEST_SENT)
	}
	
	vpe::trace ("Subscription Notify Status: ", caseString, "\n");
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::SubscriptionNotifyCB
//
// Description: This callback is called to tell us when the remote phone's NOTIFY
//	susbscription changes.
//
// Abstract: Mostly used to monitor the new call state when we've told a phone to
//	transfer.
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::SubscriptionNotifyCB (
	RvSipSubsHandle hSubscription, ///< The sip stack subscription handle
	RvSipAppSubsHandle hAppSubs, ///< The application handle for this subscription.
	RvSipNotifyHandle hNotification, ///< The new created notification object handle.
	RvSipAppNotifyHandle hAppNotification, ///< The application handle for this notification.
	RvSipSubsNotifyStatus eNotifyStatus, ///< Status of the notification object.
	RvSipSubsNotifyReason eNotifyReason, ///< Reason to the indicated status.
	RvSipMsgHandle hNotifyMsg) ///< The received notify msg.
{
	stiDEBUG_TOOL (g_stiSipMgrDebug,
		PrintSubsNotifyStatus (eNotifyStatus);
	);
	
	//pThis->Lock (); <-- Uncomment if this method ever accesses the sip manager.

	switch (eNotifyStatus)
	{
		case RVSIP_SUBS_NOTIFY_STATUS_REQUEST_RCVD:
		{
			// OK the notification to let them know that we saw it.
			RvSipNotifyAccept (hNotification);

			break;
		}
		
		case RVSIP_SUBS_NOTIFY_STATUS_FINAL_RESPONSE_SENT:
		{
			RvInt32 n32Status;
			
			RvSipSubsGetReferFinalStatus (hSubscription, &n32Status);
			
			if (n32Status == 200)
			{
				auto  callLegKey = (size_t)hAppSubs;
				auto callLeg = callLegLookup (callLegKey);
				auto sipCall = callLeg->m_sipCallWeak.lock ();
				if (sipCall)
				{
					if (sipCall->SubscriptionGet ())
					{
						sipCall->SubscriptionSet (nullptr);
					}
				
					sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NONE);
				}
			}
			 
			break;
		}
		
		case RVSIP_SUBS_NOTIFY_STATUS_MSG_SEND_FAILURE:
		{
			// If we receive a "503 Message Send Failure" response, inform the stack to giveup.
			// The stack will terminate the subscription once we call the xxxDNSGiveUp function.
			RvStatus rvStatus = RvSipSubsDNSGiveUp (hSubscription, hNotification);
			stiASSERT (RV_OK == rvStatus);
			break;
		}

		case RVSIP_SUBS_NOTIFY_STATUS_REJECT_RCVD:
		case RVSIP_SUBS_NOTIFY_STATUS_TERMINATED:
		case RVSIP_SUBS_NOTIFY_STATUS_2XX_RCVD:
		case RVSIP_SUBS_NOTIFY_STATUS_REDIRECTED:
		case RVSIP_SUBS_NOTIFY_STATUS_UNAUTHENTICATED:
		case RVSIP_SUBS_NOTIFY_STATUS_UNDEFINED:
		case RVSIP_SUBS_NOTIFY_STATUS_IDLE:
		case RVSIP_SUBS_NOTIFY_STATUS_REQUEST_SENT:
			// Don't care about anything else.
			break;
	}

	//pThis->Unlock (); <-- Uncomment if this method ever accesses the sip manager.
}


/*!\brief Creates a call leg based on information in the SIP call object.
 * 
 * \param sipCall The SIP call object that represents the call
 */
stiHResult CstiSipManager::CallLegCreate (
    CstiSipCallSP sipCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipCallLegHandle hCallLeg = nullptr;
	CstiSipCallLegSP callLeg = nullptr;

	auto callLegKey = CstiSipCallLeg::uniqueKeyGenerate ();
	
	// Create a call leg bound to this call object
	RvStatus eStatus = RvSipCallLegMgrCreateCallLeg (m_hCallLegMgr, (RvSipAppCallLegHandle)callLegKey, &hCallLeg);
	stiTESTCOND (eStatus == RV_OK, stiRESULT_ERROR);

	sipCall->StackCallHandleSet (hCallLeg);
	callLeg = sipCall->CallLegAdd (hCallLeg);
	callLegInsert (callLegKey, callLeg);


	// Prevent the call object from being destructed by storing a shared pointer to itself
	// We will remove this reference when the call leg reaches the terminated state.  This was
	// a compromise to get around Radvision's C API, which breaks shared pointer semantics.
	sipCall->CstiCallGet()->disallowDestruction ();

STI_BAIL:

	return hResult;
}


/*!\brief Creates a call leg and readies the call to send an invite.
 *
 * In this method a call leg is created and an initial SDP object is created.
 * If ICE is being used then the ICE manager is told to start a session to 
 * gather candidates.  In the callback from the ICE manager the invite should
 * be finalized and sent.
 * 
 * \param poCall The call object that represents the call that will be placed.
 * 
 * \return Success or failure
 */
stiHResult CstiSipManager::InvitePrepare (
	CstiSipCallSP sipCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Create a call leg bound to this call object
	auto callLeg = sipCall->CallLegGet (nullptr);
	auto call = sipCall->CstiCallGet ();
	auto dialString = call->RoutingAddressGet ();
	
	// Determine which type of ip address we are using
	{
		CstiIpAddress IpAddress;
		CstiSipRegistration::SstiRegistrarSettings RegistrarSettings;
		
		RegistrarSettingsDetermine (m_UserPhoneNumbers.szPreferredPhoneNumber, &RegistrarSettings);
		if (UseInboundProxy () && !RegistrarSettings.ProxyIpAddress.empty ())
		{
			IpAddress = RegistrarSettings.ProxyIpAddress;
		}
		else
		{
			IpAddress = dialString.ipAddressStringGet ();
		}
		if (IpAddress.ValidGet ())
		{
			sipCall->m_eIpAddressType = IpAddress.IpAddressTypeGet ();
		}
	}
	
	// Determine the transport to use for the invite
	sipCall->m_eTransportProtocol = dialString.TransportGet ();
	if (estiTransportUnknown == sipCall->m_eTransportProtocol)
	{
		sipCall->m_eTransportProtocol = m_stConferenceParams.eSIPDefaultTransport;
	}
	
	// Only do ICE if transport is not UDP and we are not tunnelling.
	//
	if (sipCall->UseIceForCalls ())
	{
		// Attach SDP to the outgoing INVITE
		
		hResult = sipCall->InitialSdpOfferCreate (&callLeg->m_sdp, false);
		stiTESTRESULT ();
	
		callLeg->m_eNextICEMessage = estiOfferAnswerINVITE;

		hResult = IceSessionStart (CstiIceSession::estiICERoleOfferer, callLeg->m_sdp, nullptr, callLeg);
		stiTESTRESULT ();
	}
	else
	{
		// Attach SDP to the outgoing INVITE
		hResult = sipCall->InitialSdpOfferCreate (&callLeg->m_sdp, true);
		stiTESTRESULT ();
	
		hResult = InviteSend (sipCall, &callLeg->m_sdp);
		stiTESTRESULT ();
	}
	
STI_BAIL:

	return hResult;
}


/*!\brief Set the outbound proxy information if needed
 *
 * \param sipCall The call object to set the outbound proxy information for
 * \param callLeg The call leg to set the outbound proxy information for
 * \param pProxy A pointer to a standard string object to receive the proxy address in (may be NULL)
 * \param peTransport A pointer to an EstiTranport to receive the transport that will be used for the outbound proxy (may be NULL)
 */
stiHResult CstiSipManager::OutboundDetailsSet (
	CstiSipCallSP sipCall,
	CstiSipCallLegSP callLeg,
	std::string *pProxy,
	EstiTransport *peTransport)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string Proxy;
	EstiTransport eTransport = sipCall->m_eTransportProtocol;
	auto call = sipCall->CstiCallGet ();
	
	//
	// If NAT Traversal is enabled and we have a proxy address
	// then use it.
	//
	if (IsUsingOutboundProxy (call))
	{
		RvSipTransportOutboundProxyCfg proxyCfg;
		memset (&proxyCfg, 0, sizeof (proxyCfg));

		// Use agent realm for these modes if not registered
		if (!IsRegistered () && (IsLocalAgentMode ()))
		{
			//
			// We are in a phone mode that requires the use of the agent domain.
			//
			Proxy = m_stConferenceParams.SIPRegistrationInfo.AgentDomain;

			stiTESTCOND (m_proxyDomainResolver && !m_proxyDomainResolver->m_ResolvedIPAddress.empty (), stiRESULT_ERROR);

			proxyCfg.strHostName = (RvChar*)m_proxyDomainResolver->m_pProxyAddress->c_str ();

			proxyCfg.strIpAddress = (RvChar*)m_proxyDomainResolver->m_ResolvedIPAddress.c_str ();
			proxyCfg.port = m_proxyDomainResolver->m_un16ResolvedPort;
			eTransport = m_proxyDomainResolver->m_eResolvedTransport;

			//
			// If one of the allowed transports is TLS then use that.  Otherwise,
			// use TCP.
			//
			if (m_stConferenceParams.nAllowedProxyTransports & estiTransportTLS)
			{
				eTransport = estiTransportTLS;
			}
			else if (m_stConferenceParams.nAllowedProxyTransports & estiTransportTCP)
			{
				eTransport = estiTransportTCP;
			}
			else
			{
				stiASSERT (estiFALSE);
				eTransport = estiTransportTLS;
			}

			switch (eTransport)
			{
				case estiTransportTCP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TCP;
					break;

				case estiTransportUDP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					break;

				case estiTransportUnknown:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					stiASSERT (estiFALSE);
					break;

				case estiTransportTLS:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TLS;
					break;
			}

			rvStatus = RvSipCallLegSetOutboundDetails (sipCall->m_hCallLeg, &proxyCfg, sizeof (proxyCfg));
			stiTESTRVSTATUS ();
		}
		else if (!m_proxyDomainIpAddress.empty () && IsRegistered ())
		{
			//
			// We are using NAT Traversal.  If we have a private domain then use it.
			//
			Proxy = m_proxyDomain;
			
			proxyCfg.strIpAddress = (RvChar*)m_proxyDomainIpAddress.c_str ();
			proxyCfg.port = m_proxyDomainPort;
			eTransport = m_eLastNATTransport;
			
			//
			// Use the correct protocol when calling to the proxy.
			//
			switch (m_eLastNATTransport)
			{
				case estiTransportTCP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TCP;
					break;
					
				case estiTransportUDP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					break;

				case estiTransportUnknown:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					stiASSERT (estiFALSE);
					break;
					
				case estiTransportTLS:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TLS;
					break;
			}

			rvStatus = RvSipCallLegSetOutboundDetails (sipCall->m_hCallLeg, &proxyCfg, sizeof (proxyCfg));
			stiTESTRVSTATUS ();
		}
		else if (!m_stConferenceParams.SIPRegistrationInfo.PublicDomain.empty ())
		{
			//
			// We are NOT using NAT Traversal.  Try using the public domain.
			//
			Proxy = m_stConferenceParams.SIPRegistrationInfo.PublicDomain;
			
			// Set an IP address instead of a domain name. This resolves issues when calling logged out
			// and a DNS lookup returns a different IP address for the PublicDomain than the one the call was started on.
			std::string ProxyIPAddress;

			hResult = stiDNSGetHostByName (m_stConferenceParams.SIPRegistrationInfo.PublicDomain.c_str(),
							 m_stConferenceParams.SIPRegistrationInfo.PublicDomainAlt.c_str(),
							 &ProxyIPAddress);
			
			proxyCfg.strHostName = (RvChar*)ProxyIPAddress.c_str ();

			//
			// If one of the allowed transports is TLS then use that.  Otherwise,
			// use TCP.
			//
			if (m_stConferenceParams.nAllowedProxyTransports & estiTransportTLS)
			{
				eTransport = estiTransportTLS;
			}
			else if (m_stConferenceParams.nAllowedProxyTransports & estiTransportTCP)
			{
				eTransport = estiTransportTCP;
			}
			else
			{
				stiASSERT (estiFALSE);
				eTransport = estiTransportTLS;
			}

			switch (eTransport)
			{
				case estiTransportTCP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TCP;
					break;

				case estiTransportUDP:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					break;

				case estiTransportUnknown:
					proxyCfg.eTransport = RVSIP_TRANSPORT_UDP;
					stiASSERT (estiFALSE);
					break;

				case estiTransportTLS:
					proxyCfg.eTransport = RVSIP_TRANSPORT_TLS;
					break;
			}

			rvStatus = RvSipCallLegSetOutboundDetails (sipCall->m_hCallLeg, &proxyCfg, sizeof (proxyCfg));
			stiTESTRVSTATUS ();
		}

		rvStatus = RvSipCallLegSetForceOutboundAddrFlag (sipCall->m_hCallLeg, RV_TRUE);
		stiTESTRVSTATUS ();
	}
	
	if (pProxy)
	{
		if (!Proxy.empty ())
		{
			*pProxy = Proxy;
		}
		else
		{
			pProxy->clear ();
		}
	}
	
	if (peTransport)
	{
		*peTransport = eTransport;
	}
	
STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::InviteRequirementsAdd
//
// Description: Add to the message the requirements for a valid INVITE message
//
// Abstract: Utility used by this class to add the parameters that qualify the INVITE
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipManager::InviteRequirementsAdd (
	RvSipMsgHandle hMsg,
	bool bIsBridgedCall,
	bool geolocationRequested)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Attach the ALLOW fields to the outgoing message
	RvSipAllowHeaderHandle hAllow;
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_INVITE, nullptr);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_ACK, nullptr);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_CANCEL, nullptr);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_BYE, nullptr);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_OTHER, (RvChar*)MESSAGE_METHOD);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_OTHER, (RvChar*)INFO_METHOD);
	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_PRACK, nullptr);

	//
	// We don't allow interpreters, tech support agents or the hold server to be
	// transferrred. VRI mode does need the Refer for group call though.
	//
#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER)
	if (estiINTERPRETER_MODE != LocalInterfaceModeGet ()
	 && estiTECH_SUPPORT_MODE != LocalInterfaceModeGet ())
	{
		RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
		RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_REFER, nullptr);
		RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
		RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_SUBSCRIBE, nullptr);
	}
#endif

	RvSipAllowHeaderConstructInMsg (hMsg, RV_FALSE, &hAllow);
	RvSipAllowHeaderSetMethodType (hAllow, RVSIP_METHOD_NOTIFY, nullptr);

	RvSipOtherHeaderHandle hHeader;

	// Attach the Accept line to the outgoing INVITE
	RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
	RvSipOtherHeaderSetName (hHeader, (RvChar*)"Accept");
	RvSipOtherHeaderSetValue (hHeader, (RvChar*)SDP_MIME);
	
	RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
	RvSipOtherHeaderSetName (hHeader, (RvChar*)"Accept");
	RvSipOtherHeaderSetValue (hHeader, (RvChar*)DISPLAY_SIZE_MIME);

	RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
	RvSipOtherHeaderSetName (hHeader, (RvChar*)"Accept");
	RvSipOtherHeaderSetValue (hHeader, (RvChar*)MEDIA_CONTROL_MIME);

	if (LocalShareContactSupportedGet ())
	{
		// Add a Recv-Info header
		RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
		RvSipOtherHeaderSetName (hHeader, (RvChar*)RECV_INFO);
		RvSipOtherHeaderSetValue (hHeader, (RvChar*)CONTACT_PACKAGE);
	}

	//
	// For the audio bridge we don't want to signal that we support video.
	//
	if (!bIsBridgedCall)
	{
		// Attach the Accept-Contact line to the outgoing INVITE
		// (prefer to route this call to a VIDEO phone)
		RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
		RvSipOtherHeaderSetName (hHeader, (RvChar*)"Accept-Contact");
		RvSipOtherHeaderSetValue (hHeader, (RvChar*)"*; video");
	}
	
	// Add our public IP for use in VRS dial around example: "Call-Info: <sip:10.1.1.1>; purpose=trs-user-ip"
	RvChar szOriginalIdentity[50];

	snprintf (szOriginalIdentity, sizeof(szOriginalIdentity), "<sip:%s>; purpose=trs-user-ip", publicIpGet().c_str ());
	RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
	RvSipOtherHeaderSetName (hHeader, (RvChar*)"Call-Info");
	RvSipOtherHeaderSetValue (hHeader, szOriginalIdentity);
	
	if (UseInboundProxy () && m_bLoggedIn)
	{
		// If we have NAT enabled and are not an interpreter or tech support phone we should send our P-Preferred-Identity
		RvChar szPPreferredlIdentity[50];
		snprintf (szPPreferredlIdentity, sizeof(szPPreferredlIdentity), "<sip:%s@%s>", m_UserPhoneNumbers.szPreferredPhoneNumber,
					 m_stConferenceParams.SIPRegistrationInfo.PublicDomain.c_str ());
		RvSipOtherHeaderConstructInMsg (hMsg, RV_FALSE, &hHeader);
		RvSipOtherHeaderSetName (hHeader, (RvChar*)"P-Preferred-Identity");
		RvSipOtherHeaderSetValue (hHeader, szPPreferredlIdentity);
	}
	
	// If we have a geolocation (emergency call), add it.
	if (geolocationRequested)
	{
		geolocationAdd (hMsg);
	}

	return hResult;
}


/*!\brief Adds a private SVRS Geolocation header to the outgoing message.
 *
 * \param hMsg A handle to the outgoing message.
 *
 */
stiHResult CstiSipManager::geolocationAdd (
	RvSipMsgHandle msgHandle)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	
	if (!m_geolocation.empty())
	{
		RvSipOtherHeaderHandle header;
		
		rvStatus = RvSipOtherHeaderConstructInMsg (msgHandle, RV_FALSE, &header);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipOtherHeaderSetName (header, (RvChar*)g_szSVRSGeolocation);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipOtherHeaderSetValue (header, (RvChar*)m_geolocation.c_str());
		stiTESTRVSTATUS ();

		std::stringstream geoLocationLog;
		geoLocationLog << "EventType=Geolocation GeolocationData=\"" << m_geolocation << "\"";
		stiRemoteLogEventSend (geoLocationLog.str ().c_str ());
	}

STI_BAIL:
	
	return hResult;
}


/*!\brief Sets the call ID.
 *
 * \param sipCall The SIP call object that has reached the connected state.
 * \param callLeg The leg of the connected call
 */
static void CallIDSet(
    CstiSipCallSP sipCall,
    RvSipCallLegHandle hCallLeg)
{
	RvInt32 nActualLen = 0;
	RvChar szCallId[ID_BUF_LEN];
	RvStatus rvStatus = RV_OK;

	rvStatus = RvSipCallLegGetCallId(
		hCallLeg,
		ID_BUF_LEN,
		szCallId,
		&nActualLen);

	// If succeeded, copy the CallId to the passed in string object
	if (RV_OK == rvStatus)
	{
		sipCall->CallIdSet(szCallId);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::InviteSend
//
// Description: Send a SIP INVITE message
//
// Abstract: Utitlity used by this class to actually send the INVITE
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipManager::InviteSend (
	CstiSipCallSP sipCall, ///< The call object to attach to this call
	const CstiSdp *pSdp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string toLine;
	std::string calledName;
	std::string ToUri;
	std::string ModifiedDialString;
	std::string ModifiedAddress;
	auto call = sipCall->CstiCallGet ();
	CstiSipCallLegSP callLeg = sipCall->CallLegGet (nullptr);
	CstiRoutingAddress dialString; ///< The URI to send the INVITE to
	std::string fromAddress;
	std::string address;
	auto outboundMessage = callLeg->outboundMessageGet();
	std::string displayName = m_displayName;
	EstiTransport eTransport = sipCall->m_eTransportProtocol;
	std::string Proxy;
	std::string phoneNumber;
	const char *pszDomain = nullptr;
	unsigned int unPort = nDEFAULT_SIP_LISTEN_PORT;
	const char *pszProtocol = "sip";
	const int nMAX_LOGGED_OUT_DISPLAY_NAME = 36;	// It will contain a string formatted like "Unregistered Caller XXXXXXXXXXXX"
	char szTmpDisplay[nMAX_LOGGED_OUT_DISPLAY_NAME];  // Declare here since we set a pointer to access it

	if (!callLeg)
	{
		sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
		stiTESTCOND (callLeg, stiRESULT_ERROR); // bail out of this function
	}
	
	SIP_DBG_EVENT_SEPARATORS("Creating a new %s INVITE", estiTransportTCP == sipCall->m_eTransportProtocol ? "TCP" : "UDP");

	if (pSdp != nullptr)
	{
		hResult = outboundMessage.bodySet (*pSdp);
		stiTESTRESULT ();
	
		callLeg->m_bAnswerPending = true;
	}
	
	// Add what is required for a call.
	hResult = outboundMessage.inviteRequirementsAdd();
	stiTESTRESULT ();

	hResult = OutboundDetailsSet (sipCall, callLeg, &Proxy, &eTransport);
	stiTESTRESULT ();
	
	// Determine the from address
	switch (m_eLocalInterfaceMode)
	{
		case estiINTERPRETER_MODE:
			pszDomain = INTERPRETER_FROM_DOMAIN;
			eTransport = estiTransportUnknown;

			if (sipCall->IsBridgedCall ())
			{
				// In a bridged call use our terp vp number so that
				// the terp's desk phone caller id always shows something
				displayName = m_UserPhoneNumbers.szPreferredPhoneNumber;
				phoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
			}
			else if (m_UserPhoneNumbers.szHearingPhoneNumber[0] != '\0')
			{
				phoneNumber = m_UserPhoneNumbers.szHearingPhoneNumber;
			}
			else
			{
				displayName = "";
				pszDomain = g_szAnonymousDomain;
				phoneNumber = g_szAnonymousUser;
			}
			break;

		case estiVRI_MODE:
			pszDomain = VRI_FROM_DOMAIN;
			eTransport = estiTransportUnknown;

			if (sipCall->IsBridgedCall ())
			{
				// In a bridged call use our terp vp number so that
				// the terp's desk phone caller id always shows something
				displayName = m_UserPhoneNumbers.szPreferredPhoneNumber;
				phoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
			}
			break;

		case estiTECH_SUPPORT_MODE:
			pszDomain = TECH_SUPPORT_FROM_DOMAIN;
			eTransport = estiTransportUnknown;
			break;

		case estiSTANDARD_MODE:
		case estiPUBLIC_MODE:
		case estiKIOSK_MODE:
		case estiABUSIVE_CALLER_MODE:
		case estiPORTED_MODE:
		case estiHEARING_MODE:
			if (!Proxy.empty())
			{
				pszDomain = Proxy.c_str();
				eTransport = estiTransportUnknown;
				if (m_stConferenceParams.bBlockCallerID)
				{
					displayName = "";
					phoneNumber = g_szAnonymousUser;
					if (0 < strlen (m_UserPhoneNumbers.szPreferredPhoneNumber))
					{
						pszDomain = g_szAnonymousDomain;
					}
					else
					{
						// Unregistered user should go to the unregistered domain
						pszDomain = g_szUnregisteredDomain;
					}
				}
				else if (call->DialedOwnRingGroupGet())
				{
					phoneNumber = m_UserPhoneNumbers.szLocalPhoneNumber;
				}
				else
				{
					if (0 < strlen(m_UserPhoneNumbers.szPreferredPhoneNumber))
					{
						phoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
					}
					else
					{
						// Unregistered user
						pszDomain = g_szUnregisteredDomain;
						phoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
					}

				}
			}
			else
			{
				unPort = LocalListenPortGet ();
	
				if (m_stConferenceParams.bBlockCallerID)
				{
					displayName = "";
					phoneNumber = g_szAnonymousUser;
					if (0 < strlen (m_UserPhoneNumbers.szPreferredPhoneNumber))
					{
						pszDomain = g_szAnonymousDomain;
					}
					else
					{
						// Unregistered user should go to the unregistered domain
						pszDomain = g_szUnregisteredDomain;
					}
				}
				else
				{
					// From the phone's ip address
					std::string *pIpAddress = &m_stConferenceParams.PublicIPv4;

					if (sipCall->m_eIpAddressType == estiTYPE_IPV6 && !m_stConferenceParams.PublicIPv6.empty ())
					{
						pIpAddress = &m_stConferenceParams.PublicIPv6;
					}

					pszDomain = pIpAddress->c_str ();

					if (call->DialedOwnRingGroupGet())
					{
						// Dialing within ring group so use the device's unique individual number
						phoneNumber = m_UserPhoneNumbers.szLocalPhoneNumber;
					}
					else
					{
						// Dialing outside world so prefer ring group number if available
						phoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
					}
				}
			}
			break;

		case estiINTERFACE_MODE_ZZZZ_NO_MORE:
			stiTHROW (stiRESULT_ERROR);
			break;
	}
	
	// If this is not a bridged call and the user is not blocking their caller ID
	// then we should add user interface group data if we are a memeber of one. Fixes bug 20225.
	if (!m_stConferenceParams.bBlockCallerID
		 && !sipCall->IsBridgedCall ()
	     && !m_localReturnCallDialString.empty())
	{
		phoneNumber = m_localReturnCallDialString;
		
		// Set the display name to be that supplied by the user interface group.
		displayName = m_displayName;
		
		// If we are in the interpreter interface mode but are using UIG information
		// then reset the domain to ensure we are not using anonymous for our domain.
		if (estiINTERPRETER_MODE == m_eLocalInterfaceMode)
		{
			pszDomain = INTERPRETER_FROM_DOMAIN;
		}
	}

	// If a override from name was supplied, always use it for the display name
	{
		auto fromNameOverride = call->fromNameOverrideGet();
		if (fromNameOverride)
		{
			displayName = *fromNameOverride;
		}
	}

	address = AddressStringCreate(pszDomain, phoneNumber.c_str(), unPort, eTransport, pszProtocol);

	// If we are going to use a gateway for this call we cannot add the plus bug #19987.
	ModifiedAddress = AddPlusToPhoneNumber(address);

	if (estiTECH_SUPPORT_MODE != m_eLocalInterfaceMode
	 && estiINTERPRETER_MODE != m_eLocalInterfaceMode
	 && ((!m_bLoggedIn && m_UserPhoneNumbers.szLocalPhoneNumber[0] == '\0')
		 || !IsRegistered ())
	 && displayName.empty())
	{
		if (phoneNumber.empty ())
		{
			// If the user is not logged in, format the from address to look like:
			// From: "Unregistered Caller XXXXXXXXXXXX" sip:911YYYYYY@unregistered.sorenson.com
			// where XXXXXXXXXXXX is the mac address without colons and
			// YYYYYY is the least significant 5 characters of the mac address converted to decimal
			// and padded with 0's after 911 such that the number ends up being a 10-digit number.
			std::string uniqueID;
			char szUniqueDecimalID[nMAX_MAC_ADDRESS_NO_COLONS_STRING_LENGTH];
			stiOSGetUniqueID (&uniqueID);
			uniqueID.erase (std::remove (uniqueID.begin (), uniqueID.end (), ':'), uniqueID.end ());
			
			uint64_t decimalID = 0;
			std::stringstream ss;
			ss << std::hex << uniqueID;
			ss >> decimalID;
			decimalID &= 0xFFFFF;
			
			// We want this to be a 7-digit number.  If it is less than 7, pad with 0 in the
			// most significant locations (e.g. 0001234, not 1234000).
			sprintf (szUniqueDecimalID, "%07llu", static_cast<long long unsigned>(decimalID));
			
			ModifiedAddress = "sip:911";
			ModifiedAddress += szUniqueDecimalID;
			ModifiedAddress += "@";
			ModifiedAddress += g_szUnregisteredDomain;
			
			sprintf (szTmpDisplay, "Unregistered Caller %s", uniqueID.c_str());
			displayName = szTmpDisplay;
		}
		else
		{
			std::stringstream unregisteredAddr;
			unregisteredAddr << "sip:" << phoneNumber << "@" << g_szUnregisteredDomain;
			ModifiedAddress = unregisteredAddr.str();
		}
		
		call->unregisteredCallSet (true);
	}

	// Set the FROM: address (no ";transport=")
	fromAddress = AddressHeaderStringCreate (
		"From",
		ModifiedAddress,
		displayName);
		
	// Since we may have used an address above (e.g. when in interpreter mode) that won't
	// work for the CONTACT header, we need to create a new address again.  Since we are 
	// re-creating it, it doesn't matter if we had been using TCP or UDP, we will handle it here appropriately 
	// by sending in the protocol.
	
	//
	// Attach the Contact: header
	//
	CallLegContactAttach (callLeg->m_hCallLeg);
	
	//
	// If not using UDP then set the persistency level so the stack
	// uses the connection we have already setup.
	//
	if (eTransport != estiTransportUDP)
	{
		RvSipCallLegSetPersistency (callLeg->m_hCallLeg, RVSIP_TRANSPORT_PERSISTENCY_LEVEL_TRANSC_USER);
	}

	// Create the TO: address
	call->CalledNameGet (&calledName);
	dialString = call->RoutingAddressGet ();

	ModifiedDialString = AddPlusToPhoneNumber (dialString.UriGet ());

	toLine = AddressHeaderStringCreate (
		"To",
		ModifiedDialString,
		calledName);
	
	call->UriSet (ModifiedDialString);

	//
	// Create the remote contact (the target URI)
	//
	{
		RvSipAddressHandle hContactAddress = nullptr;
		std::string RemoteContact = ModifiedDialString;

		// If we have an IP address, add the transport.
		if (IPAddressValidate (dialString.ipAddressStringGet ().c_str ()))
		{
			switch (sipCall->m_eTransportProtocol)
			{
				case estiTransportTCP:
					RemoteContact.append (";transport=tcp");
					break;

				case estiTransportTLS:
					RemoteContact.append (";transport=tls");
					break;

				case estiTransportUnknown:
				case estiTransportUDP:
					break;
				//default: // Commented out for type safety
			}
		}

		// Attach the remote contact address to the call leg
		rvStatus = RvSipCallLegGetNewMsgElementHandle (callLeg->m_hCallLeg, RVSIP_HEADERTYPE_UNDEFINED, RVSIP_ADDRTYPE_URL, (void**)&hContactAddress);
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipAddrParse (hContactAddress, (RvChar*)RemoteContact.c_str ());
		stiTESTRVSTATUS ();
		
		rvStatus = RvSipCallLegSetRemoteContactAddress (callLeg->m_hCallLeg, hContactAddress);
		stiTESTRVSTATUS ();
	}

	// If this call should always be accepted, add the allow call header
	if (call->AlwaysAllowGet() || (estiTECH_SUPPORT_MODE == m_eLocalInterfaceMode))
	{
		// Add the AllowIncomingCall header
		hResult = outboundMessage.allowIncomingCallHeaderAdd();
		stiTESTRESULT ();
	}

	// If this call is going to be converted to H323 we need to add the SIP to H323 header
	if (sipCall->m_bAddSipToH323Header)
	{
		hResult = outboundMessage.convertSipToH323HeaderAdd();
		stiTESTRESULT ();
	}
	
	// Send the INVITE
	rvStatus = RvSipCallLegMake (callLeg->m_hCallLeg, const_cast<RvChar *>(fromAddress.c_str ()), const_cast<RvChar *>(toLine.c_str ()));
	stiASSERTMSG (rvStatus == RV_OK, "Sip leg %p unable to INVITE From:%s To:%s", (void*)callLeg->m_hCallLeg, fromAddress.c_str (), toLine.c_str ());
	stiTESTRVSTATUS ();

	CallIDSet(sipCall, callLeg->m_hCallLeg);

	DBG_MSG ("Created:\n\tCstiCall = %p\n\tCstiSipCall=%p\n\tRvSipCallLegHandle=%p", call.get(), sipCall.get(), callLeg->m_hCallLeg);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
		sipCall->StackCallHandleSet (nullptr);

		if (rvStatus == RV_ERROR_OUTOFRESOURCES)
		{
			std::string eventMessage = "EventType=SipStackRestart Reason=InviteSendOutOfResources";
			stiRemoteLogEventSend (&eventMessage);
			StackRestart ();
		}
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransferDial
//
// Description: This is a special "Dial" method to be used in the case of a Transfer
//
// Abstract: Unlike normal dial, wraps a Call object around an existing Radvision call leg.
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipManager::TransferDial (
    CstiSipCallSP originalSipCall,
	RvSipSubsHandle hSubscription,
    std::string host)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string toUri;
	bool bCallSettingsSaved = false;
	std::string user;
	
	RvSipCallLegHandle hCallLeg = nullptr;
	RvSipCommonStackObjectType eObjType = RVSIP_COMMON_STACK_OBJECT_TYPE_UNDEFINED;
	auto originalCall = originalSipCall->CstiCallGet ();
	CstiSipCallLegSP newCallLeg = nullptr;
	
	logCallSignal.Emit (originalCall, originalSipCall->CallLegGet(nullptr));

	// Cancel the lost connection and re-invite timers in case either is running on
	// the original call object.  Without doing so, the reference that was added to
	// the CstiCall object will potentially get released on the new call object (since
	// we switch the CstiSip call object under the hood.
	originalSipCall->LostConnectionTimerCancel ();
	originalSipCall->ReinviteTimerCancel ();
	originalSipCall->ReinviteCancelTimerCancel ();

	// Make sure the original data streams don't cause contention for the platform layer
	originalSipCall->MediaClose();

	//
	// Create a new CstiCall object and a new protocol call object.
	//
	auto newCall = std::make_shared<CstiCall>(
	            m_pCallStorage, m_UserPhoneNumbers, m_userId, m_groupUserId,
	            m_displayName, m_alternateName, m_vrsCallId, &m_stConferenceParams);

	OptString originalRelayLanguage;
	originalCall->LocalPreferredLanguageGet (&originalRelayLanguage);
	newCall->LocalPreferredLanguageSet (originalRelayLanguage, originalCall->LocalPreferredLanguageGet ());

	auto newSipCall = std::make_shared<CstiSipCall>(&m_stConferenceParams, this, m_vrsFocusedRouting);
	newSipCall->m_stConferenceParams.eSecureCallMode = originalSipCall->m_stConferenceParams.eSecureCallMode;

	// Move the original caller ID name to the new object
	std::string originalRemoteName;
	originalSipCall->RemoteNameGet (&originalRemoteName);
	newSipCall->RemoteDisplayNameSet (originalRemoteName.c_str ());
	
	// Set the local return call info so the dialed system knows where we want them to call back to.
	newCall->LocalReturnCallInfoSet (m_eLocalReturnCallDialMethod, m_localReturnCallDialString);
	
	//
	// Don't let the application layer know about this call.
	//
	newCall->NotifyAppOfCallSet(estiFALSE);
	
	//
	// Now assign the new SIP Call to the original CstiCall object and 
	// the original SIP call to the new CstiCall object.
	//
	newCall->ProtocolCallSet (originalSipCall);
	originalCall->ProtocolCallSet(newSipCall);

	DBG_MSG ("Original CstiCall: %p, CstiSipCall: %p, Temporary CstiCall: %p, New CstiSipCall: %p\n",
	         originalCall.get(), originalSipCall.get(), newCall.get(), newSipCall.get());
	
	auto newCallLegKey = CstiSipCallLeg::uniqueKeyGenerate ();
	rvStatus = RvSipSubsReferAccept (hSubscription, (void*)newCallLegKey, eObjType, &eObjType, (void**)(void *)&hCallLeg);
	stiTESTRVSTATUS ();
	
	newSipCall->StackCallHandleSet (hCallLeg);
	newCallLeg = newSipCall->CallLegAdd (hCallLeg);
	callLegInsert (newCallLegKey, newCallLeg);

	DBG_MSG ("Attempting to dial \"transfer-to\" call. %p", originalCall.get());
	newSipCall->TransferInfoSet (host, estiINCOMING);

	// Link the transfer subscription to the new call.
	newSipCall->SubscriptionSet (hSubscription);

	// Hold the current call
	//originalCall->Hold ();

	RvSipAddressHandle hToAddress;
	rvStatus = RvSipCallLegGetRemoteContactAddress (hCallLeg, &hToAddress);
	stiTESTRVSTATUS ();
	
	sipAddrGet (hToAddress, m_hAppPool, &toUri);
	
	DBG_MSG ("CstiSipManager::TransferDial originalCall=%p newSipCall=%p", originalCall.get(), newSipCall.get());

	originalCall->TransferredSet ();
	
	//
	// Save the original call information in the new call object.
	//
	m_eOriginalDirection = originalCall->DirectionGet ();
	newCall->RoutingAddressSet (originalCall->RoutingAddressGet());
	newCall->StateSet (originalCall->StateGet(), originalCall->SubstateGet());
	
	bCallSettingsSaved = true;
	
	// Set the direction and the dial string in the call object
	originalCall->DirectionSet (estiOUTGOING);
	originalCall->RoutingAddressSet(toUri);
	
	user = originalCall->RoutingAddressGet ().userGet ();

	//
	// Get the call ID from the original call so that it can be logged.
	//
	originalSipCall->CallIdGet(&newSipCall->m_OriginalCallID);

	// If we were not transferred by a hold server, interpreter, or TS,
	// and we have a User portion in the routing address then we will perform a directory resolve.
	if (newCall->RemoteInterfaceModeGet () != estiINTERPRETER_MODE
		&& newCall->RemoteInterfaceModeGet () != estiVRI_MODE
	    && newCall->RemoteInterfaceModeGet () != estiTECH_SUPPORT_MODE
	    && newCall->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) == estiFALSE
	    && UserIsPhoneNumber (user))
	{
		auto call = newSipCall->CstiCallGet ();
		call->StateSet (esmiCS_TRANSFERRING);
		call->TransferDialStringSet (user);
		directoryResolveSignal.Emit (call);
	}
	else
	{
		// If this is a DHV call set the video active state to be the same on the new call as the original call.
		if (originalSipCall->m_dhviCall)
		{
			newSipCall->m_videoStream.setActive (originalSipCall->m_videoStream.isActive ());
		}

		hResult = InvitePrepare (newSipCall);
		stiTESTRESULT ();
		
		// Decouple any existing DHV calls if we succeeded in sending the invite for a new call.
		if (originalSipCall->m_dhviCall)
		{
			CstiSipCallSP dhvSipCall = std::static_pointer_cast<CstiSipCall>(originalSipCall->m_dhviCall->ProtocolCallGet());
			
			newSipCall->m_dhviCall = originalSipCall->m_dhviCall;
			originalSipCall->m_dhviCall = nullptr;
			
			dhvSipCall->m_dhviCall = newSipCall->CstiCallGet ();
			newSipCall->dhvSwitchAfterHandoffSet (true);
			originalSipCall->dhvSwitchAfterHandoffSet (true);
			newCall->dhviStateSet (originalCall->dhviStateGet ());
		}

		// We were transferred by a hold server, terp, or TS
		originalCall->RelayTransferSet(true);

		// Store that this call is a transferred call.
		originalCall->TransferSet (estiTRUE);

		//
		// Place the call back into an initialized state so that 
		// state transitions occur correctly.  This is needed to
		// avoid the error that occurs when transitioning
		// from a connected:held to a connecting state.  An alternative
		// approach might be to change the state machine to allow
		// this transition.
		//
	
		originalCall->StateSet (esmiCS_IDLE);
	
		originalCall->AppNotifiedOfConferencingSet (estiFALSE);
	
		callTransferringSignal.Emit (originalCall);
	
		// We're making a call
		NextStateSet (originalCall, esmiCS_CONNECTING, estiSUBSTATE_CALLING);

		// Start a ringing sound
		OutgoingToneSet (estiTONE_RING);
	}
	
	m_originalCall = originalCall;
	m_originalSipCall = originalSipCall;
	
	m_transferCall = newCall;
	m_transferSipCall = newSipCall;

	newCall->disallowDestruction ();

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (newSipCall)
		{
			newSipCall->TransferInfoSet ("", estiUNKNOWN);
		}
		
		//
		// Swap the call objects back.
		//
		newCall->ProtocolCallSet (newSipCall);
		originalCall->ProtocolCallSet(originalSipCall);
		
		if (bCallSettingsSaved)
		{
			//
			// Restore the call information from the new call object.
			//
			originalCall->DirectionSet (m_eOriginalDirection);
			originalCall->RoutingAddressSet (newCall->RoutingAddressGet());
			originalCall->StateSet (newCall->StateGet(), newCall->SubstateGet());
		}
		
		m_originalCall = nullptr;
		m_originalSipCall = nullptr;
		
		m_transferCall = nullptr;
		m_transferSipCall = nullptr;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ContinueTransfer
//
// Description: Completes the point to point transfer process started by a sip refer
//
// Abstract:
//
// Returns:
//	
//
void CstiSipManager::ContinueTransfer (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
	hResult = InvitePrepare (sipCall);
	stiTESTRESULT ();
	
	//
	// Place the call back into an initialized state so that
	// state transitions occur correctly.  This is needed to
	// avoid the error that occurs when transitioning
	// from a connected:held to a connecting state.  An alternative
	// approach might be to change the state machine to allow
	// this transition.
	//
	
	call->StateSet (esmiCS_IDLE);
	call->AppNotifiedOfConferencingSet (estiFALSE);
	
	callTransferringSignal.Emit (call);
	
	// We're making a call
	NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_CALLING);
	
	// Start a ringing sound
	OutgoingToneSet (estiTONE_RING);
	
STI_BAIL:
	
	if (stiIS_ERROR (hResult))
	{
		if (sipCall)
		{
			call->StateSet (esmiCS_CONNECTED);
			call->ResultSet(estiRESULT_TRANSFER_FAILED);
			call->HangUp();;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ReferNotifyReadyCB
//
// Description: This callback informs us that, during a call transfer,  we should
// send a NOTIFY request on a refer-subscription.
//
// Abstract:
//
// Returns: Nothing
//
#define EXPIRES_TIMEOUT 50
void RVCALLCONV CstiSipManager::ReferNotifyReadyCB  (
	IN RvSipSubsHandle hSubscription, ///< The SIP Stack subscription handle.
	IN RvSipAppSubsHandle hAppSubscription, ///< The application handle for this subscription.
	IN RvSipSubsReferNotifyReadyReason eReason, ///< The reason for a NOTIFY request to be sent.
	IN RvInt16 nResponseCode, ///< The response code that should be set in the NOTIFY message body.
	IN RvSipMsgHandle hResponseMsg ///< The message that was received on the Refer-related object  (provisional or final response).
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipSubscriptionSubstate eSubscriptionSubState = RVSIP_SUBSCRIPTION_SUBSTATE_TERMINATED;
	RvSipSubscriptionReason eNotifyReason = RVSIP_SUBSCRIPTION_REASON_UNDEFINED;
	RvInt32 nExpires = UNDEFINED;
	RvInt16 nNotifyResponseCode = nResponseCode;

	//
	// Get the subscription state.
	//
	RvSipSubsState eSubscriptionState = RVSIP_SUBS_STATE_UNDEFINED;

	RvSipSubsGetCurrentState (hSubscription, &eSubscriptionState);

	//
	// Only send the notify if the subscription is not currently terminating or terminated.
	//
	if (eSubscriptionState != RVSIP_SUBS_STATE_TERMINATING
	 && eSubscriptionState != RVSIP_SUBS_STATE_TERMINATED)
	{
		//pThis->Lock (); <-- Uncomment if this method ever accesses the sip manager.

		switch (eReason)
		{
			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_1XX_RESPONSE_MSG_RCVD:
			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_INITIAL_NOTIFY:
				eSubscriptionSubState = RVSIP_SUBSCRIPTION_SUBSTATE_ACTIVE;
				nExpires = EXPIRES_TIMEOUT;
				break;

			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_FINAL_RESPONSE_MSG_RCVD:
				eNotifyReason = RVSIP_SUBSCRIPTION_REASON_NORESOURCE;
				break;

			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_TIMEOUT:
			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_ERROR_TERMINATION:
				nNotifyResponseCode = SIPRESP_SERVICE_UNAVAILABLE;
				eNotifyReason = RVSIP_SUBSCRIPTION_REASON_NORESOURCE;
				break;

			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_UNDEFINED:
			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_OBJ_TERMINATED:
			case RVSIP_SUBS_REFER_NOTIFY_READY_REASON_UNSUPPORTED_AUTH_PARAMS:

				break;
		}

		if (RVSIP_SUBSCRIPTION_SUBSTATE_UNDEFINED != eSubscriptionSubState)
		{
			// create the notification object
			RvSipNotifyHandle hNotify;
			RvStatus rvStatus = RV_OK;
			auto  callLegKey = (size_t)hAppSubscription;
			CstiSipCallLegSP callLeg = nullptr;
			CstiSipCallSP sipCall = nullptr;

			rvStatus = RvSipSubsCreateNotify (hSubscription, (RvSipAppNotifyHandle)hAppSubscription, &hNotify);
			stiTESTRVSTATUS ();

	#if 0
			//
			// In a future release, we want to change the subscription mechanism
			// so that the referee terminates the subscription right away
			// instead of waiting for the referer to unsubscribe.  This may
			// be possible after 3.5 is saturated.
			//
			eSubscriptionSubState = RVSIP_SUBSCRIPTION_SUBSTATE_TERMINATED;
			eNotifyReason = RVSIP_SUBSCRIPTION_REASON_NORESOURCE;
			nExpires = 0;
	#endif

			rvStatus = RvSipNotifySetSubscriptionStateParams (hNotify, eSubscriptionSubState, eNotifyReason, nExpires);
			stiTESTRVSTATUS ();

			rvStatus = RvSipNotifySetReferNotifyBody (hNotify, nNotifyResponseCode);
			stiTESTRVSTATUS ();

			rvStatus = RvSipNotifySend (hNotify); //send the notify request
			stiTESTRVSTATUS ();
	
			callLeg = callLegLookup (callLegKey);
			sipCall = callLeg->m_sipCallWeak.lock ();
			sipCall->SubscriptionSet (hSubscription);
		}
	}
	
STI_BAIL:
	if (hResult) {} // Prevent compiler warning
	
	//pThis->Unlock (); <-- Uncomment if this method ever accesses the sip manager.
}
#undef EXPIRES_TIMEOUT


void CstiSipManager::dhvSipMgrMcuDisconnectCall(CstiCallSP call)
{
	auto dhvSipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
	
	PostEvent ([this, dhvSipCall] {		
		dhvSipCall->CstiCallGet()->dhviStateSet (IstiCall::DhviState::Capable);
		dhvSipCall->logDhviDeafDisconnect ();
		dhvSipCall->sendDhviDisconnectMsg ();
		dhvSipCall->dhviMcuDisconnect (true);
	});
}

/*!\brief Set the corePublicIp
*
* \return stiRESULT_SUCCESS on success, stiRESULT_INVALID_PARAMETER on empty IP argument
*/
stiHResult CstiSipManager::corePublicIpSet(
	const std::string &publicIp)
{
	stiLOG_ENTRY_NAME(CstiProtocolManager::corePublicIpSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCONDMSG(!publicIp.empty (), stiRESULT_INVALID_PARAMETER, "Empty IP returned by Core");
	stiTESTCONDMSG(IPAddressValidate (publicIp.c_str ()), stiRESULT_INVALID_PARAMETER, "Invalid IP returned by Core");

	m_corePublicIp = publicIp;

STI_BAIL:

	return hResult;
} // end CstiSipManager::corePublicIpSet


////////////////////////////////////////////////////////////////////////////////
//; ContactReceive
//
// Description: Receives a contact sent over the wire
//
// Returns: stiRESULT_ERROR if the contact is not acceptable for any reason
//
stiHResult ContactReceive (
	RvSipMsgHandle hMsg, ///! The sip message we are looking at
	SstiSharedContact *contact) ///! The contact struct to fill out if successful
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszBuffer = nullptr;
//	char *pszValue = NULL;
//	RvSipOtherHeaderHandle hOtherHeader;
	RvSipBodyHandle hBody;
//	RvUint nLength;
	RvUint32 unBufferLen;
	RvUint32 unActualLen;
//	RvSipHeaderListElemHandle hListElement;
	
	//
	// The below code is commented out to fix bug 21804.
	// Due to the Info-Package header not being passed through Walker we cannot rely on it at this time.
	//
	
//	hOtherHeader = RvSipMsgGetHeaderByName (hMsg, (RvChar*)INFO_PACKAGE, RVSIP_FIRST_HEADER, &hListElement);
//	stiTESTCOND (hOtherHeader, stiRESULT_ERROR);
//	
//	nLength = RvSipOtherHeaderGetStringLength (hOtherHeader, RVSIP_OTHER_HEADER_VALUE);
//	stiTESTCOND (nLength, stiRESULT_ERROR);
//	
//	nLength++;
//	pszValue = new char[nLength];
//	rvStatus = RvSipOtherHeaderGetValue (hOtherHeader, pszValue, nLength, &nLength);
//	stiTESTRVSTATUS ();
	
//	stiTESTCOND (0 == strcmp (pszValue, CONTACT_PACKAGE)
//					 || 0 == strcmp (pszValue, HEARING_CALL_SPAWN_PACKAGE) , stiRESULT_ERROR);
	stiTESTCOND (contact, stiRESULT_ERROR);
	
	hBody = RvSipMsgGetBodyObject (hMsg);
	stiTESTCOND (hBody, stiRESULT_ERROR);
	
	unBufferLen = RvSipBodyGetBodyStrLength (hBody);
	stiTESTCOND (unBufferLen, stiRESULT_ERROR);
	
	pszBuffer = new char[unBufferLen + 1];
	rvStatus = RvSipBodyGetBodyStr (hBody, pszBuffer, unBufferLen, &unActualLen);
	stiTESTRVSTATUS ();
	
	pszBuffer[unActualLen] = '\0';
					
	hResult = ContactDeserialize (pszBuffer, contact);
	stiTESTRESULT ();

STI_BAIL:


//	if (pszValue)
//	{
//		delete [] pszValue;
//		pszValue = NULL;
//	}

	if (pszBuffer)
	{
		delete [] pszBuffer;
		pszBuffer = nullptr;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; SipInfoMsgReceive
//
// Description: Processes a SIP info message and returns the body in it
//
// Returns: stiRESULT_ERROR on error
//
stiHResult SipInfoMsgReceive (
	RvSipMsgHandle hMsg, ///! The sip message we are looking at
	std::string *pMsgBody) ///! The string to populate
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszBuffer = nullptr;
	RvSipBodyHandle hBody;
	int nBufferLen;
	int actualLen = 0;
	
	hBody = RvSipMsgGetBodyObject (hMsg);
	stiTESTCOND (hBody, stiRESULT_ERROR);
	
	nBufferLen = RvSipBodyGetBodyStrLength (hBody);
	stiTESTCOND (nBufferLen, stiRESULT_ERROR);
	
	pszBuffer = new char[nBufferLen + 1];
	rvStatus = RvSipBodyGetBodyStr (hBody, pszBuffer, nBufferLen, (RvUint*)&actualLen);
	stiTESTRVSTATUS ();
	
	// Ensure the string is null-terminated
	pszBuffer[nBufferLen] = '\0';
	
	*pMsgBody = pszBuffer;
	stiTESTRESULT ();
	
STI_BAIL:
	
	if (pszBuffer)
	{
		delete [] pszBuffer;
		pszBuffer = nullptr;
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegTransactionStateChangeCB
//
// Description: This callback is called when a call leg transaction changes state
//
// Abstract: We use it for both handling UPDATE messages and kicking off our state
//	machine
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::CallLegTransactionStateChangeCB (
	IN RvSipCallLegHandle hCallLeg,
	IN RvSipAppCallLegHandle hAppCallLeg,
	IN RvSipTranscHandle hTransaction,
	IN RvSipAppTranscHandle hApphTransaction,
	IN RvSipCallLegTranscState eTransactionState,
	IN RvSipTransactionStateChangeReason eReason)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	RvStatus rvStatus = RV_OK;

	auto callLegKey = (size_t)hAppCallLeg;
	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	auto pThis = (CstiSipManager*)sipCall->ProtocolManagerGet ();

	vpe::sip::Transaction transaction(hTransaction);

	pThis->Lock ();

	// Make sure the name is set
	auto inboundMessage = callLeg->inboundMessageGet();

	callLeg->remoteUANameSet(inboundMessage);

	switch (eTransactionState)
	{
		case RVSIP_CALL_LEG_TRANSC_STATE_SERVER_GEN_REQUEST_RCVD:
		{
			auto method = transaction.methodGet();

			if (method == MESSAGE_METHOD)
			{
				auto contentType = inboundMessage.contentTypeHeaderGet();

				if (contentType.empty() ||
					contentType == NUDGE_MIME ||
					contentType == TEXT_MIME)
				{
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
				}
				else
				{
					stiASSERTMSG (estiFALSE, "Unsupported MESSAGE mime type \"%s\"", contentType.c_str());
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_NOT_ACCEPTABLE);
				}
			}
			else if (method == INFO_METHOD)
			{
				// Search it for xml keyframe requests and custom messages
				bool bWasHandled = false;

				auto contentType = inboundMessage.contentTypeHeaderGet();

				if (contentType == MEDIA_CONTROL_MIME)
				{
					// The right type of message, but part xml to make sure they're asking for a keyframe
					RvSipBodyHandle hBody = RvSipMsgGetBodyObject (inboundMessage);
					int nStringLen = RvSipBodyGetBodyStrLength (hBody);
					auto szXml = new char[nStringLen + 1];
					szXml[0] = '\0';
					RvSipBodyGetBodyStr (hBody, szXml, nStringLen, (RvUint*)&nStringLen);
					#define PFU_CONST "<picture_fast_update"
					#define PFU_LEN (int)(sizeof(PFU_CONST)-1)

					for (int i = 0; i <= (nStringLen - PFU_LEN); ++i)
					{
						if (0 == strncmp (PFU_CONST, &szXml[i], PFU_LEN))
						{
							bWasHandled = true;
							sipCall->KeyframeSend (estiTRUE);
							RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
							break;
						}
					}
					delete [] szXml;
					szXml = nullptr;
				}
				else if (contentType == SORENSON_CUSTOM_MIME)
				{
					stiHResult hResult = sipCall->SorensonCustomInfoParse (hCallLeg, inboundMessage, false);

					if (!stiIS_ERROR (hResult))
					{
						bWasHandled = true;
						RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
					}
				}
				else if (contentType == CONTACT_MIME || contentType == SPAWN_CALL_MIME)
				{
					if (callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
					{
						SstiSharedContact contact;

						hResult = ContactReceive (inboundMessage, &contact);
						if (!stiIS_ERROR (hResult))
						{
							bWasHandled = true;
							
							if (contentType == SPAWN_CALL_MIME)
							{// If there will be a response, include the message the response was to.
								RvSipTranscHandle spawnCallTransactionHandle;

								rvStatus = RvSipCallLegGetTranscByMsg (hCallLeg, inboundMessage, true, &spawnCallTransactionHandle);
								stiTESTRVSTATUS ();
								
								{
									auto outboundMessage = vpe::sip::Transaction(spawnCallTransactionHandle).outboundMessageGet();

									std::string CallInfo;
									ContactSerialize(contact, &CallInfo);

									hResult = outboundMessage.bodySet(SPAWN_CALL_MIME, CallInfo);
									stiTESTRESULT ();

									// Send a 200 ok
									RvSipCallLegTranscResponse (hCallLeg, spawnCallTransactionHandle, SIPRESP_SUCCESS);
								}

								pThis->hearingCallSpawnSignal.Emit (contact);
							}
							else
							{
								RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
								pThis->contactReceivedSignal.Emit (contact);
							}
						}
					}
				}
				else if (contentType == HEARING_STATUS_MIME)
				{
					// The hearing status has changed find what the status is
					std::string HearingStatus;
					SipInfoMsgReceive(inboundMessage, &HearingStatus);

					// If we got a status see if it's one of the ones we are listening for and update the call properly.
					if (!HearingStatus.empty())
					{
						bWasHandled = true;

						if (HearingStatus == g_szHearingStatusDisconnected)
						{
							sipCall->m_eHearingCallStatus = estiHearingCallDisconnected;
							if (call && !sipCall->dhvSwitchAfterHandoffGet())
							{
								call->dhviStateSet (IstiCall::DhviState::NotAvailable);
								
								if (sipCall->m_dhviCall)
								{
									sipCall->dhviMcuDisconnect (true);
								}
							}
						}
						else if (HearingStatus == g_szHearingStatusConnecting)
						{
							sipCall->m_eHearingCallStatus = estiHearingCallConnecting;
						}
						else if (HearingStatus == g_szHearingStatusConnected)
						{
							sipCall->m_eHearingCallStatus = estiHearingCallConnected;
						}
						else
						{
							// This is not a status we are handling.
							bWasHandled = false;
							stiASSERT (estiFALSE);
						}
						
						if (bWasHandled)
						{
							// If there will be a response, include the message the response was to.
							RvSipTranscHandle hearingStatusTransactionHandle;
							
							rvStatus = RvSipCallLegGetTranscByMsg (hCallLeg, inboundMessage, true, &hearingStatusTransactionHandle);
							stiTESTRVSTATUS ();
							
							{
								auto outboundMessage = vpe::sip::Transaction(hearingStatusTransactionHandle).outboundMessageGet ();

								hResult = outboundMessage.bodySet(HEARING_STATUS_MIME, HearingStatus);
								stiTESTRESULT ();

								// Send a 200 ok
								RvSipCallLegTranscResponse (hCallLeg, hearingStatusTransactionHandle, SIPRESP_SUCCESS);
							}

							// Send a message to the UI so it can update if needed
							pThis->hearingStatusChangedSignal.Emit (call);
						}
					}
				}
				else if (contentType == NEW_CALL_READY_MIME)
				{
					// The interpreters status to receive a new call has changed find out what it is
					std::string NewCallReadyStatus;
					SipInfoMsgReceive(inboundMessage, &NewCallReadyStatus);

					// If we got a status see if it's one of the ones we are listening for and update the call properly.
					if (!NewCallReadyStatus.empty())
					{
						bWasHandled = true;

						if (NewCallReadyStatus == "True")
						{
							sipCall->m_bRelayNewCallReady = true;
						}
						else if (NewCallReadyStatus == "False")
						{
							sipCall->m_bRelayNewCallReady = false;
						}
						else
						{
							// This is not a status we are handling.
							bWasHandled = false;
							stiASSERT (estiFALSE);
						}
						
						if (bWasHandled)
						{
							// If there will be a response, include the message the response was to.
							RvSipTranscHandle newCallTransactionHandle;
							
							rvStatus = RvSipCallLegGetTranscByMsg (hCallLeg, inboundMessage, true, &newCallTransactionHandle);
							stiTESTRVSTATUS ();
							
							{
								auto outboundMessage = vpe::sip::Transaction(newCallTransactionHandle).outboundMessageGet();

								hResult = outboundMessage.bodySet(NEW_CALL_READY_MIME, NewCallReadyStatus);
								stiTESTRESULT ();

								// Send a 200 ok
								RvSipCallLegTranscResponse (hCallLeg, newCallTransactionHandle, SIPRESP_SUCCESS);
							}
						}
					}
				}
				else if (contentType == DISPLAY_SIZE_MIME)
				{
					// Get the display size requested
					std::string displaySize;
					SipInfoMsgReceive(inboundMessage, &displaySize);

					// If we got a display size process it.
					if (!displaySize.empty())
					{
						stiHResult hResult2 = stiRESULT_SUCCESS;

						hResult2 = callLeg->RemoteDisplaySizeProcess (displaySize);

						if (stiIS_ERROR (hResult2))
						{
							stiASSERTMSG(estiFALSE, "Failed to process SIP-INFO display size %s", displaySize.c_str ());
						}
					}
					else
					{
						stiASSERTMSG( estiFALSE, "Received MIME %s with no body.", contentType.c_str());
					}

					bWasHandled = true;

					// Send a 200 ok
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
				}
				else if (contentType == SINFO_MIME)
				{
					std::string sInfo;
					SipInfoMsgReceive(inboundMessage, &sInfo);
					SystemInfoDeserialize (callLeg, sInfo.c_str());

					// Notify the call that SINFO data was changed.
					call->CallInformationChanged ();

					bWasHandled = true;

					// Send a 200 ok
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
				}
				else if (contentType == DHVI_MIME)
				{
					bWasHandled = true;

					std::string message;
					SipInfoMsgReceive(inboundMessage, &message);
					auto dhviMessage = splitString (message, ' ');
					auto dhviCall = sipCall->m_dhviCall;

					if (!dhviMessage.empty() && dhviMessage.size() > 1)
					{
						if (dhviMessage[0] == g_dhviCreate)
						{
							pThis->dhviCreate (dhviMessage[1], "", call, false);

							// Notify VRCL Connecting DHVI
							pThis->dhviConnectingSignal.Emit(dhviMessage[1], "Deaf", call->dhvHearingNumberGet());
							call->dhviStateSet (IstiCall::DhviState::Connecting);
						}
						else if (dhviMessage[0] == g_dhviCapabilityCheck)
						{
							auto hearingNumber = dhviMessage[1];
							pThis->dhviHearingNumberReceived.Emit(call, hearingNumber);
						}
						else if (dhviMessage[0] == g_dhviCapable)
						{
							bool capable = false;
							std::stringstream value (dhviMessage[1]);
							value >> std::boolalpha >> capable;
							pThis->dhviCapableSignal.Emit (capable);
							call->dhviStateSet (capable ? IstiCall::DhviState::Capable : IstiCall::DhviState::NotAvailable);
						}
						else if (dhviCall &&
								!dhviCall->dhvHearingNumberGet().empty())
						{
							int roomCount = 0;
							std::vector<std::string> results;
							auto dhvMessageRegex = sci::make_unique<CstiRegEx>("member-count=\"([0-9]+)\"");

							if (dhvMessageRegex->Match(message, results))
							{
								stiTESTCONDMSG (
									boost::conversion::try_lexical_convert(results[1], roomCount),
									stiRESULT_ERROR,
									"Failed to get Room Count");

								auto dhvSipCall = std::static_pointer_cast<CstiSipCall>(dhviCall->ProtocolCallGet());
								
								if (roomCount >= 3 &&
									dhvSipCall &&
									!dhvSipCall->switchedActiveCallsGet ())
								{
									auto uri = call->RoutingAddressGet ().ResolvedURIGet ();
									std::string callID;
									dhvSipCall->CallIdGet (&callID);

									stiRemoteLogEventSend ("EventType=DHVI Reason=EnteringRoom CallID=%s Room=%s", callID.c_str (), uri.c_str ());
									
									pThis->PostEvent ([dhvSipCall] {
										dhvSipCall->sendDhviSwitchMsg ();
										dhvSipCall->switchActiveCall (true);
										dhvSipCall->CstiCallGet ()->dhvConnectingTimerStop ();
										dhvSipCall->CstiCallGet()->dhviStateSet(IstiCall::DhviState::Connected);
									});
								}
								else if (roomCount >=3 &&
									dhvSipCall->dhvSwitchAfterHandoffGet ())
								{
									pThis->PostEvent ([dhvSipCall] {
										dhvSipCall->sendDhviSwitchMsg ();
									});
									dhvSipCall->dhvSwitchAfterHandoffSet (false);
								}
								else if (roomCount == 2 &&
									dhviCall->dhviStateGet () == IstiCall::DhviState::Connecting)
								{
									std::string displayName;
									pThis->LocalDisplayNameGet (&displayName);

									auto uri = call->RoutingAddressGet ().ResolvedURIGet ();

									// Attempt to format the address to call the Hearing dialplan on the MCU.
									std::string hearingAddr = dhvUriCreate (uri, HEARING_DIALPLAN_URI_HEADER);

									pThis->dhviStartCallReceived.Emit(call,
																	 dhviCall->dhvHearingNumberGet (),
																	 pThis->m_UserPhoneNumbers.szPreferredPhoneNumber,
																	 displayName,
																	 hearingAddr);
								}
							}
						}
					}
					else if (message == g_dhviSwitch)
					{
						if (sipCall)
						{
							sipCall->switchActiveCall (false);

							// Notify VRCL Hearing Video Connected.
							pThis->dhviConnectedSignal.Emit();
							call->dhviStateSet (IstiCall::DhviState::Connected);
						}
					}
					else if (message == g_dhviDisconnect)
					{
						if (sipCall)
						{
							sipCall->dhviMcuDisconnect (false);
						}
					}

					if (bWasHandled)
					{
						// If there will be a response, include the message the response was to.
						RvSipTranscHandle newCallTransactionHandle;

						rvStatus = RvSipCallLegGetTranscByMsg (hCallLeg, inboundMessage, true, &newCallTransactionHandle);
						stiTESTRVSTATUS ();
						
						{
							auto outboundMessage = vpe::sip::Transaction(newCallTransactionHandle).outboundMessageGet ();

							hResult = outboundMessage.bodySet(DHVI_MIME, {});
							stiTESTRESULT ();

							// Send a 200 ok
							RvSipCallLegTranscResponse (hCallLeg, newCallTransactionHandle, SIPRESP_SUCCESS);
						}
					}
				}
				else if (contentType == KEEP_ALIVE_MIME)
				{
					bWasHandled = true;
					
					// Send a 200 ok
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
				}
#ifdef stiHOLDSERVER
				else
				{
					// Check for private SVRS geolocation header in message
					bWasHandled = pThis->privateGeolocationHeaderCheck (inboundMessage, call);

					// On some endpoints, the content type gets filtered out if the body is empty, use the Info-Package header instead.
					std::string header;
					bool present = false;
					std::tie (header, present) = inboundMessage.headerValueGet (INFO_PACKAGE);
					if (present && header == KEEP_ALIVE_PACKAGE)
					{
						bWasHandled = true;

						// Send a 200 ok
						RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_SUCCESS);
					}
				}
#endif

				// Reply "unacceptable" if the message was not handled
				if (!bWasHandled)
				{
					stiASSERTMSG (estiFALSE, "Unsupported INFO message \"%s\"", contentType.c_str());
					RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_NOT_ACCEPTABLE);
				}
			}
			else
			{
				RvSipCallLegTranscResponse (hCallLeg, transaction, SIPRESP_NOT_ACCEPTABLE);
			}

			break;
		}
		
		case RVSIP_CALL_LEG_TRANSC_STATE_SERVER_GEN_FINAL_RESPONSE_SENT:
			break;
			
		case RVSIP_CALL_LEG_TRANSC_STATE_CLIENT_GEN_FINAL_RESPONSE_RCVD:
		{
			if (!callLeg)
			{
				stiASSERTMSG (estiFALSE, "Call object missing.  Hangup call leg.");
				sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
			}
			else
			{
				RvSipBodyHandle hBody = RvSipMsgGetBodyObject (inboundMessage);

				auto contentType = inboundMessage.contentTypeHeaderGet();

				if (contentType == SINFO_MIME)
				{
					// Parse the contained SInfo
					int nStringLen = RvSipMsgGetStringLength (inboundMessage, RVSIP_MSG_BODY);
					auto pszSinfo = new char[nStringLen + 1];

					rvStatus = RvSipBodyGetBodyStr (hBody, pszSinfo, nStringLen, (RvUint*)&nStringLen);

					if (RV_OK == rvStatus)
					{
						// Ensure the string is null-terminated
						pszSinfo[nStringLen] = '\0';

						// Attach the SInfo to the call leg
						callLeg->m_SInfo.assign (pszSinfo);

						if (sipCall == call->ProtocolCallGet ())
						{
							// This is an original call so update the caller id now.
							// Also set the current information so we can do things like blocked caller detection
							SystemInfoDeserialize (callLeg, pszSinfo);
						}
					}
					delete [] pszSinfo;
					pszSinfo = nullptr;
				}
				else if (contentType == SORENSON_CUSTOM_MIME)
				{
					sipCall->SorensonCustomInfoParse (hCallLeg, inboundMessage, true);
				}
				else if (contentType == HEARING_STATUS_MIME)
				{
					pThis->hearingStatusSentSignal.Emit ();
				}
				else if (contentType == NEW_CALL_READY_MIME)
				{
					pThis->newCallReadySentSignal.Emit ();
				}
			}

			break;
		}

		case RVSIP_CALL_LEG_TRANSC_STATE_CLIENT_MSG_SEND_FAILURE:
		{
			RvStatus rvStatus = RV_OK;
			
			//
			// The transaction failed to send.
			// If the failure was due to a 503 response and the transaction was an INVITE then
			// we must ACK the response before terminating the transaction.
			//
			auto method = transaction.methodGet();

			if (RVSIP_TRANSC_REASON_503_RECEIVED == eReason && method == INVITE_METHOD)
			{
				rvStatus = RvSipTransactionAck(transaction, nullptr);
				stiASSERT (rvStatus == RV_OK);
			}

			//
			// Terminate the transaction
			//
			rvStatus = RvSipCallLegTranscTerminate (hCallLeg, transaction);
			stiASSERT (rvStatus == RV_OK);
			
			if (sipCall)
			{
				stiRemoteLogEventSend ("RVSIP_CALL_LEG_TRANSC_STATE_CLIENT_MSG_SEND_FAILURE Reason = %d", eReason);
				sipCall->ReInviteSend (false, 0);
			}

			break;
		}

		UNHANDLED_CASE(RVSIP_CALL_LEG_TRANSC_STATE_IDLE)
		UNHANDLED_CASE(RVSIP_CALL_LEG_TRANSC_STATE_CLIENT_GEN_REQUEST_SENT)
		UNHANDLED_CASE(RVSIP_CALL_LEG_TRANSC_STATE_CLIENT_GEN_PROCEEDING)
		UNHANDLED_CASE(RVSIP_CALL_LEG_TRANSC_STATE_TERMINATED)
		UNHANDLED_CASE(RVSIP_CALL_LEG_TRANSC_STATE_UNDEFINED)
		//default:
			break;
	}

STI_BAIL:

	pThis->Unlock ();

} // end CstiSipManager::CallLegTransactionStateChangeCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegTransactionCreatedCB
//
// Description: This callback is used to determine which specialty messages
//	can/cannot be processed by CallLegMessageCB().
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::CallLegTransactionCreatedCB (
	IN RvSipCallLegHandle hCallLeg, ///< The sip stack call-leg handle
	IN RvSipAppCallLegHandle hAppCallLeg, ///< The application handle for this call-leg. (Aka, CstiCall*)
	IN RvSipTranscHandle hTransaction, ///< The handle to the new transaction
	OUT RvSipAppTranscHandle *hAppTransaction, ///< The application handle to the transaction
	OUT RvBool *bAppHandleTransaction) ///<  RV_TRUE if the application wishes to handle the request. RV_FALSE is the application wants the call-leg to handle the request.
{
	//pThis->Lock (); <-- Uncomment if this method ever accesses the sip manager.

	auto method = vpe::sip::Transaction(hTransaction).methodGet();

	*bAppHandleTransaction = (
		method == UPDATE_METHOD
		|| method == MESSAGE_METHOD
		|| method == INFO_METHOD);

	//pThis->Unlock (); <-- Uncomment if this method ever accesses the sip manager.

} // end CstiSipManager::CallLegTransactionCreatedCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallForkDetectedCB
//
// Description: Called by stack when a call is forked.
//
// Abstract: 
//
// Returns: Nothing.
//
void RVCALLCONV CstiSipManager::CallForkDetectedCB (
	IN RvSipCallLegHandle hNewCallLeg,
	OUT RvSipAppCallLegHandle *phNewAppCallLeg,
	OUT RvBool *pbTerminate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipCallLegHandle hOriginalCallLeg;
	RvSipAppCallLegHandle hOriginalAppCallLeg;
	RvStatus rvStatus = RV_OK;

	//pThis->Lock (); <-- uncomment if this method ever accesses the sip manager

	//
	// Retrieve the SIP call by retrieving the original
	// SIP call leg handle and the oringal application call leg handle,
	// convert the original application call leg handle to the key,
	// look up the original call leg and get the SIP call through
	// the weak pointer.
	//
	RvSipCallLegGetOriginalCallLeg (hNewCallLeg, &hOriginalCallLeg);
	RvSipCallLegGetAppHandle (hOriginalCallLeg, &hOriginalAppCallLeg);
	auto  originalCallLegKey = (size_t)hOriginalAppCallLeg;
	auto originalCallLeg = callLegLookup (originalCallLegKey);
	auto sipCall = originalCallLeg->m_sipCallWeak.lock ();

	//
	// Add a call leg information structure.
	//
	auto newCallLegKey = CstiSipCallLeg::uniqueKeyGenerate ();
	auto newCallLeg = sipCall->CallLegAdd (hNewCallLeg);
	callLegInsert (newCallLegKey, newCallLeg);
	
	//
	// If not using UDP then set the persistency level so the stack
	// uses the connection we have already setup.
	//
	if (sipCall->m_eTransportProtocol != estiTransportUDP)
	{
		RvSipCallLegSetPersistency (hNewCallLeg, RVSIP_TRANSPORT_PERSISTENCY_LEVEL_TRANSC_USER);
	}

	//
	// Use the same outbound settings as the original call object.
	//
	RvSipTransportOutboundProxyCfg proxyCfg;
	memset (&proxyCfg, 0, sizeof (proxyCfg));
	char szProxyIPAddress[stiIPV6_ADDRESS_LENGTH + 1];
	char szProxyHostName[256];
	
	proxyCfg.strIpAddress = (RvChar*)szProxyIPAddress;
	proxyCfg.strHostName = (RvChar*)szProxyHostName;
	
	rvStatus = RvSipCallLegGetOutboundDetails (hOriginalCallLeg, sizeof (proxyCfg), &proxyCfg);
	
	if (rvStatus == RV_OK)
	{
		rvStatus = RvSipCallLegSetOutboundDetails (hNewCallLeg, &proxyCfg, sizeof (proxyCfg));
		stiTESTRVSTATUS ();
	}
	else
	{
		stiASSERT (estiFALSE);
	}

	{
		newCallLeg->m_sdp = originalCallLeg->m_sdp;
		if (sipCall->includeEncryptionAttributes ())
		{
			newCallLeg->dtlsContextSet (std::make_shared<vpe::DtlsContext> (*originalCallLeg->dtlsContextGet ()));
			newCallLeg->dtlsContextGet ()->remoteFingerprintSet ({});
			newCallLeg->dtlsContextGet ()->remoteFingerprintHashFuncSet (vpe::HashFuncEnum::Undefined);
		}
		
		// Copy the local SDP streams
		for (const auto& sdpStream : originalCallLeg->m_localSdpStreams)
		{
			newCallLeg->m_localSdpStreams.emplace_back(new SstiSdpStream(*sdpStream));
		}

		// We need to set answer pending in order for ICE to work properly with forked call legs.
		newCallLeg->m_bAnswerPending = true;
		
		if (originalCallLeg->m_pIceSession)
		{
			hResult = originalCallLeg->m_pIceSession->RelatedSessionStart ((size_t)newCallLegKey, &newCallLeg->m_pIceSession);
			stiTESTRESULT ();
		}
	}

	*phNewAppCallLeg = (RvSipAppCallLegHandle)newCallLegKey;
	*pbTerminate = RV_FALSE;
	
STI_BAIL:

	stiUNUSED_ARG (hResult);

	//pThis->Unlock (); <-- uncomment if this method ever accesses the sip manager
}


/*!\brief Callback called to provide an opportunity to modify the Via
 * 
 * This callback is called to provide an opportunity to modify the via.  We
 * are using it to declare an alias so that the UA server will send requests
 * back on the same connection.  See RFC 5923.
 * 
 */
RvStatus RVCALLCONV CstiSipManager::CallLegFinalDestResolvedCB (
	RvSipCallLegHandle hCallLeg,
	RvSipAppCallLegHandle hAppCallLeg,
	RvSipTranscHandle hTransc,
	RvSipAppTranscHandle hAppTransc,
	RvSipMsgHandle hMsgToSend)
{
	RvStatus rvStatus = RV_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	
	RvSipViaHeaderHandle hVia;
	RvSipHeaderListElemHandle hListElem = nullptr;
	EstiBool bAnchorMedia = estiFALSE;
	
	auto callLegKey = (size_t)hAppCallLeg;
	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	auto pThis = (CstiSipManager*)sipCall->ProtocolManagerGet ();
	auto call = sipCall->CstiCallGet ();
	
	RvSipTransport transportType;
	RvSipTransportAddressType addrType;
	RvChar address[RVSIP_TRANSPORT_LEN_STRING_IP]{};
	uint16_t destPort = 0;
		
	pThis->Lock ();
		
	// Log the address we are sending messages to.
	rvStatus = RvSipTransactionGetCurrentDestAddress(hTransc, &transportType, &addrType, address, &destPort);
	
	if (rvStatus == RV_OK)
	{
		call->remoteDestSet (address);
	}
	else
	{
		// Reset the rvStatus to ensure we don't retun an error preventing a message from being sent like an ACK to the 200 OK.
		// Resolves Bug#30668.
		rvStatus = RV_OK;
	}

	if (RvSipMsgGetMsgType(hMsgToSend) == RVSIP_MSG_REQUEST)
	{
		/* Get the top Via header. */
		hVia = (RvSipViaHeaderHandle)RvSipMsgGetHeaderByType (hMsgToSend, RVSIP_HEADERTYPE_VIA, RVSIP_FIRST_HEADER, &hListElem);
		
		auto pThis = (CstiSipManager*)sipCall->ProtocolManagerGet ();

		// If a bridge call has the URI parameter "ob" then we want the media to be anchored
		if (pThis->TunnelActive () ||
			(sipCall->IsBridgedCall() && call->RoutingAddressGet().hasParameter("ob")))
		{
			bAnchorMedia = estiTRUE;
		}
		
		if (sipCall->SipManagerGet()->IsUsingOutboundProxy (call))
		{
			if (bAnchorMedia == estiFALSE)
			{
				//
				// Set our via address so that it appears we are not NATted.  This should keep SMX from
				// anchoring the media.
				//
				std::string ReflexiveAddress;
				uint16_t un16Port;
				
				// We are attempting to use any reflexive address for any existing registration to prevent
				// an issue where we have our VIA not match our reflexive address and inadvertantly cause media to be anchored.
				if (pThis->IsRegistered ())
				{
					for (auto &registration : pThis->m_Registrations)
					{
						registration.second->ReflexiveAddressGet(&ReflexiveAddress, &un16Port);
						
						if (!ReflexiveAddress.empty ())
						{
							if (ReflexiveAddress[0] != '[' && std::string::npos != ReflexiveAddress.find (':'))
							{
								auto pszTmpBuf = new char[ReflexiveAddress.size () + 3];
								
								sprintf (pszTmpBuf, "[%s]", ReflexiveAddress.c_str ());
								rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)pszTmpBuf);
								delete [] pszTmpBuf;
								pszTmpBuf = nullptr;
								stiTESTRVSTATUS ();
							}
							else
							{
								rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)ReflexiveAddress.c_str ());
								stiTESTRVSTATUS ();
							}
							
							if (un16Port != 0)
							{
								rvStatus = RvSipViaHeaderSetPortNum (hVia, un16Port);
								stiTESTRVSTATUS ();
							}
							
							break;
						}
					}
					
					if (ReflexiveAddress.empty () &&
						!pThis->m_stConferenceParams.PublicIPv4.empty())
					{
						stiASSERTMSG(!ReflexiveAddress.empty (), "CallLegFinalDestResolvedCB failed to find Reflexive address for registered endpoint attempting to use publicIPv4 = %s", pThis->m_stConferenceParams.PublicIPv4.c_str());
						
						ReflexiveAddress = pThis->m_stConferenceParams.PublicIPv4;
					}
					
					stiASSERTMSG(!ReflexiveAddress.empty (), "CallLegFinalDestResolvedCB registered endpoint failed to find reflexive address or public IPv4");
				}
				
				if (ReflexiveAddress.empty ())
				{
					// We are not registered, in order to ensure logged out calls will work we will set our IP
					// to be our local IP. This needs to match the address in our Contact header.
					std::string LocalIp;
					
					if (pThis->m_stConferenceParams.bIPv6Enabled)
					{
						// For Bridge server, get the public IP from config
						if (pThis->m_productType != ProductType::MediaServer && pThis->m_productType != ProductType::MediaBridge)
						{
							stiGetLocalIp(&LocalIp, estiTYPE_IPV6);
						}
						else if (!pThis->m_stConferenceParams.PublicIPv6.empty())
						{
							LocalIp = pThis->m_stConferenceParams.PublicIPv6;
						}
						
						// We need to add the brackets to the address to match our contact header for the call to work.
						 CstiIpAddress IpAddress {LocalIp};
						if (IpAddress.IpAddressTypeGet () == estiTYPE_IPV6)
						{
							LocalIp = IpAddress.AddressGet (CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS);
						}
					}
					else
					{
						// For Bridge server, get the public IP from config
						if (pThis->m_productType != ProductType::MediaServer && pThis->m_productType != ProductType::MediaBridge)
						{
							LocalIp = call->localIpAddressGet();
						}
						else if (!pThis->m_stConferenceParams.PublicIPv4.empty())
						{
							LocalIp = pThis->m_stConferenceParams.PublicIPv4;
						}
					}
					
					rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)LocalIp.c_str ());
					stiTESTRVSTATUS ();
				}
			}
			else
			{
				// Fixes bug #27537 by setting the 'Alias' parameter in the Via header.
				rvStatus = RvSipViaHeaderSetAliasParam (hVia, RV_TRUE);
				stiTESTRVSTATUS ();
			}
		}
		else
		{
			//
			// Add the alias keyword when *not* using a proxy so that the remote endpoint will use
			// the same connection back.
			//
			
			/* Get the transport parameter from the top Via header. */
			auto viaHeader = viaHeaderGet (hMsgToSend);
			
			if (viaHeader.transport != RVSIP_TRANSPORT_UDP)
			{
				/* Sets the 'Alias' parameter in the Via header. */
				rvStatus = RvSipViaHeaderSetAliasParam (hVia, RV_TRUE);
				stiTESTRVSTATUS ();
				
				// Set the alias string in the sent-by part of the Via header
				if (sipCall->m_eIpAddressType == estiTYPE_IPV6 && !pThis->m_stConferenceParams.PublicIPv6.empty ())
				{
					CstiIpAddress address;
					address = pThis->m_stConferenceParams.PublicIPv6;
					auto tmp = address.AddressGet (CstiIpAddress::eFORMAT_IP | CstiIpAddress::eFORMAT_ALWAYS_USE_BRACKETS);
					rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)tmp.c_str ());
					stiTESTRVSTATUS ();
				}
				else
				{
					std::string ipAddress = call->localIpAddressGet();
					stiTESTCOND(!ipAddress.empty(), stiRESULT_ERROR);

					rvStatus = RvSipViaHeaderSetHost (hVia, (RvChar*)ipAddress.c_str());
					stiTESTRVSTATUS ();
				}
	#if 0	
				// TODO: The Radvision example had this, but I don't see why we would need to.
				// It is being left in here as an option to try, should we encounter the need.
				
				// Remove the port from the Via header
				rvStatus = RvSipViaHeaderSetPortNum (hVia, UNDEFINED);
				stiTESTRVSTATUS ();
	#endif
			}
		}
	}

STI_BAIL:

	stiUNUSED_ARG (hResult);
	
	stiASSERTMSG (rvStatus == RV_OK, "CstiSipManager::CallLegFinalDestResolvedCB is returning an error this may break calls");

	pThis->Unlock ();

	return rvStatus;
}
 

////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegPrackStateCB
//
// Description: Called by stack when a callleg-bound prack reponse arrives.
//
// Abstract: Notifies that PRACK transaction was completed. When you call
//	The function RvSipCallLegProvisionalResponseReliable() a
//	reliable provisional response is sent to the destination.
//	The destination should acknowledge that it has received the
//	provisional response by sending a PRACK request. The response
//	to the PRACK is done automatically by the transaction layer.
//	This callback is called after the response is sent suppling
//	you with the response code. Here we send an UPDATE request.
//
// Returns: Nothing.
//
void RVCALLCONV CstiSipManager::CallLegPrackStateCB (
	IN RvSipCallLegHandle hCallLeg, ///< The sip stack call-leg handle
	IN RvSipAppCallLegHandle hAppCallLeg, ///< The Call object for this call-leg.
	IN RvSipCallLegPrackState eState, ///< The PRACK process state.
	IN RvSipCallLegStateChangeReason eReason, ///< The state reason.
	IN RvInt16 prackResponseCode) ///< The response code (if applicable).
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLegKey = (size_t)hAppCallLeg;
	CstiSipCallLegSP callLeg = callLegLookup (callLegKey);
	CstiSipCallSP sipCall = nullptr;
	CstiCallSP call = nullptr;
	CstiSipManager *pThis = nullptr;

	stiTESTCOND (nullptr != callLeg, stiRESULT_ERROR);
	sipCall = callLeg->m_sipCallWeak.lock ();
	stiTESTCOND (nullptr != sipCall, stiRESULT_ERROR);
	call = sipCall->CstiCallGet ();
	stiTESTCOND (nullptr != call, stiRESULT_ERROR);
	pThis = reinterpret_cast<CstiSipManager *> (sipCall->ProtocolManagerGet ());
	stiTESTCOND (nullptr != pThis, stiRESULT_ERROR);

	pThis->Lock ();

	switch (eState)
	{
		case RVSIP_CALL_LEG_PRACK_STATE_REL_PROV_RESPONSE_RCVD:
		{
		    callLeg->m_bWaitingForPrackCompletion = true;

			//
			// We received a reliable provisional response.  Check to see if
			// it contained an SInfo header.  If it did then send our SInfo
			// in an SInfo header back in the prack.  Only do this if the remote
			// endpoint has identified itself as a Sorenson endpoint.
			//
			auto inboundMessage = callLeg->inboundMessageGet();

			callLeg->remoteUANameSet(inboundMessage);

			if (callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
			{
				//
				// This is a sorenson device.  Read the SInfo header from the provisional response
				//
				callLeg->SInfoHeaderRead (inboundMessage);

				//
				// Are we communicating with a Sorenson SIP endpoint with a version 2 SIP implementation or later?
				// If so, then send our SInfo in a header.
				//
				if (callLeg->m_nRemoteSIPVersion >= 2)
				{
					//
					// Yes. Now add the SInfo header to the PRACK
					//
					callLeg->SInfoHeaderAdd ();
				}
			}

			//
			// Check to see if the reliable response contained SDP.  If it has then pass it on to the SDP processor
			// and let it decide how to respond.  If no SDP was recevied then simply respond with
			// a PRACK.
			//
			CstiSdp Sdp;
			Sdp.initialize (inboundMessage);

			if (!Sdp.empty ())
			{
				auto result = callLeg->SdpProcess (&Sdp, estiOfferAnswerReliableResponse);
				if (stiRESULT_CODE (result) == stiRESULT_SECURITY_INADEQUATE)
				{
					call->ResultSet (estiRESULT_SECURITY_INADEQUATE);
					sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE_HERE);
				}
			}
			else
			{
				RvSipCallLegSendPrack (hCallLeg);
			}

			break;
		}

		case RVSIP_CALL_LEG_PRACK_STATE_PRACK_FINAL_RESPONSE_RCVD:
		{
		    callLeg->m_bWaitingForPrackCompletion = false;

			auto inboundMessage = callLeg->inboundMessageGet();

			callLeg->remoteUANameSet(inboundMessage);

			//
			// Check to see if the final PRACK response contained SDP.  If it has then pass it on to the SDP processor
			// and let it decide how to respond.  Only an answer is allowed in this type of message.
			//
			CstiSdp Sdp;
			Sdp.initialize (inboundMessage);

			if (!Sdp.empty ())
			{
				callLeg->SdpAnswerProcess (Sdp, estiOfferAnswerPRACKFinalResponse);
			}

			// It's not a Sorenson, so no waiting for SInfo.
			DBG_MSG ("Created Non-Sorenson:\n\tCstiCall = %p\n\tCstiSipCall=%p\n\tRvSipCallLegHandle=%p",
					 call.get(), sipCall.get(), hCallLeg);

			// Begin the state machine
			if (esmiCS_IDLE == call->StateGet ())
			{
				sipCall->AlertUserCountDownDecr ();
			}
			else if ((esmiCS_CONNECTING == call->StateGet ()) && (estiSUBSTATE_CALLING == call->SubstateGet ()))
			{
				// We're dialing a call / waiting for the remote side to answer.
				pThis->NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_WAITING_FOR_REMOTE_RESP);
			}

			break;
		}
			
		case RVSIP_CALL_LEG_PRACK_STATE_PRACK_RCVD:
		{
			auto inboundMessage = callLeg->inboundMessageGet();

			//
			// We received a prack request.  Check to see if
			// it contained an SInfo header.
			// If the remote device is a Sorenson device it's important
			// that we process SInfo before alerting the user of the call.
			//
			callLeg->remoteUANameSet(inboundMessage);

			if (callLeg->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))
			{
				//
				// This is a sorenson device.  Read the SInfo header from the provisional response
				//
				callLeg->SInfoHeaderRead (inboundMessage);
			}

			//
			// Check to see if the PRACK contained SDP.  If it has then pass it on to the SDP processor
			// and let it decide how to respond.  If no SDP was recevied then simply respond with
			// success
			//
			CstiSdp Sdp;
			Sdp.initialize (inboundMessage);

			if (!Sdp.empty ())
			{
				callLeg->SdpOfferProcess (&Sdp, estiOfferAnswerPRACK);
			}
			else
			{
				RvSipCallLegSendPrackResponse (hCallLeg, SIPRESP_SUCCESS);
			}
			
			// Check if we want to alert the user of the incoming call now.
			if (sipCall->AlertLocalUserGet ())
			{
				sipCall->AlertLocalUserSet (false);
				
				sipCall->AlertLocalUser ();
			}

			break;
		}
		
		case RVSIP_CALL_LEG_PRACK_STATE_PRACK_FINAL_RESPONSE_SENT:
		{
		    callLeg->m_bWaitingForPrackCompletion = false;

			//
			// The current PRACK operation is complete.  If this call
			// leg was accepted then call the Accept method to
			// send the final 200 OK.
			//
			if (callLeg->m_bCallLegAccepted)
			{
				sipCall->Accept ();
			}

			break;
		}

		UNHANDLED_CASE (RVSIP_CALL_LEG_PRACK_STATE_PRACK_SENT)
		UNHANDLED_CASE (RVSIP_CALL_LEG_PRACK_STATE_UNDEFINED)
	}

	pThis->Unlock ();

STI_BAIL:
	stiASSERT (!stiIS_ERROR (hResult));
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegProvisionalResponseReceivedCB
//
// Description: Called by stack when a callleg-bound provisional response arrives.
//
// Abstract: Notifies that PRACK transaction was completed. When you call
//	the function RvSipCallLegProvisionalResponseReliable() a
//	reliable provisional response is sent to the destination.
//	The destination should acknowledge that it has received the
//	provisional response by sending a PRACK request. The response
//	to the PRACK is done automatically by the transaction layer.
//	This callback is called after the response is sent supplying
//	you with the response code. Here we send an UPDATE request.
//
// Returns: Nothing.
//
void RVCALLCONV CstiSipManager::CallLegProvisionalResponseReceivedCB (
	RvSipCallLegHandle hCallLeg,
	RvSipAppCallLegHandle hAppCallLeg,
	RvSipTranscHandle hTransc,
	RvSipAppTranscHandle hAppTransc,
	RvSipCallLegInviteHandle hCallInvite,
	RvSipAppCallLegInviteHandle hAppCallInvite,
	RvSipMsgHandle hInboundMsg)
{	
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLegKey = (size_t)hAppCallLeg;
	CstiSipCallLegSP callLeg = callLegLookup (callLegKey);
	CstiSipCallSP sipCall = nullptr;
	CstiCallSP call = nullptr;
	CstiSipManager *pThis = nullptr;

	stiTESTCOND (nullptr != callLeg, stiRESULT_ERROR);
	sipCall = callLeg->m_sipCallWeak.lock ();
	stiTESTCOND (nullptr != sipCall, stiRESULT_ERROR);
	call = sipCall->CstiCallGet ();
	stiTESTCOND (nullptr != call, stiRESULT_ERROR);
	pThis = reinterpret_cast<CstiSipManager *> (sipCall->ProtocolManagerGet ());
	stiTESTCOND (nullptr != pThis, stiRESULT_ERROR);

	pThis->Lock ();
	{
		vpe::sip::Message inboundMessage (callLeg, hInboundMsg);
		callLeg->remoteUANameSet (inboundMessage);

		int nStatusCode = inboundMessage.statusCodeGet ();

		//
		// If the provisional response was a "Trying" then ignore it.
		// Anything else should be handled.
		//
		if (SIPRESP_TRYING != nStatusCode)
		{
			if (SIPRESP_RINGING == nStatusCode)
			{
				// Need to pass ringing notification up to higher levels
				if (call->StateValidate (esmiCS_CONNECTING | esmiCS_IDLE))
				{
					pThis->NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_WAITING_FOR_REMOTE_RESP);
				}
			}

			auto callLeg = sipCall->CallLegGet (hCallLeg);
			if (!callLeg)
			{
				stiASSERTMSG (estiFALSE, "Call object missing.  Hangup call leg.");
				sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
			}
			else
			{
				callLeg->AllowCapabilitiesDetermine (inboundMessage);
				callLeg->remoteUANameSet (inboundMessage);
			}
		}
	}
			
	pThis->Unlock ();

STI_BAIL:
	stiASSERT (!stiIS_ERROR (hResult));
} // end CstiSipManager::CallLegProvisionalResponseReceivedCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegMessageCB
//
// Description: Called by stack when a callleg-bound message arrives.
//
// Abstract:
//
// Returns: RV_OK to continue processing the message
//
RvStatus CstiSipManager::CallLegMessageCB (
	IN RvSipCallLegHandle hCallLeg,
	IN RvSipAppCallLegHandle hAppCallLeg,
	IN RvSipMsgHandle hMsg
)
{
	RvStatus eContinue = RV_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	vpe::sip::Message inboundMessage(hMsg);
	auto viaHeader = viaHeaderGet (hMsg);
	auto callLegKey = (size_t)hAppCallLeg;
	CstiSipCallLegSP callLeg = callLegLookup (callLegKey);
	CstiSipCallSP sipCall = nullptr;
	CstiCallSP call = nullptr;
	CstiSipManager *sipMgr = nullptr;

	stiTESTCOND (nullptr != callLeg, stiRESULT_ERROR);
	sipCall = callLeg->m_sipCallWeak.lock ();
	stiTESTCOND (nullptr != sipCall, stiRESULT_ERROR);
	call = sipCall->CstiCallGet ();
	stiTESTCOND (nullptr != call, stiRESULT_ERROR);
	sipMgr = reinterpret_cast<CstiSipManager *> (sipCall->ProtocolManagerGet ());
	stiTESTCOND (nullptr != sipMgr, stiRESULT_ERROR);
	
	// Watch for a MESSAGE message coming in on the call leg.
	if (inboundMessage.methodGet() == vpe::sip::Message::Method::Message)
	{
		auto contentType = inboundMessage.contentTypeHeaderGet();

		if (contentType == NUDGE_MIME)
		{
			// get the xml
			RvSipBodyHandle hBody = RvSipMsgGetBodyObject (hMsg);
			int nLength = RvSipBodyGetBodyStrLength (hBody);
			auto szMessage = new char[nLength + 1];
			RvSipBodyGetBodyStr (hBody, szMessage, nLength, (RvUint*)&nLength);
			szMessage[nLength] = '\0';
			for (int i = 0; szMessage[i]; i++)
			{
				if (0 == strncmp (&szMessage[i], "<poke", 5))
				{
					// Flash the light ring
					DBG_MSG ("Got lightring nudge request.");
					if (call->StateValidate (esmiCS_CONNECTED))
					{
						sipMgr->flashLightRingSignal.Emit ();
					}
					break;
				}
			}
			delete [] szMessage;
			szMessage = nullptr;
		}
		else
		{
			stiASSERTMSG (estiFALSE, "Received SIP MESSAGE of unknown data type \"%s\"", contentType.c_str());
		}
	}
	
	/* Fixes bug# 28655 If a new connection is made in the middle of a call we need
	 to send a Re-INVITE to refresh the remote target */
	if (!viaHeader.receivedAddr.empty () && viaHeader.rPort != UNDEFINED)
	{
		if ((!callLeg->receivedAddrGet().empty () && callLeg->receivedAddrGet() != viaHeader.receivedAddr) ||
			(callLeg->rportGet () != UNDEFINED && callLeg->rportGet () != viaHeader.rPort))
		{
			std::string callId;
			call->CallIdGet(&callId);
			
			stiRemoteLogEventSend("EventType=CallConnectionChange CallLegReflexive=%s:%d ViaReceieved=%s:%d CallID=%s",
								  callLeg->receivedAddrGet().c_str(), callLeg->rportGet (),
								  viaHeader.receivedAddr.c_str(), viaHeader.rPort, callId.c_str ());
			sipMgr->ReInviteSend (call, 0, false);
			
			// If we were registered and this happened we should probably ensure we re-register for future calls.
			if (sipMgr->IsRegistered ())
			{
				sipMgr->sipRegisterNeededSignal.Emit ();
			}
		}
		
		callLeg->receivedAddrSet (viaHeader.receivedAddr);
		callLeg->rPortSet (viaHeader.rPort);
	}
	
STI_BAIL:
	stiASSERT (!stiIS_ERROR (hResult));
	return eContinue;
} // end CstiSipManager::CallLegMessageCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegCreatedCB
//
// Description: Called by stack when a new call object is desired.
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::CallLegCreatedCB (
	IN RvSipCallLegHandle hCallLeg,
	OUT RvSipAppCallLegHandle *phAppCallLeg)
{
#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
		DebugTools::instanceGet()->DebugOutput("vpEngine::CstiSipManager::CallLegCreatedCB() - ...SIP - Start"); //...
#endif
	CstiSipManager *pThis = nullptr;
	RvSipCallLegMgrHandle hCallLegMgr = nullptr;

	RvSipCallLegGetCallLegMgr (hCallLeg, &hCallLegMgr);
	RvSipCallLegMgrGetAppMgrHandle (hCallLegMgr, (void**)(void *)&pThis);

	pThis->Lock ();

	auto sipCall = std::make_shared<CstiSipCall>(&pThis->m_stConferenceParams, pThis, pThis->m_vrsFocusedRouting);
	sipCall->CallValidSet (false); // This call is not yet "valid" until the INVITE is verified

	auto call = std::make_shared<CstiCall> (
		pThis->m_pCallStorage,
		pThis->m_UserPhoneNumbers,
		pThis->m_userId,
		pThis->m_groupUserId,
		pThis->m_displayName,
		pThis->m_alternateName,
		pThis->m_vrsCallId,
		&pThis->m_stConferenceParams);

	// Prevent the call object from being destructed by storing a shared pointer to itself
	// We will remove this reference when the call leg reaches the terminated state.  This was
	// a compromise to get around Radvision's C API, which breaks shared pointer semantics.
	call->disallowDestruction ();

	call->ProtocolCallSet (sipCall);
	call->DirectionSet (estiINCOMING);
	
	pThis->m_pCallStorage->store (call);

	auto callLegKey = CstiSipCallLeg::uniqueKeyGenerate ();
	RvSipCallLegSetAppHandle (hCallLeg, (RvSipAppCallLegHandle)callLegKey);
	
	sipCall->StackCallHandleSet (hCallLeg);
	auto callLeg = sipCall->CallLegAdd (hCallLeg);
	callLegInsert (callLegKey, callLeg);

	auto inboundMessage = callLeg->inboundMessageGet();
	callLeg->remoteUANameSet(inboundMessage);
	
	CallIDSet (sipCall, hCallLeg);

	*phAppCallLeg = (RvSipAppCallLegHandle)callLegKey;
	
	pThis->Unlock ();

} // end CstiSipManager::CallLegCreatedCB


/*!\brief Prints the call leg state change reason
 * 
 * \param eReason The reason to print
 */
std::string callLegStateChangeReasonGet (
	RvSipCallLegStateChangeReason eReason)
{
	std::string caseString;
	
	switch (eReason)
	{
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_DISCONNECT_REMOTE_REJECT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_REJECT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_DISCONNECTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECT_REQUESTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_REJECT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_ACCEPT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_DISCONNECTING)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_CANCELED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REQUEST_FAILURE)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_SERVER_FAILURE)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_GLOBAL_FAILURE)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_FAILURE)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_NETWORK_ERROR)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_503_RECEIVED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_OUT_OF_RESOURCES)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_TIME_OUT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_UNDEFINED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_INVITING)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_INVITING)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_REFER)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_REFER)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_REFER_NOTIFY)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_REFER_NOTIFY)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_ACCEPTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_ACCEPTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_ACK)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REDIRECTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_CALL_TERMINATED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_AUTH_NEEDED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_UNSUPPORTED_AUTH_PARAMS)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_LOCAL_CANCELLING)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_ACK_SENT)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_CALL_CONNECTED)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_PROVISIONAL_RESP)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_REFER_REPLACES)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_REMOTE_INVITING_REPLACES)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_GIVE_UP_DNS)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_CONTINUE_DNS)
		GET_CASE_STRING (RVSIP_CALL_LEG_REASON_FORKED_CALL_NO_FINAL_RESPONSE)
	}

	return caseString;
}


////////////////////////////////////////////////////////////////////////////////
//; PrintCallState
//
// Description: Utility for printing out info about call state changes
//
// Abstract:
//
// Returns: Nothing
//
void PrintCallState (
    CstiCallSP call, /// The call that is changing
	RvSipCallLegState eState, /// The state we are changing to
	RvSipCallLegStateChangeReason eReason) /// The reason why
{
	std::string caseString;

	switch (eState)
	{
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_UNDEFINED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_IDLE)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_INVITING)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_REDIRECTED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_UNAUTHENTICATED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_OFFERING)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_ACCEPTED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_CONNECTED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_DISCONNECTED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_DISCONNECTING)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_TERMINATED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_REMOTE_ACCEPTED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_CANCELLED)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_CANCELLING)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_PROCEEDING)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_PROCEEDING_TIMEOUT)
		GET_CASE_STRING(RVSIP_CALL_LEG_STATE_MSG_SEND_FAILURE)
	}

	vpe::trace (call->CallIndexGet (), ": Call switched to ", caseString, ". ", callLegStateChangeReasonGet (eReason), "\n");
} // end PrintCallState


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegReInviteCB
//
// Description: Sip callback for handling re-invite state events.
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::CallLegReInviteCB (
	RvSipCallLegHandle hCallLeg, ///< The handle to the SIP Stack CallLeg object.
	RvSipAppCallLegHandle hAppCallLeg, ///< The application handle for this CallLeg.
	RvSipCallLegInviteHandle hReInvite, ///< The handle to the SIP Stack ReInvite object.
	RvSipAppCallLegInviteHandle hAppReInvite, ///< The application handle for this ReInvite.
	RvSipCallLegModifyState eModifyState, ///< The new state of the CallLeg.
	RvSipCallLegStateChangeReason eReason)  ///< The reason for the state change.
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto  callLegKey = (size_t)hAppCallLeg;
	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	auto pThis = (CstiSipManager*)sipCall->ProtocolManagerGet ();
	pThis->Lock ();

#if 0
	if (NULL == sipCall)
	{
		stiASSERTMSG (estiFALSE, "No reinvites allowed for dropped call leg");
		RvSipCallLegReject (hCallLeg, SIPRESP_NOT_FOUND);
	}
	else
#endif
	{
		// This event keeps the connection alive as well as anything, so reset the timer
		if (pThis->IsUsingOutboundProxy (call))
		{
			pThis->ProxyConnectionKeepaliveStart ();
		}
		else
		{
			sipCall->ConnectionKeepaliveStart ();
		}

		switch (eModifyState)
		{
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_REMOTE_ACCEPTED:
			{
				//
				// The 200 OK response was received.
				//
				auto inboundMessage = callLeg->inboundMessageGet();
				callLeg->remoteUANameSet(inboundMessage);
				callLeg->AllowCapabilitiesDetermine (inboundMessage);

				// 
				// Cancel the lost connection timer if there was one.
				//
				sipCall->LostConnectionTimerCancel ();
				sipCall->ReinviteCancelTimerCancel ();

				//
				// If the 200 OK response contains SDP then process it.
				//
				CstiSdp Sdp;
				Sdp.initialize (inboundMessage);

				if (!Sdp.empty ())
				{
					callLeg->SdpProcess (&Sdp, estiOfferAnswerINVITEFinalResponse);
				}
//				else
				{
					RvSipCallLegReInviteAck (hCallLeg, hReInvite);
				}

				break;
			}

			case RVSIP_CALL_LEG_MODIFY_STATE_ACK_RCVD:
			{
				//
				// The ACK was received.
				//
				SIP_DBG_EVENT_SEPARATORS("Got an ACK to our REINVITE OK(cseq %d)",
									GetCseq (hCallLeg)
				);
			
				CstiSdp Sdp;

				//
				// Get the inbound message and see if there is SDP.  If SDP is present
				// then process it.
				//
				auto inboundMessage = callLeg->inboundMessageGet();
			
				hResult = Sdp.initialize (inboundMessage);
				stiTESTRESULT ();
			
				//
				// If the ACK request contains SDP this can only be an answer.
				// Therefore, call SdpAnswerProcess.
				//
				if (!Sdp.empty ())
				{
					callLeg->SdpAnswerProcess (Sdp, estiOfferAnswerACK);
				}
			
				break;
			}
			
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_RESPONSE_RCVD:
			{
				bool bCancelTimer = true;
				stiDEBUG_TOOL (g_stiSipMgrDebug,
						vpe::trace("Call Leg Reason: ", callLegStateChangeReasonGet (eReason), "\n");
				);

				switch (eReason)
				{
					case RVSIP_CALL_LEG_REASON_REMOTE_ACCEPTED:
				
						break;
					
					case RVSIP_CALL_LEG_REASON_REQUEST_FAILURE:
					{
						bCancelTimer = false;
						auto inboundMessage = callLeg->inboundMessageGet();

						if (!inboundMessage)
						{
							stiASSERTMSG (estiFALSE, "No inbound message");
						}
						else
						{
							// Check the received message to see if the status code is 491 (indicating that an invite
							// was detected inside of an invite.
							int nResponseCode = inboundMessage.statusCodeGet();

							switch (nResponseCode)
							{
								case SIPRESP_AUTHENTICATION_REQD:
								case SIPRESP_AUTHENTICATION_REQD_PROXY:

									//
									// Authentication procedures are handled in the RVSIP_CALL_LEG_MODIFY_STATE_ACK_SENT case.
									//
									break;

								case SIPRESP_REINVITE_INSIDE_REINVITE:

								    sipCall->InviteInsideOfReInviteCheck ();

									break;

								default:
									//
									// We received an error while detecting a lost connection.
									// End the call.
									//
								    if (sipCall->DetectingLostConnectionGet ())
									{
										sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_NONE);
									}
							}
						}
						
						break;
					}
				
					case RVSIP_CALL_LEG_REASON_SERVER_FAILURE:
					case RVSIP_CALL_LEG_REASON_GLOBAL_FAILURE:
					case RVSIP_CALL_LEG_REASON_LOCAL_FAILURE:
					case RVSIP_CALL_LEG_REASON_NETWORK_ERROR:
					case RVSIP_CALL_LEG_REASON_503_RECEIVED:
					case RVSIP_CALL_LEG_REASON_OUT_OF_RESOURCES:
					case RVSIP_CALL_LEG_REASON_LOCAL_TIME_OUT:
					case RVSIP_CALL_LEG_REASON_UNDEFINED:
					case RVSIP_CALL_LEG_REASON_LOCAL_INVITING:
					case RVSIP_CALL_LEG_REASON_REMOTE_INVITING:
					case RVSIP_CALL_LEG_REASON_LOCAL_REFER:
					case RVSIP_CALL_LEG_REASON_REMOTE_REFER:
					case RVSIP_CALL_LEG_REASON_LOCAL_REFER_NOTIFY:
					case RVSIP_CALL_LEG_REASON_REMOTE_REFER_NOTIFY:
					case RVSIP_CALL_LEG_REASON_LOCAL_ACCEPTED:
					case RVSIP_CALL_LEG_REASON_REMOTE_ACK:
					case RVSIP_CALL_LEG_REASON_REDIRECTED:
					case RVSIP_CALL_LEG_REASON_LOCAL_REJECT:
					case RVSIP_CALL_LEG_REASON_LOCAL_DISCONNECTING:
					case RVSIP_CALL_LEG_REASON_DISCONNECTED:
					case RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECTED:
					case RVSIP_CALL_LEG_REASON_CALL_TERMINATED:
					case RVSIP_CALL_LEG_REASON_AUTH_NEEDED:
					case RVSIP_CALL_LEG_REASON_UNSUPPORTED_AUTH_PARAMS:
					case RVSIP_CALL_LEG_REASON_LOCAL_CANCELLING:
					case RVSIP_CALL_LEG_REASON_REMOTE_CANCELED:
					case RVSIP_CALL_LEG_REASON_ACK_SENT:
					case RVSIP_CALL_LEG_REASON_CALL_CONNECTED:
					case RVSIP_CALL_LEG_REASON_REMOTE_PROVISIONAL_RESP:
					case RVSIP_CALL_LEG_REASON_REMOTE_REFER_REPLACES:
					case RVSIP_CALL_LEG_REASON_REMOTE_INVITING_REPLACES:
					case RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECT_REQUESTED:
					case RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_REJECT:
					case RVSIP_CALL_LEG_REASON_DISCONNECT_REMOTE_REJECT:
					case RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_ACCEPT:
					case RVSIP_CALL_LEG_REASON_GIVE_UP_DNS:
					case RVSIP_CALL_LEG_REASON_CONTINUE_DNS:
					case RVSIP_CALL_LEG_REASON_FORKED_CALL_NO_FINAL_RESPONSE:

						//call->ResultSet (estiRESULT_LOST_CONNECTION);
					    //sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_NONE);
						stiASSERTMSG (estiFALSE, "reINVITE unexpected response (%d).", eReason);
						break;
				}

				if (callLeg->m_bAnswerPending)
				{
					callLeg->m_bAnswerPending = false;
				}

				if (bCancelTimer)
				{
					// If we are not resending the ReInvite due to a challange and
					// if a timer was set for a LostConnectionCheck, cancel it.
					sipCall->LostConnectionTimerCancel ();
				}

				break;
			}

			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_RCVD:  // We got a re-invite from them
			{
				CstiSdp Sdp;

				auto inboundMessage = callLeg->inboundMessageGet();

				callLeg->AllowCapabilitiesDetermine (inboundMessage);

				hResult = Sdp.initialize (inboundMessage);
				stiTESTRESULT ();

				hResult = callLeg->SdpOfferProcess (&Sdp, estiOfferAnswerINVITE);
				if (stiRESULT_CODE (hResult) == stiRESULT_SECURITY_INADEQUATE)
				{
					sipCall->CstiCallGet ()->ResultSet (estiRESULT_SECURITY_INADEQUATE);
					sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE_HERE);
				}
				

#ifdef stiHOLDSERVER
				
				// Process potential private SVRS geolocation header in reinvite
				pThis->privateGeolocationHeaderCheck (inboundMessage, call);

#endif

				break;
			}
	
			case RVSIP_CALL_LEG_MODIFY_STATE_ACK_SENT:
				
				if (eReason == RVSIP_CALL_LEG_REASON_AUTH_NEEDED)
				{
					DBG_MSG ("Resending re-invite because authentication is needed.\n");
					pThis->ReInviteSend(call, 0, false);
				}
				
				break;
				
			case RVSIP_CALL_LEG_MODIFY_STATE_MSG_SEND_FAILURE:
			{
				RvStatus rvStatus = RV_OK;

				if (callLeg->m_bAnswerPending)
				{
					callLeg->m_bAnswerPending = false;
				}

				//
				// The re-invite failed to send.
				// If the failure was due to a 503 response then
				// we must ACK the response before terminating the re-invite.
				//
				if (RVSIP_CALL_LEG_REASON_503_RECEIVED == eReason)
				{
					rvStatus = RvSipCallLegReInviteAck (hCallLeg, hReInvite);
					stiASSERT (rvStatus == RV_OK);
				}

				//
				// Terminate the re-invite
				//
				rvStatus = RvSipCallLegReInviteTerminate (hCallLeg, hReInvite);
				stiASSERT (rvStatus == RV_OK);

				break;
			}

			case RVSIP_CALL_LEG_MODIFY_STATE_UNDEFINED:
			case RVSIP_CALL_LEG_MODIFY_STATE_IDLE:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_RESPONSE_SENT:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_SENT:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_CANCELLING:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_PROCEEDING:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_CANCELLED:
			case RVSIP_CALL_LEG_MODIFY_STATE_REINVITE_PROCEEDING_TIMEOUT:
			case RVSIP_CALL_LEG_MODIFY_STATE_TERMINATED:
				break;
		}
	}

STI_BAIL:

	pThis->Unlock ();
}


/*!\brief Retrieves the IP address in the Sent-By portion of the Via header.
 * 
 * If there is no available ip address then the buffer will be set to an empty string
 * 
 * \param hInboundMsg The inbound message handle
 * \param pRemoteIpAddress The buffer to recieve the IP address
 */
stiHResult RemoteIpAddressGet (
	RvSipMsgHandle hInboundMsg,
	std::string *pRemoteIpAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool ipIsGood = false;

	auto viaHeader = viaHeaderGet (hInboundMsg);

	ipIsGood = IPAddressValidate (viaHeader.hostAddr.c_str ());
	stiTESTCOND (ipIsGood, stiRESULT_ERROR);

	pRemoteIpAddress->assign (viaHeader.hostAddr);

STI_BAIL:

	return hResult;
}


/*!\brief Uses the ICE stack to gather transports
 * 
 * This should be used when the remote endpoint is not ICE but
 * we are using NAT traversal and need to gather either relay
 * or server reflexive transports
 * 
 * \param sipCall The SIP call that represents the call leg.
 * 
 */
stiHResult CstiSipManager::TransportsGather (
    CstiSipCallLegSP callLeg)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvAddress AudioRtp;
	RvAddress AudioRtcp;
	RvAddress TextRtp;
	RvAddress TextRtcp;
	RvAddress VideoRtp;
	RvAddress VideoRtcp;
	CstiSipCallSP sipCall = callLeg->m_sipCallWeak.lock ();

	// Select ipv4 or ipv6 for transports
	std::string *pIpAddress = &m_stConferenceParams.PublicIPv4;
	int nRvAddressType = RV_ADDRESS_TYPE_IPV4;
	
	if (sipCall->m_eIpAddressType == estiTYPE_IPV6 && !m_stConferenceParams.PublicIPv6.empty ())
	{
		pIpAddress = &m_stConferenceParams.PublicIPv6;
		nRvAddressType = RV_ADDRESS_TYPE_IPV6;
	}

	//
	// We need to setup some dummy ports so we can build an SDP to
	// pass into the ICE manager to start the process.  Use the local
	// IP address and the ports from the bottom of the port range.
	//
	RvAddressConstruct (nRvAddressType, &AudioRtp);
	RvAddressConstruct (nRvAddressType, &AudioRtcp);
	RvAddressConstruct (nRvAddressType, &TextRtp);
	RvAddressConstruct (nRvAddressType, &TextRtcp);
	RvAddressConstruct (nRvAddressType, &VideoRtp);
	RvAddressConstruct (nRvAddressType, &VideoRtcp);

	RvAddressSetString (pIpAddress->c_str (), &AudioRtp);
	RvAddressSetString (pIpAddress->c_str (), &AudioRtcp);
	RvAddressSetString (pIpAddress->c_str (), &TextRtp);
	RvAddressSetString (pIpAddress->c_str (), &TextRtcp);
	RvAddressSetString (pIpAddress->c_str (), &VideoRtp);
	RvAddressSetString (pIpAddress->c_str (), &VideoRtcp);
	
	RvAddressSetIpPort (&AudioRtp, RV_PORTRANGE_DEFAULT_START);
	RvAddressSetIpPort (&AudioRtcp, RV_PORTRANGE_DEFAULT_START + 1);
	RvAddressSetIpPort (&TextRtp, RV_PORTRANGE_DEFAULT_START + 2);
	RvAddressSetIpPort (&TextRtcp, RV_PORTRANGE_DEFAULT_START + 3);
	RvAddressSetIpPort (&VideoRtp, RV_PORTRANGE_DEFAULT_START + 4);
	RvAddressSetIpPort (&VideoRtcp, RV_PORTRANGE_DEFAULT_START + 5);
	
	//
	// Tell the SIP call about these default addresses/ports so that when
	// the SDP is created it will use them.
	//
	sipCall->DefaultAddressesSet (
		&AudioRtp, &AudioRtcp, 
		&TextRtp, &TextRtcp,
		&VideoRtp, &VideoRtcp);
	
	//
	// Construct the SDP
	//
	callLeg->SdpCreate (&callLeg->m_sdp, estiAnswer);
	stiTESTCOND (!callLeg->m_sdp.empty (), stiRESULT_ERROR);
	
	//
	// Now start the ICE session just to gather the transports.  When the gathering is complete
	// we will grab the default candidates and shutdown the ICE session.
	//
	
	hResult = IceSessionStart (CstiIceSession::estiICERoleGatherer, callLeg->m_sdp, nullptr, callLeg);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; StrippedNumberCreate
//
// Description: Given a string representation of a phone number, creates a new
//	string with just the digits.  If the given string contains invalid characters,
//	thins function will return NULL.
//
// Returns: null or new string (caller must delete [] it)
//
char *StrippedNumberCreate (const char *pszPhoneNumber)
{
	char *pszStripped = nullptr;
	
	if (pszPhoneNumber && *pszPhoneNumber)
	{
		pszStripped = new char[strlen (pszPhoneNumber) + 1];
		int j = 0;
		for (int i = 0; pszPhoneNumber[i]; i++)
		{
			if (pszPhoneNumber[i] >= '0' && pszPhoneNumber[i] <= '9')
			{
				// Save only digits in the stripped string
				pszStripped[j++] = pszPhoneNumber[i];
			}
			else if (pszPhoneNumber[i] != '-' && pszPhoneNumber[i] != '+' && pszPhoneNumber[i] != '(' && pszPhoneNumber[i] != ')' && pszPhoneNumber[i] != ' ' && pszPhoneNumber[i] != '\t')
			{
				// Invalid character, so not a phone number
				delete [] pszStripped;
				pszStripped = nullptr;
				break;
			}
		}
		if (pszStripped)
		{
			// Null terminate the string
			pszStripped[j] = '\0';
		}
	}
	
	return pszStripped;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::PhoneNumberVerify
//
// Description: Determine whether the phone number passed in matches any of ours
//
// Returns: bool
//
bool CstiSipManager::PhoneNumberVerify (const char *pszPhoneNumber)
{
	bool bMatched = true;
	
#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER) && (APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL)
	char *pszStripped = StrippedNumberCreate (pszPhoneNumber);
	if (pszStripped)
	{
		if (*pszStripped)
		{
			bMatched = false;
			char szPhoneNumber[PHONE_NUMBER_LENGTH + 1];
			int nLen = strlen (pszStripped);
			if (nLen > 3 && nLen <= PHONE_NUMBER_LENGTH)
			{
				// If they are dialing within area code, etc, add missing digits
				if (nLen != PHONE_NUMBER_LENGTH)
				{
					strcpy (szPhoneNumber, m_UserPhoneNumbers.szLocalPhoneNumber);
				}
				strcpy (&szPhoneNumber[PHONE_NUMBER_LENGTH - nLen], pszStripped);
				
				// Compare against our numbers
				if (
					(0 == strcmp (szPhoneNumber, m_UserPhoneNumbers.szLocalPhoneNumber)) ||
					(0 == strcmp (szPhoneNumber, m_UserPhoneNumbers.szRingGroupLocalPhoneNumber)) ||
					(0 == strcmp (szPhoneNumber, m_UserPhoneNumbers.szTollFreePhoneNumber)) ||
					(0 == strcmp (szPhoneNumber, m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber)) ||
					(0 == strcmp (szPhoneNumber, m_UserPhoneNumbers.szSorensonPhoneNumber))
				)
				{
					bMatched = true;
				}
			}
		}

		delete [] pszStripped;
		pszStripped = nullptr;
	}
	
#endif // stiHOLDSERVER
	return bMatched;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegContactAttach
//
// Description: Attaches the correct Contact header to a new call leg.
//
// Returns: stiHResult
//
//#define USE_PROXY_AS_CONTACT 1  // whether to use the proxy address (instead of public ip) in the contact address
stiHResult CstiSipManager::CallLegContactAttach (
	RvSipCallLegHandle hCallLeg) /// The associated sip call (with reflexive port information)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	std::string address;
	std::string sipAddress;
	RvSipAddressHandle hContactAddress = nullptr;
	const char *pszPhoneNumber = m_UserPhoneNumbers.szPreferredPhoneNumber;
	CstiCallSP call = nullptr;
	size_t callLegKey = 0;

	std::string IpAddress;
	auto port = LocalListenPortGet ();
	EstiTransport eTransport = estiTransportTCP;

	RvSipCallLegGetAppHandle (hCallLeg, (RvSipAppCallLegHandle*)&callLegKey);

	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	if (sipCall)
	{
		call = sipCall->CstiCallGet ();
	}

	// For Bridge server, get the public IP from config
	if (m_productType != ProductType::MediaServer && 
		m_productType != ProductType::MediaBridge && 
		m_productType != ProductType::HoldServer)
	{
		if (call)
		{
			IpAddress = call->localIpAddressGet();
		}
		else
		{
			IpAddress = IstiNetwork::InstanceGet()->localIpAddressGet(m_stConferenceParams.bIPv6Enabled ? estiTYPE_IPV6 : estiTYPE_IPV4, {});
		}
	}
	else if (!m_stConferenceParams.PublicIPv4.empty())
	{
		IpAddress = m_stConferenceParams.PublicIPv4;
	}
	
	// If this is not a bridge call and we have a reflexive port use that over the local port.
	auto currentRPort = callLeg->rportGet ();
	
	if (currentRPort != UNDEFINED && !sipCall->IsBridgedCall())
	{
		port = currentRPort;
	}
	
	if (sipCall)
	{
		eTransport = sipCall->m_eTransportProtocol;
	}

	// If we are registered, and this call is on a registered connection, use the information
	// from our registration for the contact header
	// OLD NOTE: must also check IsUsingOutboundProxy for when initiating outbound calls. Removed this
	// check as it broke DHV and may no longer be needed with all interpreters being registered.
	if (IsRegistered () && !sipCall->IsBridgedCall())
	{
		stiDEBUG_TOOL (g_stiSipMgrDebug,
			stiTrace("Using proxy connection ip/port in contact header\n");
		);

		// NOTE: when using the tunneling server, the reflexive ip will be the tunneling server
		// no need to special case

		ProxyConnectionReflexiveIPPortGet(&IpAddress, &port, &eTransport);
	}
	else
	{
#ifdef stiTUNNEL_MANAGER
		if (RvTunGetPolicy () == RvTun_ENABLED &&
			TunnelActive ())
		{
			stiDEBUG_TOOL (g_stiSipMgrDebug,
				stiTrace("Using tunneled ip in contact header\n");
			);

			IpAddress = m_TunneledIpAddr;
		}
		else
#endif // stiTUNNEL_MANAGER
		if (IsLocalAgentMode ())
		{
			stiDEBUG_TOOL (g_stiSipMgrDebug,
				stiTrace("Using public ip in contact header\n");
			);

			if (!sipCall->IsBridgedCall())
			{
				IpAddress = m_stConferenceParams.PublicIPv4;

				if (sipCall && sipCall->m_eIpAddressType == estiTYPE_IPV6 && !m_stConferenceParams.PublicIPv6.empty())
				{
					IpAddress = m_stConferenceParams.PublicIPv6;
				}
			}
			else
			{
				IpAddress = call->localIpAddressGet();
				stiTESTCOND(!IpAddress.empty(), stiRESULT_ERROR);
			}
		}

		// If there is a call object determine protocol from that instead
		auto inboundMessage = callLeg->inboundMessageGet();

		if (inboundMessage)
		{
			eTransport = inboundMessage.viaTransportGet ();
		}
	}

	// Create the contact address
	address = AddressStringCreate (IpAddress.c_str (), pszPhoneNumber, port, eTransport, "sip");

	sipAddress = AddPlusToPhoneNumber(address);

	// Attach the local contact address to the call leg
	rvStatus = RvSipCallLegGetNewMsgElementHandle (hCallLeg, RVSIP_HEADERTYPE_UNDEFINED, RVSIP_ADDRTYPE_URL, (void**)&hContactAddress);
	stiTESTRVSTATUS ();

	rvStatus = RvSipAddrParse (hContactAddress, (RvChar*)sipAddress.c_str());
	stiTESTRVSTATUS ();

	rvStatus = RvSipCallLegSetLocalContactAddress (hCallLeg, hContactAddress);
	stiTESTRVSTATUS ();

STI_BAIL:
	
	if (stiIS_ERROR(hResult))
	{
		stiASSERTMSG(estiFALSE, "CallLegContactAttach Reason=Cannot set local contact address SipAddress=%s", sipAddress.c_str ());
		NetworkChangeNotify ();
	}
	
	return hResult;
}


/*!\brief Determines the remote dial string from the From URI
 * 
 * The function inspects the user portion of the URI to see if it is a phone number.
 * The user portion may begin with an optional '+' sign followed by 10 or 11 digits.
 * If there are 11 digits then the first digit must be 1.
 * If the user portion of the URI is found not to be a phone number then
 * the entire URI is determined to be the remote dial string.
 * 
 * \param hCallLeg The call leg from which to extract the URI.
 * \param poCall The call object to which to assign the remote dial string
 * 
 */
void RemoteInfoDetermine (
	RvSipCallLegHandle hCallLeg,
    CstiSipCallSP sipCall)
{
	RvStatus rvStatus = RV_OK;
	auto call = sipCall->CstiCallGet();
	RvUint unLength;
	
	//
	// Get "From" information.
	//
	RvSipPartyHeaderHandle hFrom;
	char szFromUsername[un8stiNAME_LENGTH];
	
	RvSipCallLegGetFromHeader (hCallLeg, &hFrom);
	RvSipAddressHandle hFromUri = RvSipPartyHeaderGetAddrSpec (hFrom);
	
	rvStatus = RvSipAddrUrlGetUser (hFromUri, szFromUsername, sizeof (szFromUsername), &unLength);
	
	if (RV_OK != rvStatus)
	{
		szFromUsername[0] = '\0';
	}
	
	char szHost[un8stiDIAL_STRING_LENGTH + 1];
		
	rvStatus = RvSipAddrUrlGetHost (hFromUri, szHost, sizeof (szHost), &unLength);
	
	if (RV_OK != rvStatus)
	{
		szHost[0] = '\0';
	}
	
	if ((strcmp (szFromUsername, g_szAnonymousUser) == 0
	 && strcmp (szHost, g_szAnonymousDomain) == 0)
	 || (strcmp (szFromUsername, g_szPrivateUser) == 0
	 && strcmp (szHost, g_szPrivateDomain) == 0))
	{
		sipCall->RemoteDisplayNameSet(""); // No Caller ID
		call->RemoteDialStringSet ("");
		call->RemoteCallerIdBlockedSet (true);
    }
	else
	{
		sipCall->RemoteDisplayNameDetermine (hCallLeg);
		
		//
		// Determine if the "From" name is a phone number.
		// If it is then store just it as the dial string.
		// If it isn't store the whole URI as the dial string.
		//
		bool bUserIsPhonenumber = false;
		std::string fromName = szFromUsername;

		//
		// Skip past the '+', if present
		//
		if (szFromUsername[0] == '+')
		{
			fromName.erase(0, 1);
		}
		
		//
		// Check if the name is all digits
		//
		if (std::all_of(fromName.begin(), fromName.end(), ::isdigit))
		{
			auto numberOfDigits = fromName.length();

			if (numberOfDigits == 10 &&
				fromName[0] != '1')
			{
				//
				// All digits and the right length.
				// Add a one and then Store it.
				//
				fromName.insert(0, "1");

				call->RemoteDialStringSet (fromName);
				bUserIsPhonenumber = true;
			}
			else if (numberOfDigits == 11 &&
					 fromName[0] == '1')
			{
				//
				// All digits, the right length and starts with a 1.
				//
				call->RemoteDialStringSet (fromName);
				bUserIsPhonenumber = true;
			}
			else if (numberOfDigits == 3 &&
					 fromName[1] == '1' &&
					 fromName[2] == '1')
			{
				//
				// This is a N11 number.
				//
				call->RemoteDialStringSet (fromName);
				bUserIsPhonenumber = true;
			}
		}

		if (!bUserIsPhonenumber &&
			(call->RemoteInterfaceModeGet() == estiINTERPRETER_MODE ||
			 call->RemoteInterfaceModeGet() == estiVRI_MODE))
		{
			// Anything that comes from the interpreter is accepted as the dial string
			// Resolves bug #24267.
			call->RemoteDialStringSet(fromName);
			bUserIsPhonenumber = true;
		}
		
		if (!bUserIsPhonenumber)
		{
			char szRemoteDialString[un8stiDIAL_STRING_LENGTH + 1];

			if (szHost[0] != '\0')
			{
				unLength = snprintf (szRemoteDialString, sizeof (szRemoteDialString), "sip:%s@%s", szFromUsername, szHost);

				if (unLength < sizeof (szRemoteDialString))
				{
					call->RemoteDialStringSet (szRemoteDialString);
				}
			}
		}
		else
		{
			auto pCallInfo = (CstiCallInfo *)call->RemoteCallInfoGet ();
			SstiUserPhoneNumbers stRmtNumbers = *pCallInfo->UserPhoneNumbersGet ();
			std::string oDialString;
			call->RemoteDialStringGet (&oDialString);
			strcpy (stRmtNumbers.szPreferredPhoneNumber, oDialString.c_str());
			pCallInfo->UserPhoneNumbersSet (&stRmtNumbers);
		}
	}
}


/*!\brief Extracts the user and host portion from the From URI.
 *
 * \param hCallLeg The handle to the call leg to retrieve the information from
 * \param pUser Contains the user portion upon return
 * \param pHost Contains the host portion upon return
 */
static void FromUserAndHostGet (
	RvSipCallLegHandle hCallLeg,
	std::string *pUser,
	std::string *pHost)
{
	RvStatus rvStatus = RV_OK;
	RvSipPartyHeaderHandle hFrom;
	RvSipAddressHandle hFromUri;
	char szFromUsername[un8stiNAME_LENGTH];
	char szFromHost[256];

	szFromUsername[0] = '\0';
	szFromHost[0] = '\0';

	//
	// Get the "from" header.
	//
	rvStatus = RvSipCallLegGetFromHeader (hCallLeg, &hFrom);

	if (rvStatus == RV_OK)
	{
		//
		// Get the URI from the from header
		//
		hFromUri = RvSipPartyHeaderGetAddrSpec (hFrom);

		if (hFromUri)
		{
			RvUint unLength = 0;

			//
			// Get the user and host from the URI.
			//
			rvStatus = RvSipAddrUrlGetUser (hFromUri, szFromUsername, sizeof (szFromUsername), &unLength);

			if (RV_OK != rvStatus)
			{
				szFromUsername[0] = '\0';
			}

			rvStatus = RvSipAddrUrlGetHost (hFromUri, szFromHost, sizeof (szFromHost), &unLength);

			if (RV_OK != rvStatus)
			{
				szFromHost[0] = '\0';
			}
		}
	}

	*pUser = szFromUsername;
	*pHost = szFromHost;
}


/*!\brief Determines if the call is SPIT. If the call is SPIT then it logs a SPIT event and returns true.
 *
 * \param hCallLeg The handle to the call leg to be examined
 * \param sipCall The SIP call object associated with the call leg
 */
bool SpitDetermine (
	RvSipCallLegHandle hCallLeg,
    CstiSipCallSP sipCall)
{
	bool bResult = false;
	std::string FromUser;
	std::string FromHost;
	CstiSipCallLegSP callLeg = sipCall->CallLegGet(hCallLeg);
	std::string RemoteProductName;

	FromUserAndHostGet (hCallLeg, &FromUser, &FromHost);

	//
	// If the user agent starts with one of the agents in the following vector then consider it SPIT.
	//
	std::vector<std::string> productNames = {"Ozeki", "friendly-scanner", "eyeBeam", "UsaAirport", "usa"};

	callLeg->RemoteProductNameGet (&RemoteProductName);

	for (auto &&productName: productNames)
	{
		if (RemoteProductName.compare (0, productName.length (), productName) == 0)
		{
			auto call = sipCall->CstiCallGet();
			
			// Don't log SPIT.
			call->logCallSet (false);

			//
			// Log the information to splunk.
			//
			stiRemoteLogEventSend ("EventType=SPIT Reason=%s From=%s@%s InitialInviteAddress=%s",
			                       productName.c_str (), FromUser.c_str (), FromHost.c_str (), call->InitialInviteAddressGet ().c_str ());

			bResult = true;

			break;
		}
	}

	//
	// If it passed the product name check then
	// check to see if the SDP does not offer video.
	//
	if (!bResult)
	{
		auto inboundMessage = callLeg->inboundMessageGet();

		if (inboundMessage)
		{
			bool bVideoFound = false;
			CstiSdp Sdp;

			//
			// Make sure there is video media being offered
			// If there is no video media then terminate
			// the call and log the event.
			//
			Sdp.initialize (inboundMessage);

			if (!Sdp.empty ())
			{
				int nNbrMedia = Sdp.numberOfMediaDescriptorsGet ();

				for (int i = 0; i < nNbrMedia; i++) // for each 'm' line
				{
					const RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorGet (i);

					if (pMediaDescriptor)
					{
						const char *pszMediaType = rvSdpMediaDescrGetMediaTypeStr (pMediaDescriptor);

						if (strcmp (pszMediaType, "video") == 0)
						{
							bVideoFound = true;
							break;
						}
					}
				}

				if (!bVideoFound)
				{
					auto call = sipCall->CstiCallGet();
					
					// Don't log SPIT.
					call->logCallSet (false);

					//
					// Log the information to splunk.
					//
					stiRemoteLogEventSend ("EventType=SPIT Reason=NoVideo From=%s@%s InitialInviteAddress=%s",
					                       FromUser.c_str (), FromHost.c_str (), call->InitialInviteAddressGet ().c_str());

					bResult = true;
				}
			}
		}
	}

	return bResult;
}


/*!\brief Gets a formatted string containing the transport, address and report from whence the last message came
 *
 * \param hCallLeg Handle of call leg from which to the remote address
 * \param pAddress Upon return, contains a formatted string of the transport, address and port
 */
static void ReceivedFromAddressGet (
	RvSipCallLegHandle hCallLeg,
	std::string *pAddress)
{
	RvSipTransportAddr TransportAddr;
	std::stringstream AddressString;

	RvSipCallLegGetReceivedFromAddress (hCallLeg, &TransportAddr);

	switch (TransportAddr.eTransportType)
	{
		case RVSIP_TRANSPORT_UDP:
			AddressString << "UDP:";
			break;

		case RVSIP_TRANSPORT_TCP:
			AddressString << "TCP:";
			break;

		case RVSIP_TRANSPORT_TLS:
			AddressString << "TLS:";
			break;

		default:
			AddressString << "Unknown:";
			break;
	}

	AddressString << TransportAddr.strIP << ":" << TransportAddr.port;

	*pAddress = AddressString.str ();
}


////////////////////////////////////////////////////////////////////////////////
//; CheckForHeader
//
// Description: Checks a message for a specified header.
//
// Abstract:
//  This function is will look for a header matching inside the supplied
// RvSipMsgHandle
//
// Returns:
//  true if the header exists otherwise false.
//
bool CheckForHeader(RvSipMsgHandle hInboundMsg, const char *pszHeader)
{
	//Check to see if the header is in the message.
	RvSipHeaderListElemHandle hListIterator {nullptr};

	RvSipOtherHeaderHandle hOtherHeader = RvSipMsgGetHeaderByName(
		hInboundMsg, (RvChar*)pszHeader, RVSIP_FIRST_HEADER, &hListIterator);

	return hOtherHeader != nullptr;
}


bool CheckForSBCInterworkedHeader(RvSipMsgHandle hInboundMsg)
{
	//Check to see if there is a H.323 Interworked private header
	RvSipHeaderListElemHandle hListIterator = nullptr;

	RvSipOtherHeaderHandle hSBCHeader = RvSipMsgGetHeaderByName(
		hInboundMsg, (RvChar*)g_szSVRSH323InterworkedPrivateHeader, RVSIP_FIRST_HEADER, &hListIterator);

	return hSBCHeader != nullptr;
}


/*!\brief Retrieves the ip address from the P-SVRS-TRS-User-IP header
 *
 */
std::string trsUserIpGet(const vpe::sip::Message &inboundMessage)
{
	std::string ipAddress;
	bool present;

	std::tie(ipAddress, present) = inboundMessage.headerValueGet(g_szSVRSTrsUserIpPrivateHeader);

	if (!present)
	{
		return {""};
	}

	return ipAddress;
}

std::string interperterSkillsHeaderGet (const vpe::sip::Message &inboundMessage)
{
	std::string interpreterSkills;
	bool present;

	std::tie (interpreterSkills, present) = inboundMessage.headerValueGet (g_szSVRSInterpreterSkills);

	if (!present)
	{
		return {""};
	}

	return interpreterSkills;
}

std::string vriUserInfoHeaderGet (const vpe::sip::Message &inboundMessage)
{
	std::string vriInfo;
	bool present;

	std::tie (vriInfo, present) = inboundMessage.headerValueGet (g_szSVRSVriUserInfo);

	if (!present)
	{
		return {""};
	}

	return vriInfo;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::OfferingCallProcess
//
// Description: Called when we are notified of an incoming call.
// 	We must decide whether to accept or reject it.
//
// Returns: stiHResult
//
stiHResult CstiSipManager::OfferingCallProcess (
	RvSipCallLegHandle hCallLeg,
	vpe::sip::Message &inboundMessage,
    CstiSipCallSP sipCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	CstiSdp sdp;
	RvSipAddressHandle hContactAddress;
	RvSipAddressHandle hToUri;
	RvSipTransport eConnectionTransportType;
	RvSipTransportAddressType eConnectionAddressType;
	RvChar szConnectionLocalAddress[RV_ADDRESS_MAXSTRINGSIZE + 1];
	RvUint16 nConnectionLocalPort;
	std::string InitialInviteAddress;
	
	sipCall->Lock ();
	
	auto call = sipCall->CstiCallGet ();
	
	SIP_DBG_EVENT_SEPARATORS("incoming initial INVITE");
	
	RvSipCallLegProvisionalResponse (hCallLeg, SIPRESP_TRYING);

	ReceivedFromAddressGet (hCallLeg, &InitialInviteAddress);
	call->InitialInviteAddressSet (InitialInviteAddress);

	//
	// Read SINFO from any SDP present.
	//
	sdp.initialize (inboundMessage);

	if (!sdp.empty ())
	{
		auto callLeg = sipCall->CallLegGet(hCallLeg);
		if (callLeg)
		{
			callLeg->SInfoHeaderRead(sdp);

			// Notify the call that the SINFO has been changed.
			call->CallInformationChanged ();
		}
	}

#if !defined (stiINTEROP_EVENT) && (APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL)
	if (SpitDetermine (hCallLeg, sipCall))
	{
		//
		// Terminate the call.
		//
		call->NotifyAppOfIncomingCallSet (estiFALSE);
		sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE);
	}
	else
#endif
	{
		// Are we in a call currently?
		EstiBool bInCall = estiFALSE;

		if (0 != m_pCallStorage->countGet (
		 esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
		{
			bInCall = estiTRUE;
		}

		// Get the default called name
		sipCall->CalledNameRetrieve (inboundMessage);

		std::string Address;
	
		RemoteIpAddressGet (inboundMessage, &Address);

		//
		// If we retrieved an address then store it
		//
		if (!Address.empty ())
		{
			call->RemoteIpAddressSet (Address);
		}
		
		std::string sipCallInfo;
		std::tie(sipCallInfo, std::ignore) = inboundMessage.headerValueGet(g_szSVRSCallInfoHeader);

		if (!sipCallInfo.empty())
		{
			call->SIPCallInfoSet(sipCallInfo);
		}
		
		std::string sipCallId;
		call->CallIdGet(&sipCallId);
				
#ifdef stiHOLDSERVER
		std::string strAssertedIdentity;

		std::tie(strAssertedIdentity, std::ignore) = inboundMessage.headerValueGet(g_szSVRSAssertedIdentityHeader);

		if (!strAssertedIdentity.empty())
		{
			call->AssertedIdentitySet(strAssertedIdentity.c_str());
		}

		if (CheckForSBCInterworkedHeader(inboundMessage))
		{
			call->IsCallH323InterworkedSet(estiTRUE);
			
			LOG_DEBUG << "Message=\"" << g_szSVRSH323InterworkedPrivateHeader << " has been detected.\" "
					  << "SIPCallID=" << sipCallId << std::flush;
		}

		auto trsUserIpAddress = trsUserIpGet(inboundMessage);
		if (!trsUserIpAddress.empty())
		{
			LOG_DEBUG << "Message=\"" << g_szSVRSTrsUserIpPrivateHeader << " has been detected.\" "
				<< "SIPCallID=" << sipCallId << " "
				<< "ReceivedIPAddress=" << Address << " "
				<< "TRSUserIPAddress=" << trsUserIpAddress << std::flush;

			call->RemoteIpAddressSet(trsUserIpAddress.c_str());
		}

		auto interpreterSkills = interperterSkillsHeaderGet(inboundMessage);
		if (!interpreterSkills.empty())
		{
			LOG_DEBUG << "Message=\"" << g_szSVRSInterpreterSkills << "has been detected.\" "
				<< "SIPCallID=" << sipCallId << " "
				<< "SkillsHeader=\"" << interpreterSkills << "\"";

			call->interpreterSkillsSet(interpreterSkills);
		}

		auto vriUserInfo = vriUserInfoHeaderGet(inboundMessage);
		if (!vriUserInfo.empty()) 
		{
			LOG_DEBUG << "Message=\"" << g_szSVRSVriUserInfo << "has been detected.\" "
				<< "SIPCallID=" << sipCallId << " "
				<< "VriUserInfoHeader=\"" << vriUserInfo << "\"";

			call->vriUserInfoSet(vriUserInfo);
		}
#else
		// We are still checking for the P-SVRS-H323-Interworked private header for possible incoming H323 calls from 3rd parties
		if (CheckForSBCInterworkedHeader(inboundMessage))
		{
			sipCall->m_bGatewayCall = true;
			sipCall->IsHoldableSet (estiFALSE);
		}
#endif//stiHOLDSERVER
		
		// See if we are behind an IVVR, if we are we may need to behave differently than if we were not.
		call->IsBehindIVVRSet (CheckForHeader (inboundMessage, g_szSVRSIVVR));

		//
		// Determine the address type (ipv4 or ipv6) from the connection in use
		//
		rvStatus = RvSipCallLegGetTranscCurrentLocalAddress (hCallLeg, nullptr, &eConnectionTransportType,
			&eConnectionAddressType, szConnectionLocalAddress, &nConnectionLocalPort);
		stiTESTRVSTATUS ();

		if (eConnectionAddressType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP6)
		{
			sipCall->m_eIpAddressType = estiTYPE_IPV6;
		}
		else
		{
			sipCall->m_eIpAddressType = estiTYPE_IPV4;
		}

		//
		// Determine the transport the call is using
		//
		if (RV_OK == RvSipCallLegGetRemoteContactAddress (hCallLeg, &hContactAddress))
		{
			//
			// Store the transport this call is using
			// and set the local contact header.
			//

			if (RvSipAddrUrlIsSecure (hContactAddress))
			{
				sipCall->m_eTransportProtocol = estiTransportTLS;
			}
			else
			{
				///\todo: determine if we should be getting the transport from the via header
				///\	or the eConnectionTransportType retrieved above
				switch (RvSipAddrUrlGetTransport (hContactAddress))
				{
					case RVSIP_TRANSPORT_TCP:
					    sipCall->m_eTransportProtocol = estiTransportTCP;
						break;
					case RVSIP_TRANSPORT_UDP:
					    sipCall->m_eTransportProtocol = estiTransportUDP;
						break;
					case RVSIP_TRANSPORT_TLS:
					    sipCall->m_eTransportProtocol = estiTransportTLS;
						break;
					default:
					    sipCall->m_eTransportProtocol = estiTransportUnknown;
				}
			}
		}

		//
		// Get "To" information.
		//
		RvSipPartyHeaderHandle hTo;
		char szToUsername[un8stiNAME_LENGTH];
		char toHost[un8stiMAX_DOMAIN_NAME_LENGTH];

		RvSipCallLegGetToHeader (hCallLeg, &hTo);
		hToUri = RvSipPartyHeaderGetAddrSpec (hTo);
		RvUint unLength;
		
		//
		// Get "From information.
		//
		std::string fromUser;
		std::string fromHost;
		FromUserAndHostGet(hCallLeg, &fromUser, &fromHost);

		if (RV_OK != RvSipAddrUrlGetUser (hToUri, szToUsername, sizeof (szToUsername), &unLength))
		{
			szToUsername[0]='\0';
		}
		else
		{
			sipCall->CstiCallGet()->IntendedDialStringSet(szToUsername);
		}
		
		if (RV_OK != RvSipAddrUrlGetHost(hToUri, toHost, sizeof(toHost), &unLength))
		{
			toHost[0]='\0';
		}
		
		stiRemoteLogEventSend("EventType=SIPInitialInvite SipCallId=%s ToUri=%s@%s FromUri=%s@%s", sipCallId.c_str(), szToUsername, toHost, fromUser.c_str(), fromHost.c_str());

		//
		// Determine the remote display name and dial string
		//
		RemoteInfoDetermine (hCallLeg, sipCall);

		//
		// If using TCP then set the persistency level so the stack
		// uses the connection we have already setup.
		//
		RvSipCallLegSetPersistency (hCallLeg, RVSIP_TRANSPORT_PERSISTENCY_LEVEL_TRANSC_USER);

		// Attach the Contact: header
		CallLegContactAttach (hCallLeg);

#ifdef stiMEDIASERVER 

		//We are checking for Area Code 211 so that we can reject them.  211 calls should not 
		//be processed by the Media Server for Interactive Care. 
		if (m_regex211Uri.Match(szToUsername))
		{
			DBG_MSG("Reject call from Area Code 211 (Interactive Care)");
			call->NotifyAppOfIncomingCallSet(estiFALSE);
			sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_FOUND);
		}
		else
#endif
		{
#ifndef stiINTEROP_EVENT
			//
			// If we are enforcing authorized numbers
			// and the remote endpoint is not an interpreter phone
			// and the number is not one of ours then reject the call.
			//
			if (m_bEnforceAuthorizedPhoneNumber &&
				estiINTERPRETER_MODE != call->RemoteInterfaceModeGet() &&
				!PhoneNumberVerify(szToUsername))
			{
				// This call is addressed to someone else.  FAIL.
				DBG_MSG("Dialed user \"%s\" != \"%s\", \"%s\", \"%s\", or \"%s\"", szToUsername,
					m_UserPhoneNumbers.szLocalPhoneNumber, m_UserPhoneNumbers.szTollFreePhoneNumber, m_UserPhoneNumbers.szRingGroupLocalPhoneNumber, m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber);

				call->NotifyAppOfIncomingCallSet(estiFALSE);
				sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_FOUND);
				
				// Don't log SPIT.
				call->logCallSet (false);

				stiRemoteLogEventSend("EventType=SPIT Reason=WrongNumber Number=%s InitialInviteAddress=%s",
									  szToUsername, call->InitialInviteAddressGet().c_str());
			}
			else
#endif  // stiINTEROP_EVENT
			{
				std::string RemoteDialString;
				call->RemoteDialStringGet(&RemoteDialString);

				// Check to see if we have the private header to allow this incoming call...
				//  ...this could be an E911 callback or a call from CIR
				RvSipHeaderListElemHandle hListIterator = nullptr;
				RvSipOtherHeaderHandle hAllowCallHeader = RvSipMsgGetHeaderByName(
						inboundMessage, (RvChar*)g_szSVRSAllowIncomingCallPrivateHeader, RVSIP_FIRST_HEADER, &hListIterator);

				auto callLeg = sipCall->CallLegGet(hCallLeg);
				if (!callLeg)
				{
					sipCall->CallEnd(estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
					stiTESTCOND(callLeg, stiRESULT_ERROR); // bail out of this function
				}

#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER) && (APPLICATION != APP_REGISTRAR_LOAD_TEST_TOOL)
				if (
					// Auto reject is on...
					m_bAutoReject
					|| m_pBlockListManager->CallBlocked(RemoteDialString.c_str()))
				{
					stiRemoteLogEventSend("EventType=SIPBusyHere Reason=AutoReject");
					call->NotifyAppOfIncomingCallSet(estiFALSE);
					sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_BUSY_HERE);
				}
				else if (m_stConferenceParams.bBlockAnonymousCallers
					&& RemoteDialString.empty()
					&& call->RemoteInterfaceModeGet() != estiINTERPRETER_MODE)
				{
					stiRemoteLogEventSend("EventType=SIPBusyHere Reason=BlockAnonymous");
					call->NotifyAppOfIncomingCallSet(estiFALSE);
					sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_BUSY_HERE);
				}
				else
#endif //stiHOLDSERVER
				if (
					// Or, we have a call and we are not allowing multiple calls
					(m_pCallStorage->countGet() > m_maxCalls)

#if !defined(stiHOLDSERVER) && !defined(stiMEDIASERVER)
					// If we already have a call and the incoming call is through the gateway,
					// drop the call.
					|| (bInCall && sipCall->m_bGatewayCall)

					// OR if we have a "connecting" or "disconnecting" call already
					|| (m_pCallStorage->countGet(esmiCS_IDLE | esmiCS_CONNECTING | esmiCS_DISCONNECTING) > 1)
					// OR if we have a call that is in the process of leaving a message.
					|| m_pCallStorage->leavingMessageGet ()
#endif
				)
				{
					DBG_MSG("Cannot accept more than %d total calls, more than 1 non-Sorenson call, or any more calls when we are busy connecting.", m_maxCalls);
					call->NotifyAppOfIncomingCallSet(estiFALSE);
					sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_BUSY_HERE);
					
					RvInt32 nActualLen = 0;
					RvChar szCallID[ID_BUF_LEN];
					
					rvStatus = RvSipCallLegGetCallId(
													 hCallLeg,
													 ID_BUF_LEN,
													 szCallID,
													 &nActualLen);
					
					// If succeeded, copy the CallId to the passed in string object
					if (RV_OK != rvStatus)
					{
						szCallID[0] = '\0';
					}
					
					stiRemoteLogEventSend("EventType=SIPBusyHere Reason=MaxCalls "
										  "IsLocalAgentMode=%d "
										  "CalledParty=\"%s\" "
										  "CallID=%s "
										  "CallCount=%d "
										  "IdleCount=%d "
										  "ConnectingCount=%d "
										  "ConnectedCount=%d "
										  "HoldLocalCount=%d "
										  "HoldRemoteCount=%d "
										  "HoldBothCount=%d "
										  "DisconnectingCount=%d "
										  "DisconnectedCount=%d "
										  "CriticalErrorCount=%d "
										  "InitTransferCount=%d "
										  "TransferringCount=%d "
										  "LeavingMessage=%d",
										  IsLocalAgentMode() ? 1 : 0,
										  szToUsername[0] != '\0' ? szToUsername : "Unknown",
										  szCallID[0] != '\0' ? szCallID : "Unknown",
										  m_pCallStorage->countGet(),
										  m_pCallStorage->countGet(esmiCS_IDLE),
										  m_pCallStorage->countGet(esmiCS_CONNECTING),
										  m_pCallStorage->countGet(esmiCS_CONNECTED),
										  m_pCallStorage->countGet(esmiCS_HOLD_LCL),
										  m_pCallStorage->countGet(esmiCS_HOLD_RMT),
										  m_pCallStorage->countGet(esmiCS_HOLD_BOTH),
										  m_pCallStorage->countGet(esmiCS_DISCONNECTING),
										  m_pCallStorage->countGet(esmiCS_DISCONNECTED),
										  m_pCallStorage->countGet(esmiCS_CRITICAL_ERROR),
										  m_pCallStorage->countGet(esmiCS_INIT_TRANSFER),
										  m_pCallStorage->countGet(esmiCS_TRANSFERRING),
										  m_pCallStorage->leavingMessageGet() ? 1 : 0);
				}

				else if (m_bDisallowIncomingCalls && !hAllowCallHeader)
				{
					// Disallow-- this is not CIR call or 911 callback with the appropriate AllowCallHeader
					stiRemoteLogEventSend("EventType=SIPBusyHere Reason=No EULA; rejecting call (except CIR and 911).");
					call->NotifyAppOfIncomingCallSet(estiFALSE);
					sipCall->CallEnd(estiSUBSTATE_LOCAL_HANGUP, SIPRESP_BUSY_HERE);
				}
				else
				{
					// Make sure the call has not disconnected while we were waiting.
					if (call->StateValidate (esmiCS_IDLE))
					{
						CstiSdp Sdp;

						OutboundDetailsSet (sipCall, callLeg, nullptr, nullptr);

						callLeg->AllowCapabilitiesDetermine (inboundMessage);

						hResult = Sdp.initialize (inboundMessage);
						stiTESTRESULT ();

						hResult = callLeg->SdpOfferProcess (&Sdp, estiOfferAnswerINVITE);

						if (stiIS_ERROR (hResult))
						{
							// This call is for me, but has bad SDP.
							sipCall->CstiCallGet ()->NotifyAppOfIncomingCallSet (estiFALSE);
							sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE_HERE);
						}
#ifdef stiHOLDSERVER
						else
						{
							// Process potential private SVRS geolocation header in invite
							privateGeolocationHeaderCheck (inboundMessage, call);
						}
#endif
					}
				}
			}
		}
	stiUNUSED_ARG (bInCall);  // Load test tool gets a compile error if we don't have this here.

	}
	
STI_BAIL:

	sipCall->Unlock();
	
	return hResult;
}


/*!\brief Processes a call when it reaches the connected state.
 *
 */
stiHResult CstiSipManager::ConnectedCallProcess (
    CstiSipCallLegSP callLeg,
	RvSipCallLegDirection eDirection)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = callLeg->m_sipCallWeak.lock();
	auto call = sipCall->CstiCallGet();
	
	// We need to get our SDP from the body of their OK or our OK's ACK
	SIP_DBG_EVENT_SEPARATORS("Got %s to INVITE",
			eDirection != RVSIP_CALL_LEG_DIRECTION_INCOMING ? "OK" : "ACK");

	// This is a real call, so terminate any pending LOOKUP calls.
	if (m_hLookupCallLeg)
	{
		RvSipCallLegDisconnect (m_hLookupCallLeg);
		m_hLookupCallLeg = nullptr;
	}
	
	auto inboundMessage = callLeg->inboundMessageGet();

	// If this is an OK to our INVITE
	if (eDirection == RVSIP_CALL_LEG_DIRECTION_OUTGOING)
	{
		std::string sipCallInfo;
		std::tie(sipCallInfo, std::ignore) = inboundMessage.headerValueGet(g_szSVRSCallInfoHeader);
		
		if (!sipCallInfo.empty ())
		{
			call->SIPCallInfoSet (sipCallInfo);
		}
		
		//
		// Load the SInfo for the call leg that was chosen
		//
		SystemInfoDeserialize (callLeg, callLeg->m_SInfo.c_str ());
		
		//
		// This is a sorenson device.  Read the SInfo header from the provisional response
		//
		callLeg->SInfoHeaderRead (inboundMessage);

		//
		// Try to extract the AlternateDisplayName
		// from the To: header in order to support 3rd party terp devices.
		//
		std::string DisplayName;
		sipCall->RemoteAlternateNameGet (&DisplayName);
		
		if (DisplayName.empty ())
		{
			std::string AltName;
			//
			// Only fill in the alternate name if the remote name has the SVRS: prefix
			//
			callLeg->toHeaderRemoteNameGet (&AltName, true);
			if (!AltName.empty())
			{
				sipCall->RemoteAlternateNameSet (AltName.c_str());
			}
		}

		// Now check if remote display name is set...
		DisplayName.clear();
		sipCall->RemoteDisplayNameGet (&DisplayName);
		
		if (DisplayName.empty ())
		{
			std::string RemoteName;
			callLeg->toHeaderRemoteNameGet (&RemoteName, false);
			if (!RemoteName.empty())
			{
				sipCall->RemoteDisplayNameSet (RemoteName.c_str());
			}
		}

		// We only need to check the Allow Header on the 200-OK and not the Ack
		callLeg->AllowCapabilitiesDetermine(inboundMessage);
	}
		
	//
	// If the original call object pointer is set and
	// is equal to this call object then we must be in a transfer
	// scenario.  Shutdown the media from the previous call object.
	//
	if (m_originalCall == call)
	{
		m_originalCall->DirectionSet (m_eOriginalDirection);
		
		//
		// We need to shutdown the media of the original call object so that new media can flow.
		//
		if (m_transferCall)
		{
			std::static_pointer_cast<CstiSipCall>(m_transferCall->ProtocolCallGet())->MediaClose();
		}
		
#ifdef stiREFERENCE_DEBUGGING
		stiTrace ("Releasing original call object %p.\n", m_originalCall.get());
#endif
		m_originalCall = nullptr;
		m_originalSipCall = nullptr;
	}
	
	// Continue with call
	DBG_MSG ("Call %p is now connected.  App notified.", call.get());
	
	//
	// If we are now conferencing we have lost our opportunity to leave a signmail.
	// Set our state to conferencing.
	//
	call->LeaveMessageSet (estiSM_NONE);
	
	NextStateSet (call, esmiCS_CONNECTED, estiSUBSTATE_ESTABLISHING);
	
	if (callLeg->m_pIceSession)
	{
		if (!callLeg->m_bRemoteIsICE || callLeg->m_pIceSession->m_eState == CstiIceSession::estiNominationsFailed)
		{
			//
			// We started an ICE session but the remote endpoint does not
			// use ICE so we need to get the transports allocated by the ICE
			// stack and send them to the playback data tasks.
			//
			hResult = callLeg->m_pIceSession->SdpUpdate (&callLeg->m_sdp);
			stiTESTRESULT ();

			hResult = sipCall->DefaultTransportsApply (callLeg);
			stiTESTRESULT ();

			sipCall->IceSessionsEnd ();
		}
		else if (callLeg->m_pIceSession->m_eState == CstiIceSession::estiComplete && !callLeg->m_pIceSession->m_nominationsApplied)
		{
			callLeg->IceNominationsApply ();
		}
	}
	
	if (callLeg->m_pIceSession == nullptr || callLeg->m_pIceSession->m_eState == CstiIceSession::estiComplete
	 || callLeg->m_pIceSession->m_eState == CstiIceSession::estiNominationsFailed)
	{
		// If we are not doing ICE or ICE has completed we can move to conferencing state.
		// Otherwise this will be set once ICE completes to prevent sending Re-Invites early causing black video.
		NextStateSet (call, esmiCS_CONNECTED, estiSUBSTATE_CONFERENCING);
		if (callLeg->isDtlsNegotiated ())
		{
			sipCall->dtlsBegin (true);
		}
		else
		{
			//
			// Make sure the incoming media is open.
			//
			if (eDirection == RVSIP_CALL_LEG_DIRECTION_OUTGOING)
			{
				hResult = sipCall->IncomingMediaUpdate ();
				stiTESTRESULT ();
			}

			//
			// Open the outgoing media.
			//
			hResult = sipCall->OutgoingMediaOpen (true);
			stiTESTRESULT ();
		}
	}
	
	//
	// Terminate the other call legs.
	//
	sipCall->ForkedCallLegsTerminate ();
	{
		auto dhviCall = sipCall->m_dhviCall;
		if (dhviCall && sipCall->dhvSwitchAfterHandoffGet ())
		{
			PostEvent (
				[sipCall, dhviCall]
				{
					auto uri = dhviCall->RoutingAddressGet ().ResolvedURIGet ();
					auto destAddress = dhvUriCreate (uri, SIP_PREFIX);

					sipCall->switchedActiveCallsSet (true);
					sipCall->sendDhviCapableMsg (true);
					sipCall->sendDhviCreateMsg (destAddress);
				});
		}
	}
	
STI_BAIL:

	return hResult;
}


stiHResult CstiSipManager::DisconnectedCallProcess (
	RvSipCallLegHandle hCallLeg,
    CstiSipCallSP sipCall,
	RvSipCallLegStateChangeReason eReason)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (sipCall)
	{
		auto call = sipCall->CstiCallGet ();
		auto callLeg = sipCall->CallLegGet(hCallLeg);
		
		stiDEBUG_TOOL (g_stiSipMgrDebug,
		    vpe::trace ("Disconnected CstiCallCall: ", call.get (), ", ", callLegStateChangeReasonGet (eReason), "\n");
		);

		call->collectTrace (formattedString ("%s: reason %s", __func__, callLegStateChangeReasonGet (eReason).c_str ()));

		stiDEBUG_TOOL (g_stiLoadTestLogging,
				if (RVSIP_CALL_LEG_REASON_REMOTE_CANCELED == eReason)
				{
					stiTrace ("<SipMgr::DisconnectedCallProcess> Incoming call canceled (%p)\n", call.get ());
				}
		);

		auto inboundMessage = callLeg->inboundMessageGet();

		if (!inboundMessage)
		{
			stiASSERTMSG (estiFALSE, "No inbound message");
		}
		
		if (inboundMessage)
		{
			// We moved to disconnected as at the request of the remote side
			int nResponseCode = inboundMessage.statusCodeGet();
			switch (nResponseCode)
			{
				case SIPRESP_BUSY_HERE:
				case SIPRESP_BUSY_EVERYWHERE:
					// Remote side told us it was busy.
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_BUSY);
					sipCall->CallEnd (estiSUBSTATE_BUSY, SIPRESP_NONE);
					break;
				case SIPRESP_TIMEOUT:
				case SIPRESP_DECLINE:
				case SIPRESP_AUTHENTICATION_REQD:
				case SIPRESP_AUTHENTICATION_REQD_PROXY:
				case SIPRESP_NO_ANONYMOUS:
					// Remote side did not answer.
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_REJECTED);
					sipCall->CallEnd (estiSUBSTATE_BUSY, SIPRESP_NONE);
					break;
				case SIPRESP_TEMPORARILY_UNAVAILABLE:
				case SIPRESP_NOT_FOUND:
				{
					// Determine if we should send this call to VRS

					//	- Only do this if using server based call routing.
					auto routingAddress = call->RoutingAddressGet ();

					//	- Only attempt to connect to VRS if the dial string is valid.
					auto userPortion = routingAddress.userGet ();
					bool isValidPhoneNumber = CstiPhoneNumberValidator::InstanceGet ()->PhoneNumberIsValid (userPortion.c_str ());
					
					if ((routingAddress.ipAddressStringGet () == stiSIP_UNRESOLVED_DOMAIN || userPortion == SUICIDE_LIFELINE)
						&& isValidPhoneNumber)
					{
						// Only standard and public modes should place VRS calls.
						if (estiSTANDARD_MODE == m_eLocalInterfaceMode || estiPUBLIC_MODE == m_eLocalInterfaceMode)
						{
							sipCall->CallEnd (estiSUBSTATE_CREATE_VRS_CALL, SIPRESP_NONE);
						}
						else
						{
							sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
							// Set the call result after CallEnd, because can overwrite the call result.
							call->ResultSet (estiRESULT_VRS_CALL_NOT_ALLOWED);
						}
					}
					else
					{
						sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
						// Set the call result after CallEnd, because can overwrite the call result.
						if (SIPRESP_TEMPORARILY_UNAVAILABLE == nResponseCode)
						{
							call->ResultSet (estiRESULT_REMOTE_SYSTEM_TEMPORARILY_UNAVAILABLE);
						}
						else
						{
							call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
						}
					}
					break;
				}
				case SIPRESP_BAD_REQUEST:
				case SIPRESP_NOT_ACCEPTABLE:
				case SIPRESP_LOOP_DETECTED:
				case SIPRESP_SERVICE_UNAVAILABLE:
				case SIPRESP_SERVER_TIMEOUT:
				case SIPRESP_INTERNAL_ERROR:
				case SIPRESP_NOT_IMPLEMENTED:
					// The phone number was not correct for the dialed device.
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
					sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
					break;
				case SIPRESP_NOT_ACCEPTABLE_HERE:
				{
					if (!call->callLegCreatedGet ())
					{
						callLeg = sipCall->CallLegAdd (hCallLeg);
					}
					else
					{
						callLeg = sipCall->CallLegGet (hCallLeg);
					}
					
					auto inboundMessage = callLeg->inboundMessageGet();

					CstiSdp sdp;
					sdp.initialize (inboundMessage);
					
					if (!sdp.empty ())
					{
						callLeg->SdpParse (sdp, estiAnswer, nullptr, nullptr);
					}
					
					if (!callLeg->m_SdpStreams.empty () && callLeg->m_SdpStreams.front ()->MediaProtocol == RV_SDPPROTOCOL_RTP_SAVP)
					{
						call->ResultSet (estiRESULT_ENCRYPTION_REQUIRED);
					}
					else
					{
						call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
					}
					
					sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
					break;
				}
				default:
				{
				    if (call->StateGet() == esmiCS_CONNECTING)
					{
						// Log an event so we know what response code we are not handling
						// and set the result and substate to allow the call to go to SignMail
						// if available. Fixes bug #25986.
						std::string callId;
						
						call->CallIdGet(&callId);
						
						stiRemoteLogEventSend (
											   "EventType = UnHandledDisconnectedCallProcess ResponseCode = %d CallID = %s",
											   nResponseCode, callId.c_str());
						
						call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
						sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
					}
					else
					{
						sipCall->CallEnd (estiSUBSTATE_UNKNOWN, SIPRESP_NONE);
					}
					break;
				}
			}

			stiDEBUG_TOOL (g_stiLoadTestLogging,
					stiTrace ("<SipMgr::DisconnectedCallProcess> Call disconnected due to SIP Response %d\n", nResponseCode);
			);
		}
		else
		{
			// Call was disconnected for some other reason.
			switch (eReason)
			{
				case RVSIP_CALL_LEG_REASON_DISCONNECT_REMOTE_REJECT:
				case RVSIP_CALL_LEG_REASON_LOCAL_REJECT:
				case RVSIP_CALL_LEG_REASON_DISCONNECTED:
				case RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECTED:
				case RVSIP_CALL_LEG_REASON_REMOTE_DISCONNECT_REQUESTED:
				case RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_REJECT:
				case RVSIP_CALL_LEG_REASON_DISCONNECT_LOCAL_ACCEPT:
				case RVSIP_CALL_LEG_REASON_LOCAL_DISCONNECTING:

					// Normal hang up
				    sipCall->CallEnd (estiSUBSTATE_UNKNOWN, SIPRESP_NONE);
					break;

				case RVSIP_CALL_LEG_REASON_REMOTE_CANCELED:
					// Call cancelled
				    if (estiOUTGOING == call->DirectionGet ())
					{
						// Remote side told us it was busy.
						call->ResultSet (estiRESULT_REMOTE_SYSTEM_BUSY);
						sipCall->CallEnd (estiSUBSTATE_BUSY, SIPRESP_NONE);
					}
					else
					{
						// Remote decided not to call us after all.
						// If this endpoint is behind an IVVR we need to log a missed call.
						if (call->IsBehindIVVRGet())
						{
							call->AddMissedCallSet (true);
						}
						sipCall->CallEnd (estiSUBSTATE_REMOTE_HANGUP, SIPRESP_NONE);
					}
					break;

				case RVSIP_CALL_LEG_REASON_REQUEST_FAILURE:
				case RVSIP_CALL_LEG_REASON_SERVER_FAILURE:
				case RVSIP_CALL_LEG_REASON_GLOBAL_FAILURE:
				case RVSIP_CALL_LEG_REASON_LOCAL_FAILURE:
				case RVSIP_CALL_LEG_REASON_NETWORK_ERROR:
				case RVSIP_CALL_LEG_REASON_OUT_OF_RESOURCES:
				case RVSIP_CALL_LEG_REASON_LOCAL_TIME_OUT:
				case RVSIP_CALL_LEG_REASON_UNDEFINED:
				case RVSIP_CALL_LEG_REASON_LOCAL_INVITING:
				case RVSIP_CALL_LEG_REASON_REMOTE_INVITING:
				case RVSIP_CALL_LEG_REASON_LOCAL_REFER:
				case RVSIP_CALL_LEG_REASON_REMOTE_REFER:
				case RVSIP_CALL_LEG_REASON_LOCAL_REFER_NOTIFY:
				case RVSIP_CALL_LEG_REASON_REMOTE_REFER_NOTIFY:
				case RVSIP_CALL_LEG_REASON_LOCAL_ACCEPTED:
				case RVSIP_CALL_LEG_REASON_REMOTE_ACCEPTED:
				case RVSIP_CALL_LEG_REASON_REMOTE_ACK:
				case RVSIP_CALL_LEG_REASON_REDIRECTED:
				case RVSIP_CALL_LEG_REASON_CALL_TERMINATED:
				case RVSIP_CALL_LEG_REASON_AUTH_NEEDED:
				case RVSIP_CALL_LEG_REASON_UNSUPPORTED_AUTH_PARAMS:
				case RVSIP_CALL_LEG_REASON_LOCAL_CANCELLING:
				case RVSIP_CALL_LEG_REASON_ACK_SENT:
				case RVSIP_CALL_LEG_REASON_CALL_CONNECTED:
				case RVSIP_CALL_LEG_REASON_REMOTE_PROVISIONAL_RESP:
				case RVSIP_CALL_LEG_REASON_REMOTE_REFER_REPLACES:
				case RVSIP_CALL_LEG_REASON_REMOTE_INVITING_REPLACES:
				case RVSIP_CALL_LEG_REASON_GIVE_UP_DNS:
				case RVSIP_CALL_LEG_REASON_CONTINUE_DNS:
				case RVSIP_CALL_LEG_REASON_FORKED_CALL_NO_FINAL_RESPONSE:
					// The connection was lost.
				    if (call->StateValidate (esmiCS_CONNECTING | esmiCS_IDLE))
					{
						if (call->DialMethodGet() == estiDM_BY_VRS_PHONE_NUMBER ||
						     call->DialMethodGet() == estiDM_BY_VRS_WITH_VCO)
						{
							stiASSERTMSG(estiFALSE, "Failed to connect VRS call Reason=%d", callLegStateChangeReasonGet (eReason).c_str ());
						}
						// If there was no established call, a lost invite means the dialed phone is unreachable
						call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
						sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
					}
					else
					{
						// If we were in a call, a lost invite means that the network connection has been broken
						call->ResultSet (estiRESULT_LOST_CONNECTION);
						sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_NONE);
					}
					break;
					
				case RVSIP_CALL_LEG_REASON_503_RECEIVED:
				{
					// Try next DNS entry (if any)
					RvSipTranscHandle hTransaction = nullptr;
					RvStatus rvStatus = RV_OK;

					rvStatus = RvSipCallLegDNSContinue (hCallLeg, hTransaction, &hTransaction);
					if (rvStatus == RV_OK)
					{
						rvStatus = RvSipCallLegDNSReSendRequest (hCallLeg, hTransaction);
					}
					if (rvStatus != RV_OK)
					{
						DBG_MSG ("503 response and ran out of ip addresses to try");
						
						// The connection was lost.
						if (call->StateValidate (esmiCS_CONNECTING | esmiCS_IDLE))
						{
							// If there was no established call, a lost invite means the dialed phone is unreachable
							call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
							sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
						}
						else
						{
							// If we were in a call, a lost invite means that the network connection has been broken
							call->ResultSet (estiRESULT_LOST_CONNECTION);
							sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_NONE);
						}
					}

					break;
				}
			}
		}
		
		// If it was a LOOKUP then we never got our redirected notification,
		// therefore send an error up to the app.

		if (m_hLookupCallLeg == hCallLeg)
		{
			if (m_bSwitchProtocols)
			{
				call->ContinueDial ();
			}
			else
			{
				// Hang it up and flag it invalid
				call->ResultSet (estiRESULT_NOT_FOUND_IN_DIRECTORY);
				call->HangUp ();
			}

			m_hLookupCallLeg = nullptr;

			DBG_MSG ("Lookup TERMINATED!");
		}
	}
			
	return hResult;
}


/*!\brief Checks if the call is able to be placed again using the VRS failover server.
 *
 * Failover is allowed if the call is in a connecting state, is a VRS call, and we did not
 * hang up locally or already failover.
 *
 *  \return true or false.
 */
bool VRSFailoverAllowed (
    CstiCallSP call,
    RvSipCallLegStateChangeReason reason)
{
	return (call->StateGet() == esmiCS_CONNECTING &&
	        (call->DialMethodGet() == estiDM_BY_VRS_PHONE_NUMBER || call->DialMethodGet() == estiDM_BY_VRS_WITH_VCO) &&
	        (reason != RVSIP_CALL_LEG_REASON_LOCAL_CANCELLING || call->UseVRSFailoverGet ()) &&
	        !call->VRSFailOverServerGet().empty());
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegStateChangeCB
//
// Description: Sip callback for handling call leg's state events.
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::CallLegStateChangeCB (
	RvSipCallLegHandle hCallLeg,
	RvSipAppCallLegHandle hAppCallLeg,
	RvSipCallLegState eState,
	RvSipCallLegStateChangeReason eReason)
{
	RvSipCallLegDirection eDirection;
	RvSipCallLegGetDirection (hCallLeg, &eDirection);

	auto  callLegKey = (size_t)hAppCallLeg;
	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	auto pThis = (CstiSipManager *)sipCall->ProtocolManagerGet ();

	pThis->Lock ();

	auto inboundMessage = callLeg->inboundMessageGet();

	stiDEBUG_TOOL (g_stiSipCallBackTracking,
	    PrintCallState (call, eState, eReason);
	);

	call->collectTrace (formattedString ("%s: state %d, reason %s", __func__, eState, callLegStateChangeReasonGet (eReason).c_str ()));
	
	switch (eState)
	{
		case RVSIP_CALL_LEG_STATE_OFFERING:
			if (!inboundMessage)
			{
				stiASSERTMSG (estiFALSE, "No inbound message");
			}

			if (RVSIP_CALL_LEG_DIRECTION_OUTGOING != eDirection)
			{
				if (pThis)
				{
					if (!pThis->m_isAvailable)
					{
						RvSipCallLegReject (hCallLeg, SIPRESP_SERVICE_UNAVAILABLE);
					}
					else
					{
						pThis->OfferingCallProcess (hCallLeg, inboundMessage, sipCall);
						
						// iOS must be notified of a preincoming call to inform the UI that
						// it has received the SIP invite it received a push notification for.
						// ntouch iOS assumes if the SIP INVITE for a push notification doesn't
						// arrive quickly, it won't ever arrive and thus stops ringing, causing
						// the app to suspend while we're doing ICE negotiation.

						if (sipCall->CstiCallGet()->NotifyAppOfIncomingCallGet())
						{
							pThis->preIncomingCallSignal.Emit (call);
						}
					}
				}
				else
				{
					stiASSERTMSG (estiFALSE, "Someone attempted to call us, but we can't take SIP calls.");
					RvSipCallLegReject (hCallLeg, SIPRESP_NOT_IMPLEMENTED);
				}
			}
			break;

		case RVSIP_CALL_LEG_STATE_CANCELLED:
			// Need to flag the call as invalid if it was an inbound call and the ui was never notified.
		    if (sipCall && call->StateValidate (esmiCS_IDLE))
			{
				sipCall->CallValidSet (false);
				call->NotifyAppOfCallSet(estiFALSE);
			}
			break;

		case RVSIP_CALL_LEG_STATE_DISCONNECTING:
			// We are trying to disconnect them.
		    if (sipCall)
			{
				sipCall->CallEnd (estiSUBSTATE_UNKNOWN, SIPRESP_NONE);
			}
			break;

		case RVSIP_CALL_LEG_STATE_MSG_SEND_FAILURE:

			// These resources must be freed prior to a stack restart
			if (call)
			{
				if (call->StateGet() == esmiCS_CONNECTING)
				{
					//
					// We reached this state while trying to setup a new call
					// Set the call result to unreachable and disconnect the call leg.
					//
					call->ResultSet(estiRESULT_REMOTE_SYSTEM_UNREACHABLE);

					RvSipCallLegDisconnect (hCallLeg);
				}
				else
				{
					//
					// We reached this state while trying to disconnect the call.
					// Terminate the call.
					// NOTE: RvSipCallLegDisconnect does not work in this state
					// when trying to disconnect the call.  Therefore, we must call
					// RvSipCallLegTerminate.
					//
					RvSipCallLegTerminate (hCallLeg);
				}
				stiASSERTMSG(estiFALSE, "RVSIP_CALL_LEG_STATE_MSG_SEND_FAILURE: Reason = %s %s ", callLegStateChangeReasonGet (eReason).c_str (), sipCall->IsBridgedCall() ? "BridgeCall" : "");
			}

#if defined stiHOLDSERVER || APPLICATION == APP_NTOUCH_IOS
			if (eReason == RVSIP_CALL_LEG_REASON_NETWORK_ERROR)
			{
				stiASSERTMSG (estiFALSE, "CstiSipManager::CallLegStateChangeCB: Calling 'StackRestart()' when Transaction State is \"%d\" and Transaction state change reason is  \"%s\"", eState, callLegStateChangeReasonGet (eReason).c_str ());

				DBG_MSG ("CstiSipManager::CallLegStateChangeCB, calling 'StackRestart()' State %d, Reason %s ", eState, callLegStateChangeReasonGet (eReason).c_str ());

				// Notify the Manager to retry a stack restart
				pThis->StackRestart ();
			}
#endif

			break;

		case RVSIP_CALL_LEG_STATE_DISCONNECTED:
			
			// If this is a VRS call and the user is not choosing to hang up,
			// failover if we have not yet done so.
		    if (VRSFailoverAllowed (call, eReason))
			{
				std::string address = call->UriGet ();
				
				// Fall back to the remote IP which we know works if for whatever reason we dont have a uri value.
				if (address.empty ())
				{
					address = call->RemoteIpAddressGet ();
				}
				
				stiRemoteLogEventSend ("EventType=VRSFailover Reason=FailoverTimerFired Address=%s", address.c_str());
				
				call->UseVRSFailoverSet (false);
				sipCall->VRSFailOverConfigure ();
			}
			else if (pThis)
			{
				pThis->DisconnectedCallProcess (hCallLeg, sipCall, eReason);
			}
			
			break;

		case RVSIP_CALL_LEG_STATE_PROCEEDING_TIMEOUT:
		{
			//
			// Only do the normal operation if the call leg handle
			// matches the one assigned to the sip call object.  If it doesn't
			// match then it must be a forked call leg that wasn't answered.
			//
		    if (sipCall && hCallLeg == sipCall->StackCallHandleGet ())
			{
				// The call timed out.  Flag as "busy", and cancel it.
				call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
				sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_NONE);
			}
			else
			{
				RvSipCallLegTerminate (hCallLeg);
			}
			
			break;
		}

		case RVSIP_CALL_LEG_STATE_TERMINATED:
	    {
		    auto callLeg = sipCall->CallLegGet (hCallLeg);

			// If we are going to use a failover server delete the sip call object,
			// reset the state, and place the call again. Otherwise continue terminating the call as usual.
			if (pThis && call)
			{
				// Only log calls we want to. Currently all calls are logged except calls we flag as SPIT.
				if (call->logCallGet())
				{
					pThis->logCallSignal.Emit(call, callLeg);
				}
				
				if (sipCall->m_useVRSFailoverServer)
				{
					pThis->NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_RESOLVE_NAME);
					auto sipCall = std::make_shared<CstiSipCall> (&pThis->m_stConferenceParams, pThis, pThis->m_vrsFocusedRouting);
					call->ProtocolCallSet (sipCall);
					sipCall->CstiCallSet (call);
					call->ContinueDial ();
				}
				else if (call->SubstateGet () == estiSUBSTATE_CREATE_VRS_CALL)
				{
					auto sipCall = std::make_shared<CstiSipCall> (&pThis->m_stConferenceParams, pThis, pThis->m_vrsFocusedRouting);
					call->ProtocolCallSet (sipCall);
					sipCall->CstiCallSet (call);

					std::string dialString {};
					call->OriginalDialStringGet (&dialString);
					// If Suicide Lifeline, forward to VRS immediately, otherwise ask user
					if (dialString != SUICIDE_LIFELINE)
					{
						call->vrsPromptRequiredSet (true);
					}

					call->StateSet (esmiCS_IDLE);
					call->ResultSet (estiRESULT_UNKNOWN);

					pThis->createVrsCallSignal.Emit (call);
				}
				else
				{
					if (call->ForcedVRSFailoverGet () && call->ResultGet () != estiRESULT_CALL_SUCCESSFUL)
					{
						stiRemoteLogEventSend ("EventType=VRSFailover Reason=CallResult Result=%s",
							call->ResultNameGet ().c_str ());
					}
					//
					// Only do the normal operation if the call leg handle
					// matches the one assigned to the sip call object.  If it doesn't
					// match then it must be a forked call leg that wasn't answered.
					//
					if (hCallLeg == sipCall->StackCallHandleGet ())
					{
						//
						// Make sure the call is ended properly.
						//
						sipCall->CallEnd (estiSUBSTATE_UNKNOWN, SIPRESP_NONE);

						// if this is the bridged call, remove from call storage too
						if (sipCall->IsBridgedCall ())
						{
							pThis->m_pCallStorage->remove (call.get ());
						}

						if (pThis->m_transferSipCall == sipCall)
						{
							pThis->m_transferSipCall = nullptr;
						}

						if (pThis->m_transferCall == call)
						{
							pThis->m_transferCall = nullptr;
						}

						//
						// If the original call object pointer refers
						// to the call being terminated then set the pointer
						// to null to free the reference.
						//
						if (pThis->m_originalSipCall == sipCall)
						{
							pThis->m_originalSipCall = nullptr;
						}

						if (pThis->m_originalCall == call)
						{
							pThis->m_originalCall = nullptr;
						}

						// Remove the internal shared pointor so this call can reach a refcount
						// of zero and be destructed.  This is to get around Radvision's C API.
						call->allowDestruction ();
					}
				}
			}
			// Remove the weak_ptr from m_callLegMap
			// This map is used by Radvision sip stack callbacks to lookup CallLeg/SipCall instances
			if (callLeg)
			{
				auto callLegKey = callLeg->uniqueKeyGet ();

				m_callLegMapMutex.lock ();
				if (m_callLegMap.count(callLegKey) > 0)
				{
					m_callLegMap.erase (callLegKey);
				}
				m_callLegMapMutex.unlock ();
			}

			break;
	    }

		case RVSIP_CALL_LEG_STATE_REMOTE_ACCEPTED:
		{
			if (!inboundMessage)
			{
				stiASSERTMSG (estiFALSE, "No inbound message");
			}

			//
			// We received the 200 OK.
			//
			CstiSdp Sdp;

			//
			// Was there SDP in the 200 OK?
			//
			Sdp.initialize (inboundMessage);

			if (!Sdp.empty ())
			{
				//
				// Yes, process the SDP.
				//
				auto callLeg = sipCall->CallLegGet (hCallLeg);
				auto hResult = callLeg->SdpProcess (&Sdp, estiOfferAnswerINVITEFinalResponse);
				if (stiRESULT_CODE (hResult) == stiRESULT_SECURITY_INADEQUATE)
				{
					sipCall->CstiCallGet ()->ResultSet (estiRESULT_SECURITY_INADEQUATE);
					sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE_HERE);
				}
			}
			else
			{
				//
				// No, just ack the response.
				//
				RvSipCallLegAck (hCallLeg);
			}

			break;
		}

		case RVSIP_CALL_LEG_STATE_CONNECTED:
			if (!inboundMessage)
			{
				stiASSERTMSG (estiFALSE, "No inbound message");
			}

			//If its not a proxy lookup, connect it and tell the app about it.
		    if (call && pThis)
			{
				CallIDSet(sipCall, hCallLeg);
				
				//
				// Set the call leg to that of the call that answered.
				//
				sipCall->StackCallHandleSet (hCallLeg);
				auto callLeg = sipCall->CallLegGet (hCallLeg);
				if (!callLeg)
				{
					stiASSERTMSG (estiFALSE, "Call object missing.  Hangup call leg.");
					sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
				}
				else
				{
					pThis->ConnectedHeaderParse (inboundMessage, sipCall); //We are parsing the SIP header, currently it always succeeds so we aren't checking the hResult

					stiHResult hResult = pThis->ConnectedCallProcess (callLeg, eDirection);
					if (stiIS_ERROR (hResult))
					{
						sipCall->CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
					}
					
					// Reset the login counter in case the proxy demands authentication for something else
					sipCall->m_bProxyLoginAttempted = false;

					//
					// If the "send offer when ready" boolean is true then
					// ICE nominations were completed after the call was anwered.
					// Send a re-invite to complete the ICE setup.
					//
					if (callLeg->m_bSendOfferWhenReady)
					{
						callLeg->m_bSendOfferWhenReady = false;

						pThis->ReInviteSend(call, 0, false);
					}
				}
			}
			break;

		case RVSIP_CALL_LEG_STATE_UNAUTHENTICATED:
		    if (pThis && call)
			{
				if (sipCall->m_bProxyLoginAttempted)
				{
					DBG_MSG ("Username/password incorrect");
					// Hang it up and flag it invalid
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
					call->HangUp ();
				}
				else
				{
					sipCall->m_bProxyLoginAttempted = true;

					DBG_MSG ("The sip proxy says we must authenticate, so we shall.");
					
					{
						auto outboundMessage = callLeg->outboundMessageGet();

						auto hResult = outboundMessage.inviteRequirementsAdd ();
						stiASSERT(!stiIS_ERROR(hResult));

						auto callLeg = sipCall->CallLegGet (hCallLeg);

						// If this call should always be accepted, add the allow call header
						if (call->AlwaysAllowGet() || (estiTECH_SUPPORT_MODE == pThis->m_eLocalInterfaceMode))
						{
							// Add the AllowIncomingCall header
							auto hResult = outboundMessage.allowIncomingCallHeaderAdd();
							stiASSERT(!stiIS_ERROR(hResult));
						}

						// If this call is going to be converted to H323 we need to add the SIP to H323 header
						if (sipCall->m_bAddSipToH323Header)
						{
							auto hResult = outboundMessage.convertSipToH323HeaderAdd();
							stiASSERT(!stiIS_ERROR(hResult));
						}

						{
							//
							// Add SDP to the body if there is SDP to be added.
							// This could be improved by making sure that the request being challenged
							// is an INVITE, UPDATE or a PRACK.  These are the only requests that allow SDP.
							//
							stiHResult hResult = stiRESULT_SUCCESS;
							
							if (!callLeg->m_sdp.empty ())
							{
								hResult = outboundMessage.bodySet (callLeg->m_sdp);
								if (stiIS_ERROR (hResult))
								{
									stiASSERTMSG (estiFALSE, "failed to attach SDP");
									call->HangUp ();
								}
							}

							//
							// If there were no issues then send the request with authentication.
							//
							if (!stiIS_ERROR (hResult))
							{
								RvSipCallLegAuthenticate (hCallLeg);
							}
						}
					}
				}
			}
			break;

		case RVSIP_CALL_LEG_STATE_REDIRECTED:
			if (pThis && inboundMessage)

			{
				pThis->Redirect (call, inboundMessage, hCallLeg);
			}
			break;

		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_INVITING)
		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_IDLE)
		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_ACCEPTED)
		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_CANCELLING)
		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_PROCEEDING)
		UNHANDLED_CASE(RVSIP_CALL_LEG_STATE_UNDEFINED)
		//default:
			break;
	}
	
	RV_UNUSED_ARG (hAppCallLeg);
	RV_UNUSED_ARG (eReason);

	pThis->Unlock ();
} // end CstiSipManager::CallLegStateChangeCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ConnectedHeaderParse
//
// Description: Parses the SIP Header of the call
// Our connection to the MCU is determined by the isfocus in the sip header
//
// Abstract:
//
// Returns:
//  stiRESULT_SUCCESS on success
//
stiHResult CstiSipManager::ConnectedHeaderParse (
	vpe::sip::Message inboundMsg,
    CstiSipCallSP sipCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto contactHeaders = inboundMsg.contactHeadersGet();

	if (!contactHeaders.empty())
	{
		auto otherParams = contactHeaders[0].otherParamsGet();

		// If we succeeded and if the other params contains "isfocus", set the call to know that it is
		// connected with an MCU.
		if (otherParams.find ("isfocus") != std::string::npos)
		{
			auto call = sipCall->CstiCallGet ();
			auto dialString = call->RoutingAddressGet ().UriGet ();

			// Is it a conference call with a Group Video Chat room?
			if (m_GvcUri->Match (dialString))
			{
				// The dialString is of the pattern for a GVC
				call->ConnectedWithMcuSet (estiMCU_GVC);

				// Extract the Conference ID from the uri and store it with the Call object
				std::string Guid = dialString.substr (nMCU_GUID_START_POS_IN_URI, nMCU_GUID_LEN_IN_URI);

				call->ConferencePublicIdSet (&Guid);
			}
			else
			{
				call->ConnectedWithMcuSet (estiMCU_GENERIC);
			}

			// Inform the application that we are connected with an MCU.
			conferencingWithMcuSignal.Emit (call);
		}
	}

	return hResult;
}

/*!\brief Check if the current call is from a registered proxy connection
 *
 * NOTE: this is used in certain places instead of
 * IsUsingOutboundProxy () which favors a current transaction, and
 * then falls back to IsUsingOutboundProxy ()
 */
bool CstiSipManager::IsCallOnRegisteredProxyConnection (const CstiCallSP &call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus;
	bool callUsingProxy = false;
	CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall> (call->ProtocolCallGet ());
	RvSipTransportConnectionHandle callConnectionHandle = nullptr;
	RvSipTranscHandle transactionHandle = nullptr;

	RvSipCallLegHandle callLegHandle = sipCall->StackCallHandleGet ();
	stiTESTCOND (callLegHandle != nullptr, stiRESULT_ERROR);

	// TODO: can't get the handle from the call leg
	// rvStatus = RvSipCallLegGetConnection (callLegHandle, &callConnectionHandle);

	// Get from the transaction instead
	rvStatus = RvSipCallLegGetActiveTransaction (callLegHandle, &transactionHandle);
	if (RV_OK == rvStatus)
	{
		rvStatus = RvSipTransactionGetConnection (transactionHandle, &callConnectionHandle);
		stiTESTRVSTATUS ();

		for (const auto &keyValue : m_Registrations)
		{
			// Check if connection belongs to a registration
			if (keyValue.second->ConnectionHandleGet () == callConnectionHandle)
			{
				callUsingProxy = true;
				break;
			}
		}
	}

	stiUNUSED_ARG (hResult);

STI_BAIL:

	return callUsingProxy;
}

/*!
 * \brief Get reflexive ip/port from registration info
 */
bool CstiSipManager::ProxyConnectionReflexiveIPPortGet (std::string *ipAddress, uint16_t *port, EstiTransport *transport)
{
	bool found = false;

	for (const auto &keyValue : m_Registrations)
	{
		if (keyValue.second)
		{
			keyValue.second->ReflexiveAddressGet (ipAddress, port);
			*transport = keyValue.second->TransportGet ();
			found = true;
		}
	}

	return found;
}


/*!\brief Indicates whether or not the endpoint is configured to use an outbound proxy
 */
bool CstiSipManager::IsUsingOutboundProxy (
    CstiCallSP call)
{
	bool bUsingProxy = false;
	
	if (m_productType != ProductType::MediaServer && m_productType != ProductType::MediaBridge)
	{

	// Cases:

	// Interpreter modes:
	// NOTE: same results for registered and unregistered phones
	// * calling this function without a call: true
	// * bridge call (implicitly outgoing): false
	// * outgoing call: true
	// * NEW: when call is incoming, should use the same path as incoming (seems like this would be true for all cases)
	// * Incoming Calls (TODO: should this function get used for incoming calls?):
	//   * registered: use proxy
	//   * unregistered: not using proxy

	// Customer mode:
	// * Calling this function without a call: ? (previously false)
	// * Registered: true
	// * Unregistered: false

	// Ported mode:
	// * always true

	// Other modes: ?

	// TODO:
	// for interpreters, always true unless a bridge call
	// for ported: always true
	// for customers, if regisetered, otherwise not

		if (IsLocalAgentMode ())
		{
			if (call == nullptr)
			{
				bUsingProxy = true;
			}
			else
			{
				//
				// If we have a call object and it is not a call bridge 
				// and it is an outbound call then indicte that we
				// are using an outbound proxy.
				//
				CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());

				// If a bridge call has the "ob" URI parameter then we want the call to go through
				// the registered proxy connection.

				if ((!sipCall->IsBridgedCall() || call->RoutingAddressGet().hasParameter("ob")) && call->DirectionGet() == estiOUTGOING)
				{
					bUsingProxy = true;
				}
			}
		}
		else if (UseInboundProxy() || estiPORTED_MODE == m_eLocalInterfaceMode)
		{
			bUsingProxy = true;
		}
	}

	return bUsingProxy;
}


/*!\brief Indicates whether or not the endpoint is configured to use an inbound proxy
 *
 */
bool CstiSipManager::UseInboundProxy () const
{
	//
	// For all modes, check the NAT Traversal setting and the
	// presence of the myPhone numbers (which require NAT Traversal).
	//
	return (m_productType != ProductType::MediaBridge && m_productType != ProductType::MediaServer) //Media Server Products should return false
		&& (m_stConferenceParams.eNatTraversalSIP != estiSIPNATDisabled //NatTraversal Enabled or
			|| m_UserPhoneNumbers.szRingGroupLocalPhoneNumber[0] != '\0' //Has a MyPhoneGroup number
			|| m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber[0] != '\0');
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::Redirect
//
// Description: Handle a SIP redirect condition
//
// Returns: nothing
//
#define URI_MAX_SIZE 128
void CstiSipManager::Redirect (
    CstiCallSP call, ///< The call which will be redirected (can be NULL if this was a "ported mode" lookup)
	RvSipMsgHandle hMsg, ///< The radvision message containing redirect information
	RvSipCallLegHandle hCallLeg) ///< The call leg this message came in on
{
	std::string host;
	char szUser[URI_MAX_SIZE]; szUser[0] = '\0';
	int nPort = 0;
	RvSipHeaderListElemHandle hListElem = nullptr;
	RvSipContactHeaderHandle hContactHeader;
	const char *pszProtocol = "sip";

	hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hMsg, RVSIP_HEADERTYPE_CONTACT, RVSIP_FIRST_HEADER, &hListElem);
	if (hContactHeader)
	{
		float nQVal = 1.0F;
		float nBestQVal = 0.0F;
		while (hContactHeader)
		{
			// Find which is the highest preference one
			// If no qval (preference level) is specified for an entry, assume entries are in preferred order.
			char szQVal[7];
			RvUint nQValLen;
			if (RV_OK == RvSipContactHeaderGetQVal (hContactHeader, (RvChar*)szQVal, 7, &nQValLen))
			{
				auto  nTempQVal = (float)atof (szQVal);
				if ((nTempQVal > 0.00000001F))
				{
					nQVal = nTempQVal;
				}
				else
				{
					nQVal -= 0.001F;
				}
			}
			else
			{
				nQVal -= 0.001F;
			}

			RvSipAddressHandle hAddress = RvSipContactHeaderGetAddrSpec (hContactHeader);
			if (!hAddress)
			{
				stiASSERTMSG (estiFALSE, "Unable to find contact address");
			}
			else
			{
				if (nQVal > nBestQVal)
				{
					nBestQVal = nQVal;
					AddrUrlGetHost (hAddress, &host);
					nPort = AddrUrlGetPortNum (hAddress);
					if (call)
					{
						int nUserLen = sizeof (szUser);
						RvSipAddrUrlGetUser (hAddress, (RvChar*)szUser, nUserLen, (RvUint*)&nUserLen);
						pszProtocol = "sip";
					}
				}
				hContactHeader = (RvSipContactHeaderHandle)RvSipMsgGetHeaderByType (hMsg, RVSIP_HEADERTYPE_CONTACT, RVSIP_NEXT_HEADER, &hListElem);
			}
		}
	}

	if (!call)
	{
		stiASSERTMSG (estiFALSE, "Call leg did not match to a call object.");
		RvSipCallLegDisconnect (hCallLeg);
	}
	else if (host.empty ())
	{
		// Hang it up and flag it invalid
		call->ResultSet (estiRESULT_NOT_FOUND_IN_DIRECTORY);
		call->HangUp ();
	}
	else
	{
		std::stringstream address;

		address << pszProtocol << ":" << szUser << "@" << host << ":" << nPort;

		DBG_MSG ("Got \"redirected\" proxy response %s",  address.str ().c_str ());

		// Test for dial self.
		std::string LocalIPv4;
		std::string LocalIPv6;
		stiGetLocalIp(&LocalIPv4, estiTYPE_IPV4);
#ifdef IPV6_ENABLED
		stiGetLocalIp (&LocalIPv6, estiTYPE_IPV6);
#endif
		if (
			0 == strcmp (szUser, m_UserPhoneNumbers.szLocalPhoneNumber) ||
			0 == strcmp (szUser, m_UserPhoneNumbers.szTollFreePhoneNumber) ||
			0 == strcmp (szUser, m_UserPhoneNumbers.szRingGroupLocalPhoneNumber) ||
			0 == strcmp (szUser, m_UserPhoneNumbers.szRingGroupTollFreePhoneNumber) ||
			0 == strcmp (szUser, m_UserPhoneNumbers.szSorensonPhoneNumber) ||
			host == "127.0.0.1" ||
			host == "::1" ||
			(LocalIPv4 == host && m_stConferencePorts.nListenPort == nPort) ||
			(LocalIPv6 == host && m_stConferencePorts.nListenPort == nPort) ||
			(!m_proxyDomain.empty () &&
				(m_stConferenceParams.PublicIPv4 == host || m_stConferenceParams.PublicIPv6 == host) &&
				m_stConferencePorts.nListenPort == nPort
			) // This one is only valid if no proxy is involved. (1-to-1 NAT)
		)
		{
			call->ResultSet (estiRESULT_REMOTE_SYSTEM_REJECTED);
			//call->ResultSet (estiRESULT_DIALING_SELF); // TODO: USe this instead?
			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			sipCall->CallEnd (estiSUBSTATE_REJECT, SIPRESP_NONE);
		}
		else
		{
			CstiRoutingAddress RoutingAddress (address.str ());
			call->RoutingAddressSet(RoutingAddress);
			m_bSwitchProtocols = true;
		}
	}

	if (!m_bSwitchProtocols && hCallLeg == m_hLookupCallLeg)
	{
		m_hLookupCallLeg = nullptr;
	}
	RvSipCallLegDisconnect (hCallLeg);
} // end CstiSipManager::Redirect
#undef URI_MAX_SIZE


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::AuthenticationLoginCB
//
// Description: Callback to ask us for the registrar login credentials.
//
// Abstract:
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::AuthenticationLoginCB (
	IN RvSipAuthenticatorHandle hAuthenticator,
	IN RvSipAppAuthenticatorHandle hAppAuthenticator,
	IN void* hObject,
	IN void* peObjectType,
	IN RPOOL_Ptr *pRpoolRealm,
	OUT RPOOL_Ptr *pRpoolUserName, /// The user name to log in with
	OUT RPOOL_Ptr *pRpoolPassword) /// The password to log in with
{
	auto pThis = (CstiSipManager*)hAppAuthenticator;
	
	pThis->Lock ();

	// Attempt to find per-phone-number login information
	// TODO: May want to add more case's for other types of Radvision objects
	
	switch (*(RvSipCommonStackObjectType*)peObjectType)
	{
		case RVSIP_COMMON_STACK_OBJECT_TYPE_REG_CLIENT:
		{
			std::map<std::string, std::string>::iterator it;const char *pszUserName = nullptr;
			RvSipAppRegClientHandle hAppRegClient;
			RvSipRegClientGetAppHandle ((RvSipRegClientHandle)hObject, &hAppRegClient);
			if (!hAppRegClient)
			{
				stiASSERTMSG (estiFALSE, "Register object missing");
			}
			else
			{
				pszUserName = ((CstiSipRegistration*)hAppRegClient)->PhoneNumberGet ();

				it = pThis->m_stConferenceParams.SIPRegistrationInfo.PasswordMap.find (pszUserName);

				if (it != pThis->m_stConferenceParams.SIPRegistrationInfo.PasswordMap.end ())
				{
					// Appends the username to the given page.
					RPOOL_AppendFromExternalToPage (
						pRpoolUserName->hPool,
						pRpoolUserName->hPage,
						(void*) pszUserName,
						strlen (pszUserName) + 1,
						& (pRpoolUserName->offset));
					
					// Appends the password to the given page.
					RPOOL_AppendFromExternalToPage (
						pRpoolPassword->hPool,
						pRpoolPassword->hPage,
						(void*) it->second.c_str(),
						strlen (it->second.c_str()) + 1,
						& (pRpoolPassword->offset));

					//DBG_MSG ("Sending credentials: %s, %s\n", pszUserName, it->second.c_str ());
				}
			}
			
			break;
		}
			
		case RVSIP_COMMON_STACK_OBJECT_TYPE_CALL_LEG:
		{
			auto pThis = (CstiSipManager *)hAppAuthenticator;
			
			if (pThis)
			{
				CstiSipRegistration::SstiRegistrarSettings RegSettings;
				
				// Determine the credentials for whichever phone number we are authenticating
				char szPhoneNumberInUse[un8stiCANONICAL_PHONE_LENGTH + 1];
				szPhoneNumberInUse[0] = '\0';
				
				RvSipCallLegDirection eDirection;
				RvSipCallLegGetDirection ((RvSipCallLegHandle)hObject, &eDirection);
				if (RVSIP_CALL_LEG_DIRECTION_INCOMING == eDirection)
				{
					RvSipPartyHeaderHandle hPartyHeader;
					RvStatus rvStatus;
					rvStatus = RvSipCallLegGetToHeader ((RvSipCallLegHandle)hObject, &hPartyHeader);
					if (RV_OK == rvStatus)
					{
						RvUint nActualLen;
						RvSipAddressHandle hOurAddress = RvSipPartyHeaderGetAddrSpec (hPartyHeader);
						RvSipAddrUrlGetUser (hOurAddress, (RvChar*)szPhoneNumberInUse, sizeof (szPhoneNumberInUse), &nActualLen);
					}
				}
				else
				{
					RvSipPartyHeaderHandle hPartyHeader;
					RvStatus rvStatus;
					rvStatus = RvSipCallLegGetFromHeader ((RvSipCallLegHandle)hObject, &hPartyHeader);
					if (RV_OK == rvStatus)
					{
						RvUint nActualLen;
						RvSipAddressHandle hOurAddress = RvSipPartyHeaderGetAddrSpec (hPartyHeader);
						RvSipAddrUrlGetUser (hOurAddress, (RvChar*)szPhoneNumberInUse, sizeof (szPhoneNumberInUse), &nActualLen);
					}
				}
				
				//Remove the + prefix if present in the phone number or SMX authentication will fail
				if (0 < strlen(szPhoneNumberInUse) && szPhoneNumberInUse[0] == '+')
				{
					for (size_t i = 0; i < strlen(szPhoneNumberInUse); i++)
					{
						szPhoneNumberInUse[i] = szPhoneNumberInUse[i + 1];
					}
				}

				std::map<std::string, std::string>::iterator it;

				it = pThis->m_stConferenceParams.SIPRegistrationInfo.PasswordMap.find (szPhoneNumberInUse);

				if (it != pThis->m_stConferenceParams.SIPRegistrationInfo.PasswordMap.end ())
				{
					// Appends the username to the given page.
					RPOOL_AppendFromExternalToPage (
						pRpoolUserName->hPool,
						pRpoolUserName->hPage,
						(void*) szPhoneNumberInUse,
						strlen (szPhoneNumberInUse) + 1,
						& (pRpoolUserName->offset));
					
					// Appends the password to the given page.
					RPOOL_AppendFromExternalToPage (
						pRpoolPassword->hPool,
						pRpoolPassword->hPage,
						(void*) it->second.c_str (),
						strlen (it->second.c_str ()) + 1,
						& (pRpoolPassword->offset));

					//DBG_MSG ("Sending credentials: %s, %s\n", szPhoneNumberInUse, it->second.c_str ());
				}
			}
			
			break;
		}
		
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_UNDEFINED)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_TRANSACTION)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_CALL_TRANSACTION)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_SUBSCRIPTION)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_NOTIFICATION)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_PUB_CLIENT)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_CONNECTION)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_APP_OBJECT)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_CALL_INVITE)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_TRANSMITTER)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_SECURITY_AGREEMENT)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_SECURITY_OBJECT)
		UNHANDLED_CASE (RVSIP_COMMON_STACK_OBJECT_TYPE_COMPARTMENT)
	}

	pThis->Unlock ();
} // end CstiSipManager::AuthenticationLoginCB


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::AuthenticationMD5CB
//
// Description: Callback to have us generate a MD5 password hash.
//
// Abstract: The stack does not create these obscured passwords internally
//		and relies on the application (us) to generate it.
//
// Returns: Nothing
//
void RVCALLCONV CstiSipManager::AuthenticationMD5CB (
	IN RvSipAuthenticatorHandle hAuthenticator,
	IN RvSipAppAuthenticatorHandle hAppAuthenticator,
	IN RPOOL_Ptr *pRpoolMD5Input,
	IN RvUint32 length,
	OUT RPOOL_Ptr *pRpoolMD5Output)
{
	std::vector<unsigned char> input;
	RvChar strResponse[33];
	RvUint8 digest[20];
	MD5_CTX mdc;

	//pThis->Lock (); <-- uncomment if this method ever accesses the sip manager.

	input.resize (length);

	// Gets the string out of the page.
	RPOOL_CopyToExternal (pRpoolMD5Input->hPool,
		pRpoolMD5Input->hPage,
		pRpoolMD5Input->offset,
		 (void*) input.data (),
		length);

	// Pass it through the MD5 algorithm.

	MD5_Init (&mdc);
	MD5_Update (&mdc, input.data (), length);
	MD5_Final (digest, &mdc);
	// Changes the digest into a string format.
	for (unsigned int i = 0; i < 16; i++)
	{
		sprintf (&strResponse[i * 2], "%02x", digest[i]);
	}
	
	//DBG_MSG ("\tResult: %s", strResponse);
	// Inserts the MD5 output to the page that is supplied in pRpoolMD5Output.
	RPOOL_AppendFromExternalToPage (pRpoolMD5Output->hPool,
		pRpoolMD5Output->hPage,
		 (void*)strResponse,
		strlen (strResponse) + 1, & (pRpoolMD5Output->offset));
	
	//pThis->Unlock (); <-- uncomment if this method ever accesses the sip manager.

} // end CstiSipManager::AuthenticationMD5CB


////////////////////////////////////////////////////////////////////////////////
//; AddMissingAddress
//
// Description: Makes sure the address portion of the sip contact matches the given address
//
// Example:
//	szStr[256] = "bobby <sip:bob@blah.com:1234>; foo=231"
//	addMissingAddress(szStr, 256, "127.0.0.1")
//	szStr == "bobby <sip:bob@127.0.0.1>; foo=231"
//
// Returns: modified pszFixMe
//
static void AddMissingAddress (
	char *pszFixMe,
	int nBufSize,
	const char *pszMissingAddress)
{
	int nEnd = 0;
	for (; pszFixMe[nEnd] != '\0' && pszFixMe[nEnd] != '<'; ++nEnd)
	{
	}

	for (; pszFixMe[nEnd] != '\0' && pszFixMe[nEnd] != ':'; ++nEnd)
	{
	}

	if (pszFixMe[nEnd] == ':')
	{
		for (; pszFixMe[nEnd] != '\0' && pszFixMe[nEnd] != '>' && pszFixMe[nEnd] != '@'; ++nEnd)
		{
		}

		int nPost = nEnd;
		for (; pszFixMe[nPost] != '\0' && pszFixMe[nPost] != '>'; ++nPost)
		{
		}

		if (pszFixMe[nPost] == '>')
		{
			++nPost;
		}

		auto pszPost = new char [strlen (&pszFixMe[nPost]) + 1];
		strcpy (pszPost, &pszFixMe[nPost]);
		snprintf (&pszFixMe[nEnd], nBufSize - nEnd, "@%s>%s", pszMissingAddress, pszPost);
		delete [] pszPost;
		pszPost = nullptr;
	}
} // end AddMissingAddress


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransportParseErrorCB
//
// Description: Attempt to fix bad message header syntax
//
// Returns: RV_OK on success
//
#define MAX_DATE_LEN 31
RvStatus RVCALLCONV CstiSipManager::TransportParseErrorCB (
	IN RvSipTransportMgrHandle hTransportMgr,
	IN RvSipAppTransportMgrHandle hAppTransportMgr,
	IN RvSipMsgHandle hMsgReceived,
	OUT RvSipTransportBsAction *peAction)
{
	*peAction = RVSIP_TRANSPORT_BS_ACTION_CONTINUE_PROCESS;
	RvSipHeaderListElemHandle hListElem = nullptr;
	RvSipHeaderType eHeaderType;

	RvChar szBuf[256];
	RvUint unBufLen = sizeof (szBuf) - 1;
	szBuf[0] = '\0';

	//pThis->Lock (); <-- uncomment this if this method ever accesses the sip manager


	// Look through all the bad headers
	void *pHeader = RvSipMsgGetBadSyntaxHeader (hMsgReceived, RVSIP_FIRST_HEADER, &hListElem, &eHeaderType);
	while (pHeader && *peAction == RVSIP_TRANSPORT_BS_ACTION_CONTINUE_PROCESS)
	{
		switch (eHeaderType)
		{
			case RVSIP_HEADERTYPE_FROM:
			{
				auto  hFrom = (RvSipPartyHeaderHandle)pHeader;
				RvSipPartyHeaderGetStrBadSyntax (hFrom, szBuf, unBufLen, &unBufLen);

				// This message can clutter output.
				//DBG_MSG ("Fixing FROM field:\n\t%s", szBuf);

				unBufLen = sizeof (szBuf) - 1;

				// Discern the missing address from the via header
				auto  viaHeader = viaHeaderGet (hMsgReceived);
				auto ipPort = viaHeader.hostAddr + std::to_string (viaHeader.hostPort);

				AddMissingAddress (szBuf, unBufLen, ipPort.c_str ());
				if (RV_OK != RvSipPartyHeaderFix (hFrom, false, szBuf))
				{
					stiASSERTMSG (estiFALSE, "Fixup failed:\n\t%s", szBuf);
				}
				break;
			}
			case RVSIP_HEADERTYPE_DATE:
				// If the date stamp is weird, it's probably pretty close to right now.
				{
					// Format is like: Sat, 13 Nov 2010 23:29:00 GMT
					char szDate[MAX_DATE_LEN];
					time_t nTime = time (nullptr);
					struct tm *psTime = gmtime (&nTime);
					if (!nTime || 0 == strftime (szDate, MAX_DATE_LEN, "%a, %d %b %Y %H:%M:%S GMT", psTime))
					{
						stiASSERTMSG (estiFALSE, "Error getting system date.");
					}

					RvSipDateHeaderFix ((RvSipDateHeaderHandle)pHeader, (RvChar*)szDate);
				}
				break;
				
			default:
				stiASSERTMSG (estiFALSE, "Syntax error unhandled header %d", eHeaderType);
				*peAction = RVSIP_TRANSPORT_BS_ACTION_REJECT_MSG;
		}
		pHeader = RvSipMsgGetBadSyntaxHeader (hMsgReceived, RVSIP_NEXT_HEADER, &hListElem, &eHeaderType);
	}

	//pThis->Unlock (); <-- uncomment this if this method ever accesses the sip manager

	return RV_OK; // Indicates the callback is OK, not that the problem is solved
} // end CstiSipManager::TransportParseErrorCB
#undef MAX_DATE_LEN


/*!\brief Callback which is called when an alias tag is sent in the via.
 * 
 * If this is called, an alias tag has been sent in the via.  We are granting 
 * permission to reuse the connection.  Stronger security could and probably should
 * be managed at this point.  See RFC 5923.
 * 
 */
void RVCALLCONV CstiSipManager::TransportConnectionServerReuseCB (
	RvSipTransportMgrHandle hTransportMgr,
	RvSipAppTransportMgrHandle hAppTransportMgr,
	RvSipTransportConnectionHandle hConn,
	RvSipTransportConnectionAppHandle hAppConn)
{
	RvStatus rvStatus = RV_OK;
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	
	rvStatus = RvSipTransportConnectionEnableConnByAlias(hConn);
	stiTESTRVSTATUS ();
	
STI_BAIL:
	
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::UserPhoneNumbersSet
//
// Description: Called whenever phone numbers are set.  This may force a re-register.
//
// Returns: stiHResult
//
stiHResult CstiSipManager::UserPhoneNumbersSet (
		const SstiUserPhoneNumbers *psUserPhoneNumbers)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock();
	
	hResult = CstiProtocolManager::UserPhoneNumbersSet (psUserPhoneNumbers);

	if (eventLoopIsRunning ())
	{
		PostEvent (
		    std::bind(&CstiSipManager::eventNatTraversalConfigure, this));
	}
	
	Unlock ();
	
	return hResult;
}


/*!\brief Adds the user agent header to the SIP header of the outbound message specified
 *
 * \param hOutboundMessage The message to which the user agent header is added
 */
stiHResult CstiSipManager::UserAgentHeaderAdd (
	RvSipMsgHandle hOutboundMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::stringstream UserAgent;
	std::string productName = productNameGet ();

	//
	// Add the product version
	//
	UserAgent << productName << SIP_VERSION_SEPARATOR;

	if (!m_version.empty())
	{
		UserAgent << m_version;
	}
	else
	{
		stiASSERT (estiFALSE);

		UserAgent << "0.0";
	}

	//
	// Add the Sorenson SIP implementation version
	//
	UserAgent << " " SORENSON_SIP_VERSION_STRING << SIP_VERSION_SEPARATOR << g_nSVRSSIPVersion;

	RvSipOtherHeaderHandle hOtherHeader;
	RvSipOtherHeaderConstructInMsg (hOutboundMessage, RV_TRUE, &hOtherHeader);
	RvSipOtherHeaderSetName (hOtherHeader, (RvChar*)"User-Agent");
	RvSipOtherHeaderSetValue (hOtherHeader, (RvChar*)UserAgent.str ().c_str ());

	return hResult;
}

/*!\brief Adds the Privacy header to the SIP header of the outbound message specified
 *
 * \param hOutboundMessage The message to which the Privacy header is added
 */
stiHResult PrivacyHeaderAdd (
	RvSipMsgHandle hOutboundMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	RvSipOtherHeaderHandle hOtherHeader;
	RvSipOtherHeaderConstructInMsg (hOutboundMessage, RV_FALSE, &hOtherHeader);
	RvSipOtherHeaderSetName (hOtherHeader, (RvChar*)"Privacy");
	RvSipOtherHeaderSetValue (hOtherHeader, (RvChar*)"id");
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallLegMessageSendCB
//
// Description: Called when we are about to send a message.
//
// Abstract: We can use this to attach our User-Agent for the outbound message.
//
// Returns: status
//
RvStatus RVCALLCONV CstiSipManager::CallLegMessageSendCB (
	IN RvSipCallLegHandle hCallLeg, ///< The SIP Stack call-leg handle.
	IN RvSipAppCallLegHandle hAppCallLeg, ///< The application handle for this call-leg.
	IN RvSipMsgHandle hMsgToSend ///< The handle to the outgoing message.
)
{
	auto  callLegKey = (size_t)hAppCallLeg;
	auto callLeg = callLegLookup (callLegKey);
	auto sipCall = callLeg->m_sipCallWeak.lock ();
	auto pThis = (CstiSipManager*)sipCall->ProtocolManagerGet ();
	
	pThis->Lock ();

	pThis->UserAgentHeaderAdd (hMsgToSend);
	
	// If we are an interpreter phone and did not receive a hearing phone number or
	// we have Block Caller ID enabled we need to add a Privacy header to our messages.
	if (!sipCall->m_bIsBridgedCall && ((estiINTERPRETER_MODE == pThis->m_eLocalInterfaceMode
	 && pThis->m_UserPhoneNumbers.szHearingPhoneNumber[0] == '\0')
	 || (pThis->m_stConferenceParams.bBlockCallerID && pThis->m_bLoggedIn)))
	{
		PrivacyHeaderAdd (hMsgToSend);
	}

	pThis->Unlock ();

	return RV_OK;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransactionMessageSendCB
//
// Description: Called when we are about to send a message.
//
// Abstract: We can use this to attach our User-Agent for the outbound message.
//
// Returns: status
//
RvStatus RVCALLCONV CstiSipManager::TransactionMessageSendCB (
	IN RvSipTranscHandle hTransaction, ///< The transaction handle.
	IN RvSipTranscOwnerHandle hTransactionOwner, ///< The application handle for this transaction.
	IN RvSipMsgHandle hMsgToSend ///< The handle to the outgoing message.
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bLocked = false;

	auto pThis = (CstiSipManager *)hTransactionOwner;

	if (pThis)
	{
		pThis->Lock ();
		bLocked = true;

		hResult = pThis->UserAgentHeaderAdd (hMsgToSend);
		stiTESTRESULT ();
	}
	
STI_BAIL:

	stiUNUSED_ARG (hResult);

	if (bLocked)
	{
		pThis->Unlock ();
	}

	return RV_OK;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransactionCreatedCB
//
// Description: Called when we received a brand new message.
//
// Abstract: We can use this to filter what types of messsages are acceptable.
//
// Returns: Nothing.
//
void RVCALLCONV CstiSipManager::TransactionCreatedCB (
	IN RvSipTranscHandle hTransaction,
	IN void *context,
	OUT RvSipTranscOwnerHandle *phTransactionOwner,
	OUT RvBool *pbHandleTransaction)
{
	*phTransactionOwner = (RvSipTranscOwnerHandle)context;

	//pThis->Lock (); <-- uncomment if this method ever accesses the sip manager

	auto method = vpe::sip::Transaction(hTransaction).methodGet();

	*pbHandleTransaction = (method == OPTIONS_METHOD);

	//pThis->Unlock (); <-- uncomment this if this method ever accesses the sip manager
}


/*!\brief Checks for the presence of a remote request header, validates and processes it.
 *
 */
stiHResult CstiSipManager::OptionsRequestProcess (
	const vpe::sip::Transaction &transaction)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvSipHeaderListElemHandle hListIterator = nullptr;
	RvSipOtherHeaderHandle hHeartbeatRequest = nullptr;

	auto inboundMessage = transaction.inboundMessageGet();

	hHeartbeatRequest = RvSipMsgGetHeaderByName (
			inboundMessage, (RvChar*)g_szSVRSHeartbeatRequest, RVSIP_FIRST_HEADER, &hListIterator);

	if (hHeartbeatRequest)
	{
		char szRequestString[128];
		unsigned int unLength = sizeof(szRequestString);

		rvStatus = RvSipOtherHeaderGetValue (hHeartbeatRequest, szRequestString, unLength, &unLength);
		stiTESTRVSTATUS ();

		//
		// Retrieve version, sequence and command
		//
		std::string Request;

		hResult = DecryptString ((unsigned char *)szRequestString, unLength, &Request);
		stiTESTRESULT ();

		size_t End = Request.find (" ");
		stiTESTCOND (End != std::string::npos, stiRESULT_ERROR);

		//
		// Found version
		//
		int nVersion = atoi(Request.substr (0, End).c_str ());

		if (nVersion == 1)
		{
			//
			// Get the sequence
			// First skip past white space
			//
			size_t Start = Request.find_first_not_of (" ", End + 1);
			stiTESTCOND (Start != std::string::npos, stiRESULT_ERROR);

			//
			// Find next whitepace
			//
			End = Request.find (" ", Start);
			stiTESTCOND (End != std::string::npos, stiRESULT_ERROR);

			//
			// Found the sequence
			//
			unsigned int unSequence = atoi (Request.substr (Start, End - Start).c_str ());

			//
			// If the sequence number is greater than or equal to the next one expected or if
			// enough time has elapsed since the last options request
			// then go ahead and continue processing the request.
			//
			bool bAllowed = false;

			if (unSequence >= m_unLastOptionsRequestSequence + 1)
			{
				bAllowed = true;
			}
			else
			{
				//
				// If it is the same sequence number we will allow it if enough time has elapsed.
				//
				time_t Now;

				TimeGet (&Now);

				if (Now - m_LastOptionsRequestTime > stiOPTIONS_REQUEST_ALLOWED_ELAPSED_TIME)
				{
					bAllowed = true;
				}
			}

			if (bAllowed)
			{
				m_unLastOptionsRequestSequence = unSequence;
				TimeGet (&m_LastOptionsRequestTime);

				//
				// Now get the command
				// First skip past whitespace
				//
				Start = Request.find_first_not_of (" ", End + 1);
				stiTESTCOND (Start != std::string::npos, stiRESULT_ERROR);

				//
				// Found the command
				//
				unsigned int unCommand = atoi (Request.substr (Start, std::string::npos).c_str ());

				if (unCommand == stiOPTIONS_REQUEST_HEARTBEAT)
				{
					heartbeatRequestSignal.Emit ();
				}
				else
				{
					//
					// Unsupported command.
					//
					stiTHROW (stiRESULT_ERROR);
				}
			}
		}
		else
		{
			//
			// Unsupported version
			//
			stiTHROW (stiRESULT_ERROR);
		}
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransactionStateChangedCB
//
// Description: Called when a transaction state message state change happens.
//
// Abstract: We can use this to handle OPTIONS messages.
//
// Returns: Nothing.
//
void RVCALLCONV CstiSipManager::TransactionStateChangedCB (
	IN RvSipTranscHandle hTransaction, ///< The transaction handle.
	IN RvSipTranscOwnerHandle hTransactionOwner, ///< The transaction owner handle for this transaction.
	IN RvSipTransactionState eState, ///< The new transaction state.
	IN RvSipTransactionStateChangeReason eReason) ///< The reason for the state change.
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	auto pThis = (CstiSipManager *)hTransactionOwner;

	pThis->Lock ();

	vpe::sip::Transaction transaction(hTransaction);

	switch (eState)
	{
		case RVSIP_TRANSC_STATE_SERVER_GEN_REQUEST_RCVD: // Received a new out-of call transaction
		{
			auto method = transaction.methodGet();

			if (method == OPTIONS_METHOD) // It is an OPTIONS message
			{
				if (!pThis->m_isAvailable)
				{
					// Reply 503
					transaction.sendResponse(SIPRESP_SERVICE_UNAVAILABLE);
				}
				else
				{
					// Attach the invite requirements

					stiDEBUG_TOOL (g_stiSipMgrDebug,
						stiTrace ("Message=\"Sip Options received\"\n");
					);

					auto outboundMessage = transaction.outboundMessageGet();

					pThis->InviteRequirementsAdd (outboundMessage, false, false);

					hResult = transaction.sendResponse(SIPRESP_SUCCESS);
					stiTESTRESULT();

					stiDEBUG_TOOL (g_stiSipMgrDebug,
						stiTrace ("Message=\"200-OK Response to Sip Options sent\"\n");
					);

					//
					// Check to see if there is a heartbeat request.  If so,
					// then inform the application to send a heartbeat.
					//
					pThis->OptionsRequestProcess (transaction);
				}
			}
			else // It is an unknown transaction.  Respond 501-Not implemented.
			{
				transaction.sendResponse(SIPRESP_NOT_IMPLEMENTED);
			}

			break;
		}

		case RVSIP_TRANSC_STATE_TERMINATED:
		{
#if defined stiHOLDSERVER || APPLICATION == APP_NTOUCH_IOS
			if (pThis && eReason == RVSIP_TRANSC_REASON_NETWORK_ERROR)
			{
				stiASSERTMSG(estiFALSE, "CstiSipManager::TransactionStateChangedCB: Calling StackRestart() when Transaction State is \"%d\" and Transaction state change reason is  \"%d\"", eState, eReason);

				DBG_MSG("CstiSipManager::TransactionStateChangedCB, calling 'StackRestart()' State %d, Reason %d ", eState, eReason);

				pThis->StackRestart ();
			}
#endif
			break;
		}
		default:
			break;
	}

STI_BAIL:

	pThis->Unlock ();
}


/*!
 * \brief Callback for when an event has been posted to the event queue
 *        This works a little differently than most instances of the event queue
 *        because Radvision is performing the select.  But it is running in
 *        the event queue's thread. NOTE: EventTimers are currently not supported
 *        on CstiSipManager
 */
void RVCALLCONV CstiSipManager::SipMgrPipeReadCB (
	RvInt fd,
	RvSipMidSelectEvent sEvent,
	RvBool bError,
	void* userData)
{
	auto pThis = (CstiSipManager*)userData;

	// The file descriptor passed in (fd) ought to be the same as that in the
	// base class
	stiASSERT (pThis->queueFileDescriptorGet () == fd);

	if (RVSIP_MID_SELECT_READ == sEvent)
	{
			pThis->processNextEvent ();
	}
	else
	{
		stiASSERTMSG (estiFALSE, "SIP: PipeReadCB Received an unregistered event\n");
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StackLogCB ()
//
// Description: Radvision stack logging callback
//
// Abstract:
//	Gets called whenever Radvision wants to add a log message.  It will only
// be accessible if the SIP_MODE makefile flag is set to debug,
// to enable stack's internal logging features.
//
// Returns:
//	none
//
void RVCALLCONV CstiSipManager::StackLogCB (
	IN void* context, /// Application context. (not used)
	IN RvSipLogFilters filter, /// A filter for more granular logging capabilities
	IN const RvChar *formattedText /// The string to log
)
{
	stiTrace ("\t~RVLOG~ %s\n", formattedText);
} // end CstiSipManager::StackLogCB ()


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::NetworkChangeNotify
//
// Description: network settings change
//
// Abstract:
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
stiHResult CstiSipManager::NetworkChangeNotify()
{
	PostEvent (
				std::bind(&CstiSipManager::eventNetworkChangeNotify, this));

	return stiRESULT_SUCCESS;
}


/*!\brief Sets if we should be using the proxy keep alive code or not.
 *
 *\note If set to false we will stop the keep alive timer. To allow the timer to
 * 	start again this needs to be called with true. Default is true.
 */
void CstiSipManager::useProxyKeepAliveSet (bool useProxyKeepAlive)
{
	if (!useProxyKeepAlive)
	{
		m_proxyConnectionKeepaliveTimer.Stop ();
		m_lastKeepAliveTime = {};
	}
	
	m_useProxyKeepAlive = useProxyKeepAlive;
}


#ifdef stiLOG_SLP_CHANGES
void Test (RvSipTransportMgrHandle hTransportMgr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipTransportLocalAddrHandle hLocalAddr = nullptr;
	RvStatus rvStatus;
	RvSipTransportAddr stAddressDetails;
	RvSipTransport Transports[] = {RVSIP_TRANSPORT_UDP, RVSIP_TRANSPORT_TCP, RVSIP_TRANSPORT_TLS};
	int nNumTransports = sizeof (Transports) / sizeof (Transports[0]);

	stiDEBUG_TOOL (g_stiSipMgrDebug,
			stiTrace ("The following addresses are currently registered with the SIP stack:\n");
	);

	
	for (int i = 0; i < nNumTransports; i++)
	{
		// Get the first address
		rvStatus = RvSipTransportMgrLocalAddressGetFirst (hTransportMgr, Transports[i], &hLocalAddr);
		
		if (rvStatus != RV_OK)
		{
			continue;
		}

		while (RV_OK == rvStatus && hLocalAddr)
		{
			rvStatus = RvSipTransportMgrLocalAddressGetDetails (hLocalAddr, sizeof (stAddressDetails), &stAddressDetails);
			stiTESTCOND (rvStatus == RV_OK, stiRESULT_ERROR);

			stiDEBUG_TOOL (g_stiSipMgrDebug,
				stiTrace ("\tIP = %s\tPort=%d\t%s\n", stAddressDetails.strIP, stAddressDetails.port,
						stAddressDetails.eTransportType == RVSIP_TRANSPORT_UDP ? "UDP" :
						stAddressDetails.eTransportType == RVSIP_TRANSPORT_TCP ? "TCP" :
						stAddressDetails.eTransportType == RVSIP_TRANSPORT_TLS ? "TLS" : "unknown transport");
			);

			// Now, do we have any more?
			rvStatus = RvSipTransportMgrLocalAddressGetNext (hLocalAddr, &hLocalAddr);
		}
	}

STI_BAIL:
	if (hResult) {}
}
#endif //#ifdef stiLOG_SLP_CHANGES


/*!\brief A Radvision callback to report the TCP connection state (used for debugging)
 * 
 */
#define DBG_MSG_TCP(S) DBG_MSG ("TCP Connection State: %s", stiCONST_TO_STRING (S));
RvStatus RVCALLCONV CstiSipManager::TCPConnectionStateCB (
	RvSipTransportConnectionHandle              hConn,
	RvSipTransportConnectionOwnerHandle         hObject,
	RvSipTransportConnectionState               eState,
	RvSipTransportConnectionStateChangedReason  eReason)
{
	CstiSipManager *pThis = nullptr;			
	RvSipTransportConnectionGetAppHandle (hConn, (RvSipTransportConnectionAppHandle *)&pThis);
	
	switch (eState)
	{
		case RVSIP_TRANSPORT_CONN_STATE_UNDEFINED:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_UNDEFINED);
			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_IDLE:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_IDLE);
			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_READY:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_READY);
			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_CONNECTING:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_CONNECTING);
			
			// RvSipTransportConnectionSetSynRetriesSockOption is only supported on Linux and only when not tunneled.
#if APPLICATION == APP_NTOUCH_VP2 || APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4

			if (RvTunGetPolicy() != RvTun_ENABLED)
			{
				RvStatus rvStatus = RV_OK;
				if (pThis)
				{
					pThis->Lock();
					//
					// If this connection is our persistent proxy connection
					// then change the number of TCP SYN retries to two so
					// that if it fails it will fail more quickly.
					//
					if (pThis->m_hPersistentProxyConnection == hConn)
					{
						rvStatus = RvSipTransportConnectionSetSynRetriesSockOption(hConn, MAX_TCP_PROXY_CONNECTION_SYN_RETRIES);
						stiASSERTMSG (rvStatus == RV_OK, "rvStatus = %d", rvStatus);
					}

					pThis->Unlock ();
				}
			}
#endif

			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_TCP_CONNECTED:
		{
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_TCP_CONNECTED);
			
			if (pThis)
			{
				DBG_MSG ("Stopping proxy watchdog timer.");
				pThis->m_proxyWatchdogTimer.Stop ();
			}
			
			break;
		}
			
		case RVSIP_TRANSPORT_CONN_STATE_CLOSING:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_CLOSING);
			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_CLOSED:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_CLOSED);
			break;
			
		case RVSIP_TRANSPORT_CONN_STATE_TERMINATED:
		{
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_TERMINATED);
			// Check if we need to add this transport back and register.
			if (pThis)
			{
				pThis->TransportConnectionCheck (false);
			}
			
			break;
		}
			
		case RVSIP_TRANSPORT_CONN_STATE_SCTP_CONNECTED:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_SCTP_CONNECTED);
			break;

		case RVSIP_TRANSPORT_CONN_STATE_CONNECTED:
			DBG_MSG_TCP (RVSIP_TRANSPORT_CONN_STATE_CONNECTED);
			break;

	}

	if (pThis)
	{
		pThis->rvTcpConnectionStateChangedSignal.Emit (eState);
	}
	
	return RV_OK;
}
#undef DBG_MSG_TCP


/*!\brief A Radvision callback to report the TCP connection status (used for debugging)
 * 
 */
RvStatus CstiSipManager::TCPConnectionStatusCB (
	RvSipTransportConnectionHandle hConn,
	RvSipTransportConnectionOwnerHandle hOwner,
	RvSipTransportConnectionStatus eStatus,
	void *pInfo)
{
	switch (eStatus)
	{
		case RVSIP_TRANSPORT_CONN_STATUS_UNDEFINED:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_UNDEFINED));
			break;
			
		case RVSIP_TRANSPORT_CONN_STATUS_ERROR:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_ERROR));
			break;

		case RVSIP_TRANSPORT_CONN_STATUS_MSG_SENT:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_MSG_SENT));
			break;

		case RVSIP_TRANSPORT_CONN_STATUS_MSG_NOT_SENT:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_MSG_NOT_SENT));
			break;

		case RVSIP_TRANSPORT_CONN_STATUS_SCTP_PEER_ADDR_CHANGED:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_SCTP_PEER_ADDR_CHANGED));
			break;

		case RVSIP_TRANSPORT_CONN_STATUS_SCTP_REMOTE_ERROR:
			stiTrace ("Connection status %s\n", stiCONST_TO_STRING(RVSIP_TRANSPORT_CONN_STATUS_SCTP_REMOTE_ERROR));
			break;
	}
	
	return RV_OK;
}


/*!\brief A Radvision callback which is called when a call leg creates a new connection.
 * 
 * This callback allows us to set the application handle of the new connection.
 * 
 * \param hCallLeg The call leg that created the new connection
 * \param hAppCallLeg The application handle for the call leg
 * \param hConn The connection
 * \param bNewConnCreated Indicates whether or not the connection is a new connection
 */
RvStatus RVCALLCONV CstiSipManager::NewConnectionCB (
	RvSipCallLegHandle hCallLeg,
	RvSipAppCallLegHandle hAppCallLeg,
	RvSipTransportConnectionHandle hConn,
	RvBool bNewConnCreated)
{
	RvStatus rvStatus = RV_OK;
	auto  callLegKey = (size_t)hAppCallLeg;

	if (bNewConnCreated)
	{
		auto callLeg = callLegLookup (callLegKey);
		auto sipCall = callLeg->m_sipCallWeak.lock ();
		CstiSipManager *pThis = sipCall->SipManagerGet();
		auto call = sipCall->CstiCallGet();
		
		pThis->Lock ();

		rvStatus = RvSipTransportConnectionSetAppHandle (hConn, (RvSipTransportConnectionAppHandle)pThis);
		
		// If we have a call and a new connection is created send a Re-Invite.
		if (call && call->StateValidate (esmiCS_CONNECTED))
		{			
			std::string callId;
			call->CallIdGet (&callId);
			stiRemoteLogEventSend ("EventType=CallConnectionChange NewConnection CallId=%s", callId.c_str ());
			
			pThis->ReInviteSend (call, false, 0);
		}

		pThis->Unlock ();
	}
	
	return rvStatus;
}


/*!\brief Detaches from the persistent TCP connection.
 *
 */
void CstiSipManager::TCPConnectionDestroy ()
{
	if (m_hPersistentProxyConnection)
	{
		RvSipTransportConnectionDetachOwner (m_hPersistentProxyConnection, (RvSipTransportConnectionOwnerHandle)this);
		m_hPersistentProxyConnection = nullptr;
	}
}


/*!\brief Creates persistent TCP connection.
 * 
 */
stiHResult CstiSipManager::TCPConnectionCreate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszTmpAddress = nullptr;

	//
	// Make sure the current connection is destroyed.
	//
	TCPConnectionDestroy ();
	
	if (!m_proxyDomainIpAddress.empty ())
	{
		RvSipTransportConnectionEvHandlers TransportEventHandlers;
		RvSipTransportConnectionCfg ConnectionConfig;

		memset (&TransportEventHandlers, 0, sizeof (TransportEventHandlers));
		TransportEventHandlers.pfnConnStateChangedEvHandler = TCPConnectionStateCB;
#if TCP_CONNNECTION_DEBUG
		TransportEventHandlers.pfnConnStausEvHandler = TCPConnectionStatusCB;
#endif

		rvStatus = RvSipTransportMgrCreateConnection (m_hTransportMgr, (RvSipTransportConnectionOwnerHandle)this, &TransportEventHandlers, sizeof (TransportEventHandlers), &m_hPersistentProxyConnection);
		stiTESTRVSTATUS();

		rvStatus = RvSipTransportConnectionSetAppHandle (m_hPersistentProxyConnection, (RvSipTransportConnectionAppHandle)this);
		stiTESTRVSTATUS ();

		memset (&ConnectionConfig, 0, sizeof (ConnectionConfig));

		if (m_eLastNATTransport == estiTransportTLS)
		{
			ConnectionConfig.eTransportType = RVSIP_TRANSPORT_TLS;
		}
		else
		{
			ConnectionConfig.eTransportType = RVSIP_TRANSPORT_TCP;
		}
		ConnectionConfig.strLocalIp = nullptr;
		ConnectionConfig.localPort = 0;
		ConnectionConfig.strHostName = (RvChar *)m_proxyDomain.c_str(); // TLS requires host name
		
		// enclose ipv6 in brackets if it isn't already
		if (strchr (m_proxyDomainIpAddress.c_str (), ':') && m_proxyDomainIpAddress[0] != '[')
		{
			pszTmpAddress = new char[m_proxyDomainIpAddress.size() + 3];
			sprintf (pszTmpAddress, "[%s]", m_proxyDomainIpAddress.c_str ());
			
			ConnectionConfig.strDestIp = (RvChar *)pszTmpAddress;
		}
		else
		{
			ConnectionConfig.strDestIp = (RvChar *)m_proxyDomainIpAddress.c_str ();
		}
		
		ConnectionConfig.destPort = m_proxyDomainPort;

		DBG_MSG ("Creating TCP connection for %s:%d", ConnectionConfig.strDestIp, ConnectionConfig.destPort);

		rvStatus = RvSipTransportConnectionInit (m_hPersistentProxyConnection, &ConnectionConfig, sizeof (ConnectionConfig));
		stiTESTRVSTATUS ();
		
		DBG_MSG ("Starting proxy watchdog timer.");
		m_proxyWatchdogTimer.Start ();
	}
	
STI_BAIL:

	if (pszTmpAddress)
	{
		delete [] pszTmpAddress;
		pszTmpAddress = nullptr;
	}

	if (stiIS_ERROR (hResult))
	{
		if (rvStatus == RV_ERROR_OUTOFRESOURCES)
		{
			std::string eventMessage = "EventType=SipStackRestart Reason=TCPConnectionCreate";
			stiRemoteLogEventSend (&eventMessage);
			StackRestart ();
		}
		else
		{
			TCPConnectionDestroy ();
			TransportConnectionCheck (true);
		}
	}

	return hResult;
}


/*!
 * \brief Event handler for network settings change
 */
void CstiSipManager::eventNetworkChangeNotify ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipTransportLocalAddrHandle hLocalAddr = nullptr;
	RvStatus rvStatus = RV_OK;
	RvSipTransportAddr stAddressDetails;
	std::string LocalIpAddress;
	//Do not call network change notify while there is an active call.
	if (!m_pCallStorage->activeCallCountGet())
	{
		m_networkChangePending = false;
#ifdef IPV6_ENABLED
		std::string LocalIpv6Address;
#endif // IPV6_ENABLED
		const RvSipTransport Transports[] = {RVSIP_TRANSPORT_UDP, RVSIP_TRANSPORT_TCP,
											 RVSIP_TRANSPORT_TLS};
		unsigned int nNumTransports = sizeof(Transports) / sizeof(Transports[0]);

#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC

		// Setup new Address for the correct address type.
		if (m_stConferenceParams.bIPv6Enabled)
		{
			stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
			strcpy(stAddressDetails.strIP, RADVISION_ALL_ADAPTER_ADDRESSES);
		}
		else
		{
			stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
			strcpy(stAddressDetails.strIP, "0.0.0.0");
		}

		for (size_t i = 0; i < nNumTransports; i++)
		{
			rvStatus = RvSipTransportMgrLocalAddressGetFirst(m_hTransportMgr, Transports[i],
															 &hLocalAddr);
			if (rvStatus == RV_OK && hLocalAddr)
			{
				rvStatus = RvSipTransportMgrLocalAddressCloseSocket(hLocalAddr);
				stiASSERT (RV_OK == rvStatus);

				// remove the old address
				rvStatus = RvSipTransportMgrLocalAddressRemove(m_hTransportMgr, hLocalAddr);
				stiASSERT (RV_OK == rvStatus);
			}
			// Always setup the new address even if the above failed.
			stAddressDetails.eTransportType = Transports[i];
			stAddressDetails.port = Transports[i] == RVSIP_TRANSPORT_TLS
									? m_stConferencePorts.nTLSListenPort
									: LocalListenPortGet();

			// add the new address
			rvStatus = RvSipTransportMgrLocalAddressAdd(m_hTransportMgr,
														&stAddressDetails, sizeof(stAddressDetails),
														RVSIP_FIRST_ELEMENT, NULL,
														&hLocalAddr);
		}
		// rvStatus needs to be ok for RvSipTransportMgrLocalAddressAdd
		stiTESTRVSTATUS ();

#else

#ifndef stiUSE_ZEROS_FOR_IP
		// The VP is currently the only endpoint that was not always using 0's for a network change.
		// If the VP is tunneled we want it to use 0's otherwise we will allow it to register with the
		// IP returned from stiGetLocalIp.
		hResult = stiGetLocalIp (&LocalIpAddress, estiTYPE_IPV4);
		stiTESTRESULT ();

#ifdef IPV6_ENABLED
		hResult = stiGetLocalIp (&LocalIpv6Address, estiTYPE_IPV6);
		stiTESTRESULT ();
#endif // not IPV6_ENABLED
	
#ifdef stiTUNNEL_MANAGER
		if (RvTunGetPolicy () == RvTun_ENABLED &&
			TunnelActive ())
		{

				LocalIpAddress.assign (m_TunneledIpAddr);
		}
#endif // stiTUNNEL_MANAGER

		//
		// For each transport, remove the old address and add the new one.  This
		// will terminate all the current registration clients, drop the current connection
		// and re-lookup the proxy, and then kick off the registration clients again.
		//
		for (unsigned int i = 0; i < nNumTransports; i++)
		{
			hLocalAddr = nullptr;

			rvStatus = RvSipTransportMgrLocalAddressGetFirst (m_hTransportMgr, Transports[i], &hLocalAddr);
			stiTESTRVSTATUS();

			while (hLocalAddr)
			{
				rvStatus = RvSipTransportMgrLocalAddressGetDetails (hLocalAddr, sizeof (stAddressDetails), &stAddressDetails);
				stiTESTRVSTATUS();

				// Now, check to see if the IP address has changed
				if (
#ifdef IPV6_ENABLED
				(stAddressDetails.eAddrType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP6
				 && LocalIpv6Address != stAddressDetails.strIP)
				||
#endif // IPV6_ENABLED
				(stAddressDetails.eAddrType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP
				 && LocalIpAddress != stAddressDetails.strIP))
				{
					RvSipTransportLocalAddrHandle hTmpAddr = nullptr;

#ifdef IPV6_ENABLED
					// Add the new address
					if (stAddressDetails.eAddrType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP6)
					{
						strcpy(stAddressDetails.strIP, LocalIpv6Address.c_str ());
					}
					else
#endif // IPV6_ENABLED
					{
						strcpy(stAddressDetails.strIP, LocalIpAddress.c_str ());
					}

					//
					// Try adding the new address first.  If that succeeds then we will remove the old address.
					//
					rvStatus = RvSipTransportMgrLocalAddressAdd(m_hTransportMgr,
						&stAddressDetails, sizeof(stAddressDetails), RVSIP_FIRST_ELEMENT, nullptr,
						&hTmpAddr);
					stiASSERT (rvStatus == RV_OK);

					if (rvStatus == RV_OK)
					{
						// The IP address has changed.  Need to update the addresses to listen on
						rvStatus = RvSipTransportMgrLocalAddressRemove(m_hTransportMgr, hLocalAddr);
						stiASSERT (rvStatus == RV_OK);
					}
				}

				// Now, do we have any more?
				rvStatus = RvSipTransportMgrLocalAddressGetNext (hLocalAddr, &hLocalAddr);
			}
		}
#else

#ifdef IPV6_ENABLED
	LocalIpAddress.assign (RADVISION_ALL_ADAPTER_ADDRESSES);
#else
	// The VP is currently the only endpoint that was not always using 0's for a network change.
	// If the VP is tunneled we want it to use 0's otherwise we will allow it to register with the
	// IP returned from stiGetLocalIp.
	hResult = stiGetLocalIp (&LocalIpAddress, estiTYPE_IPV4);
	stiTESTRESULT ();

#ifdef stiTUNNEL_MANAGER
	if (RvTunGetPolicy () == RvTun_ENABLED &&
		TunnelActive ())
	{
		
		LocalIpAddress.assign (m_TunneledIpAddr);
	}
#endif // stiTUNNEL_MANAGER
#endif // not IPV6_ENABLED

	//
	// For each transport, remove the old address and add the new one(s).  This
	// will terminate all the current registration clients, drop the current connection
	// and re-lookup the proxy, and then kick off the registration clients again.
	//
	for (unsigned int i = 0; i < nNumTransports; i++)
	{
		hLocalAddr = NULL;

		rvStatus = RvSipTransportMgrLocalAddressGetFirst (m_hTransportMgr, Transports[i], &hLocalAddr);
		stiASSERT (rvStatus == RV_OK);
		
		if (rvStatus == RV_OK)
		{
			rvStatus = RvSipTransportMgrLocalAddressGetDetails (hLocalAddr, sizeof (stAddressDetails), &stAddressDetails);
			stiASSERT (rvStatus == RV_OK);
		}

#ifdef stiTUNNEL_MANAGER
		if (RvTunGetPolicy () == RvTun_ENABLED &&
			TunnelActive () &&
			rvStatus != RV_OK)
		{
			// We are tunneling and failed to get address details for this transport. This means we will fail to attempt to add this address.
			// Since we know what our information needs to be we can fill in the details ourselves and then attempt to add the address.
			stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
			stAddressDetails.eTransportType = Transports[i];
			if (Transports[i] != RVSIP_TRANSPORT_TLS)
			{
				stAddressDetails.port = LocalListenPortGet ();
			}
			else
			{
				stAddressDetails.port = nDEFAULT_TLS_SIP_LISTEN_PORT;
			}
			// Set to blank to ensure the IP is different than our local address.
			strcpy(stAddressDetails.strIP, "");
			rvStatus = RV_OK; // Reset the status so we wont bail out.
		}
#endif
		stiTESTRVSTATUS();

		// Remove all the old addresses for this transport
		while (hLocalAddr)
		{
			// Close the socket to ensure we have no issues when attempting to add the address.
			rvStatus = RvSipTransportMgrLocalAddressCloseSocket(hLocalAddr);
			stiTESTRVSTATUS();

			// Remove it
			rvStatus = RvSipTransportMgrLocalAddressRemove (m_hTransportMgr, hLocalAddr);
			stiTESTRVSTATUS ();
			
			RvSipTransportMgrLocalAddressGetNext (hLocalAddr, &hLocalAddr);
		}

		// Add the new address
		strcpy(stAddressDetails.strIP, LocalIpAddress.c_str ());
		
		rvStatus = RvSipTransportMgrLocalAddressAdd(m_hTransportMgr,
			&stAddressDetails, sizeof(stAddressDetails), RVSIP_FIRST_ELEMENT, NULL,
			&hLocalAddr);
		stiTESTRVSTATUS();
	}
#endif
#endif

	if (m_hResolverMgr)
	{
		// Since we have the possibility of switching between IPv6 and IPv4 networks
		// we are going to force a new look up.
		ProxyDomainResolverDelete ();

//If we are using a google dns, we do not want to wipe it out on a network change.
//This fixes issues with android ipv6 DNS.
#ifdef stiUSE_GOOGLE_DNS
			if (!g_bUseGoogleDNS)
#endif
			{
				RvSipResolverMgrRefreshDnsServers(m_hResolverMgr);

#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
				if (!m_stConferenceParams.bIPv6Enabled)
				{
					SIPResolverIPv4DNSServersSet ();
				}
#endif
			}

		}

		hResult = Unregister(nullptr);
		stiASSERT (stiRESULT_SUCCESS == hResult);

		// Resolve and register
		PostEvent (std::bind(&CstiSipManager::ProxyRegistrarLookup, this));

#ifdef stiLOG_SLP_CHANGES
		Test(m_hTransportMgr);
#endif //#ifdef stiLOG_SLP_CHANGES
	}
	else
	{
		m_networkChangePending = true;
		std::string eventMessage = "EventType=NetworkChangeNotify Reason=Queued pending call completion";
		stiRemoteLogEventSend (&eventMessage);
	}
STI_BAIL:

	if (rvStatus != RV_OK)
	{
		std::string eventMessage = "EventType=SipStackRestart Reason=NetworkChangeFailed";
		stiRemoteLogEventSend (&eventMessage);
		StackRestart ();
	}
	else if (stiIS_ERROR (hResult))
	{
		TransportConnectionCheck (false);
		DBG_MSG ("<CstiSipManager::eventNetworkChangeNotify> IP Address(es) failed to update.\n");
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::TransportConnectionCheck
//
// Description: Verifies transport information is valid.
//
// Abstract:
//  This method will ensure that our transport manager has information for UDP,TCP, and TLS connections.
// If it detects an error it will force those connections be recreated.
//
// Returns:
//  Success if we found valid connection information or we were able to add missing connections.
//	 Failure if we could not add missing connection information.
//
stiHResult CstiSipManager::TransportConnectionCheck (bool performNetworkChange)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipTransportLocalAddrHandle hLocalAddr = nullptr;
	RvStatus rvStatus;
	RvSipTransportAddr stAddressDetails;
	RvSipTransport Transports[] = {RVSIP_TRANSPORT_UDP, RVSIP_TRANSPORT_TCP, RVSIP_TRANSPORT_TLS};
	int nNumTransports = sizeof (Transports) / sizeof (Transports[0]);
	
	for (int i = 0; i < nNumTransports; i++)
	{
		// Get the first address
		rvStatus = RvSipTransportMgrLocalAddressGetFirst (m_hTransportMgr, Transports[i], &hLocalAddr);
		
		if (!hLocalAddr)
		{
			// This transport is either not found or unusable we need to add it again.
			
			// Setup new Address
			// We use zero to force the network change.
			if (m_stConferenceParams.bIPv6Enabled)
			{
				stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP6;
				strcpy (stAddressDetails.strIP, RADVISION_ALL_ADAPTER_ADDRESSES);
			}
			else
			{
				stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
				strcpy (stAddressDetails.strIP, "0.0.0.0");
			}
			
			stAddressDetails.eTransportType = Transports[i];
			
			if (Transports[i] == RVSIP_TRANSPORT_TLS)
			{
				stAddressDetails.port = m_stConferencePorts.nTLSListenPort;
			}
			else
			{
				stAddressDetails.port = LocalListenPortGet ();
			}
			
			// add the new address
			rvStatus = RvSipTransportMgrLocalAddressAdd (m_hTransportMgr,
																		&stAddressDetails, sizeof (stAddressDetails), RVSIP_FIRST_ELEMENT, nullptr,
																		&hLocalAddr);
			stiTESTRVSTATUS ();
			performNetworkChange = true;
		}
	}
	
	// Something changed lets perform a network change.
	if (performNetworkChange)
	{
		NetworkChangeNotify ();
	}
	
STI_BAIL:
	
	if (rvStatus == RV_ERROR_OUTOFRESOURCES)
	{
		std::string eventMessage = "EventType=SipStackRestart Reason=TransportOutOfResources";
		stiRemoteLogEventSend (&eventMessage);
		StackRestart ();
	}
	
	return hResult;
}


/*!
 * \brief Dials the call
 */
void CstiSipManager::eventCallDial (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (CstiSipManager::eventCallDial);
	CallDial (call);
}


/*!
 * \brief Dials the call
 */
stiHResult CstiSipManager::CallDial (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (CstiSipManager::CallDial);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiRoutingAddress dialString;
	CstiSipCallSP sipCall = nullptr;
	stiTESTCOND (call, stiRESULT_ERROR);
	
	sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());

	//
	// Make sure dial string is valid.
	//
	dialString = call->RoutingAddressGet ();
#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	static char szMessage[256];
	sprintf(szMessage, "vpEngine:::CstiSipManager::CallDial() - Original DialString: %s", dialString.originalStringGet ().c_str ());
	DebugTools::instanceGet()->DebugOutput(szMessage);
#endif

	if (dialString.originalStringGet ().empty ())
	{
		stiTHROW (stiRESULT_ERROR);
	}

	if (dialString.protocolGet () != estiSIP)
	{
		// We have a non SIP URI, we will assume it's an IP or domain so append the SIP protocol to the start.
		std::string orgDialString = dialString.originalStringGet();

		orgDialString = dialString.ResolvedLocationGet();
		auto userString = dialString.userGet();

		// If we have user information we need to make sure we put it back with the resolved location.
		if (!userString.empty())
		{
			userString.insert (0, "sip:");
			userString.append("@");
			orgDialString.insert(0, userString);
		}
		else
		{
			orgDialString.insert(0, "sip:");
		}

		call->RoutingAddressSet(orgDialString);
		
		// We will need to add a private header.
		sipCall->m_bAddSipToH323Header = true;
		sipCall->m_bGatewayCall = true;
		sipCall->IsHoldableSet (estiFALSE);
	}
	
	//
	// Create the call.
	//
	hResult = CallLegCreate (sipCall);
	stiTESTRESULT ();
	
	hResult = InvitePrepare (sipCall);
	stiTESTRESULT ();

	// We're making a call
	NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_CALLING);

	// Start a ringing sound
	OutgoingToneSet (estiTONE_RING);
	
	if (!sipCall->IsBridgedCall())
	{
		// TODO: Remove magic numbers
		call->pleaseWaitTimer.TimeoutSet(stiPLEASE_WAIT_TIMEOUT_DEFAULT * MSEC_PER_SEC);
		call->pleaseWaitTimer.Start();
	}

	// Only start the failover timer on VRS calls if we have not done so before
	// and we have a valide timeout value.
	if ((call->DialMethodGet () == estiDM_BY_VRS_WITH_VCO ||
	    call->DialMethodGet () == estiDM_BY_VRS_PHONE_NUMBER) &&
		m_stConferenceParams.vrsFailoverTimeout > 0 &&
	    call->vrsFailOverTimer.startCountGet () == 0)
	{
		call->vrsFailOverTimer.TimeoutSet (m_stConferenceParams.vrsFailoverTimeout * MSEC_PER_SEC);
		call->vrsFailOverTimer.Start ();
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Start the fast busy
		OutgoingToneSet (estiTONE_FAST_BUSY);
		if (sipCall)
		{
			sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_INTERNAL_ERROR);
		}
	}
	
	return hResult;
} // end CstiSipManager::CallDial


CstiCallSP CstiSipManager::CallDial (
		EstiDialMethod eDialMethod,
		const CstiRoutingAddress &DialString,
		const std::string &originalDialString,
		const OptString &fromNameOverride,
		const OptString &callListName,
		CstiSipCallSP bridgedSipCall,
		CstiSipCallSP backgroundCall,
		const std::vector<SstiSipHeader>& additionalHeaders)
{
	return CallDial(eDialMethod, DialString, originalDialString, fromNameOverride, callListName, nullptr, 0, bridgedSipCall, backgroundCall, additionalHeaders);
}


/*!
 * \brief Creates a new call object and schedules it to be dialed from the
 *        sip manager's thread
 */
CstiCallSP CstiSipManager::CallDial (
	EstiDialMethod eDialMethod, ///< The method to be used to dial this call
	const CstiRoutingAddress &DialString, ///< The dial string object to dial
	const std::string &originalDialString,
	const OptString &fromNameOverride,
	const OptString &callListName,
	const OptString &relayLanguage,
	int nRelayLanguageId,
	CstiSipCallSP bridgedSipCall,
	CstiSipCallSP backgroundCall,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	stiLOG_ENTRY_NAME (CstiSipManager::CallDial);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto sipCall = std::make_shared<CstiSipCall>(&m_stConferenceParams, this, m_vrsFocusedRouting, bridgedSipCall, backgroundCall);

	auto call = std::make_shared<CstiCall> (
	                m_pCallStorage, m_UserPhoneNumbers, m_userId, m_groupUserId,
	                m_displayName, m_alternateName, m_vrsCallId, &m_stConferenceParams);

	stiTESTCOND (call, stiRESULT_ERROR);

	call->ProtocolCallSet (sipCall);
	call->OriginalDialStringSet (eDialMethod, originalDialString);
	call->RemoteCallListNameSet (callListName.value_or (""));
	call->fromNameOverrideSet (fromNameOverride);
	call->additionalHeadersSet (additionalHeaders);
	if (bridgedSipCall)
	{
		call->NotifyAppOfCallSet(estiFALSE);
		sipCall->m_stConferenceParams.eSecureCallMode = estiSECURE_CALL_NOT_PREFERRED;
	}

	// DBG_MSG ("CstiSipManager::CallDial New call object [%p]", call.get());

	// Set the local return call info so the dialed system knows where we want them to call back to.
	call->LocalReturnCallInfoSet (m_eLocalReturnCallDialMethod, m_localReturnCallDialString);

	if (!bridgedSipCall && !backgroundCall)
	{
		// No need to store bridged calls, since we keep a copy
		m_pCallStorage->store (call);
	}

	// Set the direction and the dial string in the call object
	call->DirectionSet (estiOUTGOING);
	call->RemoteDialStringSet (originalDialString);
	call->RoutingAddressSet (DialString);

	// Store the remote ip we are contacting so the data tasks can filter properly
	// (On SDP response we'll set this to what the "o" or "c" line says
	// because the sip ip is not necessarily the media ip.)

	call->DialMethodSet (eDialMethod);
	
	if (relayLanguage || nRelayLanguageId)
	{
		call->LocalPreferredLanguageSet(relayLanguage, nRelayLanguageId);
	}

	PostEvent (
	    std::bind(&CstiSipManager::eventCallDial, this, call));

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (call)
		{
			m_pCallStorage->remove (call.get());
			call = nullptr;
		}
	}

	return call;
} // end CstiSipManager::CallDial


/*!
 * \brief Dials the passed in address, this method continues with the call
 */
void CstiSipManager::eventConferenceJoin (
    CstiCallSP currentCall,
    CstiCallSP heldCall,
	std::string dialString)
{
	stiLOG_ENTRY_NAME (CstiSipManager::eventConferenceJoin);

	stiHResult hResult = stiRESULT_SUCCESS;

	// TODO:  This looks very similar to TransferDial and should probably be
	//        refactored into a common method
	CstiSipCallSP newSipCall = nullptr;
	CstiSipCallSP currentSipCall = nullptr;
	CstiCallSP newCall = nullptr;
	
	CstiRoutingAddress RoutingAddress (dialString);
	bool bReferComplete = false;

	std::string Guid = dialString.substr (nMCU_GUID_START_POS_IN_URI, nMCU_GUID_LEN_IN_URI);

	stiTESTCOND (currentCall && heldCall && !dialString.empty (), stiRESULT_ERROR);

	if (!currentCall->StateValidate (esmiCS_CONNECTED))
	{
		// Must be the other one ... switch.
		std::swap (currentCall, heldCall);
	}


	currentSipCall = std::static_pointer_cast<CstiSipCall>(currentCall->ProtocolCallGet());

	// Cancel the lost connection and re-invite timers in case either is running on
	// the original call object.  Without doing so, the reference that was added to
	// the CstiCall object will potentially get released on the new call object (since
	// we switch the CstiSip call object under the hood.
	currentSipCall->LostConnectionTimerCancel ();
	currentSipCall->ReinviteTimerCancel ();
	currentSipCall->ReinviteCancelTimerCancel ();

	newSipCall = std::make_shared<CstiSipCall>(&m_stConferenceParams, this, vrsFocusedRoutingGet());

	// TODO:  Does this call need to be added to call storage?
	newCall = std::make_shared<CstiCall> (
	            m_pCallStorage, m_UserPhoneNumbers, m_userId, m_groupUserId,
	            m_displayName, m_alternateName, m_vrsCallId, &m_stConferenceParams);

	stiTESTCOND (newCall, stiRESULT_ERROR);

	newCall->disallowDestruction ();

	// Close the media channels of the current call so that we don't have contention with the platform layer (between
	// it and the new call).
	currentSipCall->MediaClose ();

	//
	// Don't let the application layer know about this call.
	//
	newCall->NotifyAppOfCallSet(estiFALSE);

	//
	// Now assign the new SIP Call to the current CstiCall object and
	// the current SIP call to the new CstiCall object.
	//
	currentCall->Lock ();
	newCall->ProtocolCallSet (currentSipCall);
	currentCall->ProtocolCallSet (newSipCall);

	// Update the RoutingAddress object in the two call objects
	newCall->RoutingAddressSet (currentCall->RoutingAddressGet());
	currentCall->RoutingAddressSet (RoutingAddress);

	// Copy the state from the current call into the new call
	newCall->StateSet (currentCall->StateGet (), currentCall->SubstateGet ());

	// Now set the state of the current call back to idle.
	currentCall->StateSet (esmiCS_IDLE);

	currentCall->AppNotifiedOfConferencingSet (estiFALSE);
	currentCall->DialMethodSet (estiDM_BY_DIAL_STRING);

	currentCall->Unlock ();

	//
	// Make sure dial string is valid.
	//
	if (RoutingAddress.originalStringGet ().empty ())
	{
		stiTHROW (stiRESULT_ERROR);
	}

	//
	// Refer the existing calls to the conference room
	//
	heldCall->TransferToAddress (dialString);
	newCall->TransferToAddress (dialString);
	bReferComplete = true;

	//
	// Create the call.
	//
	hResult = CallLegCreate (newSipCall);
	stiTESTRESULT ();

	hResult = InvitePrepare (newSipCall);
	stiTESTRESULT ();

	currentCall->ConnectedWithMcuSet (estiMCU_GVC);

	stiDEBUG_TOOL (g_stiGVC_Debug,
	    stiTrace ("<CstiSipManager::eventConferenceJoin> Connecting with GVC MCU.  ConferenceID = [%s]\n", Guid.c_str ());
	);

	// We're making a call
	NextStateSet (currentCall, esmiCS_CONNECTING, estiSUBSTATE_CALLING);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Start the fast busy
		OutgoingToneSet (estiTONE_FAST_BUSY);
		if (newSipCall && !bReferComplete)
		{
			newSipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_INTERNAL_ERROR);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ConferenceJoin
//
// Description: Post a message to the protocol task to join a conference room
//
// Abstract:
//	This method simply calls a function that posts a message to the
//	task informing it to join one or more calls into a conference room.
//
// Returns:
//	An stiHResult.
//
stiHResult CstiSipManager::ConferenceJoin (
    CstiCallSP call1,
    CstiCallSP call2,
	const std::string &DialString)
{
	stiLOG_ENTRY_NAME (CstiSipManager::ConferenceJoin);

	PostEvent (
	    std::bind(&CstiSipManager::eventConferenceJoin, this, call1, call2, DialString));

	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; RemoveSLPFromDynamicListenAddresses
//
// Description: Removes the second listen port from the listen addresses.
//
// Abstract:
//  This function assumes there is a second listen port and proceeds to locate
//  and remove it.
// NOTE: This method MUST be called from within the SipManager thread.  All startup
//  and shutdown calls are currently expected to be from the same thread.
//
// Returns:
//  stiRESULT_SUCCESS or stiRESULT_ERROR (ORed with an appropriate error code)
//
stiHResult RemoveSLPFromDynamicListenAddresses (RvSipTransportMgrHandle hTransportMgr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipTransportLocalAddrHandle hLocalAddr = nullptr;
	RvStatus rvStatus;

	/* Find and remove the second UDP address */
	rvStatus = RvSipTransportMgrLocalAddressGetFirst (hTransportMgr, RVSIP_TRANSPORT_UDP, &hLocalAddr);
	stiASSERT (rvStatus == RV_OK);

	if (rvStatus == RV_OK)
	{
		rvStatus = RvSipTransportMgrLocalAddressRemove(hTransportMgr, hLocalAddr);
		stiASSERT (rvStatus == RV_OK);
	}

	/* Find and remove the second TCP address */
	rvStatus = RvSipTransportMgrLocalAddressGetFirst (hTransportMgr, RVSIP_TRANSPORT_TCP, &hLocalAddr);
	stiASSERT (rvStatus == RV_OK);

	if (rvStatus == RV_OK)
	{
		RvSipTransportMgrLocalAddressRemove(hTransportMgr, hLocalAddr);
		stiASSERT (rvStatus == RV_OK);
	}

//STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; AddSLPToDynamicListenAddresses
//
// Description: Adds a Second Listen Port to the listen addresses
//
// Abstract:
// This method assumes that the stack has been configured for two UDP listen
// addresses.  It adds to the first position a second listen port defined by the
// passed in port number.
// NOTE: This method MUST be called from within the SipManager thread.  All startup
//  and shutdown calls are currently expected to be from the same thread.
//
// Returns:
//  stiRESULT_SUCCESS or stiRESULT_ERROR (ORed with an appropriate error code)
//
stiHResult CstiSipManager::AddSLPToDynamicListenAddresses (RvSipTransportMgrHandle hTransportMgr, uint16_t un16SipListenPort)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSipTransportLocalAddrHandle hLocalAddr = nullptr;
	RvSipTransportAddr stAddressDetails;
	RvStatus rvStatus;
	std::string LocalIpAddress;

#ifdef IPV6_ENABLED
	LocalIpAddress.assign (RADVISION_ALL_ADAPTER_ADDRESSES);
#else
	hResult = stiGetLocalIp (&LocalIpAddress, estiTYPE_IPV4);
	stiTESTRESULT ();

	if (m_stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling)
	{
		if (!m_stConferenceParams.TunneledIpAddr.empty ())
		{
			LocalIpAddress.assign ("0.0.0.0");
		}
	}
#endif // not IPV6_ENABLED

	/* Add the new UDP address */
	stAddressDetails.eAddrType = RVSIP_TRANSPORT_ADDRESS_TYPE_IP;
	stAddressDetails.eTransportType = RVSIP_TRANSPORT_UDP;
	stAddressDetails.port = un16SipListenPort;
	strcpy(stAddressDetails.strIP, LocalIpAddress.c_str ());
	rvStatus = RvSipTransportMgrLocalAddressAdd(hTransportMgr,
		&stAddressDetails, sizeof(stAddressDetails), RVSIP_FIRST_ELEMENT, nullptr,
		&hLocalAddr);
	stiASSERT (rvStatus == RV_OK);

	/* Add the new TCP address */
	stAddressDetails.eTransportType = RVSIP_TRANSPORT_TCP;
	rvStatus = RvSipTransportMgrLocalAddressAdd(hTransportMgr,
		&stAddressDetails, sizeof(stAddressDetails), RVSIP_FIRST_ELEMENT, nullptr,
		&hLocalAddr);
	stiASSERT (rvStatus == RV_OK);

#ifndef IPV6_ENABLED
STI_BAIL:
#endif

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::ConferencePortsSet
//
// Description: Sets the firewall ports that are open for conferencing.
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiSipManager::ConferencePortsSet (
	const SstiSipConferencePorts& conferencePorts)
{
	stiLOG_ENTRY_NAME (CstiSipManager::ConferencePortsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();
	
	if (!eventLoopIsRunning ())
	{
		m_stConferencePorts = conferencePorts;

		Unlock ();

		// Notify the application that the ports have been successfully set
		conferencePortsSetSignal.Emit (true);
	}
	else
	{
		Unlock ();

		PostEvent (
		    std::bind(&CstiSipManager::eventConferencePortsSet, this, conferencePorts));
	}

	return (hResult);

} // end CstiSipManager::ConferencePortsSet


/*!
 * \brief Change the firewall ports used for conferencing
 *        This function modifies the ports that are used to open control
 *        channels, media channels, and second listen ports through the
 *        firewall.  This requires that the stack be shutdown and restarted
 *        with the new port information.  This method MUST be called From
 *        within the SipManager eventloop.  All startup and shutdown calls are
 *        currently expected to be from the same thread.
 * \param pstPorts
 */
void CstiSipManager::eventConferencePortsSet (
	const SstiSipConferencePorts &ports)
{
	stiLOG_ENTRY_NAME (CstiSipManager::eventConferencePortsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (0 == m_pCallStorage->countGet (
			esmiCS_CONNECTING | esmiCS_CONNECTED |
			esmiCS_HOLD_LCL | esmiCS_HOLD_RMT |
			esmiCS_HOLD_BOTH),
		stiRESULT_ERROR);

	DBG_MSG ("<CstiSipManager::eventConferencePortsSet>  Changing ports:  from port %d to %d\n",
				m_stConferencePorts.nListenPort, ports.nListenPort);

	// Did the listen port change?
	if (m_stConferencePorts.nListenPort != ports.nListenPort)
	{
		// Yes.  Store the new SLP
		m_stConferencePorts.nListenPort = ports.nListenPort;

		// Yes, Dynamically update the listen ports with the stack.
		hResult = RemoveSLPFromDynamicListenAddresses (m_hTransportMgr);
		stiTESTRESULT ();

		hResult = AddSLPToDynamicListenAddresses (m_hTransportMgr, m_stConferencePorts.nListenPort);
		stiTESTRESULT ();

		DBG_MSG ("SIP Stack dynamically modified to use new port for SLP.\n");

		ConferenceAddressSet ();
	}

#ifdef stiLOG_SLP_CHANGES
	Test (m_hTransportMgr);
#endif //#ifdef stiLOG_SLP_CHANGES

	// Notify the application that the ports have been successfully set
	conferencePortsSetSignal.Emit (true);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Notify the application that the ports did not get set
		conferencePortsSetSignal.Emit (false);

		DBG_MSG ("<CstiSipManager::eventConferencePortsSet>  Failed to change ports.\n");
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RemoteLightRingFlash
//
// Description: Flashes the remote light ring for the given call.
//
// Returns:
//  estiOK (Success) or estiERROR (Failure).
//
stiHResult CstiSipManager::RemoteLightRingFlash (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call, stiRESULT_ERROR);

	PostEvent (
		[call]
		{
			auto sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			sipCall->RemoteLightRingFlash ();
		});

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallReject
//
// Description: Called when we are ringing a call, but the user rejects it.
//
// Returns:
//  estiOK (Success) or estiERROR (Failure).
//
stiHResult CstiSipManager::CallReject (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = nullptr;

	stiTESTCOND (call, stiRESULT_ERROR);
	
	sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
	stiTESTCOND (sipCall, stiRESULT_ERROR);
	
	hResult = sipCall->Reject (SIPRESP_BUSY_EVERYWHERE);

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CallHoldHandle
//
// Description: Places the given call on hold
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
void eventCallHold (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = nullptr;

	stiTESTCOND (call, stiRESULT_ERROR);

	sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());

	if (sipCall &&

		// If it is in a Connected state, Conferencing substate
	    ((call->StateValidate (esmiCS_CONNECTED) && call->SubstateValidate (estiSUBSTATE_CONFERENCING)) ||

		// OR
		// If it is in the Hold Remote state, Held substate
	    (call->StateValidate (esmiCS_HOLD_RMT) && call->SubstateValidate (estiSUBSTATE_HELD))))
	{
		auto dhviCall = sipCall->dhvCstiCallGet ();
		hResult = sipCall->Hold (true);
		stiTESTRESULT ();

		if (dhviCall)
		{
			auto dhvSipCall = std::static_pointer_cast<CstiSipCall>(dhviCall->ProtocolCallGet());

			hResult =  dhvSipCall->Hold (true);
			stiTESTRESULT ();
		}
	}

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallHold
//
// Description: Called when we receive a hold message from the remote phone.
//
// Returns:
//  estiOK (Success) or estiERROR (Failure).
//
stiHResult CstiSipManager::CallHold (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND (call, stiRESULT_ERROR);

	preHoldCallSignal.Emit (call);

	PostEvent (
		[call]
		{
			eventCallHold (call);
		});

STI_BAIL:

	return hResult;
}


/*!
 * \brief Takes the given call off hold
 */
void CstiSipManager::eventCallResume (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = nullptr;
	
	stiTESTCOND (call, stiRESULT_ERROR);
	
	sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
	
	if (sipCall &&

		// Only if it is in a HoldLocal or HoldBoth state
	    call->StateValidate (esmiCS_HOLD_LCL | esmiCS_HOLD_BOTH) &&

		// AND it needs to not already be negotiating a local resume
	    !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
	{
		auto dhviCall = sipCall->dhvCstiCallGet ();
		hResult = sipCall->Hold (false);
		stiTESTRESULT();
		
		if (dhviCall)
		{
			auto dhvSipCall = std::static_pointer_cast<CstiSipCall>(dhviCall->ProtocolCallGet());
			
			hResult =  dhvSipCall->Hold (false);
			stiTESTRESULT ();
		}
	}
#if defined(stiMVRS_APP) || APPLICATION == APP_NTOUCH_PC
	else
	{
		EsmiCallState myState = call->StateGet ();
		EstiSubState mySubState = (EstiSubState)call->SubstateGet();
		stiTrace("Unable to resume call - State: %d SubState: %d\n", myState, mySubState);
		if (call->SubstateValidate(estiSUBSTATE_NEGOTIATING_LCL_HOLD | estiSUBSTATE_NEGOTIATING_LCL_RESUME))
		{
			// Try again later
			usleep (500000);
			CallResume (call);
		}
	}
#endif

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallResume
//
// Description: Called when we receive a resume message from the remote phone.
//
// Returns:
//  estiOK (Success) or estiERROR (Failure).
//
stiHResult CstiSipManager::CallResume (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND (call, stiRESULT_ERROR);

	preResumeCallSignal.Emit (call);

	PostEvent (
		[this, call]
		{
			eventCallResume (call);
		});

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StateDisconnectingProcess
//
// Description: Gets invoked when the call is entering a "disconnecting" state
//
// Abstract: Should perform call teardown prodecures.
//		TODO: There are a few details that need to still be worked out.
// Returns:
//  estiOK (Success) or estiERROR (Failure).
//
stiHResult CstiSipManager::StateDisconnectingProcess (
    CstiCallSP call, ///< The call which is disconnecting
	EsmiCallState eNewState,  ///< The state we are in
	uint32_t un32NewSubstate) ///< More detailed state information
{
	stiLOG_ENTRY_NAME (CstiSipManager::StateDisconnectingProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	EstiBool bShuttingDown = estiFALSE;

	if (call)
	{
		switch (un32NewSubstate)
		{
			case estiSUBSTATE_SHUTTING_DOWN:
				bShuttingDown = estiTRUE;
				break;

			case estiSUBSTATE_BUSY:
			case estiSUBSTATE_REJECT:
				OutgoingToneSet (estiTONE_BUSY);
				break;

			case estiSUBSTATE_UNREACHABLE:
				OutgoingToneSet (estiTONE_FAST_BUSY);
				break;

			case estiSUBSTATE_CREATE_VRS_CALL:
			case estiSUBSTATE_LOCAL_HANGUP:
			case estiSUBSTATE_REMOTE_HANGUP:
			case estiSUBSTATE_UNKNOWN:
			default:
				break;
		} // end switch

		// Check to make sure we are making a legal state transition
		if (eNewState != call->StateGet ())
		{
			switch (call->StateGet ())
			{
				case esmiCS_IDLE:
					// In the case of an incoming call while in Do-Not-Disturb mode, we will get to
					// here with the call object in the CS_IDLE state.  Don't do anything, simply allow
					// the state change.
					break;

				case esmiCS_CONNECTING:
				    if (estiSUBSTATE_RESOLVE_NAME != call->SubstateGet ())
					{
						// Handle this as a missed call.
						missedCallSignal.Emit (call);
					}
					// vvvv FALL THROUGH TO esmiCS_CONNECTED! vvvv
					// vvvv FALL THROUGH TO esmiCS_CONNECTED! vvvv
					// FALL THROUGH
				case esmiCS_CONNECTED:
				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
				case esmiCS_INIT_TRANSFER:
				case esmiCS_TRANSFERRING:
					// If this is a remote hangup we need to call the CstiCall object's
					// hangup method.  If this is simply a local "Hangup", it will have
					// already called this method.
					if (estiSUBSTATE_REMOTE_HANGUP == un32NewSubstate)
					{
						// Hangup the call.  Ignore the return code though.  In some
						// instances, the call handles are already closed and therefore
						// the return from the call to the stack can be an error.  We
						// don't want this to cause us to error.

						// TODO: Think about this
						//call->HangUp ();
					}  // end if

					// If this is a local "Reject" or a hangup/shutdown, we need to post
					// a hangup message so that we don't get out of sync with the state changes.
					// Otherwise, we get a state change inside of a state change and
					// this results in the wrong state at the end.
					else if (bShuttingDown ||
					        (estiINCOMING == call->DirectionGet () &&
							estiSUBSTATE_REJECT == un32NewSubstate))
					{
						// TODO: Think about this
						//call->HangUp ();

						//CallReject (call);
					}  // end else if
					break;

				// These are all invalid state transitions
				case esmiCS_DISCONNECTED:
				case esmiCS_CRITICAL_ERROR:
				case esmiCS_DISCONNECTING:
					if ((eNewState != esmiCS_DISCONNECTING) &&
						(eNewState != esmiCS_DISCONNECTED) &&
						(eNewState != esmiCS_CRITICAL_ERROR))
					{
						stiTHROW (stiRESULT_ERROR);
						break;
					}
					// FALLTHRU
				case esmiCS_UNKNOWN:

				    stiASSERTMSG (estiFALSE, "Bad state transition from %d to %d(%d)",
					              call->StateGet (), eNewState, (int)un32NewSubstate);

					// No one should ever set us to an invalid state.
					stiTHROW (stiRESULT_ERROR);
					break;
			} // end switch
		}
	} // end if
	else
	{
		// Invalid handle.  Log an error.
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	return hResult;
}


// These are not necessary for the SIP implementation of ProtocolManager.
// TODO:  Then we should delete them and probably collapse ProtocolManager
stiHResult CstiSipManager::StateHoldLclProcess (
    CstiCallSP /*call*/, EsmiCallState /*newState*/, uint32_t /*newSubstate*/)
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiSipManager::StateHoldRmtProcess (
    CstiCallSP /*call*/, EsmiCallState /*newState*/, uint32_t /*newSubState*/)
{
	return stiRESULT_SUCCESS;
}

stiHResult CstiSipManager::StateHoldBothProcess (
    CstiCallSP /*call*/, EsmiCallState /*newState*/, uint32_t /*newSubState*/)
{
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StateDisconnectedProcess
//
// Description: Handle the Disconnected State
//
// Abstract:
//  When the system is requested to go to the Disconnected State, this method is
//  called to handle the necessary items.
//
// Returns:
//  estiOK when successful estiERROR otherwise
//
stiHResult CstiSipManager::StateDisconnectedProcess (
    CstiCallSP call,	// The call object
	EsmiCallState eNewState,		// The desired state
	uint32_t un32NewSubstate)	// The desired sub-state
{
	stiLOG_ENTRY_NAME (CstiSipManager::StateDisconnectedProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (call)
	{
		// Ensure that substate is valid for this state.
		switch (un32NewSubstate)
		{
			case estiSUBSTATE_NONE:
			//case estiSUBSTATE_WAITING_FOR_USER_RESP:
			case estiSUBSTATE_LEAVE_MESSAGE:
			case estiSUBSTATE_MESSAGE_COMPLETE:
				// These are allowed for DISCONNECTED
				break;
			default:
				// Other states are invalid
				stiASSERT (estiFALSE);
				break;
		} // end switch

		// Check to make sure we are making a legal state transition
		switch (call->StateGet ())
		{
			case esmiCS_CONNECTING:
				break;

			case esmiCS_DISCONNECTED:
			case esmiCS_DISCONNECTING:
			{
				if (call->m_isDhviMcuCall)
				{
					
					auto state = IstiCall::DhviState::Capable;
					
					if (call->ResultGet() == estiRESULT_CALL_SUCCESSFUL)
					{
						// Notify VRCL that the call was disconnected.
						// Currently call is always disconnected by deaf user
						dhviDisconnectedSignal.Emit(false);
					}
					else
					{
						// Notify VRCL that the call failed.
						dhviConnectionFailedSignal.Emit();
						state = IstiCall::DhviState::Failed;
					}
					
					auto sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
					
					if (sipCall)
					{
						auto dhviCall = sipCall->m_dhviCall;
						if (dhviCall)
						{
							auto dhviSipCall = std::static_pointer_cast<CstiSipCall>(dhviCall->ProtocolCallGet ());

							if (dhviCall->dhviStateGet () != IstiCall::DhviState::NotAvailable)
							{
								dhviCall->dhviStateSet (state);
							}

							if (sipCall->m_videoStream.isActive ())
							{
								dhviSipCall->switchActiveCall (true);
							}

							dhviSipCall->switchedActiveCallsSet (false);
							sipCall->m_dhviCall = nullptr;
						}
					}
				}


				break;
			} // end block

			// These are all invalid state transitions
			case esmiCS_CONNECTED:
			case esmiCS_CRITICAL_ERROR:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			case esmiCS_UNKNOWN:
			case esmiCS_IDLE:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:

				// No one should ever set us to an invalid state.
				stiTHROW (stiRESULT_ERROR);
				break;
		} // end switch
	} // end if
	else
	{
		// Invalid handle.  Log an error.
		stiTHROW (stiRESULT_ERROR);
	} // end else

	// Notify of bridge connection.
	{
		CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		if (sipCall->IsBridgedCall () && sipCall->m_spCallBridge)
		{
			callBridgeStateChangedSignal.Emit (sipCall->m_spCallBridge);
		}
	}

STI_BAIL:

	return (hResult);
} // end CstiSipManager::StateDisconnectedProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StateConnectedProcess
//
// Description: Handle the Connected State
//
// Abstract:
//  When the system is requested to go to the Connected State, this
//  method is called to handle the necessary items.
//
// Returns:
//  estiOK when successful estiERROR otherwise
//
stiHResult CstiSipManager::StateConnectedProcess (
    CstiCallSP call,
	EsmiCallState eNewState, ///> The desired state
	uint32_t un32NewSubstate) ///> The desired sub-state
{
	stiLOG_ENTRY_NAME (CstiSipManager::StateConnectedProcess);

	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (call)
	{
		// If we are going into the connected state, then we need to have a valid
		// substate
		if (estiSUBSTATE_CONFERENCING == un32NewSubstate)
		{
			// We are sure, at this point, that the call has been accepted.
			call->ResultSet (estiRESULT_CALL_SUCCESSFUL);

			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			if (sipCall->m_videoRecord)
			{
				auto sipCallLeg = sipCall->CallLegGet(sipCall->m_hCallLeg);
				unsigned int width = 0;
				unsigned int height = 0;

				sipCallLeg->RemoteDisplaySizeGet(&width, &height);

				sipCall->m_videoRecord->RemoteDisplaySizeSet (width, height);
			}
		}  // end if

		// The only other valid case is a substate of establishing or the negotiation of
		// a call hold.
		// Anything else needs to cause an error to occur.
		else if (estiSUBSTATE_ESTABLISHING != un32NewSubstate &&
				 0 == (estiSUBSTATE_NEGOTIATING_LCL_HOLD & un32NewSubstate) &&
				 0 == (estiSUBSTATE_NEGOTIATING_RMT_HOLD & un32NewSubstate))
		{
			// Log an error.
			stiTHROW (stiRESULT_ERROR);
		} // end else if

		if (!stiIS_ERROR (hResult))
		{
			// Check to make sure we are making a legal state transition
			switch (call->StateGet ())
			{
				// If we are being set the the connected state again, make sure
				// our substate has changed.
				case esmiCS_CONNECTED:
					if (estiSUBSTATE_ESTABLISHING == un32NewSubstate)
					{
							// This is an error!
						stiTHROW (stiRESULT_ERROR);
					} // end if
					break;

				case esmiCS_CONNECTING:
				    switch (call->SubstateGet ())
					{
						case estiSUBSTATE_ANSWERING:
						case estiSUBSTATE_CALLING:
						case estiSUBSTATE_WAITING_FOR_USER_RESP:
						case estiSUBSTATE_WAITING_FOR_REMOTE_RESP:
							break;

						default:
							// Log an error
							stiTHROW (stiRESULT_ERROR);
							break;
					} // end switch
					break;

				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
					// We have already done all the negotiations.  Simply allow
					// the transition to be completed.
					break;


				// These are all invalid state transitions
				case esmiCS_DISCONNECTING:
				case esmiCS_DISCONNECTED:
				case esmiCS_CRITICAL_ERROR:
				default:

					// No one should ever set us to an invalid state.
					stiTHROW (stiRESULT_ERROR);
					break;
			} // end switch
		}  // end if

		// Notify of bridge connection.
		{
			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			if (sipCall->IsBridgedCall () && sipCall->m_spCallBridge)
			{
				callBridgeStateChangedSignal.Emit (sipCall->m_spCallBridge);
			}
		}
		
		// Stop the VRS Failover timers.
		call->FailureTimersStop ();
	} // end if
	else
	{
		// Invalid handle.  Log an error.
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	return (hResult);
} // end CstiSipManager::StateConnectedProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StateCriticalErrorProcess
//
// Description: Handle the Critical Error State
//
// Abstract:
//  When the system is requested to go to the Error State, this method is
//  called to handle the necessary items.
//
// Returns:
//  estiOK when successful estiERROR otherwise
//
stiHResult CstiSipManager::StateCriticalErrorProcess (
    CstiCallSP call,
	EsmiCallState eNewState,		// The desired state
	uint32_t un32NewSubstate)	// The desired sub-state
{
	stiLOG_ENTRY_NAME (CstiSipManager::StateCriticalErrorProcess);
	stiHResult hResult = stiRESULT_SUCCESS;

	// TODO: Report this error up the chain of command.  Pass it first to the CM
	// to see if it can be handled there.
	//ErrorReport (m_pstErrorLog);

	if (call)
	{
		// If we are currently in a call, disconnect it.
		switch (call->StateGet ())
		{
			case esmiCS_CONNECTED:
			case esmiCS_CONNECTING:
			    call->HangUp ();
				break;

			// All other cases can simply fall through
			default:
				break;
		}

		// Notify of bridge connection.
		{
			CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
			if (sipCall->IsBridgedCall () && sipCall->m_spCallBridge)
			{
				callBridgeStateChangedSignal.Emit (sipCall->m_spCallBridge);
			}
		}
	}

	return (hResult);
} // end CstiSipManager::StateCriticalErrorProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StateConnectingProcess
//
// Description: Handle the Connecting State
//
// Abstract:
//  When the system is requested to go to the Connecting State, this
//  method is called to handle the necessary items.
//
// Returns:
//  estiOK when successful estiERROR otherwise
//
stiHResult CstiSipManager::StateConnectingProcess (
    CstiCallSP call,
	EsmiCallState eNewState,		// The desired state
	uint32_t un32NewSubstate)	// The desired sub-state
{
	stiLOG_ENTRY_NAME (CstiSipManager::StateConnectingProcess);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (call)
	{
		// If we are going into the connecting state, then we need to have a valid
		// substate
		switch (un32NewSubstate)
		{
			// These are valid new sub-states
			case estiSUBSTATE_CALLING:
			case estiSUBSTATE_ANSWERING:
			case estiSUBSTATE_RESOLVE_NAME:
			case estiSUBSTATE_WAITING_FOR_USER_RESP:
				break;
			// Stop the failover and please wait timer.
			case estiSUBSTATE_WAITING_FOR_REMOTE_RESP:
			{
			    call->FailureTimersStop ();
				break;
			}

			default:
				// No one should ever set us to an invalid state.
				stiTHROW (stiRESULT_ERROR);
				break;
		} // end switch

		if (!stiIS_ERROR (hResult))
		{
			// Check to make sure we making a legal state transition
			switch (call->StateGet ())
			{
				case esmiCS_IDLE:
				case esmiCS_CONNECTED:
				case esmiCS_CONNECTING:
				case esmiCS_DISCONNECTING:
				case esmiCS_INIT_TRANSFER:
				case esmiCS_TRANSFERRING:
					switch (un32NewSubstate)
					{
						case estiSUBSTATE_ANSWERING:
							// If we've come from a Calling Substate, we errored.
						    if (estiSUBSTATE_CALLING == call->SubstateGet ())
							{
								// We've made an invalid state change.  Log an error
								stiASSERT (estiFALSE);
							}  // end if

							// Accept the call.
							hResult = CallAccept (call);

							// Did we fail on the accept?
							if (stiIS_ERROR (hResult))
							{
								// Yes.  Post a message to hangup locally.
								// NOTE:  This must be posted since the
								// NextStateSet method only sets the state after
								// calling this sub-process.  If we call
								// something here that causes a state change, it
								// would erroneously be set again upon returning
								PostEvent (
								    std::bind(&CstiSipManager::eventCallHangup, this, call, SIPRESP_NONE));
							}

							break;

						case estiSUBSTATE_WAITING_FOR_USER_RESP:
						case estiSUBSTATE_WAITING_FOR_REMOTE_RESP:
						case estiSUBSTATE_CALLING:
						case estiSUBSTATE_RESOLVE_NAME:
							break;

						default:
							// No one should ever set us to an invalid state.
							stiTHROW (stiRESULT_ERROR);
							break;
					} // end switch
					break;

				// These are all invalid state transitions
				case esmiCS_CRITICAL_ERROR:
				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
				default:

					// No one should ever set us to an invalid state.
					stiTHROW (stiRESULT_ERROR);
					break;
			} // end switch
		}  // end if
	} // end if
	else
	{
		// Invalid handle.  Log an error.
		stiTHROW (stiRESULT_ERROR);
	} // end else

STI_BAIL:

	return (hResult);
} // end CstiSipManager::StateConnectingProcess


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallHangup
//
// Description: Hangup the current call
//
// Abstract:
//  This method causes the current call to be disconnected.
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
stiHResult CstiSipManager::CallHangup (
    CstiCallSP call,
    SipResponse sipRejectCode)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (call)
	{
		PostEvent (
		    std::bind(&CstiSipManager::eventCallHangup, this, call, sipRejectCode));
	}

	return hResult;
}


/*!
 * \brief Hangup the current call
 * \param poCall
 * \param un16SipRejectCode
 */
void CstiSipManager::eventCallHangup (
    CstiCallSP call,
    SipResponse sipRejectCode)
{
	stiLOG_ENTRY_NAME (CstiSipManager::eventCallHangup);

	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG (hResult);

	CstiSipCallSP sipCall = nullptr;
	
	stiTESTCOND (call, stiRESULT_ERROR);
	
	// If the call object has already been disconnected, just ignore it.
	if (esmiCS_DISCONNECTED != call->StateGet ())
	{
		sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());

		// Need to verify what state we are in.  If we are currently waiting to
		// have a directory service name resolved, calling hangup will cause the
		// call state to transition appropriately to the disconnected state.
		if (esmiCS_CONNECTING == call->StateGet () &&
		    estiSUBSTATE_RESOLVE_NAME == call->SubstateGet ())
		{
			// Drop the reserved stack call handle
			sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, sipRejectCode);

		}  // end if
		else
		{
			// Hangup the call.  If for any reason we have already started the
			// hangup (say due to a remote hangup), don't do anything.
			// We have to be connecting, connected or (in the case of an incoming
			// call that we have rejected) disconnecting with a reject substate.
			if (call->StateValidate (esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH |
									esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_TRANSFERRING |
									esmiCS_INIT_TRANSFER) ||
			    (estiINCOMING == call->DirectionGet () &&
			    esmiCS_DISCONNECTING == call->StateGet () &&
			    estiSUBSTATE_REJECT == call->SubstateGet ()))
			{
				sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, sipRejectCode);
			}  // end if
		}  // end else
	} // end else
	// Check to see if we were viewing a greeting or recording a message.
	else if (esmiCS_DISCONNECTED == call->StateGet () &&
	         call->SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE | estiSUBSTATE_MESSAGE_COMPLETE))
	{
		NextStateSet (call, esmiCS_DISCONNECTED, estiSUBSTATE_NONE);
	}

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallAnswer
//
// Description: Answers the incoming call
//
// Abstract:
//  Answers the incoming call
//
// Returns:
//  estiOK (Success), estiERROR (Failure)
//
stiHResult CstiSipManager::CallAnswer (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (CstiSipManager::CallAnswer);
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call, stiRESULT_ERROR);

	preAnsweringCallSignal.Emit (call);

	PostEvent (
	    std::bind(&CstiSipManager::eventCallAnswer, this, call));

STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallTransfer
//
// Description: Transfer a call to another device
//
// Returns:
//	stiHResult
//
stiHResult CstiSipManager::CallTransfer (
    CstiCallSP call,
    std::string dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiTESTCOND (call, stiRESULT_ERROR);

	PostEvent (
	    std::bind(&CstiSipManager::eventCallTransfer, this, call, dialString));

STI_BAIL:
	return hResult;
}


/*!
 * \brief Transfer a call to another device
 * \param poCall
 * \param dialString
 */
void CstiSipManager::eventCallTransfer (
    CstiCallSP call,
	std::string dialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = nullptr;
	
	stiTESTCOND (call, stiRESULT_ERROR);
	stiTESTCOND (!dialString.empty(), stiRESULT_ERROR);
	
	sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
	stiTESTCOND (sipCall, stiRESULT_ERROR);
	
	if (call->StateGet() == esmiCS_TRANSFERRING)
	{
		// We are going to try and connect to the number listed in TransferDialString.
		// To resolve SignMail issues we will set RemoteDialString to be the same as this number.
		std::string transferDialString;
		std::string remoteDialString;
		call->RemoteDialStringGet(&remoteDialString);
		call->TransferDialStringGet(&transferDialString);
		call->RemoteDialStringSet(transferDialString);

		ContinueTransfer(call);
	}
	else if (stiStrNICmp (dialString.c_str(), "SIP:", 4) == 0
	          || IPAddressValidate (dialString.c_str())) // If we have a SIP: URI or IP address already then transfer.
	{
		hResult = sipCall->TransferToAddress (dialString);
		call->TransferDialStringSet ("");
	}
	else
	{
		// Set transferring number and call state and perform Directory Resolve
		call->TransferDialStringSet (dialString);
		call->StateSet (esmiCS_INIT_TRANSFER);

		directoryResolveSignal.Emit (call);
	}

	stiTESTRESULT ();
	
STI_BAIL:
	if (stiIS_ERROR (hResult))
	{
		EsmiCallState state = call->StateGet();

		if (state == esmiCS_TRANSFERRING || state == esmiCS_INIT_TRANSFER)
		{
			call->StateSet (esmiCS_CONNECTED);
		}

		callTransferFailedSignal.Emit (call);
	}
}


/*!\brief Sends the device's preferred display size to remote endpoints.
 *
 * This method will send a SIP INFO message specifying the preferred display
 * size it would like to receive.
 *
 * \return Success or failure
 */
void CstiSipManager::eventPreferredDisplaySizeChanged ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto calls = m_pCallStorage->listGet ();
	for (auto call : calls)
	{
		CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		stiTESTCOND (sipCall, stiRESULT_ERROR);
		
		hResult = sipCall->PreferredDisplaySizeSend();
		stiTESTRESULT ();
	}
	
STI_BAIL:
	
	return;
}


/*!\brief Updates the geolocation member variable and, if not empty,
 *  sends the device's geolocation to those calls needing it.
 *
 * This method updates geolocation and sends a SIP INFO message specifying geolocation.
 *
 * \return void
 */
void CstiSipManager::eventGeolocationChanged (
	const std::string& geolocation)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (!geolocation.empty ())
	{
		// Set member variable.
		m_geolocation = geolocation;
		
		// Send geolocation SIP INFO message for any calls requesting geolocation
		auto calls = m_pCallStorage->listGet ();
		for (auto call : calls)
		{
			if (call->geolocationRequestedGet ())
			{
				CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
				stiTESTCOND (sipCall, stiRESULT_ERROR);
				
				hResult = sipCall->GeolocationSend ();
				stiTESTRESULT ();
			}
		}
	}
	
STI_BAIL:
	
	return;
}


/*!\brief Clears the geolocation member variable.
 *
 * \return void
 */
void CstiSipManager::eventGeolocationClear ()
{
	m_geolocation.clear ();
}


/*!\brief Attemps to get a public IPv4 address
 *
 * \return IPv4 address
 */
const std::string CstiSipManager::publicIpGet()
{
	std::string publicIp;

	// try getting from conference params
	if (!m_stConferenceParams.PublicIPv4.empty ())
	{
		publicIp = m_stConferenceParams.PublicIPv4;
		stiRemoteLogEventSend("EventType=publicIpGet Reason=\"Using IPv4 from conference params (%s)\"", publicIp.c_str ());
	}
	// try converting from IPv6 to IPv4
	else if (!m_stConferenceParams.PublicIPv6.empty ())
	{
		stiMapIPv6AddressToIPv4 (m_stConferenceParams.PublicIPv6, &publicIp);
		stiRemoteLogEventSend ("EventType=publicIpGet Reason=\"Using IPv6 converted to IPv4 from conference params (%s -> %s)\"", 
			m_stConferenceParams.PublicIPv6.c_str (), publicIp.c_str ());
	}
	// try fallback to public ip from core
	else if (!m_corePublicIp.empty ())
	{
		publicIp = m_corePublicIp;
		stiRemoteLogEventSend("EventType=publicIpGet Reason=\"Using public IP from Core (%s)\"", publicIp.c_str ());
	}
	else
	{
		stiASSERTMSG(false, "No public IP found");
	}
	
	return publicIp;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::DSNameResolved
//
// Description: Notifies the Conference Manager of a resolved name
//
// Abstract:
//	This method is called once the Directory Services has resolved a name.  It
//	passes in the dialing information need to complete the call.  The dial
//	string is formatted and information is updated in the remote system data
//	structure then the message is passed to the CM task informing it to complete
//	the call.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiSipManager::DSNameResolved (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (call)
	{
		preCompleteCallSignal.Emit (call);

		PostEvent (
		    std::bind(&CstiSipManager::eventDsNameResolved, this, call));
	}

	return hResult;
}


/*!
 * \brief This method is called once Directory Services has resolved a name.
 * \param poCall
 */
void CstiSipManager::eventDsNameResolved (
    CstiCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call, stiRESULT_ERROR);
	
	hResult = CallDial (call);
	stiTESTRESULT ();

STI_BAIL:
	return;
}


/*!
 * \brief Answers the incoming call
 * \param poCall
 */
void CstiSipManager::eventCallAnswer (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (void CstiSipManager::eventCallAnswer);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (call, stiRESULT_ERROR);
	
	// This should only be called if we are in the state "esmiCS_CONNECTING"
	// and the substate "SUBSTATE_WAITING_FOR_USER_RESP".
	// If we are in any other substate, ignore it.
	if (call->StateValidate (esmiCS_CONNECTING) &&
	    call->SubstateValidate (estiSUBSTATE_WAITING_FOR_USER_RESP))
	{
		// Make the appropriate change to the state.
		hResult = NextStateSet (call, esmiCS_CONNECTING, estiSUBSTATE_ANSWERING);
		stiTESTRESULT ();
	}  // end if

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::PrintMessageContents ()
//
// Description: Debug tool to print a sip message
//
// Abstract:
//
// Returns:
//  nothing
void CstiSipManager::PrintMessageContents (
	IN RvSipMsgHandle hMsg) /// The sip message to print
{
	std::string message;

	sipMessageGet (hMsg, m_hLog, &message);

	DBG_MSG ("SIP MESSAGE: %s", message.c_str ());
	
} // end CstiSipManager::PrintMessageContents ()


/*!\brief This method sends a video flow control to the sip call object.
 * 
 * This method is in place to avoid a deadlock in the Radvision stack.
 *
 * \param sipCall The call object for which the flow control is needed.
 * \param pIChannel The video playback channel on which the control is needed.
 * \param nNewRate The new data rate (in 100kbps units).
 */
stiHResult CstiSipManager::VideoFlowControlSend (
    CstiSipCallSP sipCall,
	std::shared_ptr<CstiVideoPlayback> videoPlayback,
	int newRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (sipCall)
	{
		PostEvent (
			[sipCall, videoPlayback, newRate]
			{
				sipCall->VideoFlowControlSendHandle (videoPlayback, newRate);
			});
	}
	
	return hResult;
}


/*!\brief Is the local interface mode an "agent"
 */
bool CstiSipManager::IsLocalAgentMode ()
{
	bool agentMode = false;

	switch (LocalInterfaceModeGet ())
	{
	case estiINTERPRETER_MODE:
	case estiTECH_SUPPORT_MODE:
	case estiVRI_MODE:
		// Agent modes
		agentMode = true;
		break;
	default:
		break;
	}

	return agentMode;
}

/*!\brief Creates the list of preferred video playback protocols
 *
 */
stiHResult CstiSipManager::PreferredVideoPlaybackProtocolsCreate (
	std::list<SstiPreferredMediaMap> *pProtocols)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// What packetizations are we able to support for video playback?
	std::list <EstiPacketizationScheme> PktSchemes;
	VideoPlaybackPacketizationSchemesGet (&PktSchemes);
	PreferredVideoProtocolsGet (pProtocols, PktSchemes, true);

	return hResult;
}


/*!\brief Creates the list of preferred video record protocols
 *
 */
stiHResult CstiSipManager::PreferredVideoRecordProtocolsCreate (
	std::list<SstiPreferredMediaMap> *pProtocols)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// What packetizations are we able to support for video record?
	std::list <EstiPacketizationScheme> PktSchemes;
	VideoRecordPacketizationSchemesGet (&PktSchemes);
	PreferredVideoProtocolsGet (pProtocols, PktSchemes, false);

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::PreferredVideoProtocolsGet
//
// Description: Gets a list of all supported video protocols compatible with the given packetization scheme.
//
// Returns:
//	the list
//
void CstiSipManager::PreferredVideoProtocolsGet (
	std::list<SstiPreferredMediaMap> *pProtocols, // The list to fill in
	const std::list <EstiPacketizationScheme> &pktSchemes, // The packetization schemes for this media direction.
	bool bIsPlayback) // if true it is for playback, false = record.
{
	std::list<EstiVideoCodec> Codecs;

	if (bIsPlayback)
	{
		VideoPlaybackCodecsGet (&Codecs);
	}
	else
	{
		VideoRecordCodecsGet (&Codecs);
	}
	
	if (m_stConferenceParams.rtcpFeedbackNackRtxSupport != SignalingSupport::Disabled)
	{
		Codecs.push_back(estiVIDEO_RTX);
	}

	// If there is a preferred codec and this is for playback, find it and move it to the top
	if (bIsPlayback && estiVIDEO_NONE != m_stConferenceParams.ePreferredVideoCodec)
	{
		for (auto it = Codecs.begin (); it != Codecs.end (); it++)
		{
			EstiVideoCodec eCodec = estiVIDEO_NONE;

			// Convert from the EstiVideoCodec to EstiVideoCodec so that we are testing the appropriate thing.
			switch (*it)
			{
				case estiVIDEO_NONE:
					eCodec = estiVIDEO_NONE;
					break;

				case estiVIDEO_H263:
					eCodec = estiVIDEO_H263;
					break;

				case estiVIDEO_H264:
					eCodec = estiVIDEO_H264;
					break;

				case estiVIDEO_H265:
					eCodec = estiVIDEO_H265;
					break;
					
				case estiVIDEO_RTX:
					eCodec = estiVIDEO_RTX;
			}

			if (eCodec == m_stConferenceParams.ePreferredVideoCodec)
			{
				Codecs.push_front (*it);
				Codecs.erase (it);

				// Stop processing since we have found and moved the preferred.
				break;
			}
		}
	}

	for (auto &codec: Codecs)
	{
		switch (codec)
		{
			case estiVIDEO_H263:
				for (auto &scheme: pktSchemes)
				{
					switch (scheme)
					{
						case estiH263_RFC_2190: /*!< h.263 ratified method */
						{
							SstiPreferredMediaMap protocol;
							SstiVideoCapabilities stCaps;
							protocol.nCodec = estiVIDEO_H263;
							protocol.eCodec = estiH263_VIDEO;
							protocol.n8PayloadTypeNbr = RV_RTP_PAYLOAD_H263;
							protocol.protocolName = "H263";
							protocol.PacketizationSchemes.push_back (estiH263_RFC_2190);

							// Get the codec capabilities (along with the supported profiles)
							if (bIsPlayback)
							{
								PlaybackCodecCapabilitiesGet (estiVIDEO_H263, &stCaps);
							}
							else
							{
								RecordCodecCapabilitiesGet (estiVIDEO_H263, &stCaps);
							}

							protocol.Profiles = stCaps.Profiles;

							pProtocols->push_back (protocol);
							break;
						}

						case estiH263_RFC_2429: /*!< h.263+ or 1998 method */
						{
							SstiPreferredMediaMap protocol;
							SstiVideoCapabilities stCaps;
							protocol.nCodec = estiVIDEO_H263;
							protocol.eCodec = estiH263_1998_VIDEO;
							protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
							protocol.protocolName = "H263-1998";
							protocol.PacketizationSchemes.push_back (estiH263_RFC_2429);

							// Get the codec capabilities (along with the supported profiles)
							if (bIsPlayback)
							{
								PlaybackCodecCapabilitiesGet (estiVIDEO_H263, &stCaps);
							}
							else
							{
								RecordCodecCapabilitiesGet (estiVIDEO_H263, &stCaps);
							}
							protocol.Profiles = stCaps.Profiles;

							pProtocols->push_back (protocol);
							break;
						}

						default:
							break;
					}
				}
				break;

			case estiVIDEO_H264:
			{
				SstiPreferredMediaMap protocol;
				protocol.nCodec = estiVIDEO_H264;
				protocol.eCodec = estiH264_VIDEO;
				protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
				protocol.protocolName = "H264";

				SstiH264Capabilities stCaps;

				// Get the codec capabilities (along with the supported profiles)
				if (bIsPlayback)
				{
					PlaybackCodecCapabilitiesGet (estiVIDEO_H264, &stCaps);
				}
				else
				{
					RecordCodecCapabilitiesGet (estiVIDEO_H264, &stCaps);
				}

				// Loop first on the profiles
				protocol.Profiles = stCaps.Profiles;

				// Now loop on the packetizations
				for (auto &scheme: pktSchemes)
				{
					switch (scheme)
					{
						case estiH264_SINGLE_NAL: /*!< required for H.264, default method */
						case estiH264_NON_INTERLEAVED: /*!< Support for NAL, STAP-A, and FU-A types */
						case estiH264_INTERLEAVED:
							protocol.PacketizationSchemes.push_back (scheme);
							break;

						case estiH263_Microsoft:
						case estiH263_RFC_2190:
						case estiH263_RFC_2429:
						default:
							break;
					}
				}

				pProtocols->push_back (protocol);
				break;
			}
			
			case estiVIDEO_H265:
			{
				SstiPreferredMediaMap protocol;
				protocol.nCodec = estiVIDEO_H265;
				protocol.eCodec = estiH265_VIDEO;
				protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
				protocol.protocolName = "H265";

				SstiH265Capabilities stCaps;

				// Get the codec capabilities (along with the supported profiles)
				if (bIsPlayback)
				{
					PlaybackCodecCapabilitiesGet(estiVIDEO_H265, &stCaps);
				}
				else
				{
					RecordCodecCapabilitiesGet(estiVIDEO_H265, &stCaps);
				}

				// Loop first on the profiles
				protocol.Profiles = stCaps.Profiles;

				// Now loop on the packetizations
				for (auto &scheme: pktSchemes)
				{
					switch (scheme)
					{
					case estiH265_NON_INTERLEAVED: /*!< Default H265 */
						protocol.PacketizationSchemes.push_back(scheme);
						break;
					default:
						break;
					}
				}

				//Only add the protocol if it has packetization schemes supporting it.
				if (!protocol.PacketizationSchemes.empty ())
				{
					pProtocols->push_back(protocol);
				}
				break;
			}
				
			case estiVIDEO_RTX:
			{
				SstiPreferredMediaMap protocol;
				protocol.nCodec = estiVIDEO_RTX;
				protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
				protocol.protocolName = "rtx";
				protocol.eCodec = estiRTX;
				
				pProtocols->push_back (protocol);
				
				break;
			}

			default:
				break;
		}
	}
}


/*!\brief Determine whether or not a specific audio protocol is allowed by ConferenceParams
 *
 * NOTE: If the list of allowed protocols is empty/not used, then we will allow everything.
 */
bool CstiSipManager::AudioProtocolAllowed (const std::string &protocol)
{
	bool bAllowed = true;
	
	if (!m_stConferenceParams.AllowedAudioCodecs.empty ())
	{
		bAllowed = false;
		for (auto &codec: m_stConferenceParams.AllowedAudioCodecs)
		{
			if (codec == protocol)
			{
				bAllowed = true;
				break;
			}
		}
	}
	
	return bAllowed;
}


/*!\brief Creates the list of preferred audio protocols
 *
 */
stiHResult CstiSipManager::PreferredAudioProtocolsCreate (
	std::list<SstiPreferredMediaMap> *pProtocols,
	std::list<SstiPreferredMediaMap> *pFeatures)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (pProtocols->empty ())
	{
		// What codecs does our platform layer support?
		std::list<EstiAudioCodec> AudioCodecs;
		AudioCodecsGet (&AudioCodecs);

		// If there is a preferred codec, find it and move it to the top
		if (estiAUDIO_NONE != m_stConferenceParams.ePreferredAudioCodec)
		{
			for (auto it = AudioCodecs.begin (); it != AudioCodecs.end (); it++)
			{
				if ((*it) == m_stConferenceParams.ePreferredAudioCodec)
				{
					AudioCodecs.push_front (*it);
					AudioCodecs.erase (it);

					// Stop processing since we have found and moved the preferred.
					break;
				}
			}
		}

		// Now loop through each of the codecs and create a new list of them that
		// contains all the necessary items needed to negotiate with it.
		SstiPreferredMediaMap protocol;
		for (auto &codec: AudioCodecs)
		{
			switch (codec)
			{
				case estiAUDIO_G711_ALAW:
					protocol.protocolName = "PCMA";
					if (AudioProtocolAllowed (protocol.protocolName))
					{
						protocol.nCodec = estiAUDIO_G711_ALAW;
						protocol.eCodec = estiG711_ALAW_56K_AUDIO;
						protocol.n8PayloadTypeNbr = RV_RTP_PAYLOAD_PCMA;

						pProtocols->push_back (protocol);
					}
					break;

				case estiAUDIO_G711_MULAW:
					protocol.protocolName = "PCMU";
					if (AudioProtocolAllowed (protocol.protocolName))
					{
						protocol.nCodec = estiAUDIO_G711_MULAW;
						protocol.eCodec = estiG711_ULAW_64K_AUDIO;
						protocol.n8PayloadTypeNbr = RV_RTP_PAYLOAD_PCMU;

						pProtocols->push_back (protocol);
					}
					break;

				case estiAUDIO_G722:
					protocol.protocolName = "G722";
					if (AudioProtocolAllowed (protocol.protocolName))
					{
						protocol.nCodec = estiAUDIO_G722;
						protocol.eCodec = estiG722_64K_AUDIO;
						protocol.n8PayloadTypeNbr = RV_RTP_PAYLOAD_G722;

						pProtocols->push_back (protocol);
					}
					break;

				default:
					break;

			} // end switch
		} // end for
	} // end if

	if (pFeatures->empty ())
	{
		// dtmf
		if (m_stConferenceParams.bDTMFEnabled)
		{
			SstiPreferredMediaMap protocol;
			protocol.nCodec = estiAUDIO_DTMF;
			protocol.eCodec = estiTELEPHONE_EVENT;
			// Some vendors are erroneously expecting payload type 101 for DTMF
			// TODO: what did their sdp offer/response look like? (ie: did they falsely advertise?)
			protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
			protocol.preferredAudioFeatureDynamicPayloadType = PAYLOAD_TYPE_DTMF;
			protocol.protocolName = "telephone-event";
			pFeatures->push_back (protocol);
		}
	}
	else if (!m_stConferenceParams.bDTMFEnabled) // Was DTMF enabled and now we need to turn it off?
	{
		pFeatures->clear();
	}

	return hResult;
}


/*!\brief Creates the list of preferred text protocols
 *
 */
stiHResult CstiSipManager::PreferredTextProtocolsCreate (
	std::list<SstiPreferredMediaMap> *pProtocols)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (pProtocols->empty ())
	{
		// Set all supported codecs
		{ // T.140 Redundancy
			SstiPreferredMediaMap protocol;
			protocol.nCodec = estiTEXT_T140_RED;
			protocol.eCodec = estiT140_RED_TEXT;
			protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
			protocol.protocolName = "red";
			pProtocols->push_back (protocol);
		}

		{ // T.140
			SstiPreferredMediaMap protocol;
			protocol.nCodec = estiTEXT_T140;
			protocol.eCodec = estiT140_TEXT;
			protocol.n8PayloadTypeNbr = INVALID_DYNAMIC_PAYLOAD_TYPE;
			protocol.protocolName = "t140";
			pProtocols->push_back (protocol);
		}

		// If there is a preferred codec, find it and move it to the top
		if (estiTEXT_NONE != m_stConferenceParams.ePreferredTextCodec)
		{
			for (auto it = pProtocols->begin (); it !=  pProtocols->end (); it++)
			{
				if (it->nCodec == m_stConferenceParams.ePreferredTextCodec)
				{
					pProtocols->push_front(*it);
					pProtocols->erase(it);
					break;
				}
			}
		}
	}

	return hResult;
}

/*!\brief Handles the an ice gathering trying message by trying another server
 * and sending a sip trying message.
 *
 * NOTE: this should only be called when the ice manager has been initialized
 *
 * \param call leg key
 */
void CstiSipManager::eventIceGatheringTryingAnother(size_t callLegKey)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLeg = callLegLookup (callLegKey);

	if (m_iceManager && callLeg)
	{
		auto sipCall = callLeg->m_sipCallWeak.lock ();
		RvSipCallLegHandle hCallLeg = sipCall->StackCallHandleGet ();

		sipCall->CstiCallGet ()->collectTrace (std::string{__func__});

		// We cannot try another server on the current ICE session without potentially crashing.
		// Start a new ICE session with data from the previous one. Fixes bug #24491.
		hResult = m_iceManager->IceSessionTryAnother (m_stConferenceParams.PublicIPv4,
			m_stConferenceParams.PublicIPv6,
			callLeg->uniqueKeyGet(),
			IceProtocolGet (callLeg),
			&callLeg->m_pIceSession);

		if (stiIS_ERROR (hResult))
		{
			stiASSERTMSG(estiFALSE, "CstiIceManager::IceSessionTryAnother failed");
		}

		// Send a trying message
		RvSipCallLegProvisionalResponse (hCallLeg, SIPRESP_TRYING);
	}
}

stiHResult CstiSipManager::CallObjectRemove (
	CstiCallSP call)
{
	return m_pCallStorage->remove (call.get());
}


/**
 * \brief Turn on or off the ability to receive incoming calls (except CIR and 911 callbacks)
 * 
 * \param bAllow If false, only allow incoming calls from CIR and 911 callback
 */
void CstiSipManager::AllowIncomingCallsModeSet(bool bAllow)
{
	Lock ();
	// Invert bAllow; If bAllow is false, we want to disallow incoming calls
	//  ... yes, that is a bit confusing... but naming for this feature was problematic!
	m_bDisallowIncomingCalls = !bAllow;
	
	Unlock ();
}


/*!
 * \brief Event handler for creating a bridge
 * \param sipCall
 * \param uri
 */
void eventBridgeCreate (
    CstiSipCallSP sipCall,
	std::string uri)
{
	// Only create a call bridge if audio is setup, otherwise just set the URI
	if (sipCall->isAudioInitialized())
	{
		stiRemoteLogEventSend("EventType=BridgeCreate Reason=Attempting Bridge Creation URI=%s", uri.c_str());
		sipCall->BridgeCreate (uri);
	}
	else
	{
		stiRemoteLogEventSend("EventType=BridgeCreate Reason=Audio Not Ready storing URI=%s", uri.c_str());
		sipCall->bridgeUriSet(uri);
	}
}


stiHResult CstiSipManager::BridgeCreate (
	const std::string &uri,
    CstiCallSP call)
{
	// Handle the event asynchronously.
	PostEvent (
		[call, uri]
		{
			eventBridgeCreate (std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet()), uri);
		});

	return stiRESULT_SUCCESS;
}


//TODO: Conference ID may no longer be needed once MCU is updated to provide member status through SIP
/*!
 * \brief Event handler for creating a backgroundCall
 * \param uri - URI we are going to be creating.
 * \param conferenceId the conference ID to be used for getting conference stats.
 * \param sipCall shared pointer for sip call object.
 * \param sendInfo indicates if we should send an INFO message to the remote endpoint to join this conference
 */
void CstiSipManager::dhviCreate (
	const std::string &uri,
	const std::string &conferenceId,
	CstiCallSP call,
	bool sendInfo)
{
	// Handle the event asynchronously.
	PostEvent ([this, call, uri, conferenceId, sendInfo] {
		auto sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		sipCall->dhviCreate (uri, conferenceId, sendInfo);
	});
}

//TODO: Conference ID may no longer be needed once MCU is updated to provide member status through SIP
/*!
 * \brief Event handler for creating a backgroundCall
 * \param sipCall shared pointer for sip call object.
 * \param uri - URI we are going to be creating.
 * \param conferenceId the conference ID to be used for getting conference stats.
 * \param sendInfo indicates if we should send an INFO message to the remote endpoint to join this conference
 */
// TODO: This doesn't appear to be used by anything. Can this be deleted?
void eventDhviCreate (
	CstiSipCallSP sipCall,
	const std::string &uri,
	const std::string &conferenceId,
	bool sendInfo)
{
	sipCall->dhviCreate (uri, conferenceId, sendInfo);
}


///////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::RvStatsGet
//
// Description: Get stats about the various objects within the RV system.
//
// Abstract:
//
// Returns:
//  none
//
void CstiSipManager::RvStatsGet (SstiRvSipStats *pstRvStats)
{
	// To build and access the following methods:
	// - the VideophoneEngine needs to be built with SIP_DEBUG defined.
	// - the Radvision Stack needs to be built with SIP_DEBUG defined.  (I added it to CFLAGS in default.mak.x86)

	// Get the Stack Resources information using RvSipStackGetResources.
	// The various resource structures are defined in the SIP Stack Reference Guide (v6.1)
	// The resource modules are defined on pg 1032

	Lock ();

	if (m_hStackMgr)
	{
		// Get the status of the call-leg module
		RvSipCallLegResources stCallLeg;	// pg 1519
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_CALL, &stCallLeg))
		{
			pstRvStats->CallLegCalls.un32NumOfAllocatedElements = stCallLeg.calls.numOfAllocatedElements;
			pstRvStats->CallLegCalls.un32NumOfUsedElements = stCallLeg.calls.currNumOfUsedElements;
			pstRvStats->CallLegCalls.un32MaxUsageOfElements = stCallLeg.calls.maxUsageOfElements;
			pstRvStats->CallLegTrnsLst.un32NumOfAllocatedElements = stCallLeg.transcLists.numOfAllocatedElements;
			pstRvStats->CallLegTrnsLst.un32NumOfUsedElements = stCallLeg.transcLists.currNumOfUsedElements;
			pstRvStats->CallLegTrnsLst.un32MaxUsageOfElements = stCallLeg.transcLists.maxUsageOfElements;
			pstRvStats->CallLegTrnsHndl.un32NumOfAllocatedElements = stCallLeg.transcHandles.numOfAllocatedElements;
			pstRvStats->CallLegTrnsHndl.un32NumOfUsedElements = stCallLeg.transcHandles.currNumOfUsedElements;
			pstRvStats->CallLegTrnsHndl.un32MaxUsageOfElements = stCallLeg.transcHandles.maxUsageOfElements;
			pstRvStats->CallLegInvtLst.un32NumOfAllocatedElements = stCallLeg.inviteLists.numOfAllocatedElements;
			pstRvStats->CallLegInvtLst.un32NumOfUsedElements = stCallLeg.inviteLists.currNumOfUsedElements;
			pstRvStats->CallLegInvtLst.un32MaxUsageOfElements = stCallLeg.inviteLists.maxUsageOfElements;
			pstRvStats->CallLegInvtObjs.un32NumOfAllocatedElements = stCallLeg.inviteObjects.numOfAllocatedElements;
			pstRvStats->CallLegInvtObjs.un32NumOfUsedElements = stCallLeg.inviteObjects.currNumOfUsedElements;
			pstRvStats->CallLegInvtObjs.un32MaxUsageOfElements = stCallLeg.inviteObjects.maxUsageOfElements;
		}

// 		// Get the status of the Compartment module
// 		RvSipCompartmentResources stCompartment;
// 		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_COMPARTMENT, &stCompartment))
// 		{
// 			pstRvStats->CompCompartmnts.un32NumOfAllocatedElements = stCompartment.compartments.numOfAllocatedElements;
// 			pstRvStats->CompCompartmnts.un32NumOfUsedElements = stCompartment.compartments.currNumOfUsedElements;
// 			pstRvStats->CompCompartmnts.un32MaxUsageOfElements = stCompartment.compartments.maxUsageOfElements;
// 			pstRvStats->CompSigCmpIdElm.un32NumOfAllocatedElements = stCompartment.sigCompIdElem.numOfAllocatedElements;
// 			pstRvStats->CompSigCmpIdElm.un32NumOfUsedElements = stCompartment.sigCompIdElem.currNumOfUsedElements;
// 			pstRvStats->CompSigCmpIdElm.un32MaxUsageOfElements = stCompartment.sigCompIdElem.maxUsageOfElements;
// 		}

// 			// Get the status of the Message module
// 			RvSipBlah stMessage;   // Can't find such a beast.
// 			RvSipStackGetResources (m_hStackMgr, RVSIP_MESSAGE, &stMessage);

		// Get the status of the Publish-Client module
		RvSipPubClientResources stPubClient;  // Page 1528
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_PUBCLIENT, &stPubClient))
		{
			pstRvStats->PublishClient.un32NumOfAllocatedElements = stPubClient.pubClients.numOfAllocatedElements;
			pstRvStats->PublishClient.un32NumOfUsedElements = stPubClient.pubClients.currNumOfUsedElements;
			pstRvStats->PublishClient.un32MaxUsageOfElements = stPubClient.pubClients.maxUsageOfElements;
		}

		// Get the status of the Register-Client module
		RvSipRegClientResources stRegClient;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_REGCLIENT, &stRegClient))
		{
			pstRvStats->RegisterClient.un32NumOfAllocatedElements = stRegClient.regClients.numOfAllocatedElements;
			pstRvStats->RegisterClient.un32NumOfUsedElements = stRegClient.regClients.currNumOfUsedElements;
			pstRvStats->RegisterClient.un32MaxUsageOfElements = stRegClient.regClients.maxUsageOfElements;
		}

		// Get the status of the Resolver module
		RvSipResolverResources stResolver;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_RESOLVER, &stResolver))
		{
			pstRvStats->Resolver.un32NumOfAllocatedElements = stResolver.resolvers.numOfAllocatedElements;
			pstRvStats->Resolver.un32NumOfUsedElements = stResolver.resolvers.currNumOfUsedElements;
			pstRvStats->Resolver.un32MaxUsageOfElements = stResolver.resolvers.maxUsageOfElements;
		}

// 		// Get the status of the Security-Agreement module
// 		RvSipSecAgreeResources stSecAgree;
// 		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_SEC_AGREE, &stSecAgree))
// 		{
// 			sprintf (szLogMessage + strlen (szLogMessage), "  secAgrees          %8u%9u%5u\n", stSecAgree.secAgrees.numOfAllocatedElements, stSecAgree.secAgrees.currNumOfUsedElements, stSecAgree.secAgrees.maxUsageOfElements);
// 			sprintf (szLogMessage + strlen (szLogMessage), "  ownersHash         %8u%9u%5u\n", stSecAgree.ownersHash.numOfAllocatedElements, stSecAgree.ownersHash.currNumOfUsedElements, stSecAgree.ownersHash.maxUsageOfElements);
// 			sprintf (szLogMessage + strlen (szLogMessage), "  ownersLists        %8u%9u%5u\n", stSecAgree.ownersLists.numOfAllocatedElements, stSecAgree.ownersLists.currNumOfUsedElements, stSecAgree.ownersLists.maxUsageOfElements);
// 			sprintf (szLogMessage + strlen (szLogMessage), "  ownersHandles      %8u%9u%5u\n\n", stSecAgree.ownersHandles.numOfAllocatedElements, stSecAgree.ownersHandles.currNumOfUsedElements, stSecAgree.ownersHandles.maxUsageOfElements);
// 		}
//
// 		// Get the status of the Security module
// 		RvSipSecurityResources stSecurity;
// 		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_SECURITY, &stSubscription))
// 		{
// 			sprintf (szLogMessage + strlen (szLogMessage), "  secobjPoolElements %8u%9u%5u\n", stSecurity.secobjPoolElements.numOfAllocatedElements, stSecurity.secobjPoolElements.currNumOfUsedElements, stSecurity.secobjPoolElements.maxUsageOfElements);
// 			sprintf (szLogMessage + strlen (szLogMessage), "  ipsecSessPoolElmnts%8u%9u%5u\n\n", stSecurity.ipsecsessionPoolElements.numOfAllocatedElements, stSecurity.ipsecsessionPoolElements.currNumOfUsedElements, stSecurity.ipsecsessionPoolElements.maxUsageOfElements);
// 		}

		// Get the status of the Stack Manager module
		RvSipStackResources stStack;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_STACK, &stStack))
		{
			pstRvStats->StkMgrMsgPoolEl.un32NumOfAllocatedElements = stStack.msgPoolElements.numOfAllocatedElements;
			pstRvStats->StkMgrMsgPoolEl.un32NumOfUsedElements = stStack.msgPoolElements.currNumOfUsedElements;
			pstRvStats->StkMgrMsgPoolEl.un32MaxUsageOfElements = stStack.msgPoolElements.maxUsageOfElements;
			pstRvStats->StkMgrGenPoolEl.un32NumOfAllocatedElements = stStack.generalPoolElements.numOfAllocatedElements;
			pstRvStats->StkMgrGenPoolEl.un32NumOfUsedElements = stStack.generalPoolElements.currNumOfUsedElements;
			pstRvStats->StkMgrGenPoolEl.un32MaxUsageOfElements = stStack.generalPoolElements.maxUsageOfElements;
			pstRvStats->StkMgrHdrPoolEl.un32NumOfAllocatedElements = stStack.headerPoolElements.numOfAllocatedElements;
			pstRvStats->StkMgrHdrPoolEl.un32NumOfUsedElements = stStack.headerPoolElements.currNumOfUsedElements;
			pstRvStats->StkMgrHdrPoolEl.un32MaxUsageOfElements = stStack.headerPoolElements.maxUsageOfElements;
			pstRvStats->StkMgrTmrPool.un32NumOfAllocatedElements = stStack.timerPool.numOfAllocatedElements;
			pstRvStats->StkMgrTmrPool.un32NumOfUsedElements = stStack.timerPool.currNumOfUsedElements;
			pstRvStats->StkMgrTmrPool.un32MaxUsageOfElements = stStack.timerPool.maxUsageOfElements;
		}

		// Get the status of the Subscription module
		RvSipSubsResources stSubscription;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_SUBSCRIPTION, &stSubscription))
		{
			pstRvStats->SubsSubscriptns.un32NumOfAllocatedElements = stSubscription.subscriptions.numOfAllocatedElements;
			pstRvStats->SubsSubscriptns.un32NumOfUsedElements = stSubscription.subscriptions.currNumOfUsedElements;
			pstRvStats->SubsSubscriptns.un32MaxUsageOfElements = stSubscription.subscriptions.maxUsageOfElements;
			pstRvStats->SubsNotificatns.un32NumOfAllocatedElements = stSubscription.notifications.numOfAllocatedElements;
			pstRvStats->SubsNotificatns.un32NumOfUsedElements = stSubscription.notifications.currNumOfUsedElements;
			pstRvStats->SubsNotificatns.un32MaxUsageOfElements = stSubscription.notifications.maxUsageOfElements;
			pstRvStats->SubsNotifyLists.un32NumOfAllocatedElements = stSubscription.notifyLists.numOfAllocatedElements;
			pstRvStats->SubsNotifyLists.un32NumOfUsedElements = stSubscription.notifyLists.currNumOfUsedElements;
			pstRvStats->SubsNotifyLists.un32MaxUsageOfElements = stSubscription.notifyLists.maxUsageOfElements;
		}

		// Get the status of the Transaction module
		RvSipTranscResources stTrans;		// pg 1521
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_TRANSACTION, &stTrans))
		{
			pstRvStats->Transaction.un32NumOfAllocatedElements = stTrans.transactions.numOfAllocatedElements;
			pstRvStats->Transaction.un32NumOfUsedElements = stTrans.transactions.currNumOfUsedElements;
			pstRvStats->Transaction.un32MaxUsageOfElements = stTrans.transactions.maxUsageOfElements;
		}

		// Get the status of the Transmitter module
		RvSipTransmitterResources stTransmitter;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_TRANSMITTER, &stTransmitter))
		{
			pstRvStats->Transmitter.un32NumOfAllocatedElements = stTransmitter.transmitters.numOfAllocatedElements;
			pstRvStats->Transmitter.un32NumOfUsedElements = stTransmitter.transmitters.currNumOfUsedElements;
			pstRvStats->Transmitter.un32MaxUsageOfElements = stTransmitter.transmitters.maxUsageOfElements;
		}

		// Get the status of the Transport module
		RvSipTransportResources stTransport;
		if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_TRANSPORT, &stTransport))
		{
			pstRvStats->TrnsprtCon.un32NumOfAllocatedElements = stTransport.connections.numOfAllocatedElements;
			pstRvStats->TrnsprtCon.un32NumOfUsedElements = stTransport.connections.currNumOfUsedElements;
			pstRvStats->TrnsprtCon.un32MaxUsageOfElements = stTransport.connections.maxUsageOfElements;
			pstRvStats->TrnsprtConNoOwn.un32NumOfAllocatedElements = stTransport.connectionsNoOwners.numOfAllocatedElements;
			pstRvStats->TrnsprtConNoOwn.un32NumOfUsedElements = stTransport.connectionsNoOwners.currNumOfUsedElements;
			pstRvStats->TrnsprtConNoOwn.un32MaxUsageOfElements = stTransport.connectionsNoOwners.maxUsageOfElements;
			pstRvStats->TrnsprtpQueEl.un32NumOfAllocatedElements = stTransport.pQueueElements.numOfAllocatedElements;
			pstRvStats->TrnsprtpQueEl.un32NumOfUsedElements = stTransport.pQueueElements.currNumOfUsedElements;
			pstRvStats->TrnsprtpQueEl.un32MaxUsageOfElements = stTransport.pQueueElements.maxUsageOfElements;
			pstRvStats->TrnsprtpQueEvnt.un32NumOfAllocatedElements = stTransport.pQueueEvents.numOfAllocatedElements;
			pstRvStats->TrnsprtpQueEvnt.un32NumOfUsedElements = stTransport.pQueueEvents.currNumOfUsedElements;
			pstRvStats->TrnsprtpQueEvnt.un32MaxUsageOfElements = stTransport.pQueueEvents.maxUsageOfElements;
			pstRvStats->TrnsprtRdBuff.un32NumOfAllocatedElements = stTransport.readBuffers.numOfAllocatedElements;
			pstRvStats->TrnsprtRdBuff.un32NumOfUsedElements = stTransport.readBuffers.currNumOfUsedElements;
			pstRvStats->TrnsprtRdBuff.un32MaxUsageOfElements = stTransport.readBuffers.maxUsageOfElements;
			pstRvStats->TrnsprtConHash.un32NumOfAllocatedElements = stTransport.connHash.numOfAllocatedElements;
			pstRvStats->TrnsprtConHash.un32NumOfUsedElements = stTransport.connHash.currNumOfUsedElements;
			pstRvStats->TrnsprtConHash.un32MaxUsageOfElements = stTransport.connHash.maxUsageOfElements;
			pstRvStats->TrnsprtOwnHash.un32NumOfAllocatedElements = stTransport.ownersHash.numOfAllocatedElements;
			pstRvStats->TrnsprtOwnHash.un32NumOfUsedElements = stTransport.ownersHash.currNumOfUsedElements;
			pstRvStats->TrnsprtOwnHash.un32MaxUsageOfElements = stTransport.ownersHash.maxUsageOfElements;
			pstRvStats->TrnsprtOOResEv.un32NumOfAllocatedElements = stTransport.oorEvents.numOfAllocatedElements;
			pstRvStats->TrnsprtOOResEv.un32NumOfUsedElements = stTransport.oorEvents.currNumOfUsedElements;
			pstRvStats->TrnsprtOOResEv.un32MaxUsageOfElements = stTransport.oorEvents.maxUsageOfElements;
		}

#ifdef RV_SIP_STATISTICS_ENABLED
		// Get the Stack Statistics
		RvSipStackStatistics stStats;
		if (RV_OK == RvSipStackGetStatistics (m_hStackMgr, &stStats))
		{
			pstRvStats->un32RcvdInvite = stStats.rcvdINVITE;
			pstRvStats->un32RcvdInviteRetrans = stStats.rcvdINVITERetrans;
			pstRvStats->un32RcvdNonInviteReq = stStats.rcvdNonInviteReq;
			pstRvStats->un32RcvdNonInviteReqRetrans = stStats.rcvdNonInviteReqRetrans;
			pstRvStats->un32RcvdResponse = stStats.rcvdResponse;
			pstRvStats->un32RcvdResponseRetrans = stStats.rcvdResponseRetrans;
			pstRvStats->un32SentInvite = stStats.sentINVITE;
			pstRvStats->un32SentInviteRetrans = stStats.sentINVITERetrans;
			pstRvStats->un32SentNonInviteReq = stStats.sentNonInviteReq;
			pstRvStats->un32SentNonInviteReqRetrans = stStats.sentNonInviteReqRetrans;
			pstRvStats->un32SentResponse = stStats.sentResponse;
			pstRvStats->un32SentResponseRetrans = stStats.sentResponseRetrans;
		}
#endif
	}

	// Get the mid layer resources that are in use
	RvSipMidResources stMid;
	if (m_hMidMgr && RV_OK == RvSipMidGetResources (m_hMidMgr, &stMid))
	{
		pstRvStats->MidLayUserFds.un32NumOfAllocatedElements = stMid.userFds.numOfAllocatedElements;
		pstRvStats->MidLayUserFds.un32NumOfUsedElements = stMid.userFds.currNumOfUsedElements;
		pstRvStats->MidLayUserFds.un32MaxUsageOfElements = stMid.userFds.maxUsageOfElements;
		pstRvStats->MidLayUserTimrs.un32NumOfAllocatedElements = stMid.userTimers.numOfAllocatedElements;
		pstRvStats->MidLayUserTimrs.un32NumOfUsedElements = stMid.userTimers.currNumOfUsedElements;
		pstRvStats->MidLayUserTimrs.un32MaxUsageOfElements = stMid.userTimers.maxUsageOfElements;
	}

	if (m_hAppPool)
	{
		RvUint32 un32NumOfAllocatedElements;
		RvUint32 un32NumOfUsedElements;
		RvUint32 un32MaxUsageOfElements;

		// Get the RPOOL resources
		RPOOL_GetResources (m_hAppPool,
								&un32NumOfAllocatedElements,
								&un32NumOfUsedElements,
								&un32MaxUsageOfElements);

		pstRvStats->RPool.un32NumOfAllocatedElements = un32NumOfAllocatedElements;
		pstRvStats->RPool.un32NumOfUsedElements = un32NumOfUsedElements;
		pstRvStats->RPool.un32MaxUsageOfElements = un32MaxUsageOfElements;
	}

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::StackRestart
//
// Description: Post StackRestart event
//
// Returns:
//	none
//
void CstiSipManager::StackRestart()
{
	PostEvent(
		std::bind(&CstiSipManager::eventStackRestart, this));
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::eventStackRestart
//
// Description: Restarts the SIP stack
//
// Abstract: If we have no calls this will immediately call eventStackStartup,
// if there are calls it sets m_bRestartStackPending to true so eventCallObjectRemoved
// will call eventStackStartup.
//
// Returns:
//	none
//
void CstiSipManager::eventStackRestart ()
{
	// We need to restart the stack but only if we have no SIP calls
	if (m_pCallStorage->activeCallCountGet() == 0)
	{
		eventStackStartup ();
		
		m_bRestartStackPending = false;
	}
	else
	{
		// Store the need to run this event when we no longer have SIP calls in the system
		m_bRestartStackPending = true;
		std::string eventMessage = "EventType=SipStackRestart Reason=Queued pending call completion";
		stiRemoteLogEventSend (&eventMessage);
	}
}


/*!
 * \brief Event handler to check whether our stack resources are OK
 */
void CstiSipManager::eventStackResourcesCheck ()
{
	bool bRestartTimer = true;

	// Check to see if we are in trouble with our Transport Connection resources and need to restart the stack

	// Get the status of the Transport module
	RvSipTransportResources stTransport;
	if (RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_TRANSPORT, &stTransport))
	{
		if (stTransport.connections.currNumOfUsedElements >= stTransport.connections.numOfAllocatedElements)
		{
			// Don't restart the timer since the eventStackStartup will have done so.
			bRestartTimer = false;

			// We need to restart the stack but only if we have no SIP calls
			std::string eventMessage = "EventType=SipStackRestart Reason=SipTransportConnectionsOutOfResources";
			stiRemoteLogEventSend (&eventMessage);
		}
	}
	
	RvSipStackResources stStack;
	if (bRestartTimer && RV_OK == RvSipStackGetResources (m_hStackMgr, RVSIP_STACK, &stStack))
	{
		 if (stStack.generalPoolElements.currNumOfUsedElements >= stStack.generalPoolElements.numOfAllocatedElements)
		 {
			 // Don't restart the timer since the eventStackStartup will have done so.
			 bRestartTimer = false;

			// We need to restart the stack but only if we have no SIP calls
			std::string eventMessage = "EventType=SipStackRestart Reason=SipGeneralPoolOutOfResources";
			stiRemoteLogEventSend (&eventMessage);
		 }
	}

	// Do we need to restart the timer?
	if (bRestartTimer)
	{
		m_resourceCheckTimer.Start ();
	}
	else
	{
		StackRestart ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallObjectRemoved
//
// Description: Inform the CM task that a call object has been removed.
//
// Abstract:
//	This method simply posts a message informing that a call object has been
//	removed from the call storage.
//	object.
//
// Returns:
//	none
//
void CstiSipManager::CallObjectRemoved ()
{
	PostEvent (
	    std::bind(&CstiSipManager::eventCallObjectRemoved, this));
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::CallObjectRemovedHandle
//
// Description: Handle changes that needed to wait for a call to complete.
//
// Abstract:
//
// Returns:
//	stiRESULT_SUCCESS upon success.  stiRESULT_ERROR ORed with a result code otherwise.
//
void CstiSipManager::eventCallObjectRemoved ()
{
	// Check to see if we are to restart the stack ...
	if (m_bRestartStackPending && 0 == m_pCallStorage->activeCallCountGet())
	{
		eventStackStartup ();

		m_bRestartStackPending = false;
	}
	// Check to see if we are changing networks
	else if (m_networkChangePending && 0 == m_pCallStorage->activeCallCountGet())
	{
		m_networkChangePending = false;
		NetworkChangeNotify();
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::LocalShareContactSupportedGet
//
// Description: Determine if sending contacts is allowed on our end
//
// Returns:
//	true or false
//
bool CstiSipManager::LocalShareContactSupportedGet ()
{
	bool bAllowed = false;
	
#ifdef stiALLOW_SHARE_CONTACT
	switch (m_eLocalInterfaceMode)
	{
		case estiSTANDARD_MODE:
		case estiHEARING_MODE:
			bAllowed = true;
			break;
		
		case estiINTERPRETER_MODE:
		case estiPUBLIC_MODE:
		case estiKIOSK_MODE:
		case estiTECH_SUPPORT_MODE:
		case estiVRI_MODE:
		case estiABUSIVE_CALLER_MODE:
		case estiPORTED_MODE:
		case estiINTERFACE_MODE_ZZZZ_NO_MORE:
			bAllowed = false;
			break;
			
		// default: commented out for type safety
	};
#endif // stiALLOW_SHARE_CONTACT
	
	return bAllowed;
}


////////////////////////////////////////////////////////////////////////////////
//; SIPHeaderValueGet
//
// Description:  Returns the sip Header value
//
// Abstract:
//  This method will search for the header in the message and fetch the header value
//  if the header is present in the messsage.
//
// Returns:
//  stiRESULT_SUCCESS when successful stiRESULT_ERROR otherwise
//
/*!
 * \brief Checks for the presence of a private SVRS geolocation header and emits
 * 		  remoteGeolocationChangedSignal if present.
 *
 * \param inboundMsg The SIP message to be checked
 *
 * \return true if a private SVRS geolocation header is present, false otherwise
 */
bool CstiSipManager::privateGeolocationHeaderCheck (const vpe::sip::Message &inboundMsg, CstiCallSP poCall)
{
	bool headerPresent = false;
	std::string geolocation;

	std::tie(geolocation, headerPresent) = inboundMsg.headerValueGet(g_szSVRSGeolocation);

	if (!geolocation.empty ())
	{
		headerPresent = true;

		remoteGeolocationChangedSignal.Emit (geolocation);

		poCall->SIPGeolocationSet (geolocation);

		geolocationUpdateSignal.Emit (poCall);
	}

	return headerPresent;
}


/*! \brief add ability to modify default ice session settings on a per-call basis
 */
CstiIceSession::Protocol CstiSipManager::IceProtocolGet (
	const CstiSipCallLegSP &callLeg)
{
	// Get the defaults
	CstiIceSession::Protocol protocol = CstiIceSession::Protocol::None;

	if (m_iceManager)
	{
		protocol = m_iceManager->DefaultProtocolGet ();
	}

	if (callLeg->IsBridgedCall () && CstiIceSession::Protocol::Turn == protocol)
	{
		protocol = CstiIceSession::Protocol::Stun;
	}

	return protocol;
}


stiHResult CstiSipManager::IceSessionStart (
	CstiIceSession::EstiIceRole eRole,
	const CstiSdp &Sdp,
	IceSupport *iceSupport,
    CstiSipCallLegSP callLeg)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto call = callLeg->m_sipCallWeak.lock ()->CstiCallGet ();
	auto startingIceSupport = iceSupport ? (int64_t)*iceSupport : (int64_t)IceSupport::Unspecified;
	auto resultingIceSupport = (int64_t)IceSupport::Unspecified;

	stiTESTCOND (m_iceManager, stiRESULT_ERROR);

	hResult = m_iceManager->SessionStart (eRole, Sdp, iceSupport, callLeg->uniqueKeyGet(),
		m_stConferenceParams.PublicIPv4, m_stConferenceParams.PublicIPv6,
		IceProtocolGet (callLeg),
		&callLeg->m_pIceSession);

STI_BAIL:

	resultingIceSupport = iceSupport ? (int64_t)*iceSupport : resultingIceSupport;
	call->collectTrace (formattedString ("%s: result %d, role %d, iceSupport %d -> %d", __func__, hResult, eRole, startingIceSupport, resultingIceSupport));

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::SIPResolverIPv4DNSServersSet
//
// Description: Sets only IPv4 DNS servers to be used by Radvision SIP Resolver.
// We are seeing a problem with IPv6 DNS servers causing performance issues when attempting
// to resolve a domain name that only maps to IPv4. So if a network has both IPv4 and IPv6
// DNS servers we are choosing to only attempt to use the IPv4 DNS servers.
//
// Abstract:
//  This function will get a list of currently available DNS servers knwon to Radvision.
// If there is at least one IPv4 server it will set a new list of only IPv4 DNS servers.
//
// Returns:
//  Nothing.
//
stiHResult CstiSipManager::SIPResolverIPv4DNSServersSet ()
{
	RvInt32 nDNSServers = stiMAX_DNS_SERVERS; // This is the max number of servers we want Radvision to return.
	std::unique_ptr<RvSipTransportAddr[]> DNSAddr (new RvSipTransportAddr[stiMAX_DNS_SERVERS]);
	std::unique_ptr<RvSipTransportAddr[]> IPv4DNSServers (new RvSipTransportAddr[stiMAX_DNS_SERVERS]);
	int nNumIPv4Servers = 0;
	RvStatus rvStatus = RV_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bIPv6DNSServerFound = false;
	
	// After calling RvSipResolverMgrGetDnsServers nDNSServers is now the number of servers Radvision found.
	rvStatus = RvSipResolverMgrGetDnsServers (m_hResolverMgr, DNSAddr.get (), &nDNSServers);
	stiTESTRVSTATUS();
	
	for (int i = 0; i < stiMAX_DNS_SERVERS && i < nDNSServers; i++)
	{
		if (DNSAddr[i].eAddrType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP)
		{
			IPv4DNSServers[nNumIPv4Servers] = DNSAddr[i];
			nNumIPv4Servers++;
		}
		else if (DNSAddr[i].eAddrType == RVSIP_TRANSPORT_ADDRESS_TYPE_IP6)
		{
			bIPv6DNSServerFound = true;
		}
	}
	
	if (bIPv6DNSServerFound && nNumIPv4Servers)
	{
		rvStatus = RvSipResolverMgrSetDnsServers (m_hResolverMgr, IPv4DNSServers.get (), nNumIPv4Servers);
		stiTESTRVSTATUS();
	}
	
STI_BAIL:
	
	return hResult;
}


bool CstiSipManager::AvailableGet ()
{
	return m_isAvailable;
}


void CstiSipManager::AvailableSet (bool isAvailable)
{
	m_isAvailable = isAvailable;
}


/*!\brief Posts event for SipManager thread to us VRS failover route.
 *
 * Called when the vrsFailover timer fires from CstiCall. This ensures that the
 * disconnect is handled by the SipManager thread and not the timer thread.
 *
 *  \return Succes or Failure.
 */
stiHResult CstiSipManager::VRSFailoverCallDisconnect (
    CstiCallSP call)
{
	// Run it asynchronously on the event loop
	PostEvent ([call]{
		CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		sipCall->VRSFailOverForce ();
	});
	
	return stiRESULT_SUCCESS;
}


/*!
 * \brief If in the connecting state, the call is hung up and skips directly
 *        to recording a SignMail
 */
stiHResult CstiSipManager::SignMailSkipToRecord (
    CstiCallSP call)
{
	PostEvent (
	    std::bind(&CstiSipManager::eventSignMailSkipToRecord, this, call));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Hangs up the current call and begins recording a SignMail
 */
void CstiSipManager::eventSignMailSkipToRecord (
    CstiCallSP call)
{
	stiLOG_ENTRY_NAME (CstiSipManager::eventSignMailSkipToRecord);

	// If the call is connecting then set the result and hangup the call.
	if (call->StateGet() == esmiCS_CONNECTING)
	{
		CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		call->ResultSet(estiRESULT_HANGUP_AND_LEAVE_MESSAGE);
		sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NONE);
	}

	// If the call is disconnected with a LeaveMessage substate then
	// notify CCI that we need to SkipToRecord.
	else if ((call->StateGet() == esmiCS_DISCONNECTED && 
			 call->SubstateGet() == estiSUBSTATE_LEAVE_MESSAGE))
	{
		signMailSkipToRecordSignal.Emit ();
	}
}


/*!
 * \brief Given a call leg key, return a shared_ptr to the CstiCallLeg instance
 *        or nullptr if in doesn't exist, or was already destroyed
 * \note  This is a static method
 */
CstiSipCallLegSP CstiSipManager::callLegLookup (
    size_t callLegKey)
{
	std::lock_guard<std::recursive_mutex> lock (m_callLegMapMutex);

	// If it's not in the map, return nullptr
	if (m_callLegMap.count(callLegKey) == 0)
	{
		return nullptr;
	}

	return m_callLegMap[callLegKey].lock ();
}


/*!
 * \brief Inserts a call leg into the call leg map, overwriting any conflicts
 * \note This is as static method
 */
void CstiSipManager::callLegInsert (
    size_t callLegKey,
    CstiSipCallLegSP callLeg)
{
	std::lock_guard<std::recursive_mutex> lock (m_callLegMapMutex);
	callLeg->uniqueKeySet (callLegKey);
	m_callLegMap[callLegKey] = callLeg;
}


/*!
 * \brief Checks if we are using tunneling with an active tunnel.
 *
 *\return Returns true if we are configured to use tunneling and have a tunnel address.
 *	Otherwise returns False.
 */
bool CstiSipManager::TunnelActive ()
{
	return (m_stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling &&
			!m_stConferenceParams.TunneledIpAddr.empty ());
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::localAlternateNameGet
//
// Description: Returns the local alernate name.
//
// Abstract:
//  This method causes the local alternate name to be returned in the
//  supplied std::string
//
void CstiSipManager::localAlternateNameGet (std::string *alternateName) const
{
	std::lock_guard<std::recursive_mutex> lk(m_vrsInfoMutex);
	*alternateName = m_alternateName;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::localAlternateNameSet
//
// Description: Change the Alternate Name that is transmitted to the remote endpoint
//
// Abstract:
//  This method causes the Local Alternate Name (the user's name) to be updated in the
//  SInfo message that is transmitted to the remote endpoint during call setup.
//
// Returns:
//  estiOK (Success) estiERROR (Failure)estiRAS_NOT_REGISTERED
//
stiHResult CstiSipManager::localAlternateNameSet (
	const std::string &alternateName)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::localAlternateNameSet);

	std::lock_guard<std::recursive_mutex> lk(m_vrsInfoMutex);
	m_alternateName = alternateName;

	auto connectedCall = m_pCallStorage->get(esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH);
	if (connectedCall)
	{
		PostEvent ([this, connectedCall]{

				CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(connectedCall->ProtocolCallGet());

				auto call = sipCall->CstiCallGet ();
				if (call)
				{
					// Update the data on the call object used by SysetmInfoSerialize
					call->localAlternateNameSet(m_alternateName);

					updateVrsCallInfo(sipCall);
				}
		});
	}

	return stiRESULT_SUCCESS;
} // end CstiSipManager::localAlternateNameSet


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::vrsCallIdGet
//
// Description: Returns the VRS Call ID.
//
// Abstract:
//  This method causes the VRS Call ID to be returned in the
//  supplied std::string
//
void CstiSipManager::vrsCallIdGet (std::string *callId) const
{
	std::lock_guard<std::recursive_mutex> lk(m_vrsInfoMutex);
	*callId = m_vrsCallId;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::vrsCallIdSet
//
// Description: Sets the VRS Call ID.
//
// Abstract:
//  This method causes the VRS Call ID to be updated in the
//  SInfo message that is transmitted to the remote endpoint during call setup.
//
// Returns:
//  stiRESULT_SUCCESS (Success)
//
stiHResult CstiSipManager::vrsCallIdSet (
	const std::string &callId)
{
	stiLOG_ENTRY_NAME (CstiProtocolManager::vrsCallIdSet);

	std::lock_guard<std::recursive_mutex> lk(m_vrsInfoMutex);
	m_vrsCallId = callId;

	auto connectedCall = m_pCallStorage->get(esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH);
	if (connectedCall)
	{
		PostEvent ([this, connectedCall]{

				CstiSipCallSP sipCall = std::static_pointer_cast<CstiSipCall>(connectedCall->ProtocolCallGet());

				auto call = sipCall->CstiCallGet ();
				if (call)
				{
					// Update the data on the call object used by SysetmInfoSerialize
					call->vrsCallIdSet(m_vrsCallId);

					updateVrsCallInfo(sipCall);
				}
		});
	}

	return stiRESULT_SUCCESS;
} // end CstiSipManager::vrsCallIdSet


////////////////////////////////////////////////////////////////////////////////
//; CstiSipManager::updateVrsCallIdSet
//
// Description: Sets the VRS Call ID.
//
// Abstract:
//  This method causes the VRS Call ID to be updated in the
//  SInfo message that is transmitted to the remote endpoint during call setup.
//
// Returns:
//  stiRESULT_SUCCESS (Success)
//
void CstiSipManager::updateVrsCallInfo(
	CstiSipCallSP sipCall)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string sInfo;

	hResult = SInfoGenerate(&sInfo, sipCall);
	stiTESTRESULT();

	hResult = sipCall->SipInfoSend(SINFO_MIME, INFO_PACKAGE, S_INFO_PACKAGE, sInfo);
	stiTESTRESULT();

STI_BAIL:

	return;
}

// end file CstiSipManager.cpp
