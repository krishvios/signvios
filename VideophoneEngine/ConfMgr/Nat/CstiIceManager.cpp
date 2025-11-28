////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiIceManager
//
//  File Name:	CstiIceManager.cpp
//
//	Abstract:
//		The Ice Manager is responsible for interfacing with the Radvision ICE
//		engine (the stack).  It is used to manage NAT traversal using the
//		ICE standard.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiIceManager.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "stiOS.h" // OS Abstraction definitions
#include "CstiHostNameResolver.h"
#include "CstiIpAddress.h"
#include "RadvisionLogging.h"

// ICE stack
#include "RvIceSession.h"
#include "rvnetaddress.h"
#include "rtp.h"
#include "rtcp.h"
#include <memory>


//
// Constants
//
#define STANDARD_STUN_PORT 3478

//
// Forward Declarations
//

//
// 	This next bit is a neat trick to get it to print the file reference (Do not "optimize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiIceMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )
//#define DBG_MSG(fmt,...) stiDEBUG_TOOL (1, stiTrace (fmt "\n", ##__VA_ARGS__); )

/**
 *  \brief create and set up a STUN/TURN server to use
 *
 *  \return stiHResult
 */
static stiHResult ServerSet (
	const char *pszAddress,
	uint16_t un16Port,
	const boost::optional<std::string> &username,
	const boost::optional<std::string> &password,
	RvIceStunServerCfg *pRvServerCfg)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	memset (pRvServerCfg, 0, sizeof (*pRvServerCfg));

	pRvServerCfg->bDontSendSrvDnsRequest = RV_TRUE; // Our Sorenson DNS lookup should have handled this
	RvAddressConstruct (IPv4AddressValidate (pszAddress) ? RV_ADDRESS_TYPE_IPV4 : RV_ADDRESS_TYPE_IPV6, &pRvServerCfg->stunServerAddress);
	RvAddressSetString (pszAddress, &pRvServerCfg->stunServerAddress);
	RvAddressSetIpPort (&pRvServerCfg->stunServerAddress, un16Port);

	//
	// Copy the username and password
	//
	if (username)
	{
		pRvServerCfg->usernameLen = username->size ();

		if (pRvServerCfg->usernameLen > 0)
		{
			pRvServerCfg->username = new char [pRvServerCfg->usernameLen + 1];
			strcpy ((char *)pRvServerCfg->username, username->c_str ());

			//
			// Copy the password if there is one.
			//
			if (password)
			{
				pRvServerCfg->passwordLen = password->size ();

				if (pRvServerCfg->passwordLen > 0)
				{
					pRvServerCfg->password = new char [pRvServerCfg->passwordLen + 1];
					strcpy ((char *)pRvServerCfg->password, password->c_str ());
				}
			}
		}
	}

	return hResult;
}


/*! \brief Set the STUN and/or TURN servers for ICE to use
 *
 * Allow setting default cadidate settings, that can be overridden on
 * a per-session basis
 *
 * \return stiHResult
 */
stiHResult CstiIceManager::ServersSet (
	const boost::optional<CstiServerInfo> &server,
	const boost::optional<CstiServerInfo> &serverAlt,
	CstiIceSession::Protocol protocol)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_defaultProtocol = protocol;

	if (m_server)
	{
		m_server.reset ();
	}

	if (server)
	{
		m_server = server;
	}

	if (m_serverAlt)
	{
		m_serverAlt.reset ();
	}

	if (serverAlt)
	{
		m_serverAlt = serverAlt;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::IceStackInitialize
//
// Description: Initializes the Radvision ICE stack.
//
// Returns:
//  stiHResult
//
stiHResult CstiIceManager::IceStackInitialize (int maxIceSessions)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	// Fill out the configuration structure
	RvIceStackCfg iceCfg;
	RvIceGetDefaultStackCfg (&iceCfg);
	// All the sessions are allocated at startup so watch out for over allocation
	iceCfg.maxSessions = maxIceSessions;
	iceCfg.maxMediaStreams = 3 * maxIceSessions; // One for each of the three media streams (Video, Audio, Text)
	/*iceCfg.maxPairsPerSession = 60; // This is from the pdf sample*/
	iceCfg.hStun = nullptr;
#ifdef VPE_RV_LOGGING
	iceCfg.pLogManager = m_logManager;
	iceCfg.iceLogFilters = vpe::RadvisionLogging::iceLogFilterGet();
	iceCfg.stunLogFilters = vpe::RadvisionLogging::iceLogFilterGet ();
	iceCfg.turnLogFilters = vpe::RadvisionLogging::iceLogFilterGet ();
	iceCfg.commonLogFilters = vpe::RadvisionLogging::iceLogFilterGet ();
#endif
#if 1 // Must choose one of these modes of operation.
	iceCfg.iceSessionCfg.bCreateTransport = RV_TRUE;
#else
	iceCfg.pfnCandAddrInit = AppIceAddressInitEv;
	iceCfg.pfnCandAddrClose = AppIceAddressCloseEv;
	iceCfg.pfnCandAddrSendBuff = AppIceAddressSendBufferEv;
#endif
	
	iceCfg.iceSessionCfg.bIsLiteMode = m_useLiteMode ? RV_TRUE : RV_FALSE;

	// The below settings allow us to use an even port for RTP and reserver the next port for RTCP
	iceCfg.iceSessionCfg.bTurnRtpEvenPort = RV_TRUE;
	iceCfg.iceSessionCfg.bTurnRtcpOddPort = RV_TRUE;

	// Fill out the default session configuration
//	iceCfg.iceSessionCfg.eNominatePolicy = RV_ICE_NOMINATE_AT_END; // Only notify us when all results are in.
	
	//
	// This setting needs to be set to TRUE to accommodate the cases where
	// the videophone is not behind a NAT but the firewall only trusts outbound
	// traffic to certain entities (one of those entities will have to be the TURN
	// server).  If this is not set to true the TURN is removed from the list when
	// the endpoint is not behind a NAT.
	//
	iceCfg.iceSessionCfg.bCreateNonNATRelayedCandidates = RV_TRUE;
	
	iceCfg.iceSessionCfg.pfnSessionCandsGatherCompleted = IceSessionCandidatesGatheringCompletedCB;
	iceCfg.iceSessionCfg.pfnSessionCompleted = IceSessionCompleteCB;
	iceCfg.iceSessionCfg.pfnSessionCandOnGatheringEvent = TurnAllocationCB;

	iceCfg.iceSessionCfg.bCreateTcpActive = RV_FALSE;
	iceCfg.iceSessionCfg.bCreateTcpPassive = RV_FALSE;
	iceCfg.iceSessionCfg.bCreateTcpRelayed = RV_FALSE;
	iceCfg.iceSessionCfg.bTcpActiveUsePortRange = RV_FALSE;
	iceCfg.iceSessionCfg.bTcpRelayedUsePortRange = RV_FALSE;
	// Setting 10 second timeout to fail ICE connection checks. Tried some lower values and those did not work.
	iceCfg.iceSessionCfg.connChecksTimeout1 = 10000;
	
	
	iceCfg.iceSessionCfg.bMultiplexRtcp = RV_TRUE;
	iceCfg.iceSessionCfg.trickleIceSupport = RV_ICE_TRICKLE_HALF;

	// Attempt to start it.
	rvStatus = RvIceStackConstruct (&iceCfg, &m_hStackHandle);
	stiTESTRVSTATUS ();

	RvIceStackSetAppHandle (m_hStackHandle, (RvIceAppStackHandle)this);

STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::IceStackUninitialize
//
// Description: Uninitializes the Radvision ICE stack.
//
// Returns:
//  stiHResult
//
stiHResult CstiIceManager::IceStackUninitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_hStackHandle)
	{
		stiDEBUG_TOOL (g_stiIceMgrDebug,
			stiTrace ("Unintializing Ice Manager stack\n");
		);
		
		RvIceStackSetAppHandle (m_hStackHandle, (RvIceAppStackHandle)nullptr);
		RvIceStackDestruct (m_hStackHandle);
		m_hStackHandle = nullptr;
	}
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::SessionStart
//
// Description: Start the ICE session.
//
// Returns:
//  stiHResult
//
stiHResult CstiIceManager::SessionStart (
	CstiIceSession::EstiIceRole eRole,
	const CstiSdp &Sdp,
	IceSupport *iceSupport,
	size_t CallbackParam,
	const std::string &PublicIPv4,
	const std::string &PublicIPv6,
	CstiIceSession::Protocol protocol,
	CstiIceSession **ppIceSession)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiIceSession *pIceSession = nullptr;
	auto iceSupportLocal = IceSupport::Unknown;

	hResult = AccountCreateSession (eRole, CallbackParam, &pIceSession);
	stiTESTRESULT ();

	hResult = AccountStartSession (pIceSession, Sdp, &iceSupportLocal, PublicIPv4, PublicIPv6, protocol);
	stiTESTRESULT ();

	// We declare success if an error was not returned, the role was not Answerer or if ICE was supported
	// by the SDP. If the role was Answerer and there was not ICE support found
	// in the SDP then shutdown the ICE session.
	if (pIceSession->m_eRole != CstiIceSession::estiICERoleAnswerer
	 || iceSupportLocal == IceSupport::Supported)
	{
		if (m_rejectHostOnlyRequests &&
			pIceSession->AllRemoteCandidatesArePrivateHost ())
		{
			stiTHROWMSG (stiRESULT_INVALID_MEDIA, "All ICE candidates were private host candidates. Rejecting call.");
		}
		*ppIceSession = pIceSession;
	}
	else
	{
		pIceSession->SessionEnd ();
		pIceSession = nullptr;
	}

	if (iceSupport)
	{
		*iceSupport = iceSupportLocal;
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (pIceSession)
		{
			pIceSession->SessionEnd ();
			pIceSession = nullptr;
		}
	}

	return hResult; // Returning any errors will stop the ice manager completely.
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::AccountCreateSession
//
// Description: Create a new ICE session.
//
// Returns:
//  stiHResult
//
stiHResult CstiIceManager::AccountCreateSession (
	CstiIceSession::EstiIceRole eRole,
	size_t CallbackParam,
	CstiIceSession **ppIceSession)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	DBG_MSG ("%s - Creating ICE session", IceRoleNameGet (eRole));

	auto pIceSession = new CstiIceSession;

	pIceSession->m_pIceManager = this;
	pIceSession->m_CallbackParam = CallbackParam;
	pIceSession->m_eRole = eRole;
	pIceSession->m_hStackHandle = m_hStackHandle;
	pIceSession->m_prioritizePrivacy = m_prioritizePrivacy;

	RvIceSessionCfg IceSessionCfg;
	RvIceGetDefaultSessionCfg (m_hStackHandle, &IceSessionCfg);

	//
	// Change the preference to steer the selection toward the use of the TURN server
	// when the debug flag is non-zero.
	//
	if (g_stiPreferTURN != 0)
	{
		IceSessionCfg.priorTypePreferenceHost = 0;
		IceSessionCfg.priorTypePreferencePeerReflexive = 1;
		IceSessionCfg.priorTypePreferenceSrvReflexive = 2;
		IceSessionCfg.priorTypePreferenceMediaRelayed = 126;
	}

	// Create ICE session - give the application session object as the application handle.
	rvStatus = RvIceCreateSession (
		m_hStackHandle,
		(RvIceAppSessionHandle)pIceSession, // app handle
		&IceSessionCfg,
		&pIceSession->m_hSession);
	stiASSERTMSG(rvStatus != RV_ERROR_OUTOFRESOURCES, "Could not create a new Ice Session since the maximum number of sessions has already been reached.");
	stiTESTRVSTATUS ();

	if (pIceSession->m_eRole != CstiIceSession::estiICERoleAnswerer)
	{
		rvStatus  = RvIceSessionSetOfferer (pIceSession->m_hSession);
		stiTESTRVSTATUS ();
	}
	else
	{
		rvStatus = RvIceSessionSetAnswerer (pIceSession->m_hSession);
		stiTESTRVSTATUS ();
	}

	*ppIceSession = pIceSession;

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::AccountStartSession
//
// Description: Activates an ICE session according to its role.
//
// Abstract: For both a completed callback is called when the gathering is completed.
//	If there is an error reported via the callback, this may be called again to try the
//	next available server.  If there are none available, this function will 
//  return stiRESULT_SERVER_BUSY. 
//
// Returns:
// stiHResult
//
stiHResult CstiIceManager::AccountStartSession (
	CstiIceSession *pIceSession,
	const CstiSdp &Sdp,
	IceSupport *iceSupport,
	const std::string &PublicIPv4,
	const std::string &PublicIPv6,
	CstiIceSession::Protocol protocol)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszTurnDomain = nullptr;
	char *pszTurnDomainAlt = nullptr;
	char *pszStunDomain = nullptr;
	char *pszStunDomainAlt = nullptr;
	RvAddress LocalRvIPv4Address;
#ifdef IPV6_ENABLED
	RvAddress LocalRvIPv6Address;
	bool bUseIpv6 = false;
#endif
	std::string LocalIPv4Address;
	std::string LocalIPv6Address;
	bool bUseIpv4 = true;
	bool bIterationStarted = false;
	bool proceed = true;
	
	// if this is a second server attempt, do not set things up again
	if (!pIceSession->m_turnIpList || pIceSession->m_bRetrying)
	{
		// Make a copy of the sdp for use in the transport retrieval methods.
		pIceSession->m_originalSdp = Sdp;

		if (pIceSession->m_eRole == CstiIceSession::estiICERoleAnswerer)
		{
			IceSupport iceSupportLocal = IceSupport::Unknown;

			pIceSession->SdpReceived(Sdp, &iceSupportLocal);

			if (iceSupportLocal != IceSupport::Supported)
			{
				proceed = false;
			}

			if (iceSupport)
			{
				*iceSupport = iceSupportLocal;
			}
		}
		else
		{
			hResult = pIceSession->SdpIceSupportAdd(Sdp);
			stiTESTRESULT ();
		}
		
		if (proceed)
		{
	#ifdef IPV6_ENABLED // IPv6 is not fully implemented for the entire system.  Once we switch to using it enable the following code.
			// Determine which ip protocol versions are supported
			bUseIpv6 = !PublicIPv6.empty ();
	#if APPLICATION == APP_NTOUCH_IOS || APPLICATION == APP_DHV_IOS
			// iOS has only implemented IPV6 to function on an IPV6 only NAT64 environment.
			// Trying to perform STUN requests using IPV4 on this type of network will break calling.
			bUseIpv4 = !bUseIpv6;
	#endif //APPLICATION == APP_NTOUCH_IOS
			// Set the local IPv6 addresses to the session
			if (bUseIpv6)
			{
				pIceSession->m_PublicIPv6 = PublicIPv6;

				hResult = stiGetLocalIp (&LocalIPv6Address, estiTYPE_IPV6);
				if (!stiIS_ERROR (hResult))
				{
					// Strip off all decoration and get just the ip
					CstiIpAddress IpAddress { LocalIPv6Address };
					LocalIPv6Address = IpAddress.AddressGet (CstiIpAddress::eFORMAT_IP);

					RvAddressConstruct (RV_ADDRESS_TYPE_IPV6, &LocalRvIPv6Address);
					RvAddressSetString (LocalIPv6Address.c_str (), &LocalRvIPv6Address);
					RvAddressSetIpv6Scope (&LocalRvIPv6Address, IpAddress.ZoneGet ());
				}
			}
	#endif// IPV6_ENABLED

			// Set the local IPv4 addresses to the session
			if (bUseIpv4)
			{
				pIceSession->m_PublicIPv4 = PublicIPv4;

				hResult = stiGetLocalIp (&LocalIPv4Address, estiTYPE_IPV4);
				if (!stiIS_ERROR (hResult))
				{
					RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &LocalRvIPv4Address);
					RvAddressSetString (LocalIPv4Address.c_str (), &LocalRvIPv4Address);
				}
			}
		}
	}

	if (proceed)
	{
		if (m_server)
		{
			if (!pIceSession->m_turnIpList || pIceSession->m_bRetrying)
			{
				if (bUseIpv4)
				{
					rvStatus = RvIceSessionSetLocalRtpCandidatesToAllMediaStreams (
						pIceSession->m_hSession,
						&LocalRvIPv4Address,
						1);
					stiTESTRVSTATUS ();
				}

	#ifdef IPV6_ENABLED
				if (bUseIpv6)
				{
					rvStatus = RvIceSessionSetLocalRtpCandidatesToAllMediaStreams (
						pIceSession->m_hSession,
						&LocalRvIPv6Address,
						1);
					stiTESTRVSTATUS ();
				}
	#endif
			}

			std::string TurnAddress;
			uint16_t un16TurnPort = STANDARD_STUN_PORT;
			RvIceStunServerCfg *serverCfg = nullptr;

			//
			// Look up turn server address
			//
			if (m_server)
			{
				// Parse out the primary TURN address and port from the Address:Port string
				const char *pszTurnAddressSetting = m_server->AddressGet ();
				pszTurnDomain = new char[strlen (pszTurnAddressSetting) + 1];
				char pszTurnPort[6];
				AddressAndPortParse (pszTurnAddressSetting, pszTurnDomain, pszTurnPort);
				if (!pszTurnDomain[0])
				{
					stiTHROW (stiRESULT_ERROR);
				}
				if (pszTurnPort[0])
				{
					un16TurnPort = (uint16_t)atoi (pszTurnPort);
				}

				// Parse out the alternate TURN address and port from the Address:Port string
				pszTurnAddressSetting = m_serverAlt->AddressGet ();
				if (pszTurnAddressSetting && *pszTurnAddressSetting)
				{
					pszTurnDomainAlt = new char[strlen (pszTurnAddressSetting) + 1];
					char pszTurnPortAlt[6];
					AddressAndPortParse (pszTurnAddressSetting, pszTurnDomainAlt, pszTurnPortAlt);
				}

				// Perform a DNS lookup
				if (!pIceSession->m_turnIpList)
				{
					// Fetch a list of ip addresses
					addrinfo *pstAddrInfo;
					CstiHostNameResolver *pResolver = CstiHostNameResolver::getInstance ();
					pResolver->Resolve (pszTurnDomain, pszTurnDomainAlt, &pstAddrInfo);
					pIceSession->m_turnIpList = std::vector<std::string>();
					pResolver->IpListFromAddrinfoCreate (pstAddrInfo, &(pIceSession->m_turnIpList.get ()));
				}
				if (!pIceSession->m_turnIpList->empty ())
				{
					// Pop off the first item on the list
					TurnAddress.assign (pIceSession->m_turnIpList->at (0));
					pIceSession->m_turnIpList->erase (pIceSession->m_turnIpList->begin ());
				}
				if (TurnAddress.empty ())
				{
					stiTHROW (stiRESULT_SERVER_BUSY);
				}

				DBG_MSG ("Trying TURN server %s:%d ...", TurnAddress.c_str (), un16TurnPort);
				boost::optional<std::string> username;
				boost::optional<std::string> password;

				// Only authenticate with TURN
				// our STUN infrastruce isn't set up for STUN authentication
				// (radvision computes an HMAC if credentials are available, which our server doesn't support for STUN)
				if(CstiIceSession::Protocol::Turn == protocol)
				{
					username = m_server->UsernameGet ();
					password = m_server->PasswordGet ();
				}

				ServerSet (TurnAddress.c_str (), un16TurnPort, username, password, &pIceSession->m_serverCfg);
				serverCfg = &pIceSession->m_serverCfg;
			}

			// Start the reflexive candidates gathering
			DBG_MSG ("%s - start gathering reflexive candidates", IceRoleNameGet (pIceSession->m_eRole));
			pIceSession->m_bTurnFailureDetected = false;
			if (serverCfg &&
				RV_OK != RvIceSessionGatherReflexiveCandidatesEx (
					pIceSession->m_hSession,
					CstiIceSession::Protocol::Stun == protocol ? serverCfg : nullptr,
					CstiIceSession::Protocol::Turn == protocol ? serverCfg : nullptr,
					0)
			)
			{
				stiTHROW (stiRESULT_ERROR);
			}
		}
		else
		{
			if (!PublicIPv4.empty ())
			{
				rvStatus = RvIceSessionSetLocalRtpCandidatesToAllMediaStreams (
					pIceSession->m_hSession,
					&LocalRvIPv4Address,
					1);
				stiTESTRVSTATUS ();
			}

	#ifdef IPV6_ENABLED
			if (!PublicIPv6.empty ())
			{
				rvStatus = RvIceSessionSetLocalRtpCandidatesToAllMediaStreams (
					pIceSession->m_hSession,
					&LocalRvIPv6Address,
					1);
				stiTESTRVSTATUS ();
			}
	#endif // IPV6_ENABLED


			if (LocalIPv6Address != PublicIPv6)
			{
				//
				// If our public IP and private IP are different then add server reflexive ports with the information
				// we have.
				//
				pIceSession->ServerReflexivePortsAdd (PublicIPv6, RV_ADDRESS_TYPE_IPV6);
			}

			if (LocalIPv4Address != PublicIPv4)
			{
				//
				// If our public IP and private IP are different then add server reflexive ports with the information
				// we have.
				//
				pIceSession->ServerReflexivePortsAdd (PublicIPv4, RV_ADDRESS_TYPE_IPV4);
			}

			//
			// Since we don't have to gather our server reflexive or TURN allocations then
			// we are done gathering.
			//
			pIceSession->m_eState = CstiIceSession::estiGatheringComplete;

			iceGatheringCompleteSignal.Emit (pIceSession->m_CallbackParam);
		}

		// We  only ever want to call generate credentials once per ICE session to prevent a resource leak bug #24930.
		rvStatus =  RvIceSessionGenerateCredentials (pIceSession->m_hSession, nullptr);

		stiTESTCONDMSG (rvStatus == RV_OK, stiRESULT_ERROR, "CstiIceManager::AccountStartSession RvIceSessionGenerateCredentials failed Role=%d Result=%d",
								 pIceSession->m_eRole, rvStatus);
	}
	
STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		if (RV_OK != RvIceSessionIterationEnd (pIceSession->m_hStackHandle, pIceSession->m_hSession))
		{
			stiASSERT (estiFALSE);
		}

		bIterationStarted = false;
	}

	if (pszTurnDomain)
	{
		delete [] pszTurnDomain;
		pszTurnDomain = nullptr;
	}
	
	if (pszTurnDomainAlt)
	{
		delete [] pszTurnDomainAlt;
		pszTurnDomainAlt = nullptr;
	}
	
	if (pszStunDomain)
	{
		delete [] pszStunDomain;
		pszStunDomain = nullptr;
	}
	
	if (pszStunDomainAlt)
	{
		delete [] pszStunDomainAlt;
		pszStunDomainAlt = nullptr;
	}
	
	pIceSession->m_bRetrying = false;
	
	pIceSession->m_remoteSdp = Sdp;
	
	return hResult;
}


// Creates a new ICE session attempting the next server in the list.
// If enableTurn/Stun optoinals not set, use the default
stiHResult CstiIceManager::IceSessionTryAnother (
	std::string PublicIPv4,
	std::string PublicIPv6,
	size_t CallbackParam,
	CstiIceSession::Protocol protocol,
	CstiIceSession **ppIceSession)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bTryingNewSession = true;
	CstiIceSession *pIceSession = nullptr;
	CstiSdp sdpCopy;

	stiTESTCONDMSG(*ppIceSession, stiRESULT_INVALID_PARAMETER, "CstiIceManager::IceSessionTryAnother called withouth ICE session");

	pIceSession = *ppIceSession;

	if (pIceSession->m_turnIpList && !pIceSession->m_turnIpList->empty ())
	{
		std::vector<std::string> TurnIpListCopy (*pIceSession->m_turnIpList);
		CstiIceSession::EstiIceRole eRole = pIceSession->m_eRole;

		sdpCopy = pIceSession->m_originalSdp;

		pIceSession->SessionEnd ();
		pIceSession = nullptr;

		hResult = AccountCreateSession (eRole, CallbackParam, &pIceSession);
		stiTESTRESULT ();

		pIceSession->m_bRetrying = true;

		pIceSession->m_turnIpList = TurnIpListCopy;

		hResult = AccountStartSession (pIceSession,
			sdpCopy,
			nullptr,
			PublicIPv4,
			PublicIPv6,
			protocol);

		if (stiIS_ERROR (hResult))
		{
			bTryingNewSession = false;
			stiASSERTMSG(estiFALSE, "CstiIceManager::IceSessionTryAnother Failed to start ICE session Role=%d", eRole);
		}
	}
	else
	{
		stiASSERTMSG (estiFALSE, "CstiIceManager::IceSessionTryAnother all TURN servers failed Role=%d", pIceSession->m_eRole);
		bTryingNewSession = false;
	}

	if (!bTryingNewSession && pIceSession)
	{
		IceCompleted (pIceSession);
	}

	*ppIceSession = pIceSession;

STI_BAIL:

	return hResult;
}

/*! \brief get the default ice candidate settings
 */
CstiIceSession::Protocol CstiIceManager::DefaultProtocolGet () const
{
	return m_defaultProtocol;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::IceSessionCandidatesGatheringCompletedCB
//
// Description: Indicates that the RvIceSessionFindReflexiveCandidates function was completed.
//	In this callback, the application should proceed with the following ICE actions:
//	- Generate local credentials for the connectivity checks.
//	- Offerer: send the SDP Offer.
//	- Answerer: send the SDP Answer and start the connectivity checks.
//
// Returns:
// 	nothing
//
RvStatus CstiIceManager::IceSessionCandidatesGatheringCompletedCB (
	RvIceSessionHandle hSession, ///< Handle to the ICE session object.
	RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
	RvIceReason eReason) ///< Indicates how the gathering was completed.
{
	auto pIceSession = (CstiIceSession *)hAppSession;
	
	DBG_MSG ("%s - Gathering of reflexive candidates was completed with %d and TURN %s work", IceRoleNameGet (pIceSession->m_eRole), eReason, pIceSession->m_bTurnFailureDetected?"did not":"did");

	switch (eReason)
	{
		case RV_ICE_REASON_UNDEFINED:
			stiASSERT (estiFALSE);
			break;
			
		case RV_ICE_REASON_GATHERING_CREDENTIALS_REQUIRED:
			stiASSERT (estiFALSE);
			break;

		case RV_ICE_REASON_GATHERING_COMPLETED:
			break;
		
		// default: // commented out for type safety
	}
	
	// If a TURN has happened, attempt to try another server.
	if (pIceSession->m_bTurnFailureDetected)
	{
		stiASSERTMSG (estiFALSE, "IceSessionCandidatesGatheringCompletedCB Turn Failure Detected");

		// Attempt to try another server
		pIceSession->m_pIceManager->iceGatheringTryingAnotherSignal.Emit (pIceSession->m_CallbackParam);
	}
	else
	{
		pIceSession->m_pIceManager->IceCompleted (pIceSession);
	}

//STI_BAIL:

	return RV_OK;
}



stiHResult CstiIceManager::IceCompleted(CstiIceSession *pIceSession)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	pIceSession->m_eState = CstiIceSession::estiGatheringComplete;

	// Post the event
	iceGatheringCompleteSignal.Emit (pIceSession->m_CallbackParam);

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::IceSessionCompleteCB
//
// Description: This callback is called when ICE processing completes.
//	It notifies the completion status:
//	- All media streams completed the ICE processing successfully.
//	- Some media streams failed.
//	- All media streams failed.
//	This callback can be called more than once if there are better results (a higher-
//	priority candidates pair) for a component Id of a media stream.
//	You can use the pfnMediaCompleted callback to keep track of those changes.
//
// Returns:
// 	nothing
//
void CstiIceManager::IceSessionCompleteCB (
	RvIceSessionHandle hSession, ///< Handle to the ICE session object.
	RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
	RvIceSessionCompleteResult complResult) ///< Indicates the application how the ICE processing was completed.
{
	auto pIceSession = (CstiIceSession *)hAppSession;

	DBG_MSG ("--------------------------------------------");
	DBG_MSG ("%s ICE session was completed with %s (%d)",
		IceRoleNameGet (pIceSession->m_eRole),
		complResult == RV_ICE_ALL_MEDIA_STREAMS_SUCCEEDED ? "Success" : "Failure", complResult);

	pIceSession->m_CompleteResult = complResult;

	if (pIceSession->m_eState != CstiIceSession::estiComplete)
	{
		if (complResult == RV_ICE_ALL_MEDIA_STREAMS_SUCCEEDED)
		{
			stiDEBUG_TOOL (g_stiIceMgrDebug,
						   pIceSession->CandidatesList ();
						   );
			
			pIceSession->m_pIceManager->iceNominationsCompleteSignal.Emit (pIceSession->m_CallbackParam, true);
		}
		else
		{
			pIceSession->m_eState = CstiIceSession::estiNominationsFailed;
			
			pIceSession->m_pIceManager->iceNominationsCompleteSignal.Emit (pIceSession->m_CallbackParam, false);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceManager::TurnAllocationCB
//
// Description: This callback is called when TURN allocation completes, be it
//	with success or failure.
//
// Returns:
// 	nothing
//
RvBool RVCALLCONV CstiIceManager::TurnAllocationCB (
	RvIceSessionHandle hSession, ///< Handle to the ICE session object.
	RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
	RvIceCandidateHandle hCandidate,  ///< Handle to the ICE candidate object.
	const RvAddress *pAddress,  ///< The candidate’s local address.
	RvBool bIsTurn,  ///< Trueif called for the TURN allocation attempt. False if called for the STUN Get-Binding request.
	RvBool bTurnAllocSuccess,  ///< Indicates whether the TURN allocation was established successfully.
	RvStunTransactionError eStunTransactionError, ///< Any associated connection failure code.
	RvInt eStunResponseError) ///< Any associated failure remote response.
{
	RvBool bSuccess = RV_TRUE;
	if (!bTurnAllocSuccess) 
	{
		auto pIceSession = (CstiIceSession *)hAppSession;
		pIceSession->m_bTurnFailureDetected = true;
		bSuccess = RV_FALSE;
	}
	return bSuccess;
}

void CstiIceManager::useLiteModeSet (bool useLiteMode)
{
	m_useLiteMode = useLiteMode;
}

void CstiIceManager::rejectHostOnlyRequestsSet (bool rejectHostOnlyRequests)
{
	m_rejectHostOnlyRequests = rejectHostOnlyRequests;
}
