////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiIceSession
//
//  File Name:	CstiIceSession.cpp
//
//	Abstract:
//		The Ice Session represents a single ICE session.  There may be multiple
//		simultaneous ICE sessions for a single call due to forking.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiIceSession.h"
#include "stiTools.h"
#include "stiSipTools.h"
#include "stiTrace.h"
#include "stiOS.h" // OS Abstraction definitions
#include "CstiHostNameResolver.h"
#include "CstiRegEx.h"
#include <algorithm>

// ICE stack
#include "RvIceMgr.h"
#include "RvIceSession.h"
#include "rvnetaddress.h"
#include "rtp.h"
#include "rtcp.h"


//
// Constants
//
#define MAX_ATTRIBUTE_STRING_LENGTH	2048
#define MAX_NUMBER_MEDIA_STREAMS 32
#define RV_SDPMEDIATYPE_TEXT 1 // TODO

//
// Typedefs
//


//
// Forward Declarations
//
class CstiIceManager;


//
// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiIceMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )
//#define DBG_MSG(fmt,...) stiDEBUG_TOOL (1, stiTrace (fmt "\n", ##__VA_ARGS__); )


/*!\brief Copies a Radvision STUN server object to another Radvision STUN server object.
 *
 * \param pDst The object to which to copy
 * \param pSrc The object from whic to copy
 */
stiHResult ServerCopy (
	RvIceStunServerCfg *pDst,
	const RvIceStunServerCfg *pSrc)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	memset (pDst, 0, sizeof (*pDst));

	RvAddressCopy (&pSrc->stunServerAddress, &pDst->stunServerAddress);

	pDst->usernameLen = pSrc->usernameLen;

	if (pSrc->usernameLen > 0)
	{
		pDst->username = new char [pDst->usernameLen + 1];
		strcpy ((char *)pDst->username, pSrc->username);
	}

	pDst->passwordLen = pSrc->passwordLen;

	if (pDst->passwordLen)
	{
		pDst->password = new char [pDst->passwordLen + 1];
		strcpy ((char *)pDst->password, pSrc->password);
	}

	return hResult;
}


CstiIceSession::CstiIceSession ()
{
	memset (&m_serverCfg, 0, sizeof (m_serverCfg));
}


CstiIceSession::CstiIceSession (
	const CstiIceSession &Other)
:
	m_eState (Other.m_eState),
	m_bRelatedSession (Other.m_bRelatedSession),
	m_CallbackParam (Other.m_CallbackParam),
	m_hSession (Other.m_hSession),
	m_eRole (Other.m_eRole),
	m_pIceManager (Other.m_pIceManager),
	m_CompleteResult (Other.m_CompleteResult),
	m_serverCfg (Other.m_serverCfg),
	m_hStackHandle (Other.m_hStackHandle)
	
{
	m_originalSdp = Other.m_originalSdp;

	m_bRetrying = Other.m_bRetrying;
	ServerCopy (&m_serverCfg, &Other.m_serverCfg);
}


CstiIceSession::~CstiIceSession ()
{
	if (m_serverCfg.username)
	{
		delete [] m_serverCfg.username;
		m_serverCfg.username = nullptr;
	}

	if (m_serverCfg.password)
	{
		delete [] m_serverCfg.password;
		m_serverCfg.password = nullptr;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceSession::SessionEnd
//
// Description: Disconnect the ICE session now.
//
// Returns:
//  stiHResult
//
stiHResult CstiIceSession::SessionEnd ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_hSession)
	{
		//
		// We can only destroy the main ICE sesssion.  The related
		// sessions are destroyed when the main one is destroyed.
		//
		if (!m_bRelatedSession)
		{
			RvIceDestroySession (m_hSession);
		}

		m_hSession = nullptr;
	}

	delete this;

	return hResult;
}


/*!\brief Passes the SDP to the ICE toolkit and reports support for ICE
 *
 */
void CstiIceSession::SdpReceived (
	const CstiSdp &Sdp,
	IceSupport *iceSupport)
{
	RvIceSdpInputResult sdpInRes;

	// Set the SDP offer (with the media streams information) to the ICE session
	RvStatus rvStatus = RvIceSessionSDPIn (m_hSession,
					   const_cast<RvSdpMsg *>(Sdp.sdpMessageGet ()),
					   RV_TRUE,
					   &sdpInRes);

	// Determine if ICE is supported in the SDP
	switch (sdpInRes.eIceSupport)
	{
		case RV_ICE_ICE_NOT_SUPPORTED:
		{
			*iceSupport = IceSupport::NotSupported;

			break;
		}

		case RV_ICE_ICE_MISMATCH:
		{
			*iceSupport = IceSupport::Mismatch;

			break;
		}

		case RV_ICE_ICE_SUPPORTED:

			if (rvStatus == 0)
			{
				*iceSupport = IceSupport::Supported;
			}
			else
			{
				*iceSupport = IceSupport::Error;
			}

			break;

		case RV_ICE_ICE_REPORTED_MISMATCH:

			*iceSupport = IceSupport::ReportedMismatch;

			break;

		case RV_ICE_TRICKLE_MISMATCH:
		{
			*iceSupport = IceSupport::TrickleIceMismatch;

			break;
		}
	}
}


/*!\brief Passes the SDP to the ICE toolkit to have ICE support added
 *
 */
stiHResult CstiIceSession::SdpIceSupportAdd (
	const CstiSdp &Sdp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvIceSdpInputResult sdpInRes;

	// Set the SDP offer (with the media streams information) to the ICE session
	RvStatus rvStatus = RvIceSessionSDPIn (m_hSession,
					   const_cast<RvSdpMsg *>(Sdp.sdpMessageGet ()),
					   RV_FALSE,
					   &sdpInRes);
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
}


stiHResult CstiIceSession::RedactCandidateRaddr (RvSdpAttribute *attr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	{
		// Verify this is a candidate
		stiTESTCONDMSG(attr, stiRESULT_INVALID_PARAMETER, "Failed to parse candidate, attribute is null");
		stiTESTCONDMSG(strcmp(rvSdpAttributeGetName(attr), "candidate") == 0, stiRESULT_INVALID_PARAMETER, "Failed to parse candidate, attribute is not a candidate");
		
		// Find the start of the raddr IP address
		auto raddr = (char *)strstr(rvSdpAttributeGetValue(attr), " raddr ");
		stiTESTCONDMSG(raddr != nullptr, stiRESULT_ERROR, "Failed to parse candidate to remove raddr");
		raddr += strlen(" raddr ");
		
		// Find the end of the raddr IP address, there will be an " rport <port>" after.
		auto afterRaddr = strchr(raddr, ' ');
		stiTESTCONDMSG(afterRaddr != nullptr, stiRESULT_ERROR, "Failed to parse candidate to remove raddr");
		
		// Find the length of the new address and the old address, make sure we don't have to resize the buffer to use the new IP address.
		auto raddrLength = afterRaddr - raddr;
		std::string newRaddr{"0.0.0.0"};
		stiTESTCONDMSG(raddrLength > 0, stiRESULT_ERROR, "Failed to replace candidate raddr, old address has an invalid length"); // ensure safe cast from ptrdiff_t to size_t
		stiTESTCONDMSG(newRaddr.length() <= (size_t)raddrLength, stiRESULT_ERROR, "Failed to replace candidate raddr, old address is shorter than new address");
		
		// Copy the new address into place
		std::copy(newRaddr.begin(), newRaddr.end(), raddr);
		
		// Delete any extra characters remaining of the (potentially longer) old address
		std::move_backward(afterRaddr, afterRaddr + strlen(afterRaddr), raddr + newRaddr.length() + strlen(afterRaddr));
		
		// Fill in the space we made at the end of the string with null characters
		std::fill_n(raddr + newRaddr.length() + strlen(afterRaddr), raddrLength - newRaddr.length(), '\0');
	}
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceSession::SdpUpdate
//
// Description: Updates some outgoing SDP.
//
// Returns:
// 	stiHResult
//
stiHResult CstiIceSession::SdpUpdate (
	CstiSdp *pSdp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	stiDEBUG_TOOL (g_stiIceMgrDebug,
		stiTrace ("Before modification: *************************************************\n");

		pSdp->Print ();

		stiTrace ("Before modification end\n");
	);

	// Fill the SDP Offer with the ICE information
	rvStatus = RvIceSessionSDPOut (m_hSession, pSdp->sdpMessageGet ());
	stiTESTRVSTATUS ();
	
	if (m_prioritizePrivacy)
	{
		// HACK: Until we fix SDP logging to obscure public IPs, remove non-relay candidates from the SDP before sending.

		RvSdpMsg *sdpMsg = pSdp->sdpMessageGet ();
		
		// For each media descriptor, remove all the non-relay candidates
		RvSdpListIter mediaIter;
		for (auto mediaDesc = rvSdpMsgGetFirstMediaDescr(sdpMsg, &mediaIter); mediaDesc != nullptr; mediaDesc = rvSdpMsgGetNextMediaDescr(&mediaIter)) {
			
			size_t index = 0;
			while (index < rvSdpMediaDescrGetNumOfAttr(mediaDesc))
			{
				auto attr = rvSdpMediaDescrGetAttribute(mediaDesc, index);
				if (!attr || strcmp(rvSdpAttributeGetName(attr), "candidate") != 0)
				{
					index++;
				}
				else if (!strstr(rvSdpAttributeGetValue(attr), "typ relay"))
				{
					// If this is a candidate and it isn't a relay candidate, remove the candidate from the SDP
					rvSdpMediaDescrRemoveAttribute(mediaDesc, index);
				}
				else
				{
					RedactCandidateRaddr (attr);
					index++;
				}
			}
		}
	}

	stiDEBUG_TOOL (g_stiIceMgrDebug,
		stiTrace ("After modification: *************************************************\n");

		pSdp->Print ();

		stiTrace ("After modification end\n");
	);

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceSession::SessionProceed
//
// Description: This is called to permit the ICE session to proceed to the next step.
//
// Returns:
// 	stiHResult
//
stiHResult CstiIceSession::SessionProceed ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	m_eState = estiNominating;

	rvStatus = RvIceSessionProceed (m_hSession);
	stiTESTRVSTATUS ();

STI_BAIL:

	return hResult;
}


/*!\brief Translates a Radvision candidate type to a Sorenson media transport type.
 *
 * \param eType The Radvision candidate type to convert
 */
EstiMediaTransportType TransportTypeGet (
	RvIceCandidateType eType)
{
	EstiMediaTransportType eTransportType = estiMediaTransportUnknown;

	switch (eType)
	{
		case RV_ICE_CANDIDATE_TYPE_UNDEFINED:

			eTransportType = estiMediaTransportUnknown;

			break;

		case RV_ICE_CANDIDATE_TYPE_HOST:

			eTransportType = estiMediaTransportHost;

			break;

		case RV_ICE_CANDIDATE_TYPE_SERVER_REFLEXIVE:

			eTransportType = estiMediaTransportReflexive;

			break;

		case RV_ICE_CANDIDATE_TYPE_MEDIA_RELAYED:

			eTransportType = estiMediaTransportRelayed;

			break;

		case RV_ICE_CANDIDATE_TYPE_PEER_REFLEXIVE:

			eTransportType = estiMediaTransportPeerReflexive;

			break;
	}

	return eTransportType;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceSession::NominationsGet
//
// Description: Gets the nominated transports that ICE has nominated for use.
//
// NOTE: Any transport pointer can be NULL.
// NOTE: If you take a transport, you become the owner and must release it!
//
// Returns:
// 	stiHResult
//
stiHResult CstiIceSession::NominationsGet (
	CstiMediaTransports *pLocalAudio,
	CstiMediaTransports *pLocalText,
	CstiMediaTransports *pLocalVideo,
	CstiMediaAddresses *pRemoteAudio,
	CstiMediaAddresses *pRemoteText,
	CstiMediaAddresses *pRemoteVideo,
	SstiIceSdpAttributes *pAttributes)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvIceCandidatesPairHandle hRtpPair, hRtcpPair;
	RvIceCandPairDetails rtpPair, rtcpPair;
	RvIceMediaStreamHandle hMedia = nullptr;
	RvIceCandidateDetails rtpLocal, rtcpLocal, rtpRemote, rtcpRemote;
	RvChar addrTxt1[256], addrTxt2[256];
	bool bIterationStarted = false;
	CstiMediaTransports *pMediaTransports = nullptr;
	CstiMediaTransports localAudio;
	CstiMediaTransports localText;
	CstiMediaTransports localVideo;
	CstiMediaAddresses *pMediaAddresses = nullptr;
	CstiMediaAddresses remoteAudio;
	CstiMediaAddresses remoteText;
	CstiMediaAddresses remoteVideo;
	int nNumMediaStreams = 0;

	//
	// It is an error to call this method before nominations are made.
	//
	stiTESTCOND (m_CompleteResult == RV_ICE_ALL_MEDIA_STREAMS_SUCCEEDED, stiRESULT_ERROR);

	rvStatus = RvIceSessionIterationBegin (m_hStackHandle, m_hSession);
	stiTESTRVSTATUS ();

	bIterationStarted = true;

	//
	// Get the first media in the list.
	//

	for (;;)
	{
		bool rtcpMuxing = false;
		if (RV_OK != RvIceSessionIterateMediaStream (m_hSession, RV_ICE_LIST_NEXT, hMedia, &hMedia, nullptr))
		{
			break;
		}

		++nNumMediaStreams;

		// Get the sdp for the particular media in question
		RvIceMediaStreamDetails MediaDetails;
		RvIceMediaGetDetails (hMedia, &MediaDetails);
		const RvSdpMediaDescr *pMediaDescriptor = m_originalSdp.mediaDescriptorGet(MediaDetails.indexInSdpMsg);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);
		EstiMediaType eMediaType = SorensonMediaTypeGet (
			rvSdpMediaDescrGetMediaType (pMediaDescriptor),
			rvSdpMediaDescrGetMediaTypeStr (pMediaDescriptor));
		
		
		rtcpMuxing = m_remoteSdp.mediaTypeContainsAttribute(rvSdpMediaDescrGetMediaTypeStr (pMediaDescriptor), "rtcp-mux");

		//
		// Get the RTP candidate pair.
		//
		rvStatus = RvIceMediaIterateCandPair (
			hMedia,
			RV_ICE_LIST_FIRST,
			nullptr,
			RV_ICE_COMPONENT_ID_RTP,
			RV_TRUE, 	// valid
			RV_TRUE, 	// nominated
			&hRtpPair);
		stiTESTRVSTATUS ();

		if (!rtcpMuxing)
		{
			//
			// Get the RTCP candidate pair.
			//
			rvStatus = RvIceMediaIterateCandPair (
				hMedia,
				RV_ICE_LIST_FIRST,
				nullptr,
				RV_ICE_COMPONENT_ID_RTCP,
				RV_TRUE, 	// valid
				RV_TRUE, 	// nominated
				&hRtcpPair);
			stiTESTRVSTATUS ();
		}

		memset (&rtpPair, 0, sizeof (rtpPair)); // keep compiler happy
		memset (&rtcpPair, 0, sizeof (rtcpPair)); // keep compiler happy

		rvStatus = RvIceCandPairGetDetails (hRtpPair, &rtpPair);
		stiTESTRVSTATUS ();

		if (!rtcpMuxing)
		{
			rvStatus = RvIceCandPairGetDetails (hRtcpPair, &rtcpPair);
			stiTESTRVSTATUS ();
		}
		else
		{
			rtcpPair = rtpPair;
		}

		memset (&rtpLocal, 0, sizeof (rtpLocal)); // keep compiler happy
		memset (&rtcpLocal, 0, sizeof (rtcpLocal)); // keep compiler happy
		memset (&rtpRemote, 0, sizeof (rtpRemote)); // keep compiler happy
		memset (&rtcpRemote, 0, sizeof (rtcpRemote)); // keep compiler happy

		rvStatus = RvIceCandidateGetDetails (rtpPair.hLocalCand, &rtpLocal);
		stiTESTRVSTATUS ();

		rvStatus = RvIceCandidateGetDetails (rtpPair.hRemoteCand, &rtpRemote);
		stiTESTRVSTATUS ();

		rvStatus = RvIceCandidateGetDetails (rtcpPair.hLocalCand, &rtcpLocal);
		stiTESTRVSTATUS ();

		rvStatus = RvIceCandidateGetDetails (rtcpPair.hRemoteCand, &rtcpRemote);
		stiTESTRVSTATUS ();

		DBG_MSG ("   RTP nominated pair (type %s)", RvIceCandTypeEnum2Text (rtpLocal.eType, NULL));
		DBG_MSG ("		  Local %s:%d   <---> Remote %s:%d",
					RvAddressGetString (&rtpLocal.address, sizeof (addrTxt1), addrTxt1), RvAddressGetIpPort (&rtpLocal.address),
					RvAddressGetString (&rtpRemote.address, sizeof (addrTxt2), addrTxt2), RvAddressGetIpPort (&rtpRemote.address));

		DBG_MSG ("   RTCP nominated pair (type %s)", RvIceCandTypeEnum2Text (rtcpLocal.eType, NULL));
		DBG_MSG ("		  Local %s:%d   <---> Remote %s:%d",
					RvAddressGetString (&rtcpLocal.address, sizeof (addrTxt1), addrTxt1), RvAddressGetIpPort (&rtcpLocal.address),
					RvAddressGetString (&rtcpRemote.address, sizeof (addrTxt2), addrTxt2), RvAddressGetIpPort (&rtcpRemote.address));

		// Match that RTP/RTCP connection to one of our media types
		if (estiMEDIA_TYPE_AUDIO == eMediaType)
		{
			pMediaTransports = &localAudio;
			pMediaAddresses = &remoteAudio;
		}
		else if (estiMEDIA_TYPE_TEXT == eMediaType)
		{
			pMediaTransports = &localText;
			pMediaAddresses = &remoteText;
		}
		else if (estiMEDIA_TYPE_VIDEO == eMediaType)
		{
			pMediaTransports = &localVideo;
			pMediaAddresses = &remoteVideo;
		}
		else
		{
			continue;
		}
		
		RvTransport RtpTransport;
		RvTransport RtcpTransport;

		rvStatus = RvIceSessionCreateMediaTransport(hMedia,RV_ICE_COMPONENT_ID_RTP,&RtpTransport);
		stiTESTRVSTATUS ();

		pMediaTransports->RtpTransportSet (RtpTransport, true);
		pMediaTransports->RtpLocalAddressSet (&rtpLocal.address);
		pMediaTransports->RtpRemoteAddressSet (&rtpRemote.address);
		pMediaTransports->RtpTypeSet(TransportTypeGet(rtpLocal.eType));

		if (!rtcpMuxing)
		{
			RvIceSessionCreateMediaTransport(hMedia,RV_ICE_COMPONENT_ID_RTCP,&RtcpTransport);
			stiTESTRVSTATUS ();
		}
		else
		{
			RtcpTransport = RtpTransport;
		}

		pMediaTransports->RtcpTransportSet (RtcpTransport, !rtcpMuxing);
		pMediaTransports->RtcpLocalAddressSet (&rtcpLocal.address);
		pMediaTransports->RtcpRemoteAddressSet (&rtcpRemote.address);
		pMediaTransports->RtcpTypeSet(TransportTypeGet(rtcpLocal.eType));
		
		pMediaTransports->isValidSet(true);
		pMediaTransports->isRtcpMuxSet (rtcpMuxing);

//		stiDEBUG_TOOL (g_stiIceMgrDebug,
//			stiTrace ("Retrieved Nominated Audio RTP Transport: %p\n", pMediaTransports->RtpTransportGet ());
//			stiTrace ("Retrieved Nominated Audio RTCP Transport: %p\n", pMediaTransports->RtcpTransportGet ());
//		);
		
		pMediaAddresses->RtpAddressSet (&rtpRemote.address);
		pMediaAddresses->RtcpAddressSet (&rtcpRemote.address);
		pMediaAddresses->isValidSet(true);
	}

	DBG_MSG ("--------------------------------------------");

	//
	// If the caller wants the ICE attributes then collect them and return
	// them in the object that pAttributes points to.
	//
	if (pAttributes)
	{
		const int nNumAttributeSets = nNumMediaStreams + 1; // One entry per media stream plus the SDP message scope attributes
		RvAddress RtpAddresses[MAX_NUMBER_MEDIA_STREAMS];
		RvAddress RtcpAddresses[MAX_NUMBER_MEDIA_STREAMS];
		RvIceTransportProtocol Protocols[MAX_NUMBER_MEDIA_STREAMS];
		char *Attributes[MAX_NUMBER_MEDIA_STREAMS];
		RvSdpParseStatus Status;
		int nLength;
		RvStatus rvStatus = RV_OK;

		//
		// Allocate buffers to hold the ICE SDP attributes for
		// each media stream and the SDP header
		//
		for (int i = 0; i < MAX_NUMBER_MEDIA_STREAMS; i++)
		{
			Attributes[i] = new char[MAX_ATTRIBUTE_STRING_LENGTH + 1]{};
		}

		//
		// Fetch the attributes from the ICE stack for this session.  We have to also fetch
		// the RTP, RTCP and protocol information but we won't use them
		//
		rvStatus = RvIceFetchIceAttrsAndAddrsFromSdp (m_hStackHandle, nullptr, m_originalSdp.sdpMessageGet(), MAX_ATTRIBUTE_STRING_LENGTH, RtpAddresses, RtcpAddresses, Protocols, Attributes, &nNumMediaStreams);
		
		stiTESTCONDMSG(rvStatus == RV_OK, stiRESULT_ERROR, "Error %d with fetching Ice Attributes", rvStatus);

		//
		// Parse and construct an SDP object for the message scope ICE attributes
		//
		nLength = strlen (Attributes[0]);

		pAttributes->pMsgAttributes = rvSdpMsgConstructParse (nullptr, Attributes[0], &nLength, &Status);

		//
		// For each media stream parse and construct an SDP object for the media stream ICE attributes
		//
		for (int i = 1; i < nNumAttributeSets; i++)
		{
			nLength = strlen (Attributes[i]);

			RvSdpMsg *pSdpMessage = rvSdpMsgConstructParse (nullptr, Attributes[i], &nLength, &Status);
			if (pSdpMessage != nullptr)
			{
				pAttributes->MediaAttributes.push_back (pSdpMessage);
			}
		}

		//
		// Now free all of the previously allocated memory
		//
		for (int i = 0; i < MAX_NUMBER_MEDIA_STREAMS; i++)
		{
			delete [] Attributes[i];
			Attributes[i] = nullptr;
		}
	}
	
	*pLocalAudio = localAudio;
	*pLocalText = localText;
	*pLocalVideo = localVideo;
	*pRemoteAudio = remoteAudio;
	*pRemoteText = remoteText;
	*pRemoteVideo = remoteVideo;

STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT (RV_OK == rvStatus);

		bIterationStarted = false;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiIceSession::DefaultTransportsGet
//
// Description: Get default transports to use for media.
//
// NOTE: Any transport pointer can be NULL.
// NOTE: If you take a transport, you become the owner and must release it!
//
// Returns:
//  stiHResult
//
stiHResult CstiIceSession::DefaultTransportsGet (
	const CstiSdp &Sdp,
	CstiMediaTransports *pAudio,
	CstiMediaTransports *pText,
	CstiMediaTransports *pVideo)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RvIceMediaStreamHandle hMedia = nullptr;
	RvIceCandidateDetails CandidateDetails;
	RvChar addrTxt1[256];
	const RvSdpConnection *pConnection = nullptr;
#ifdef IPV6_ENABLED
	const char *pszRemoteIP = "[::0]";
#else
	const char *pszRemoteIP = "0.0.0.0";
#endif
	CstiMediaTransports *pMediaTransports;
	CstiMediaTransports audio;
	CstiMediaTransports text;
	CstiMediaTransports video;

	bool bIterationStarted = false;

	if (RV_OK != RvIceSessionIterationBegin (m_hStackHandle, m_hSession))
	{
		stiTHROW (stiRESULT_ERROR);
	}

	bIterationStarted = true;

	pConnection = Sdp.connectionGet();

	if (pConnection)
	{
		pszRemoteIP = rvSdpConnectionGetAddress (pConnection);
	}

	//
	// Iterate through each media section.
	//
	for (;;)
	{
		if (RV_OK != RvIceSessionIterateMediaStream (m_hSession, RV_ICE_LIST_NEXT, hMedia, &hMedia, nullptr))
		{
			break;
		}

		RvIceCandidateHandle hCandidate = nullptr;

		RvIceMediaStreamDetails MediaDetails;
		RvIceMediaGetDetails (hMedia, &MediaDetails);
		const RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorGet(MediaDetails.indexInSdpMsg);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);

		bool rtcpMuxing = false;
		if (!m_remoteSdp.empty())
		{
			rtcpMuxing = m_remoteSdp.mediaTypeContainsAttribute(rvSdpMediaDescrGetMediaTypeStr(pMediaDescriptor), "rtcp-mux");
		}

		auto  un16Port = (uint16_t)rvSdpMediaDescrGetPort (pMediaDescriptor);
		EstiMediaType eMediaType = SorensonMediaTypeGet (
			rvSdpMediaDescrGetMediaType (pMediaDescriptor),
			rvSdpMediaDescrGetMediaTypeStr (pMediaDescriptor));
		auto pConnection = rvSdpMediaDescrGetConnection (pMediaDescriptor);

		const char *pszMediaAddress = pszRemoteIP;

		if (pConnection)
		{
			const char *pszAddress = rvSdpConnectionGetAddress (pConnection);

			if (pszAddress[0] != '0')
			{
				pszMediaAddress = pszAddress;
			}
		}

		RvAddress RtpDefault;
		RvAddressConstruct (IPv4AddressValidate (pszMediaAddress) ? RV_ADDRESS_TYPE_IPV4 : RV_ADDRESS_TYPE_IPV6, &RtpDefault);
		RvAddressSetIpPort (&RtpDefault, un16Port);
		RvAddressSetString (pszMediaAddress, &RtpDefault);

		RvAddress RtcpDefault;
		RvAddressCopy (&RtpDefault, &RtcpDefault);

		RvSdpRtcp *pRtcp = rvSdpMediaDescrGetRtcp (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

		if (pRtcp)
		{
			RvAddressSetIpPort (&RtcpDefault, (RvUint16)rvSdpRtcpGetPort (pRtcp));
		}
		else
		{
			RvAddressSetIpPort (&RtcpDefault, un16Port + 1);
		}

		//
		// Set up a couple of pointers so that we don't have to keep checking
		// our media type in the inner loop.
		//
		if (estiMEDIA_TYPE_AUDIO == eMediaType && pAudio)
		{
			pMediaTransports = &audio;
		}
		else if (estiMEDIA_TYPE_TEXT == eMediaType && pText)
		{
			pMediaTransports = &text;
		}
		else if (estiMEDIA_TYPE_VIDEO == eMediaType && pVideo)
		{
			pMediaTransports = &video;
		}
		else
		{
			continue;
		}

		//
		// For each candidate in the media section, compare the default ip address and port with the
		// candidate's ip and port.  If they match then the candidate is the default and we can
		// get the transport.
		//
		for (hCandidate = nullptr;;)
		{
			//
			// Get the candidate.
			//
			if (RV_OK != RvIceMediaIterateCandidate (
				hMedia,
				RV_TRUE,
				RV_ICE_LIST_NEXT,
				hCandidate,
				&hCandidate)
			)
			{
				break;
			}

			memset (&CandidateDetails, 0, sizeof (CandidateDetails)); // keep compiler happy

			if (RV_OK != RvIceCandidateGetDetails (hCandidate, &CandidateDetails))
			{
				stiTHROW (stiRESULT_ERROR);
			}

			DBG_MSG ("   Candidate (type %s)", RvIceCandTypeEnum2Text (CandidateDetails.eType, NULL));
			DBG_MSG ("		  Local %s:%d",
					 RvAddressGetString (&CandidateDetails.address, sizeof (addrTxt1), addrTxt1), RvAddressGetIpPort (&CandidateDetails.address));

			//
			// If we want the rtp transport and the address and port match then retrieve then transport.
			// Otherwise, if we want the rtcp transport and the address and port match then retrieve the rtcp transport.
			//
			if (RvAddressCompare (&CandidateDetails.address, &RtpDefault, RV_ADDRESS_FULLADDRESS))
			{
				RvTransport RtpTransport;

				auto status = RvIceSessionCreateMediaTransport(hMedia,RV_ICE_COMPONENT_ID_RTP,&RtpTransport);
				if (status != RV_OK || !RtpTransport)
				{
					RvIceCandidateRetrieveTransport(hCandidate, &RtpTransport);
				}

				pMediaTransports->RtpTransportSet (RtpTransport, true);
				pMediaTransports->RtpLocalAddressSet (&CandidateDetails.address);
				pMediaTransports->RtpTypeSet (TransportTypeGet (CandidateDetails.eType));

				// If we are using rtcp-muxing then there is only one transport used for both rtp and rtcp.
				// Set the rtcp transport to use the same information as the rtp transport.
				if (rtcpMuxing)
				{
					pMediaTransports->RtcpTransportSet (RtpTransport, false);
					pMediaTransports->RtcpLocalAddressSet (&CandidateDetails.address);
					pMediaTransports->RtcpTypeSet (TransportTypeGet (CandidateDetails.eType));
				}
			}
			else if (!rtcpMuxing && RvAddressCompare (&CandidateDetails.address, &RtcpDefault, RV_ADDRESS_FULLADDRESS))
			{
				RvTransport RtcpTransport;

				auto status = RvIceSessionCreateMediaTransport(hMedia,RV_ICE_COMPONENT_ID_RTCP,&RtcpTransport);
				if (status != RV_OK || !RtcpTransport)
				{
					RvIceCandidateRetrieveTransport(hCandidate, &RtcpTransport);
				}

				pMediaTransports->RtcpTransportSet (RtcpTransport, true);
				pMediaTransports->RtcpLocalAddressSet (&CandidateDetails.address);
				pMediaTransports->RtcpTypeSet (TransportTypeGet (CandidateDetails.eType));
			}
			
			pMediaTransports->isValidSet(true);
			pMediaTransports->isRtcpMuxSet(rtcpMuxing);
		}
	}

	*pAudio = audio;
	*pText = text;
	*pVideo = video;

STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		RvStatus rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT(RV_OK == rvStatus);

		bIterationStarted = false;
	}

	return hResult;
}


/*!\brief This method will return transports for the host ports selected by the ICE stack.
 *
 * \param pAudioRtp A pointer to the audio rtp transport object
 * \param pAudioRtcp A pointer to the audio rtcp port object
 * \param pTextRtp A pointer to the text rtp transport object
 * \param pTextRtcp A pointer to the text rtcp port object
 * \param pVideoRtp A pointer to the video rtp transport object
 * \param pVideoRtcp A pointer to the video rtcp transport object
 */
stiHResult CstiIceSession::LocalAddressesGet (
	RvAddress *pAudioRtp,
	RvAddress *pAudioRtcp,
	RvAddress *pTextRtp,
	RvAddress *pTextRtcp,
	RvAddress *pVideoRtp,
	RvAddress *pVideoRtcp)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RvIceMediaStreamHandle hMedia = nullptr;
	RvIceCandidateDetails CandidateDetails;
	RvChar addrTxt1[256];
	RvAddress *pRtpAddress = nullptr;
	RvAddress *pRtcpAddress = nullptr;
	bool bIterationStarted = false;
	
	if (RV_OK != RvIceSessionIterationBegin (m_hStackHandle, m_hSession))
	{
		stiTHROW (stiRESULT_ERROR);
	}

	bIterationStarted = true;

	//
	// Iterate through each media.
	//
	for (;;)
	{
		if (RV_OK != RvIceSessionIterateMediaStream (m_hSession, RV_ICE_LIST_NEXT, hMedia, &hMedia, nullptr))
		{
			break;
		}

		RvIceCandidateHandle hCandidate = nullptr;

		RvIceMediaStreamDetails MediaDetails;
		RvIceMediaGetDetails (hMedia, &MediaDetails);
		const RvSdpMediaDescr *pMediaDescriptor = m_originalSdp.mediaDescriptorGet(MediaDetails.indexInSdpMsg);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);

		bool rtcpMuxing = false;
		if (!m_remoteSdp.empty())
		{
			rtcpMuxing = m_remoteSdp.mediaTypeContainsAttribute(rvSdpMediaDescrGetMediaTypeStr(pMediaDescriptor), "rtcp-mux");
		}

		EstiMediaType eMediaType = SorensonMediaTypeGet (
			rvSdpMediaDescrGetMediaType (pMediaDescriptor),
			rvSdpMediaDescrGetMediaTypeStr (pMediaDescriptor));

		//
		// Set up a couple of pointers so that we don't have to keep checking
		// our media type in the inner loop.
		//
		if (estiMEDIA_TYPE_AUDIO == eMediaType)
		{
			if (pAudioRtp)
			{
				pRtpAddress = pAudioRtp;
			}

			if (pAudioRtcp)
			{
				pRtcpAddress = pAudioRtcp;
			}
		}
		else if (estiMEDIA_TYPE_TEXT == eMediaType)
		{
			if (pTextRtp)
			{
				pRtpAddress = pTextRtp;
			}

			if (pTextRtcp)
			{
				pRtcpAddress = pTextRtcp;
			}
		}
		else if (estiMEDIA_TYPE_VIDEO == eMediaType)
		{
			if (pVideoRtp)
			{
				pRtpAddress = pVideoRtp;
			}

			if (pVideoRtcp)
			{
				pRtcpAddress = pVideoRtcp;
			}
		}
		else
		{
			continue;
		}

		//
		// Iterate through each local candidate and retrieve the associated transport.
		//
		for (hCandidate = nullptr;;)
		{
			//
			// Get the candidate.
			//
			if (RV_OK != RvIceMediaIterateCandidate (
				hMedia,
				RV_TRUE,
				RV_ICE_LIST_NEXT,
				hCandidate,
				&hCandidate)
			)
			{
				break;
			}

			memset (&CandidateDetails, 0, sizeof (CandidateDetails)); // keep compiler happy

			if (RV_OK != RvIceCandidateGetDetails (hCandidate, &CandidateDetails))
			{
				stiTHROW (stiRESULT_ERROR);
			}

			DBG_MSG ("   Candidate (type %s)", RvIceCandTypeEnum2Text (CandidateDetails.eType, NULL));
			DBG_MSG ("		  Local %s:%d",
					 RvAddressGetString (&CandidateDetails.address, sizeof (addrTxt1), addrTxt1), RvAddressGetIpPort (&CandidateDetails.address));

			//
			// If we want the rtp transport and the address and port match then retrieve then transport.
			// Otherwise, if we want the rtcp transport and the address and port match then retrieve the rtcp transport.
			//
			if (pRtpAddress && CandidateDetails.componentID == 1 && CandidateDetails.eType == RV_ICE_CANDIDATE_TYPE_HOST)
			{
				RvAddressConstruct (CandidateDetails.address.addrtype, pRtpAddress);
				RvAddressCopy (&CandidateDetails.address, pRtpAddress);

				// If we are using rtcp-muxing then there is only one transport used for both rtp and rtcp.
				// Set the rtcp transport to use the same address as the rtp transport.
				if (rtcpMuxing)
				{
					RvAddressConstruct(CandidateDetails.address.addrtype, pRtcpAddress);
					RvAddressCopy(&CandidateDetails.address, pRtcpAddress);
				}
			}
			else if (pRtcpAddress && CandidateDetails.componentID == 2 && CandidateDetails.eType == RV_ICE_CANDIDATE_TYPE_HOST)
			{
				RvAddressConstruct (CandidateDetails.address.addrtype, pRtcpAddress);
				RvAddressCopy (&CandidateDetails.address, pRtcpAddress);
			}
		}
	}

STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		RvStatus rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT(RV_OK == rvStatus);

		bIterationStarted = false;
	}

	return hResult;
}


/*!\brief This method is used to add server reflexive ports based on the current host ports and the public IP passed in.
 *
 * \param pIceSession The ICE session
 * \param pszPublicIP The public IP of the endpoint
 */
stiHResult CstiIceSession::ServerReflexivePortsAdd (
	const std::string &PublicIP,
	RvInt nAddressType)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RvIceMediaStreamHandle hMedia = nullptr;
	bool bIterationStarted = false;
	RvStatus rvStatus = RV_OK;


	if (RV_OK != RvIceSessionIterationBegin (m_hStackHandle, m_hSession))
	{
		stiTHROW (stiRESULT_ERROR);
	}

	bIterationStarted = true;

	//
	// Iterate through each media.
	//
	for (;;)
	{
		if (RV_OK != RvIceSessionIterateMediaStream (m_hSession, RV_ICE_LIST_NEXT, hMedia, &hMedia, nullptr))
		{
			break;
		}

		RvIceCandidateHandle hCandidate = nullptr;
		RvIceCandidateDetails CandidateDetails;

		//
		// Iterate through each local candidate and retrieve the associated transport.
		//
		for (hCandidate = nullptr;;)
		{
			//
			// Get the candidate.
			//
			if (RV_OK != RvIceMediaIterateCandidate (
				hMedia,
				RV_TRUE,
				RV_ICE_LIST_NEXT,
				hCandidate,
				&hCandidate)
			)
			{
				break;
			}

			memset (&CandidateDetails, 0, sizeof (CandidateDetails)); // keep compiler happy

			if (RV_OK != RvIceCandidateGetDetails (hCandidate, &CandidateDetails))
			{
				stiTHROW (stiRESULT_ERROR);
			}

			if (CandidateDetails.eType == RV_ICE_CANDIDATE_TYPE_HOST && CandidateDetails.address.addrtype == nAddressType)
			{
				RvIceCandidateHandle hNewCandidate;
				RvIceCandidateDetails NewCandidateDetails;

				RvAddressCopy (&CandidateDetails.address, &NewCandidateDetails.address);
				RvAddressSetString (PublicIP.c_str (), &NewCandidateDetails.address);

				NewCandidateDetails.transportProtocol = RV_ICE_TRANSPORT_PROTOCOL_UDP;
				NewCandidateDetails.componentID = CandidateDetails.componentID;
				NewCandidateDetails.eType = RV_ICE_CANDIDATE_TYPE_SERVER_REFLEXIVE;
				NewCandidateDetails.bLocal = RV_TRUE;
				NewCandidateDetails.hBaseCandidate = hCandidate;
				NewCandidateDetails.hAppCandidate = nullptr;
				NewCandidateDetails.priority = 0;

				rvStatus = RvIceMediaAddLocalCandidate (hMedia, nullptr, &NewCandidateDetails, hCandidate, &hNewCandidate);
				stiTESTRVSTATUS ();
			}
		}
	}

STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		RvStatus rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT(RV_OK == rvStatus);

		bIterationStarted = false;
	}

	return hResult;
}

/*!\brief Iterate over all remote ICE candidates. Return true if all are of type "host" and have an ip address in the "private" ip block
 */
bool CstiIceSession::AllRemoteCandidatesArePrivateHost ()
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	char addrText[stiIP_ADDRESS_LENGTH + 1];
	RvIceMediaStreamHandle mediaHandle = nullptr;
	RvIceCandidateDetails candidateDetails;
	bool iterationStarted = false;
	bool allCandidatesArePrivateHost = true;
	CstiIpAddress ipAddress;
	RvIceCandidateHandle candidate = nullptr;

	if (RV_OK != RvIceSessionIterationBegin (m_hStackHandle, m_hSession))
	{
		stiTHROW (stiRESULT_ERROR);
	}

	iterationStarted = true;

	// Iterate through each media descriptor.
	// Start by assuming all candidates are private host until we find an exception
	while (allCandidatesArePrivateHost &&
		   RV_OK == RvIceSessionIterateMediaStream (
						m_hSession,
						RV_ICE_LIST_NEXT,
						mediaHandle,
						&mediaHandle,
						nullptr))
	{

		RvIceMediaStreamDetails mediaDetails;
		RvIceMediaGetDetails (mediaHandle, &mediaDetails);
		const RvSdpMediaDescr *mediaDescriptor = m_originalSdp.mediaDescriptorGet (mediaDetails.indexInSdpMsg);
		stiTESTCOND (mediaDescriptor, stiRESULT_ERROR);

		EstiMediaType mediaType = SorensonMediaTypeGet (
			rvSdpMediaDescrGetMediaType (mediaDescriptor),
			rvSdpMediaDescrGetMediaTypeStr (mediaDescriptor));

		// We only care about the candidates in the video or audio conneciton.
		if (mediaType != EstiMediaType::estiMEDIA_TYPE_VIDEO &&
			mediaType != EstiMediaType::estiMEDIA_TYPE_AUDIO)
		{
			continue;
		}

		//
		// Iterate through each local candidate and retrieve the associated transport.
		//
		candidate = nullptr;
		while (allCandidatesArePrivateHost &&
			   RV_OK == RvIceMediaIterateCandidate (
							mediaHandle,
							RV_FALSE, // Iterate over remote candidates
							RV_ICE_LIST_NEXT,
							candidate,
							&candidate))
		{
			memset (&candidateDetails, 0, sizeof (candidateDetails)); // keep compiler happy

			stiTESTCOND(RV_OK == RvIceCandidateGetDetails(candidate, &candidateDetails), stiRESULT_ERROR);

			// Skip IPv6 addresses for the host check since not all networks support them
			if (candidateDetails.eType == RV_ICE_CANDIDATE_TYPE_HOST)
			{
				if (candidateDetails.address.addrtype == RV_NET_IPV4)
				{
					RvAddressGetString (&candidateDetails.address, sizeof (addrText), addrText);
					ipAddress.assign (addrText);
					if (!ipAddress.IPv4AddressIsPrivate ())
					{
						// The host candidate is a public ip address meaning we can use it.
						// Setting to false will break out of both loops on the next iteration.
						allCandidatesArePrivateHost = false;
					}
				}
			}
			else if (candidateDetails.eType != RV_ICE_CANDIDATE_TYPE_UNDEFINED)
			{
				// If its not host and not undefined then we can use it.
				// Setting to false will break out of both loops on the next iteration.
				allCandidatesArePrivateHost = false;
			}
		}
	}

STI_BAIL:

	if (iterationStarted)
	{
		//
		// End the iteration process
		//
		RvStatus rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT (RV_OK == rvStatus);

		iterationStarted = false;
	}

	return allCandidatesArePrivateHost;
}


/*!\brief This method will return transports for the host ports selected by the ICE stack.
 *
 * \param pAudioRtp A pointer to the audio rtp transport object
 * \param pAudioRtcp A pointer to the audio rtcp port object
 * \param pTextRtp A pointer to the text rtp transport object
 * \param pTextRtcp A pointer to the text rtcp port object
 * \param pVideoRtp A pointer to the video rtp transport object
 * \param pVideoRtcp A pointer to the video rtcp transport object
 */
stiHResult CstiIceSession::CandidatesList ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	RvIceMediaStreamHandle hMedia = nullptr;
	RvIceCandidateDetails CandidateDetails;
	RvChar addrTxt1[256];
	bool bIterationStarted = false;

	if (RV_OK != RvIceSessionIterationBegin (m_hStackHandle, m_hSession))
	{
		stiTHROW (stiRESULT_ERROR);
	}

	bIterationStarted = true;

	//
	// Iterat through each media.
	//
	for (;;)
	{
		if (RV_OK != RvIceSessionIterateMediaStream (m_hSession, RV_ICE_LIST_NEXT, hMedia, &hMedia, nullptr))
		{
			break;
		}

		RvIceCandidateHandle hCandidate = nullptr;

		RvIceMediaStreamDetails MediaDetails;
		RvIceMediaGetDetails (hMedia, &MediaDetails);
		const RvSdpMediaDescr *pMediaDescriptor = m_originalSdp.mediaDescriptorGet(MediaDetails.indexInSdpMsg);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);

		//
		// Iterate through each local candidate and retrieve the associated transport.
		//
		for (hCandidate = nullptr;;)
		{
			//
			// Get the candidate.
			//
			if (RV_OK != RvIceMediaIterateCandidate (
				hMedia,
				RV_TRUE,
				RV_ICE_LIST_NEXT,
				hCandidate,
				&hCandidate)
			)
			{
				break;
			}

			memset (&CandidateDetails, 0, sizeof (CandidateDetails)); // keep compiler happy

			if (RV_OK != RvIceCandidateGetDetails (hCandidate, &CandidateDetails))
			{
				stiTHROW (stiRESULT_ERROR);
			}

			stiTrace ("Candidate (type %s), ", RvIceCandTypeEnum2Text (CandidateDetails.eType, nullptr));
			stiTrace ("Local %s:%d ", RvAddressGetString (&CandidateDetails.address, sizeof (addrTxt1), addrTxt1), RvAddressGetIpPort (&CandidateDetails.address));
		}
	}

STI_BAIL:

	if (bIterationStarted)
	{
		//
		// End the iteration process
		//
		RvStatus rvStatus = RvIceSessionIterationEnd (m_hStackHandle, m_hSession);
		stiASSERT(RV_OK == rvStatus);

		bIterationStarted = false;
	}

	return hResult;
}


stiHResult CstiIceSession::RelatedSessionStart (
	size_t CallbackParam,
	CstiIceSession **ppRelatedSession)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	auto pRelatedSession = new CstiIceSession (*this);

	pRelatedSession->m_bRelatedSession = true;
	pRelatedSession->m_CallbackParam = CallbackParam;
	pRelatedSession->m_eState = estiGatheringComplete;

	rvStatus = RvIceCreateRelatedSession(m_hSession, (RvIceAppSessionHandle)pRelatedSession, &pRelatedSession->m_hSession);
	stiTESTRVSTATUS();

	*ppRelatedSession = pRelatedSession;

STI_BAIL:

	return hResult;
}


/*!\brief A utility function to get the name of the role
 *
 * \param eRole The role for which the name is to be retrieved
 *
 */
const char *IceRoleNameGet (
	CstiIceSession::EstiIceRole eRole)
{
	const char *pszRoleName = nullptr;

	switch (eRole)
	{
		case CstiIceSession::estiICERoleOfferer:

			pszRoleName = "Offerer";

			break;

		case CstiIceSession::estiICERoleAnswerer:

			pszRoleName = "Answerer";

			break;

		case CstiIceSession::estiICERoleGatherer:

			pszRoleName = "Gatherer";

			break;
	}

	return pszRoleName;
}


