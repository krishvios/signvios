////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipCall
//
//  File Name:  CstiSipCall.cpp
//
//  Abstract:
//	  This class implements the methods specific to a Call such as the actual
//	  commands to accept a call, start a call, etc. with the RADVision SIP
//	  stack.
//
////////////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "CstiSipCall.h"
#include "Sip/Message.h"
#include "Sip/Transaction.h"
#include "stiTrace.h"
#include "stiTools.h"
#include <cctype>
#include "stiPayloadTypes.h"
#include "CstiIceManager.h"
#include "stiSipTools.h"
#include "RTCPFlowControl.h"
#include "nonstd/observer_ptr.h"

// Radvision
#include "rtp.h"
#include "rtcp.h"
#include "rvsdpenc.h"
#include "rvsdpprsaux.h"
#include "rvsdpmedia.h"
#include "RvSipContactHeader.h"
#include "RvSipContentTypeHeader.h"
#include "RvSipOtherHeader.h"
#include "RvSipBody.h"
#include "RvSipSubscription.h"
#include "RvSipAllowHeader.h"
#include "RvSipViaHeader.h"
#include "RvSipTransmitter.h"
#include "CstiAudioPlayback.h"
#include "CstiAudioRecord.h"
#include "CstiTextPlayback.h"
#include "CstiTextRecord.h"
#include "CstiVideoPlayback.h"
#include "CstiVideoRecord.h"
#include "videoDefs.h"
#include "stiRemoteLogEvent.h"
#include "IstiNetwork.h"

#include <memory>
#include <sstream>
#include <boost/lexical_cast.hpp>

//
// Constants
//

// How to send hold notifications
#define HOLD_NOTIFY_INACTIVE 1 ///< Notify holds by setting the direction to a=INACTIVE

#define INBOUND_CALL_TIMER_MS 120000 // 2min
#define PERIODIC_KEYFRAME_TIMER_MS 2500
#define LOST_CONNECTION_TIMER_MS 10000
#define PERIODIC_SIP_KEEP_ALIVE 20000
#define REINVITE_CANCEL_TIMER_MS (LOST_CONNECTION_TIMER_MS + 2000) // 2 seconds longer than the lost connection timer
static const unsigned int g_unMinimumTimeBetweenPrivacyUpdates = 1000;  // ms

#define CONTACT_NAME_INDICATOR "name="
#define CONTACT_COMPANY_NAME_INDICATOR "company_name="
#define CONTACT_DIAL_STRING_INDICATOR "dial_string="
#define CONTACT_NUMBER_TYPE_INDICATOR "number_type="

//
// Constants for reinvites.
//
#define REINVITE_MAX_TRIES 10
#if defined(stiMVRS_APP)
	#define REINVITE_DELAY 1000 // Milliseconds (was set to 10 which caused issue with video privacy at start of call)
#else
	#define REINVITE_DELAY 10 // Milliseconds
#endif

const char g_szHearingStatusConnecting[] = "Connecting";
const char g_szHearingStatusConnected[] = "Connected";
const char g_szHearingStatusDisconnected[] = "Disconnected";

const std::string g_fmtpProfileLevelID = "profile-level-id";
const std::string g_fmtpMaxMBPS = "max-mbps";
const std::string g_fmtpMaxFS = "max-fs";
const std::string g_fmtpPacketizationMode = "packetization-mode";
const std::string g_fmtpProfile = "profile";

const std::string g_dhviSwitch = "DHVISwitch";
const std::string g_dhviCapabilityCheck = "DHVICapabilityCheck";
const std::string g_dhviCapable = "DHVICapable";
const std::string g_dhviCreate = "DHVICreate";
const std::string g_dhviDisconnect = "DHVIDisconnect";

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
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiSipMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )

// Uncomment the next line for more debug info concerning the call hold state
//#define DEBUG_SIP_HOLD_STATE 1

//
// Globals
//


//
// Locals
//

uint32_t CstiSipCall::m_un32SdpSessionId = 0;

//
// Class Definitions
//

//:-----------------------------------------------------------------------------
//:
//:								MEMBER FUNCTIONS
//:
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::CstiSipCall
//
// Description: Default constructor
//
// Abstract:
//  This method constructs the CstiSipCall object
//
// Returns:
//  none
//
CstiSipCall::CstiSipCall (
	const SstiConferenceParams *pstConferenceParams,
	CstiSipManager *sipManager,
	const std::string &vrsFocusedRouting,
	CstiSipCallSP bridgedSipCall,
	CstiSipCallSP dhviCall)
:
	CstiProtocolCall (pstConferenceParams, estiSIP),
	m_audioStream(
		pstConferenceParams->stMediaDirection.bInboundAudio && dhviCall == nullptr,
		pstConferenceParams->stMediaDirection.bOutboundAudio && dhviCall == nullptr),
	m_videoStream(
		pstConferenceParams->stMediaDirection.bInboundVideo && bridgedSipCall == nullptr,
		pstConferenceParams->stMediaDirection.bOutboundVideo && bridgedSipCall == nullptr),
	m_textStream(
		pstConferenceParams->stMediaDirection.bInboundText && bridgedSipCall == nullptr && dhviCall == nullptr,
		pstConferenceParams->stMediaDirection.bOutboundText && bridgedSipCall == nullptr && dhviCall == nullptr),
    m_periodicKeyframeTimer (pstConferenceParams->nAutoKeyframeSend),
    m_inboundCallTimer (INBOUND_CALL_TIMER_MS),
    m_privacyUpdateTimer (g_unMinimumTimeBetweenPrivacyUpdates),
    m_lostConnectionTimer (LOST_CONNECTION_TIMER_MS),
    m_reinviteTimer (0),
	m_reinviteCancelTimer (REINVITE_CANCEL_TIMER_MS),
    m_connectionKeepaliveTimer (PERIODIC_SIP_KEEP_ALIVE),
    m_bIsBridgedCall(bridgedSipCall != nullptr),
    m_vrsFocusedRoutingSent (vrsFocusedRouting),
    m_sendSipKeepAlives (pstConferenceParams->sendSipKeepAlive)
{
	SipManagerSet (sipManager);

	IsHoldableSet (estiTRUE);
	AllowHangupSet (estiTRUE);
	RvInt ipType = RV_ADDRESS_TYPE_IPV4;
	
	if (pstConferenceParams->bIPv6Enabled)
	{
		m_eIpAddressType = estiTYPE_IPV6;
		ipType = RV_ADDRESS_TYPE_IPV6;
	}
	
	RvAddressConstruct (ipType, &m_DefaultAudioRtpAddress);
	RvAddressConstruct (ipType, &m_DefaultAudioRtcpAddress);
	RvAddressConstruct (ipType, &m_DefaultTextRtpAddress);
	RvAddressConstruct (ipType, &m_DefaultTextRtcpAddress);
	RvAddressConstruct (ipType, &m_DefaultVideoRtpAddress);
	RvAddressConstruct (ipType, &m_DefaultVideoRtcpAddress);

	createRtpSessions ();

	// Timer to hang up call if it isn't answered
	m_signalConnections.push_back (m_inboundCallTimer.timeoutEvent.Connect (
			[this]{ HangUp(SIPRESP_TIMEOUT); }));

	// Sends periodic keyframe requests in case remote side is unable to request them
	// Initial timeout value is set by conference params
	m_signalConnections.push_back (m_periodicKeyframeTimer.timeoutEvent.Connect (
	    [this]{
		    KeyframeSend ();
			Lock ();
			if (m_stConferenceParams.nAutoKeyframeSend > 0)
			{
				m_periodicKeyframeTimer.TimeoutSet (PERIODIC_KEYFRAME_TIMER_MS);
				m_periodicKeyframeTimer.Start ();
			}
			Unlock ();
	    }));

	// Make sure we are in sync with what we've told the remote system our privacy modes are
	m_signalConnections.push_back (m_privacyUpdateTimer.timeoutEvent.Connect (
	    [this]{
		    if ((m_textRecord  && m_textPrivacyMode  != m_textRecord ->PrivacyModeGet()) ||
			    (m_videoRecord && m_videoPrivacyMode != m_videoRecord->PrivacyModeGet()))
			{
				// TODO:  Do we need to lock to use the sipManager member?
				m_poSipManager->ReInviteSend (CstiCallGet(), 0, false);
			}
	    }));

	// Timer that fires when a remote system hasn't responded to our re-invite
	m_signalConnections.push_back (m_lostConnectionTimer.timeoutEvent.Connect (
		[this]{ if (this) {
			// TODO:  Do we need to lock to modify this state?
			m_bDetectingLostConnection = false;
			CstiCallGet()->ResultSet (estiRESULT_LOST_CONNECTION);
			HangUp ();
			}
			else
			{
				stiASSERTMSG(false, "Lost Connection Detection: this pointer is null");
			}
		}));
	
	// Timer that fires when a remote system hasn't responded to our re-invite but it's not a lost connection check.
	// This is an attempt to prevent a 491 storm where our pending re-invite is preventing a remote endpoint from updating
	// their connection information through a re-INVITE.
	m_signalConnections.push_back (m_reinviteCancelTimer.timeoutEvent.Connect (
		[this]{
			RvStatus rvStatus = RvSipCallLegCancel (m_hCallLeg);
			stiRemoteLogEventSend ("Re-INVITE cancel timer fired rvStatus = %d", rvStatus);
		}));

	// Timer that signals a reinvite timeout
	m_signalConnections.push_back (m_reinviteTimer.timeoutEvent.Connect (
	    [this]{
		    DBG_MSG ("Timeout expired. Re-Inviting...");
			// We get the value for lost connection check and reset it to false before sending the re-INVITE.
			// This resolves the case where ReIniviteSend is unable to send, saves the re-INVITE out, and sets lostConnectionCheckNeeded back to true.
			bool lostConnectionCheckNeeded = lostConnectionCheckNeededGet ();
			lostConnectionCheckNeededSet (false);
			m_poSipManager->ReInviteSend (CstiCallGet(), 0, lostConnectionCheckNeeded);
	    }));

	m_signalConnections.push_back(m_connectionKeepaliveTimer.timeoutEvent.Connect(
		[this]
		{
			SipInfoSend(KEEP_ALIVE_MIME, INFO_PACKAGE, KEEP_ALIVE_PACKAGE, " ");
		}));

	++m_un32SdpSessionId;
	sprintf (m_szSessionId, "%u", m_un32SdpSessionId);
	
	if (bridgedSipCall)
	{
		m_spCallBridge = bridgedSipCall->CstiCallGet();
	}
	
	if (dhviCall)
	{
		m_dhviCall = dhviCall->CstiCallGet ();
		m_isDhviMcuCall = true;
		m_videoStream.setActive (false);
	}
	
	// Create the various data structures of our media capabilities
	m_poSipManager->PreferredAudioProtocolsCreate (&m_PreferredAudioProtocols, &m_AudioFeatureProtocols);
	m_poSipManager->PreferredTextProtocolsCreate (&m_PreferredTextProtocols);
	m_poSipManager->PreferredVideoPlaybackProtocolsCreate (&m_PreferredVideoPlaybackProtocols);
	m_poSipManager->PreferredVideoRecordProtocolsCreate (&m_PreferredVideoRecordProtocols);

} // end CstiSipCall::CstiSipCall


/*!
 * \brief Create the shared RTP/RTCP sessions for audio, text, and video
 */
void CstiSipCall::createRtpSessions ()
{
	vpe::RtpSession::SessionParams sessionParams;
	sessionParams.portBase = RV_PORTRANGE_DEFAULT_START;
	sessionParams.portRange = RV_PORTRANGE_DEFAULT_FINISH - RV_PORTRANGE_DEFAULT_START + 1;

	if (m_eIpAddressType == estiTYPE_IPV6)
	{
		sessionParams.ipv6Enabled = true;
	}

	// Set up tunneling
	if (SipManagerGet ()->TunnelActive ())
	{
		sessionParams.tunneled = true;
	}

	if (m_audioStream.supported ())
	{
		m_audioSession = std::make_shared<vpe::RtpSession>(sessionParams);
		
		m_signalConnections.push_back (m_audioSession->encryptionStateChangedSignal.Connect ([this](vpe::EncryptionState encryptionState) {
			m_encryptionStateChangesAudio.emplace_back (Poco::Timestamp (), static_cast<int> (encryptionState));
			m_poSipManager->encryptionStateChangedSignal.Emit (CstiCallGet ());
			}));
	}

	if (m_textStream.supported ())
	{
		m_textSession  = std::make_shared<vpe::RtpSession>(sessionParams);

		m_signalConnections.push_back (m_textSession->encryptionStateChangedSignal.Connect ([this](vpe::EncryptionState encryptionState) {
			m_encryptionStateChangesText.emplace_back (Poco::Timestamp (), static_cast<int> (encryptionState));
			m_poSipManager->encryptionStateChangedSignal.Emit (CstiCallGet ());
			}));
	}

	if (m_videoStream.supported ())
	{
		m_videoSession = std::make_shared<vpe::RtpSession>(sessionParams);

		m_signalConnections.push_back (m_videoSession->encryptionStateChangedSignal.Connect ([this](vpe::EncryptionState encryptionState) {
			m_encryptionStateChangesVideo.emplace_back (Poco::Timestamp (), static_cast<int> (encryptionState));
			m_poSipManager->encryptionStateChangedSignal.Emit (CstiCallGet ());
			}));
	}
}


/*!\brief Opens the rtp session if not already open.
 *
 */
stiHResult CstiSipCall::openRtpSession (
    std::shared_ptr<vpe::RtpSession> session)
{
	auto hResult = stiRESULT_SUCCESS;

	if (SipManagerGet ()->TunnelActive ())
	{
		if (session)
		{
			session->tunneledSet (true);
		}
	}

	if (session && !session->isOpen())
	{
		hResult = session->open();
		stiTESTRESULT ();
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::~CstiSipCall
//
// Description: CstiSipCall class destructor
//
// Abstract:
//  This function frees resources associated with the CstiSipCall object.
//
// Returns:
//  none
//
CstiSipCall::~CstiSipCall ()
{
	m_inboundCallTimer.Stop ();
	m_periodicKeyframeTimer.Stop ();
	m_privacyUpdateTimer.Stop ();
	m_lostConnectionTimer.Stop ();
	m_reinviteTimer.Stop ();
	m_reinviteCancelTimer.Stop ();

	ConnectionKeepaliveStop();
	
	stiASSERTMSG(m_BridgeURI.empty(), "Bridge not created");

	IceSessionsEnd();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::Accept
//
// Description: Accept an incoming call
//
// Abstract:
//
// Returns:
//  estiOK - Success
//	estiERROR - Failure
//
stiHResult CstiSipCall::Accept ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	CstiSdp sdp;
	CstiSipManager *poSipManager = SipManagerGet ();
	auto callLeg = CallLegGet (m_hCallLeg);
	auto outboundMessage = callLeg->outboundMessageGet();

	stiTESTCOND(callLeg, stiRESULT_ERROR);

	//
	// If we have an outstanding PRACK operation
	// or if the remote endpoint is sorenson sip verison 3 and we are waiting for an offer update
	// then we need to wait before
	// we can send our final 200 OK to the INVITE request.
	// Set the "call leg accepted" boolean so that when the PRACK operation
	// is complete we can call Accept.
	//
	if (callLeg->m_bWaitingForPrackCompletion || callLeg->m_bPendingOfferUpdate)
	{
		callLeg->m_bCallLegAccepted = true;
	}
	else
	{
		Lock ();
		
		m_inboundCallTimer.Stop ();
		
		Unlock ();

		//
		// If the remote endpoint is a Sorenson endpoint then
		// get the latest info for SInfo, see if it is different than what was sent earlier
		//
		if (callLeg->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))
		{
			std::string systemInfo;

			// Get a new SInfo to see if it has changed
			hResult = poSipManager->SInfoGenerate(&systemInfo, shared_from_this());
			stiTESTRESULT ();

			// compare the sinfo that was returned...
			if (m_SInfo != systemInfo)
			{
				//they are different... clear the SinfoSent flag on the call leg...
				callLeg->m_bSInfoSent = false;

				if (callLeg->m_nRemoteSIPVersion >= 2)
				{
					callLeg->SInfoHeaderAdd();
				}
				else
				{
					stiASSERT (estiFALSE);
				}
			}
		}

		// If there is no ICE session or if the ICE session has completed or failed then go ahead and start sending media
		// and prepare to receive media.  However, if there is an ICE session and nominations have not completed or failed
		// then we will wait to send and receive media once the nominations are complete.
		//
		if (callLeg->m_pIceSession == nullptr
		 || callLeg->m_pIceSession->m_eState == CstiIceSession::estiComplete
		 || callLeg->m_pIceSession->m_eState == CstiIceSession::estiNominationsFailed)
		{
			if (callLeg->isDtlsNegotiated ())
			{
				dtlsBegin (true);
			}
			else
			{
				hResult = mediaUpdate (true);
				stiTESTRESULT ();
			}
		}

		SIP_DBG_LESSER_EVENT_SEPARATORS("generating ok to initial invite");

		// Get the outbound message and add the ALLOW header to the 200 OK.
		hResult = outboundMessage.inviteRequirementsAdd();
		stiTESTRESULT ();
	
		//
		// Construct and add the sdp to the message if we have
		// an offer we have yet to answer.
		//
		if (!callLeg->m_bOfferAnswerComplete)
		{
			hResult = callLeg->SdpCreate (&sdp, estiAnswer);
			stiTESTRESULT ();

			//
			// If we were waiting to send our candidates to the remote endpoint in
			// our final response to the INVITE then add them now.
			//
			if (callLeg->m_pIceSession && callLeg->m_eNextICEMessage == estiOfferAnswerINVITEFinalResponse)
			{
				hResult = callLeg->m_pIceSession->SdpUpdate (&sdp);
				
				hResult = callLeg->m_pIceSession->SessionProceed ();
				stiTESTRESULT ();
			}

			hResult = outboundMessage.bodySet(sdp);
			stiTESTRESULT ();
		}

		//
		// Accept the call
		//
		rvStatus = RvSipCallLegAccept (m_hCallLeg);
		stiTESTRVSTATUS ();
	
		outboundMessage.clear();
	
		callLeg->m_bOfferAnswerComplete = true;
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		//
		// If we get here and there is a non-NULL handle to the outbound message
		// we need to make sure we clear its contents or they may get sent in
		// the next successful outbound message.
		//
		if (outboundMessage)
		{
			auto hResult = outboundMessage.bodyClear();
			stiASSERT(!stiIS_ERROR(hResult));
		}

		//
		// Something went wrong.  End the call.
		// First we need to go to the CONNECTED:CONFERENCING state.
		//
		poSipManager->NextStateSet (CstiCallGet (), esmiCS_CONNECTED, estiSUBSTATE_CONFERENCING);
		CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
	}
	
	return (hResult);
} // end CstiSipCall::Accept


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::BridgeCreate
//
// Description: Creates a call bridge to bridge audio from the call in progress
//              to a new call w/ address at destAddress
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipCall::BridgeCreate (const std::string &destAddress)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;
	
	CstiRoutingAddress RoutingAddress (destAddress);
	
	stiTESTCOND(!m_bIsBridgedCall, stiRESULT_ERROR); // a bridged call can't create another bridged call
	if (m_spCallBridge)
	{
		// we don't need the old one
		m_spCallBridge->HangUp();
		m_spCallBridge = nullptr;
	}
	
	// this call must have audio enabled/working to create a bridge
	stiTESTCOND(m_audioPlayback, stiRESULT_ERROR);
	stiTESTCOND(m_audioRecord, stiRESULT_ERROR);
	
	call = m_poSipManager->CallDial(estiDM_BY_DIAL_STRING, RoutingAddress, destAddress, nullptr, nullptr, shared_from_this());
	stiTESTCOND(call, stiRESULT_ERROR);
	
	m_spCallBridge = call;

	AudioBridgeOn ();
	
STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::BridgeRemoved
//
// Description: Called if the call bridge is hung up, closed or otherwise
//              ended but the main call didn't do it.
// Returns:
//	stiRESULT_SUCCESS
//
stiHResult CstiSipCall::BridgeRemoved()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_spCallBridge)
	{
		m_poSipManager->callBridgeStateChangedSignal.Emit (m_spCallBridge);
		m_spCallBridge = nullptr;
	}

// STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::BridgeDisconnect
//
// Description: Called by VRCL to remove an existing bridge
//
// Returns:
//	stiRESULT_SUCCESS
//
stiHResult CstiSipCall::BridgeDisconnect()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_spCallBridge)
	{
		m_spCallBridge->HangUp();
		m_spCallBridge = nullptr;
		m_bIsBridgedCall = false;

		 // NOTE clean up callbridge ref
		 // will happen when call bridge closes call
		 // from sipmanager thread.
	}
// STI_BAIL:
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::BridgeStateGet
//
// Description: Called by VRCL to get the call state of a call bridge
//
// Returns:
//	stiRESULT_SUCCESS
//
stiHResult CstiSipCall::BridgeStateGet (EsmiCallState *peCallState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bIsBridgedCall)
	{
		*peCallState = m_poCstiCall.lock()->StateGet ();
	}
	else if (m_spCallBridge)
	{
		*peCallState = m_spCallBridge->StateGet ();
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


stiHResult CstiSipCall::dhviCreate (const std::string &destAddress, const std::string &conferenceId,bool sendInfo)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiCallSP call = nullptr;
	
	std::string dhvAddr = destAddress;
	
	if (sendInfo)
	{
		// Attempt to format the address to use the Deaf Dialplan.
		dhvAddr = dhvUriCreate (destAddress, DEAF_DIALPLAN_URI_HEADER);
	}

	// Interpreter dialplan just begins with SVRS no need to modify.
	CstiRoutingAddress RoutingAddress (dhvAddr);
	
	call = m_poSipManager->CallDial(estiDM_BY_DIAL_STRING, RoutingAddress, dhvAddr, nullptr, nullptr, nullptr, shared_from_this ());
	stiTESTCOND(call, stiRESULT_ERROR);
	
	if (sendInfo)
	{
		sendDhviCreateMsg (destAddress);
	}
	
	m_dhviCall = call;
	m_dhviCall->m_isDhviMcuCall = true;
	
STI_BAIL:
	return hResult;
}


/*!\brief Hangs up the DHV call and switches active state of media.
* \note Ensure this is called from the SipManager thread to ensure RV doesnt have issues.
* Failing to do so can result in bugs like #30510.
*/
stiHResult CstiSipCall::dhviMcuDisconnect (bool sendReinvite)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto dhviCall = m_dhviCall;
	if (m_isDhviMcuCall)
	{
		if (dhviCall &&  m_videoStream.isActive())
		{
			auto dhvSipCall = std::static_pointer_cast<CstiSipCall>(dhviCall->ProtocolCallGet());
			dhvSipCall->switchActiveCall (sendReinvite);
		}
		hResult = HangUp ();
		stiTESTRESULT ();
	}
	else if (dhviCall && dhviCall->m_isDhviMcuCall)
	{
		if (!m_videoStream.isActive())
		{
			switchActiveCall (sendReinvite);
		}
		
		hResult = dhviCall->HangUp ();
		m_dhviCall = nullptr;
		stiTESTRESULT ();
	}
	else
	{
		stiASSERTMSG (false, "CstiSipCall::dhviMcuDisconnect Failed to find DHV call to hangup.");
	}
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::RemoteLightRingFlash
//
// Description: Flash the remote light ring
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipCall::RemoteLightRingFlash ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (m_hCallLeg, stiRESULT_ERROR);

	if (CstiCallGet ()->StateValidate (esmiCS_CONNECTED))
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		stiTESTCOND(callLeg, stiRESULT_ERROR);

		auto transaction = callLeg->createTransaction();

		auto outboundMessage = transaction.outboundMessageGet();

		hResult = outboundMessage.bodySet(NUDGE_MIME, NUDGE_MESSAGE);
		stiTESTRESULT ();

		hResult = transaction.requestSend(MESSAGE_METHOD);
		stiTESTRESULT();

		DBG_MSG ("Sent lightring flash message");
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::TextSend
//
// Description: Sends a text message to the remote client
//
// TODO: This does not ensure we have a secure connection, nor is there any
// way for the application to access it!
//
// Parameters:
// pun16Text - a NULL-terminated array of Unicode characters.
//
// Returns:
//	stiHResult
//
stiHResult CstiSipCall::TextSend (
	const uint16_t *pwszMessage,			///< The message to send
	EstiSharedTextSource sharedTextSource)	///< How text was shared 
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (nullptr == pwszMessage || pwszMessage[0] == '\0')
	{
		stiASSERTMSG (estiFALSE, "Skipped sending blank text message.");
	}
	else
	{
		// count the chars being sent.
		int i;
		int count = 0;

		// Test each character to ensure it should be logged.
		for (i = 0; pwszMessage[i] != 0; i++)
		{
			if (pwszMessage[i] != '\b')
			{
				count++;
			}
		}

		// Increment shared text chars sent by count
		m_stCallStatistics.un32ShareTextCharsSent += count;
		if (sharedTextSource == estiSTS_SAVED_TEXT)
		{
			m_stCallStatistics.un32ShareTextCharsSentFromSavedText += count;
		}
		if (sharedTextSource == estiSTS_KEYBOARD)
		{
			m_stCallStatistics.un32ShareTextCharsSentFromKeyboard += count;
		}

		stiTESTCOND (nullptr != m_hCallLeg && m_textRecord, stiRESULT_ERROR);

		m_textRecord->TextSend (pwszMessage);
		DBG_MSG ("Sent text message");
	}
	
STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::TransferInfoSet
//
// Description: Sets the current address this call is transferring to (this does
//	not actually perform the transfer.  To do that, use TransferToAddress() )
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
#define SIP_URI_FORMAT SIP_PREFIX "%s"
stiHResult CstiSipCall::TransferInfoSet (
    const std::string &destAddress,
    EstiDirection eDirection)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_transferringTo.clear ();

	if (destAddress.empty())
	{
		m_eTransferDirection = estiUNKNOWN;
	}
	else
	{
		m_eTransferDirection = eDirection;
		if (estiUNKNOWN != eDirection)
		{
			// Make sure we have a fully-qualified sip uri
			if (0 == stiStrNICmp (destAddress.c_str(), SIP_PREFIX, sizeof (SIP_PREFIX) - 1))
			{
				m_transferringTo = destAddress;
			}
			else
			{
				// It's an ip address, so convert to sip uri
				std::stringstream ss;
				ss << SIP_PREFIX << destAddress;
				m_transferringTo = ss.str ();
			}
		}
	}

#if 0
	// Disable application notification of hold state changes when transferring,
	// and re-enable it when transfer is complete
	if (estiUNKNOWN == m_eTransferDirection)
	{
		m_poCstiCall.lock()->NotifyAppOfHoldChangeSet (estiTRUE);
	}
	else
	{
		m_poCstiCall.lock()->NotifyAppOfHoldChangeSet (estiFALSE);
	}
#endif

	return hResult;
}
#undef SIP_PREFIX
#undef SIP_URI_FORMAT


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::TransferToAddress
//
// Description: Begins the process of transferring this call to another phone.
//
// Abstract: This will return an error for non-transferable calls.
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
//#define HOLD_TRANSFEREE 1 // Enable this if you want to issue a hold before transferring the call
stiHResult CstiSipCall::TransferToAddress (
    const std::string &address)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	SIP_DBG_EVENT_SEPARATORS("Transfer call to %s", address.c_str());

	// Make sure this is an OK thing to be doing
	if (address.empty() || estiTRUE != IsTransferableGet ())
	{
		// Clear the address
		TransferInfoSet ("", estiUNKNOWN);

		stiTHROW (stiRESULT_ERROR);
	}
	else
	{
		// Set up the address
		TransferInfoSet (address, estiOUTGOING);

#ifdef HOLD_TRANSFEREE
		// Hold the current call
		Hold ();

		// When the hold completes, the rest of this operation will
		// be handled in the ReferSend() function.
#else // ifndef HOLD_TRANSFEREE
		// Complete the call transfer immediately
		ReferSend ();
#endif // HOLD_TRANSFEREE
	}

STI_BAIL:

	return hResult;
}
#ifdef HOLD_TRANSFEREE
#undef HOLD_TRANSFEREE // clean up the namespace
#endif

	 
////////////////////////////////////////////////////////////////////////////////
//; ReferSend
//
// Description: Helper to send the transfer command once we are in the correct state.
//
// Returns:
//	estiOK - Success, estiERROR - Failure
//
stiHResult CstiSipCall::ReferSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;

	// Only attempt completion if we were in the process of transferring
	if (estiOUTGOING == m_eTransferDirection && !m_transferringTo.empty())
	{
		// Create a new subscription
		RvSipSubsHandle hReferSubs;
		RvSipSubsMgrHandle hSubsMgr = SipManagerGet ()->SubscriptionManagerGet ();
		
		auto callLeg = CallLegGet (m_hCallLeg);
		stiTESTCOND(callLeg, stiRESULT_ERROR);

		rvStatus = RvSipSubsMgrCreateSubscription (hSubsMgr, m_hCallLeg, (RvSipAppSubsHandle)callLeg->uniqueKeyGet(), &hReferSubs);
		stiTESTRVSTATUS ();

		SubscriptionSet(hReferSubs);
		
		// Initiate the REFER Subscription parameters
		rvStatus = RvSipSubsReferInitStr (hReferSubs, (RvChar *)m_transferringTo.c_str(), nullptr, nullptr);
		stiTESTRVSTATUS ();

		// Send the REFER request
		rvStatus = RvSipSubsRefer (hReferSubs);
		stiTESTRVSTATUS ();
	}
	
STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::AudioBridgeOn
//
// Description: Force all audio to turn on (used when call bridging)
//
// Returns:
//	nothing
//
void CstiSipCall::AudioBridgeOn ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiSipCallSP sipCallBridge = nullptr;
	
	if (!m_spCallBridge)
	{
		stiTHROW (stiRESULT_ERROR);
	}
	sipCallBridge = std::static_pointer_cast<CstiSipCall>(m_spCallBridge->ProtocolCallGet());
	
	if (m_audioPlayback)
	{
		// Ensure existing channel is unmuted
		m_audioPlayback->Resume (estiHOLD_LOCAL);
		m_audioPlayback->Resume (estiHOLD_REMOTE);
		m_audioPlayback->PrivacyModeSet (estiOFF);
	}
	
	// Make sure bridge audio playback is valid
	if (sipCallBridge->m_audioPlayback)
	{
		// Ensure existing channel is unmuted
		sipCallBridge->m_audioPlayback->Resume (estiHOLD_LOCAL);
		sipCallBridge->m_audioPlayback->Resume (estiHOLD_REMOTE);
		sipCallBridge->m_audioPlayback->PrivacyModeSet (estiOFF);
	}

	if (m_audioRecord)
	{
		// Ensure existing channel is unmuted
		m_audioRecord->Resume (estiHOLD_LOCAL);
		m_audioRecord->Resume (estiHOLD_REMOTE);
		m_audioRecord->PrivacyModeSet (estiOFF);
	}

	// Make sure bridge audio record is valid
	if (sipCallBridge->m_audioRecord)
	{
		// Ensure existing channel is unmuted
		sipCallBridge->m_audioRecord->Resume (estiHOLD_LOCAL);
		sipCallBridge->m_audioRecord->Resume (estiHOLD_REMOTE);
		sipCallBridge->m_audioRecord->PrivacyModeSet (estiOFF);
	}

	// Make sure our m_poAudioPlaybackBridge is established
	if (nullptr == m_poAudioPlaybackBridge)
	{
		DBG_MSG ("Call %p creating audio playback bridge", m_poCstiCall.lock().get());
		m_poAudioPlaybackBridge = std::make_shared<CstiAudioBridge>();
		stiTESTCOND (m_poAudioPlaybackBridge, stiRESULT_MEMORY_ALLOC_ERROR);
	}

	if (m_audioPlayback)
	{
		m_audioPlayback->AudioOutputDeviceSet (m_poAudioPlaybackBridge);
	}

	// Make sure bridge m_poAudioPlaybackBridge is established
	if (nullptr == sipCallBridge->m_poAudioPlaybackBridge)
	{
		DBG_MSG ("Call %p creating audio playback bridge", m_spCallBridge.get());
		sipCallBridge->m_poAudioPlaybackBridge = std::make_shared<CstiAudioBridge>();
		stiTESTCOND (sipCallBridge->m_poAudioPlaybackBridge, stiRESULT_MEMORY_ALLOC_ERROR);
	}

	if (m_audioRecord)
	{
		m_audioRecord->AudioInputDeviceSet (sipCallBridge->m_poAudioPlaybackBridge);
	}

STI_BAIL:

	stiUNUSED_ARG (hResult); // Prevent compiler warning
	
}


/*!
 * \brief Called from SipManager thread when video playback's privacy mode changes
 */
void CstiSipCall::eventVideoPlaybackPrivacyModeChanged (
    bool muted)
{
	VideoPlaybackMuteSet ((EstiSwitch)muted);
}


/*!
 * \brief Called from SipManager thread when video record's privacy mode changes
 */
void CstiSipCall::eventVideoRecordPrivacyModeChanged (
    bool muted)
{
	SIP_DBG_EVENT_SEPARATORS("Application turned video mute %s", (muted? "ON" : "OFF"));
	privacyModeChange (m_videoRecord, muted, &m_videoPrivacyMode);
}


/*!
 * \brief Called from SipManager thread when audio record's privacy mode changes
 */
void CstiSipCall::eventAudioRecordPrivacyModeChanged (
    bool muted)
{
	if (!m_bIsBridgedCall && !m_spCallBridge)
	{
		if (m_videoRecord)
		{
			auto audioBitRate = muted ? 0 : static_cast<uint32_t> (m_audioRecord->MaxChannelRateGet ());
			m_videoRecord->ReserveBitRate (audioBitRate);
		}
		SIP_DBG_EVENT_SEPARATORS("Application turned audio mute %s", (muted? "ON" : "OFF"));
	}
	else
	{
		DBG_MSG ("Rejected attempt to mute audio on bridged call.");

		// Turn it back on
		AudioBridgeOn ();
	}
}


/*!
 * \brief Called from SipManager thread when text record's privacy mode changes
 */
void CstiSipCall::eventTextRecordPrivacyModeChanged (
    bool muted)
{
	SIP_DBG_EVENT_SEPARATORS("Application turned text mute %s", (muted? "ON" : "OFF"));

	// Apparently we do nothing when textRecord goes into privacy mode
	// But do something when it comes out of it
	if (!muted)
	{
		privacyModeChange (m_textRecord, muted, &m_textPrivacyMode);
	}
}

/*!
* \brief Event handler for when VideoRecord notifies us of HoldServer video completed
*/
void CstiSipCall::eventHSVideoFileCompleted ()
{
	CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NONE);
}

/*!
 * \brief Helper method for when the privacy mode changes
 */
void CstiSipCall::privacyModeChange (
    std::shared_ptr<CstiDataTaskCommonP> dataTask,
    bool muted,
    EstiSwitch *privacyMode)
{
	dataTask->PrivacyModeSet ((EstiSwitch)muted);
	auto call = CstiCallGet ();

	if (call->StateValidate (esmiCS_CONNECTED) &&
	    !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD | estiSUBSTATE_NEGOTIATING_LCL_HOLD))
	{
		// Don't allow for a SPAM attack with some trigger-happy privacy individual.
		if (m_unLastPrivacyChangeTime < (stiOSTickGet () - g_unMinimumTimeBetweenPrivacyUpdates))
		{
			if (privacyMode)
			{
				*privacyMode = dataTask->PrivacyModeGet();
			}
			DBG_MSG ("privacyModeChanged: Sending %s notification...", (muted? "mute" : "unmute"));
			m_poSipManager->ReInviteSend (call, 0, false);
		}
		else
		{
			// Set a timer to come back and clean up after things have settled down.
			m_privacyUpdateTimer.Start ();
		}
		m_unLastPrivacyChangeTime = stiOSTickGet ();
	}
}


/*!
 * \brief Called by SipManager when textPlayback receives text
 */
void CstiSipCall::eventTextPlaybackRemoteTextReceived (
	const uint16_t *textBuffer)
{
	// Count the received chars...
	int i = 0;
	if (textBuffer && textBuffer[0])
	{
		// set i to 1 since we know there is a char in textBuffer[0]
		for (i = 1; textBuffer[i] != 0; i++)
		{
		}
	}
	// add i to count of chars received
	m_stCallStatistics.un32ShareTextCharsReceived += i;
}


/*!\brief Assigns the remote addresses to the transports based on the best media for each type.
 *
 */
void CstiSipCall::RemoteTransportAddressesSet (
	bool inboundMedia,
	EstiMediaType mediaType,
	const std::list<SstiPreferredMediaMap> &preferredMedia,
	CstiMediaTransports *transports)
{
	if (inboundMedia)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			SstiSdpStreamP sdpStream = callLeg->BestSdpStreamGet(
				mediaType,
				&preferredMedia);

			if (sdpStream && RvAddressGetIpPort(&sdpStream->RtpAddress) != 0)
			{
				transports->RtpRemoteAddressSet(&sdpStream->RtpAddress);
				transports->RtcpRemoteAddressSet(&sdpStream->RtcpAddress);
			}
		}
	}
}


/*!\brief For each media type, assigns the remote addresses to the transports based on the best media for each type.
 *
 */
void CstiSipCall::RemoteTransportAddressesSet (
	CstiMediaTransports *audioTransports,
	CstiMediaTransports *textTransports,
	CstiMediaTransports *videoTransports)
{
	//
	// Set the remote address for the audio transport
	//
	RemoteTransportAddressesSet (m_audioStream.supported (),
			estiMEDIA_TYPE_AUDIO, m_PreferredAudioProtocols, audioTransports);

	//
	// Set the remote address for the text transport
	//
	RemoteTransportAddressesSet (m_textStream.supported (),
			estiMEDIA_TYPE_TEXT, *PreferredTextPlaybackProtocolsGet (), textTransports);

	//
	// Set the remote address for the video transport
	//
	RemoteTransportAddressesSet (m_videoStream.supported (),
			estiMEDIA_TYPE_VIDEO, *PreferredVideoPlaybackProtocolsGet (), videoTransports);

#ifndef GDPR_COMPLIANCE
	transportsCallTrace (__func__, audioTransports, textTransports, videoTransports);
#endif
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::TransportsSet
//
// Description: Sets the transport connections for all channels.
//
// Returns:
//	Nothing
//
void CstiSipCall::IncomingTransportsSet (
	const CstiMediaTransports &audioTransports,
	const CstiMediaTransports &textTransports,
	const CstiMediaTransports &videoTransports)
{
	//
	// Store the transport types in the call object for logging purposes.
	//
	EstiMediaTransportType eRtpTransportAudio = estiMediaTransportUnknown;
	EstiMediaTransportType eRtcpTransportAudio = estiMediaTransportUnknown;
	EstiMediaTransportType eRtpTransportText = estiMediaTransportUnknown;
	EstiMediaTransportType eRtcpTransportText = estiMediaTransportUnknown;
	EstiMediaTransportType eRtpTransportVideo = estiMediaTransportUnknown;
	EstiMediaTransportType eRtcpTransportVideo = estiMediaTransportUnknown;

	auto call = CstiCallGet ();

#ifndef GDPR_COMPLIANCE
	transportsCallTrace (__func__, &audioTransports, &textTransports, &videoTransports);
#endif

	if (audioTransports.isValid()
	 && m_audioStream.supported ())
	{
		if (m_audioPlayback)
		{
			m_audioPlayback->transportsSet (audioTransports);
		}
		else
		{
			m_audioSession->transportsSet (audioTransports);
		}

		eRtpTransportAudio = audioTransports.RtpTypeGet ();
		eRtcpTransportAudio = audioTransports.RtcpTypeGet ();

		//
		// Log where we will send Audio.
		//
		std::string remoteAddress;
		int remotePort = 0;
		m_audioSession->RemoteAddressGet (nullptr, &remotePort, nullptr, nullptr);
		call->AudioRemotePortSet (remotePort);

		//
		// Log where we will receieve Audio
		//
		int localPort = 0;
		m_audioSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
		call->AudioPlaybackPortSet (localPort);
	}

	if (textTransports.isValid()
	 && (m_textStream.supported ()))
	{
		if (m_textPlayback)
		{
			m_textPlayback->transportsSet (textTransports);
		}
		else
		{
			m_textSession->transportsSet (textTransports);
		}

		eRtpTransportText = textTransports.RtpTypeGet ();
		eRtcpTransportText = textTransports.RtcpTypeGet ();

		//
		// Log where we will send Text.
		//
		std::string remoteAddress;
		int remotePort = 0;
		m_textSession->RemoteAddressGet (nullptr, &remotePort, nullptr, nullptr);
		call->TextRemotePortSet (remotePort);

		//
		// Log where we will receieve Text
		//
		int localPort = 0;
		m_textSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
		call->TextPlaybackPortSet (localPort);
	}

	if (videoTransports.isValid()
	 && m_videoStream.supported ())
	{
		if (m_videoPlayback)
		{
			m_videoPlayback->transportsSet (videoTransports);
		}
		else
		{
			m_videoSession->transportsSet (videoTransports);
		}

		eRtpTransportVideo = videoTransports.RtpTypeGet ();
		eRtcpTransportVideo = videoTransports.RtcpTypeGet ();

		//
		// Log where we will send Video.
		//
		std::string remoteAddress;
		int remotePort = 0;
		m_videoSession->RemoteAddressGet (&remoteAddress, &remotePort, nullptr, nullptr);
		call->RemoteVideoAddrSet (remoteAddress.c_str ());
		call->VideoRemotePortSet (remotePort);

		//
		// Log where we will receieve Video
		//
		std::string localAddress;
		int localPort = 0;
		m_videoSession->LocalAddressGet (&localAddress, &localPort, nullptr, nullptr);
		call->LocalVideoAddrSet (localAddress.c_str ());
		call->VideoPlaybackPortSet (localPort);
	}

	call->MediaTransportTypesSet(
		eRtpTransportAudio, eRtcpTransportAudio,
		eRtpTransportText, eRtcpTransportText,
		eRtpTransportVideo, eRtcpTransportVideo);
}


/*!\brief Retrieves the default transports from the ICE session and applies them to the RTP session with remote addresses
 *
 */
stiHResult CstiSipCall::DefaultTransportsApply (
	CstiSipCallLegSP callLeg)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMediaTransports audioTransports;
	CstiMediaTransports textTransports;
	CstiMediaTransports videoTransports;
	auto call = callLeg->m_sipCallWeak.lock ()->CstiCallGet ();

	call->collectTrace (formattedString ("%s", __func__));

	hResult = callLeg->m_pIceSession->DefaultTransportsGet (
	    callLeg->m_sdp,
		&audioTransports,
		&textTransports,
		&videoTransports);
	stiTESTRESULT ();

	RemoteTransportAddressesSet (
			&audioTransports,
			&textTransports,
			&videoTransports);

	IncomingTransportsSet (
		audioTransports,
		textTransports,
		videoTransports);

STI_BAIL:

	call->collectTrace (formattedString ("%s: result %d", __func__, hResult));

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::DefaultAddressesSet
//
// Description: Sets the default addresses for all media channels.
//
// Returns:
//	Nothing
//
void CstiSipCall::DefaultAddressesSet (
	const RvAddress *pAudioRtpAddress,
	const RvAddress *pAudioRtcpAddress,
	const RvAddress *pTextRtpAddress,
	const RvAddress *pTextRtcpAddress,
	const RvAddress *pVideoRtpAddress,
	const RvAddress *pVideoRtcpAddress)
{
	RvAddressCopy (pAudioRtpAddress, &m_DefaultAudioRtpAddress);
	RvAddressCopy (pAudioRtcpAddress, &m_DefaultAudioRtcpAddress);
	RvAddressCopy (pTextRtpAddress, &m_DefaultTextRtpAddress);
	RvAddressCopy (pTextRtcpAddress, &m_DefaultTextRtcpAddress);
	RvAddressCopy (pVideoRtpAddress, &m_DefaultVideoRtpAddress);
	RvAddressCopy (pVideoRtcpAddress, &m_DefaultVideoRtcpAddress);

#ifndef GDPR_COMPLIANCE
	char audioRtp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pAudioRtpAddress, sizeof audioRtp, audioRtp);
	char audioRtcp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pAudioRtcpAddress, sizeof audioRtcp, audioRtcp);
	char videoRtp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pVideoRtpAddress, sizeof videoRtp, videoRtp);
	char videoRtcp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pVideoRtcpAddress, sizeof videoRtcp, videoRtcp);
	char textRtp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pTextRtpAddress, sizeof textRtp, textRtp);
	char textRtcp[stiIP_ADDRESS_LENGTH + 1];
	RvAddressGetString (pTextRtcpAddress, sizeof textRtcp, textRtcp);
	CstiCallGet()->collectTrace (formattedString (
		"%s: Video(%s, %s), Audio(%s, %s), Text(%s, %s)",
		__func__, videoRtp, videoRtcp, audioRtp, audioRtcp, textRtp, textRtcp));
#endif
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::ReInviteSend
//
// Description: Issue a sip re-invite with the current channel state
//	This should be done whenever bandwidth, mute, or hold state changes.
//
stiHResult CstiSipCall::ReInviteSend (
	bool bLostConnectionDetection, ///< Ignore the re-invite if there is already one in progress (in case all we want to know is if remote is alive)
	int nCurrentRetryCount) ///< Used to track how many times we have re-tried
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSdp Sdp;
	RvStatus rvStatus = RV_OK;
	
	SIP_DBG_EVENT_SEPARATORS("Sending re-INVITE");
	
	EsmiCallState eState = m_poCstiCall.lock()->StateGet ();
	
	//
	// We only want to send a re-invite if we are in a type of connected state
	//
	if (esmiCS_CONNECTED == eState
	 || esmiCS_HOLD_LCL == eState
	 || esmiCS_HOLD_RMT == eState
	 || esmiCS_HOLD_BOTH == eState)
	{
		auto callLeg = CallLegGet(m_hCallLeg);
		if (!callLeg)
		{
			CallEnd (estiSUBSTATE_ERROR_OCCURRED, SIPRESP_INTERNAL_ERROR);
			stiTESTCOND (callLeg, stiRESULT_ERROR);
		}
		
		// If we have an ICE session and the state is still nominating we are not configured to
		// send a Re-Invite. Doing so will cause problems like disabling media for the call.
		if (!callLeg->m_pIceSession ||
			 callLeg->m_pIceSession->m_eState != CstiIceSession::estiNominating)
		{
			
			// Create a new message
			RvSipCallLegInviteHandle hReInvite;
			auto outboundMessage = callLeg->outboundMessageGet();
			
			rvStatus = RvSipCallLegReInviteCreate (m_hCallLeg, nullptr, &hReInvite);
			stiTESTRVSTATUS ();
			
			// If geolocation is needed (emergency call), add it.
			if (CstiCallGet ()->geolocationRequestedGet ())
			{
				hResult = outboundMessage.geolocationAdd ();
				stiTESTRESULT ();
			}

			{
				// For now, extended headers aren't necessary for calling, so still send re-invite.
				auto result = outboundMessage.additionalHeadersAdd();
				stiASSERTMSG(result == stiRESULT_SUCCESS, "ReinviteSend additionalHeadersAdd returned error");
			}

			{
				// This is new functionality so if it fails we may still want to send the Re-Invite.
				stiHResult result = m_poSipManager->CallLegContactAttach (m_hCallLeg);
				stiASSERTMSG(result == stiRESULT_SUCCESS, "ReinviteSend CallLegContactAttach returned error");
			}
			
			// Set up the message contents
			hResult = callLeg->SdpCreate (&Sdp, estiOffer);
			stiTESTRESULT ();
			
			if (callLeg->m_pIceSession)
			{
				callLeg->m_pIceSession->SdpUpdate (&Sdp);
			}
			
			hResult = outboundMessage.bodySet(Sdp);
			stiTESTRESULT ();
			
			// Send the message
			rvStatus = RvSipCallLegReInviteRequest (m_hCallLeg, hReInvite);
			
			if (RV_OK == rvStatus)
			{
				DBG_MSG ("Re-invite sent.");
				
				callLeg->m_bAnswerPending = true;
				
				//
				// If this re-invite was for lost connection detection then start the
				// lost connection detection timer.
				//
				if (bLostConnectionDetection)
				{
					// Set a timer.  If it fires before we receive a response, we will hangup the call.
					Lock ();
					if (!m_bDetectingLostConnection)
					{
						m_bDetectingLostConnection = true;
						m_lostConnectionTimer.Start ();
					}
					Unlock ();
				}
				Lock ();
				m_reinviteCancelTimer.Start ();
				Unlock ();
			}
			else if (nCurrentRetryCount >= REINVITE_MAX_TRIES)
			{
				hResult = outboundMessage.bodyClear();
				stiTESTRESULT ();

				//
				// If this request is marked as "ignore if pending" then it can be thrown
				// away if there already is a request outstanding.
				//
				// Also, if we have tried to send this request too many times then
				// just let it go away.
				//
				DBG_MSG ("Re-invite ignored.");
			}
			else if (RV_ERROR_TRY_AGAIN == rvStatus || RV_ERROR_ILLEGAL_ACTION == rvStatus)
			{
				if (bLostConnectionDetection)
				{
					lostConnectionCheckNeededSet (bLostConnectionDetection);
				}
				
				hResult = outboundMessage.bodyClear();
				stiTESTRESULT ();
				
				// If there is already a re-invite negotiation going on, we need to wait our turn
				InviteInsideOfReInviteCheck ();				
			}
			else
			{
				stiTESTRVSTATUS ();
			}
		}
		else
		{
			stiASSERTMSG(estiFALSE, "Attempted to send Re-Invite before ICE nominations have completed.");
		}
	}
	
STI_BAIL:

	return hResult;
}


/*!\brief Add directionality based on allowed media send and receive capabilities
 *
 * \param bInboundAllowed Inbound media is allowed
 * \param bOutboundAllowed Outbound media is allowed
 * \param pMediaDescriptor The media descriptor to which to add directionality
 */
static void DirectionalityAdd (
	bool bInboundAllowed,
	bool bOutboundAllowed,
	RvSdpMediaDescr *pMediaDescriptor)
{
	if (bInboundAllowed && bOutboundAllowed)
	{
		rvSdpMediaDescrSetConnectionMode (pMediaDescriptor, RV_SDPCONNECTMODE_SENDRECV);
	}
	else if (bInboundAllowed)
	{
		rvSdpMediaDescrSetConnectionMode (pMediaDescriptor, RV_SDPCONNECTMODE_RECVONLY);
	}
	else if (bOutboundAllowed)
	{
		rvSdpMediaDescrSetConnectionMode (pMediaDescriptor, RV_SDPCONNECTMODE_SENDONLY);
	}
	else
	{
		rvSdpMediaDescrSetConnectionMode (pMediaDescriptor, RV_SDPCONNECTMODE_INACTIVE);
	}
}


/*!\brief Adds the audio payloads we support to the media descriptor passed in.
 *
 * \param pMediaDescriptor The media to which the payloads will be added.
 */
stiHResult CstiSipCall::SdpAudioPayloadsAdd (
	RvSdpMediaDescr *pMediaDescriptor,
	bool streamEnabled)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_audioStream.supported ())
	{
		//
		// Add the sample rate for the audio codecs.
		// Note: this needs to be revisited when adding G.722.
		//
		if (streamEnabled)
		{
			char szSampleRate[8];
			sprintf (szSampleRate, "%d", m_poSipManager->G711AudioPlaybackMaxSampleRateGet ());
			rvSdpMediaDescrAddAttr (pMediaDescriptor, "ptime", szSampleRate);
		}

		// Add all the playback protocols we support
		for (auto it = m_PreferredAudioProtocols.cbegin (); it !=  m_PreferredAudioProtocols.cend (); it++)
		{
			//
			// Check to see if this codec and packetization scheme has an associated payload number.
			//
			int nPayloadTypeNbr = -1;

			AudioPayloadMapGet ((EstiAudioCodec)(*it).nCodec, estiPktUnknown, (*it).audioClockRate, &nPayloadTypeNbr);

			if (nPayloadTypeNbr == -1)
			{
				nPayloadTypeNbr = (*it).n8PayloadTypeNbr;

				if (INVALID_DYNAMIC_PAYLOAD_TYPE == nPayloadTypeNbr)
				{
					hResult = AudioPayloadDynamicIDGet (&nPayloadTypeNbr);
					stiTESTRESULT ();
				}

				AudioPayloadMappingAdd (nPayloadTypeNbr, (EstiAudioCodec)(*it).nCodec, estiPktUnknown, nAUDIO_CLOCK_RATE);
			}

			rvSdpMediaDescrAddPayloadNumber (pMediaDescriptor, nPayloadTypeNbr);

			if (streamEnabled)
			{
				rvSdpMediaDescrAddRtpMap (pMediaDescriptor, nPayloadTypeNbr, it->protocolName.c_str (), nAUDIO_CLOCK_RATE);
			}
		}

		//
		// Currently we only have telephone event that we are adding, and it cannot be present in a bridged call.
		// If we add more telephone events then we will need to test for audio DTMF event and remove them from a
		// bridged call.
		//
		if (!m_bIsBridgedCall)
		{
			// Add all the extra feature protocols we support
			for (auto it = m_AudioFeatureProtocols.cbegin (); it !=  m_AudioFeatureProtocols.cend (); it++)
			{
				//
				// Check to see if this codec and packetization scheme has an associated payload number.
				//
				int nPayloadTypeNbr = -1;

				AudioPayloadMapGet ((EstiAudioCodec)(*it).nCodec, estiPktUnknown, (*it).audioClockRate , &nPayloadTypeNbr);

				if (nPayloadTypeNbr == -1)
				{
					nPayloadTypeNbr = (*it).n8PayloadTypeNbr;

					if (INVALID_DYNAMIC_PAYLOAD_TYPE == nPayloadTypeNbr)
					{
						if (SstiPreferredMediaMap::PREFERRED_AUDIO_FEATURE_DYNAMIC_PAYLOAD_TYPE_NONE == it->preferredAudioFeatureDynamicPayloadType)
						{
							hResult = AudioPayloadDynamicIDGet (&nPayloadTypeNbr);
							stiTESTRESULT ();
						}
						else
						{
							nPayloadTypeNbr = it->preferredAudioFeatureDynamicPayloadType;
						}
					}

					AudioPayloadMappingAdd (nPayloadTypeNbr, (EstiAudioCodec)(*it).nCodec, estiPktUnknown, nAUDIO_CLOCK_RATE);
				}

				rvSdpMediaDescrAddPayloadNumber (pMediaDescriptor, nPayloadTypeNbr);

				if (streamEnabled)
				{
					rvSdpMediaDescrAddRtpMap (pMediaDescriptor, nPayloadTypeNbr, it->protocolName.c_str (), nAUDIO_CLOCK_RATE);
				}
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Adds the text payloads we support to the media descriptor passed in.
 *
 * \param pMediaDescriptor The media to which the payloads will be added.
 */
stiHResult CstiSipCall::SdpTextPayloadsAdd (
	RvSdpMediaDescr *pMediaDescriptor,
	bool streamEnabled)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_textStream.supported ())
	{
		int nT140Payload = -1;
		int nT140RedPayload = -1;

		auto protocols = PreferredTextPlaybackProtocolsGet();
		stiTESTCOND(protocols, stiRESULT_ERROR);

		// Add all the playback protocols we support
		for (auto it = protocols->cbegin ();
				it != protocols->cend ();
				it++)
		{
			//
			// Check to see if this codec and packetization scheme has an associated payload number.
			//
			int nPayloadTypeNbr = -1;

			TextPayloadMapGet ((EstiTextCodec)(*it).nCodec, estiPktUnknown, &nPayloadTypeNbr);

			if (nPayloadTypeNbr == -1)
			{
				nPayloadTypeNbr = (*it).n8PayloadTypeNbr;

				if (INVALID_DYNAMIC_PAYLOAD_TYPE == nPayloadTypeNbr)
				{
					hResult = TextPayloadDynamicIDGet (&nPayloadTypeNbr);
					stiTESTRESULT ();
				}

				TextPayloadMappingAdd (nPayloadTypeNbr, (EstiTextCodec)(*it).nCodec, estiPktUnknown);
			}

			rvSdpMediaDescrAddPayloadNumber (pMediaDescriptor, nPayloadTypeNbr);

			if (streamEnabled)
			{
				rvSdpMediaDescrAddRtpMap (pMediaDescriptor, nPayloadTypeNbr, it->protocolName.c_str (), 1000);

				// Save the Payload Type of both the T.140 and T.140-red.
				switch ((*it).eCodec)
				{
					case estiT140_TEXT:
							nT140Payload = nPayloadTypeNbr;
							break;

					case estiT140_RED_TEXT:
							nT140RedPayload = nPayloadTypeNbr;
							break;

					case estiH263_VIDEO:
					case estiH263_1998_VIDEO:
					case estiH264_VIDEO:
					case estiH265_VIDEO:
					case estiCODEC_UNKNOWN:
					case estiG711_ALAW_56K_AUDIO:
					case estiG711_ALAW_64K_AUDIO:
					case estiG711_ULAW_56K_AUDIO:
					case estiG711_ULAW_64K_AUDIO:
					case estiG722_48K_AUDIO:
					case estiG722_56K_AUDIO:
					case estiG722_64K_AUDIO:
					case estiTELEPHONE_EVENT:
					case estiRTX:
						// ignore these
							break;
				}
			}
		}

		if (streamEnabled)
		{
			// If we have both a T.140 and a T.140-red, add an fmtp attribute
			if (nT140Payload > -1 && nT140RedPayload > -1)
			{
				// Add protocol-specific attributes
				char szFmtp[stiSDP_MAX_FMTP_SIZE];
				snprintf (szFmtp, sizeof (szFmtp), "%d %d/%d/%d", nT140RedPayload, nT140Payload, nT140Payload, nT140Payload);
				rvSdpMediaDescrAddFmtp (pMediaDescriptor, szFmtp);
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Adds the video payloads we support to the media descriptor passed in.
 *
 * \param pMediaDescriptor The media to which the payloads will be added.
 */
stiHResult CstiSipCall::SdpVideoPayloadsAdd (
	RvSdpMediaDescr *pMediaDescriptor,
	bool streamEnabled,
	EstiSDPRole role)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_videoStream.supported ())
	{
		auto callLeg = CallLegGet(nullptr);

		stiTESTCOND(callLeg, stiRESULT_ERROR);

		// TODO: Remove code to strip H264 when we  no longer utilize the SIP to H323 Gateway.
		// If this call will be sent through the SIP to H323 Gateway we do not want to advertise H264 video.
		bool bRemoveH264 = callLeg->m_sipCallWeak.lock()->m_bAddSipToH323Header;

		for (auto it = m_PreferredVideoPlaybackProtocols.cbegin (); it !=  m_PreferredVideoPlaybackProtocols.cend (); it++)
		{
			// Get the profile(s) for this type
			for (auto ProfileIt = (*it).Profiles.begin ();
					ProfileIt != (*it).Profiles.end ();
					ProfileIt++)
			{
				// Get the packetization scheme for this type
				EstiPacketizationScheme ePacketizationScheme = estiPktUnknown;
				auto PktIt = (*it).PacketizationSchemes.begin ();

				if (!(*it).PacketizationSchemes.empty ())
				{
					ePacketizationScheme = (*PktIt);
				}

				//
				// Check to see if this codec, profile and packetization scheme has an associated payload number.
				//
				int nPayloadTypeNbr = -1;

				VideoPayloadMapGet ((EstiVideoCodec)(*it).nCodec, (*ProfileIt).eProfile, ePacketizationScheme, &nPayloadTypeNbr);

				if (nPayloadTypeNbr == -1)
				{
					// Add the mapping to the acceptable list
					nPayloadTypeNbr = (*it).n8PayloadTypeNbr;

					if (INVALID_DYNAMIC_PAYLOAD_TYPE == nPayloadTypeNbr)
					{
						hResult = VideoPayloadDynamicIDGet (&nPayloadTypeNbr);
						stiTESTRESULT ();
					}

					VideoPayloadMappingAdd (
						nPayloadTypeNbr,
						(EstiVideoCodec)(*it).nCodec,
						(*ProfileIt).eProfile,
						ePacketizationScheme,
						{
							RtcpFeedbackType::TMMBR,
							RtcpFeedbackType::FIR,
							RtcpFeedbackType::PLI,
							RtcpFeedbackType::NACKRTX,
						});
				}

				rvSdpMediaDescrAddPayloadNumber (pMediaDescriptor, nPayloadTypeNbr);

				if (streamEnabled)
				{
					if (!bRemoveH264 || it->protocolName != "H264")
					{
						rvSdpMediaDescrAddRtpMap (pMediaDescriptor, nPayloadTypeNbr, it->protocolName.c_str (), 90000);
					}

					// Add protocol-specific attributes
					char szFmtp[stiSDP_MAX_FMTP_SIZE];
					switch ((*it).eCodec)
					{
						case estiH263_VIDEO:
						case estiH263_1998_VIDEO:
							if (INVALID_DYNAMIC_PAYLOAD_TYPE == (*it).n8PayloadTypeNbr)
							{
								snprintf (szFmtp, sizeof (szFmtp), "%d profile=0;CIF=1;QCIF=1", nPayloadTypeNbr);
								rvSdpMediaDescrAddFmtp (pMediaDescriptor, szFmtp);
							}
							else
							{
								snprintf (szFmtp, sizeof (szFmtp), "%d CIF=1;QCIF=1", nPayloadTypeNbr);
								rvSdpMediaDescrAddFmtp (pMediaDescriptor, szFmtp);
							}
							sdpVideoRtxPayloadsAdd(nPayloadTypeNbr, pMediaDescriptor);
							break;

						case estiH264_VIDEO:
						{
							if (!bRemoveH264)
							{
								SstiH264Capabilities stH264Caps;
								ProtocolManagerGet ()->PlaybackCodecCapabilitiesGet (estiVIDEO_H264, &stH264Caps);

								// Do we have a custom Max MBPS?
								std::string maxmbps;
								if (stH264Caps.unCustomMaxMBPS > 0)
								{
									std::stringstream tmp;
									tmp << ";max-mbps=" << stH264Caps.unCustomMaxMBPS;
									maxmbps = tmp.str ();
								}

								// Do we have a custom Max FS?
								std::string maxfs;
								if (stH264Caps.unCustomMaxFS > 0)
								{
									std::stringstream tmp;
									tmp << ";max-fs=" << stH264Caps.unCustomMaxFS;
									maxfs = tmp.str ();
								}

								// Add SDP line for highest supported packetization mode
								// Lower packetization modes are inferred for receipt
								if ((*it).PacketizationSchemes.empty ())
								{
									snprintf (szFmtp, sizeof (szFmtp),
											"%d profile-level-id=%02x%02x%02x%s%s",
											nPayloadTypeNbr,
											(*ProfileIt).eProfile,
											(*ProfileIt).un8Constraints,
											stH264Caps.eLevel,
											maxmbps.c_str(),
											maxfs.c_str ());
									rvSdpMediaDescrAddFmtp (pMediaDescriptor, szFmtp);
									
									sdpVideoRtxPayloadsAdd(nPayloadTypeNbr, pMediaDescriptor);
								}
								else
								{
									int nPacketizationMode = ((EstiPacketizationScheme)(*PktIt)) - estiH264_SINGLE_NAL;
									snprintf (szFmtp, sizeof (szFmtp),
											"%d profile-level-id=%02x%02x%02x;packetization-mode=%d%s%s",
											nPayloadTypeNbr,
											(*ProfileIt).eProfile,
											(*ProfileIt).un8Constraints,
											stH264Caps.eLevel,
											nPacketizationMode,
											maxmbps.c_str(),
											maxfs.c_str());
									rvSdpMediaDescrAddFmtp (pMediaDescriptor, szFmtp);
									
									sdpVideoRtxPayloadsAdd(nPayloadTypeNbr, pMediaDescriptor);
								}
							}
							break;
						}

						case estiH265_VIDEO:
						{
							SstiH265Capabilities stH265Caps;
							ProtocolManagerGet()->PlaybackCodecCapabilitiesGet(estiVIDEO_H265, &stH265Caps);
							//These parameters are optional, but are included here.
							snprintf(szFmtp, sizeof(szFmtp),
								"%d tier-flag=%d;profile-id=%d;level-id=%d",
								nPayloadTypeNbr,
								stH265Caps.eTier, //Tier Flag
								(*ProfileIt).eProfile, //Profile ID
								stH265Caps.eLevel); //Level ID
							rvSdpMediaDescrAddFmtp(pMediaDescriptor, szFmtp);
							
							sdpVideoRtxPayloadsAdd(nPayloadTypeNbr, pMediaDescriptor);
							break;
						}

						case estiCODEC_UNKNOWN:
						case estiG711_ALAW_56K_AUDIO:
						case estiG711_ALAW_64K_AUDIO:
						case estiG711_ULAW_56K_AUDIO:
						case estiG711_ULAW_64K_AUDIO:
						case estiG722_48K_AUDIO:
						case estiG722_56K_AUDIO:
						case estiG722_64K_AUDIO:
						case estiT140_TEXT:
						case estiT140_RED_TEXT:
						case estiTELEPHONE_EVENT:
						case estiRTX:
							// ignore these.
							break;

						// default: Leave this commented out for type checking
					}
				}
			}
		}

		if (streamEnabled)
		{
			// Report that we support FIR, if it's not explicitly disabled and
			// this is either an offer or the Offer we received supported FIR.
			if (m_stConferenceParams.rtcpFeedbackFirEnabled && (role == estiOffer || callLeg->m_firOffered))
			{
				rvSdpMediaDescrAddAttr (pMediaDescriptor, "rtcp-fb", "* ccm fir");
			}

			// Report that we support NACK PLI, if it's not explicitely disabled and
			// this is either an offer or the Offer we received supported PLI.
			if (m_stConferenceParams.rtcpFeedbackPliEnabled && (role == estiOffer || callLeg->m_pliOffered))
			{
				rvSdpMediaDescrAddAttr (pMediaDescriptor, "rtcp-fb", "* nack pli");
			}
	
			// Report that we support TMMBR, if it's not explicitly disabled,
			// remote device is supported for signaling, this is either an offer
			// or the Offer we received supported TMMBR , and the device is not configured to run in legacy mode
			if (((m_stConferenceParams.rtcpFeedbackTmmbrEnabled == SignalingSupport::SorensonOnly && ((role == estiOffer || callLeg->m_tmmbrOffered) && RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))) ||
				 (m_stConferenceParams.rtcpFeedbackTmmbrEnabled == SignalingSupport::Everyone && (role == estiOffer || callLeg->m_tmmbrOffered)
				  )) &&
				m_stConferenceParams.eAutoSpeedSetting != estiAUTO_SPEED_MODE_LEGACY)
			{
				rvSdpMediaDescrAddAttr (pMediaDescriptor, "rtcp-fb", "* ccm tmmbr");
			}
			
			// Report that we support NACK, if it's not explicitly disabled,
			// remote device is supported for signaling, and this is either an offer
			// or the Offer we received supported NACK.
			if ((m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::SorensonOnly && ((role == estiOffer || callLeg->m_nackOffered) && RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))) ||
				(m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::Everyone && (role == estiOffer || callLeg->m_nackOffered)
				))
			{
				rvSdpMediaDescrAddAttr (pMediaDescriptor, "rtcp-fb", "* nack");
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Adds the RTX video payloads to the media descriptor passed in.
 *
 * \param payloadType The payload type this RTX maps to.
 * \param mediaDescriptor The media to which the payloads will be added.
 */
stiHResult CstiSipCall::sdpVideoRtxPayloadsAdd (
	int payloadType,
	RvSdpMediaDescr *mediaDescriptor)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto callLeg = CallLegGet(nullptr);
	
	stiTESTCONDMSG(callLeg,stiRESULT_ERROR, "Unable to Validate CallLeg To Add RTX");
	if (callLeg->m_nRemoteSIPVersion < webrtcSSV &&
		((m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::SorensonOnly
		 && RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE)) ||
		m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::Everyone))
	{
		int rtxPayLoadType = -1;
		std::stringstream fmtp;
		auto existingPayload = m_mVideoPayloadMappings.find(payloadType);
		
		stiTESTCONDMSG (existingPayload != m_mVideoPayloadMappings.end(), stiRESULT_ERROR ,
						"Attempting to add RTX payload but Payload Type %d was not found", payloadType);
		
		if (existingPayload->second.rtxPayloadType == INVALID_DYNAMIC_PAYLOAD_TYPE)
		{
			hResult = VideoPayloadDynamicIDGet (&rtxPayLoadType);
			stiTESTRESULT ();
			
			existingPayload->second.rtxPayloadType = rtxPayLoadType;
			
			VideoPayloadMappingAdd (
				rtxPayLoadType,
				estiVIDEO_RTX,
				estiPROFILE_NONE,
				estiPktUnknown,
				{
					RtcpFeedbackType::TMMBR,
					RtcpFeedbackType::FIR,
					RtcpFeedbackType::PLI,
					RtcpFeedbackType::NACKRTX,
				});
		}
		else
		{
			rtxPayLoadType = existingPayload->second.rtxPayloadType;
		}
		
		rvSdpMediaDescrAddPayloadNumber (mediaDescriptor, rtxPayLoadType);
		rvSdpMediaDescrAddRtpMap (mediaDescriptor, rtxPayLoadType, "rtx", nVIDEO_CLOCK_RATE);
		fmtp << rtxPayLoadType << " apt=" << payloadType;
		rvSdpMediaDescrAddFmtp(mediaDescriptor, fmtp.str().c_str());
	}
	
STI_BAIL:
	
	return hResult;
}


/*!
 *
 * \brief Create the SDP message body in a new offer (INVITE) message
 *
 * Features rolling dynamic ID's to help prevent unwanted data.
 *
 * NOTE: sometimes this needs to be called without a call leg.
 * Not sure if this is a good idea, but in that case, it has to be "hacked".
 * Maybe more work is necessary with this...
 *
 * \return estiOK - Success, estiERROR - Failure
 */
stiHResult CstiSipCall::InitialSdpOfferCreate (
	CstiSdp *pSdp,
	bool bCreateChannels)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLeg = CallLegGet(nullptr);
	std::shared_ptr<CstiDataTaskCommonP> pTextChannel;
	std::shared_ptr<CstiDataTaskCommonP> pAudioChannel;
	std::shared_ptr<CstiDataTaskCommonP> pVideoChannel;
	int nAudioRtpPort = 0;
	int nTextRtpPort = 0;
	int nVideoRtpPort = 0;
	int nMaxAudioRate = 0;
//	int nMaxTextRate = 0; stiUNUSED_ARG (nMaxTextRate);
	int nMaxVideoRate = 0;
	CstiSdp Sdp;
	bool bConvertedToSIP = false;
	RvSdpProtocol rtpProtocol = RV_SDPPROTOCOL_RTP;

	stiTESTCOND(callLeg, stiRESULT_ERROR);

	Sdp.initialize (nullptr);

	hResult = callLeg->SdpHeaderAdd (&Sdp);
	stiTESTRESULT ();
	
	// If this call will be sent through the SIP to H323 Gateway we do not want to advertise H264 video.
	bConvertedToSIP = callLeg->m_sipCallWeak.lock()->m_bAddSipToH323Header;
	
	if (bCreateChannels)
	{
		// Create incoming/playback media channels
		hResult = IncomingMediaInitialize ();
		stiTESTRESULT ();

		pAudioChannel = m_audioPlayback;
		pTextChannel = m_textPlayback;
		pVideoChannel = m_videoPlayback;

		if (nullptr == pAudioChannel && nullptr == pTextChannel && nullptr == pVideoChannel)
		{
			// If there is no playback media, then we must create the record
			// media channels now in order to get rtp port numbers
			hResult = OutgoingMediaOpen (false);
			if (stiIS_ERROR (hResult))
			{
				stiASSERTMSG (estiFALSE, "Unable to open record channels.");
			}
			else
			{
				pAudioChannel = m_audioRecord;
				pTextChannel = m_textRecord;
				pVideoChannel = m_videoRecord;
			}
		}

		if (pAudioChannel)
		{
			RvRtpSession hRtpSession = m_audioSession->rtpGet ();

			if (hRtpSession)
			{
				nAudioRtpPort = RvRtpGetPort (hRtpSession);
			}

			nMaxAudioRate = pAudioChannel->MaxChannelRateGet () / 1000;
		}
		
		if (pTextChannel)
		{
			RvRtpSession hRtpSession = m_textSession->rtpGet ();

			if (hRtpSession)
			{
				nTextRtpPort = RvRtpGetPort (hRtpSession);
			}

//			nMaxTextRate = pTextChannel->MaxChannelRateGet () / 1000;
		}
		
		if (pVideoChannel)
		{
			RvRtpSession hRtpSession = m_videoSession->rtpGet ();

			if (hRtpSession)
			{
				nVideoRtpPort = RvRtpGetPort (hRtpSession);
			}

			nMaxVideoRate = m_stConferenceParams.GetMaxRecvSpeed(estiSIP) / 1000;
		}
	}
	else
	{
		if (m_audioStream.supported ())
		{
			nAudioRtpPort = RV_PORTRANGE_DEFAULT_START;
		}

		// currently, we're only supporting
		// audio for bridged calls.  This causes the outgoing
		// invite to not send any video media lines.
		if (m_videoStream.supported ())
		{
			nVideoRtpPort = SipManagerGet ()->GetChannelPortBase () + 2;

			//
			// If we are not going to handle inbound audio then adjust
			// the video data rate so that it uses the max receive speed.
			//
			if (m_audioStream.inboundSupported ())
			{
				nMaxAudioRate = 64;
			}

			nMaxVideoRate = m_stConferenceParams.GetMaxRecvSpeed(estiSIP) / 1000;
		}

		if (m_textStream.supported ())
		{
			nTextRtpPort = SipManagerGet ()->GetChannelPortBase () + 4;
		}
	}

	// Clear any streams since they are being created again
	if(!callLeg->localSdpStreamsAreEmpty())
	{
		callLeg->localSdpStreamsClear();
	}

	// Offer SAVP?

	// NOTE: linphone SAVP even when it isn't required, and
	// then follows up with another offer if it's rejected

	// IMTC best practices dictate offering AVP with a crypto line to
	// allow but not require encryption
	switch(m_stConferenceParams.eSecureCallMode)
	{
	case estiSECURE_CALL_NOT_PREFERRED:
	case estiSECURE_CALL_PREFERRED:
		rtpProtocol = RV_SDPPROTOCOL_RTP;
		break;
	case estiSECURE_CALL_REQUIRED:
		rtpProtocol = RV_SDPPROTOCOL_RTP_SAVP;
		break;
	}

	// Add all the audio codecs we support
	if (m_audioStream.supported ())
	{
		RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorAdd (RV_SDPMEDIATYPE_AUDIO, nAudioRtpPort, rtpProtocol);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);
		rvSdpMediaDescrAddBandwidth (pMediaDescriptor, "AS", nMaxAudioRate);

		// Clear the existing protocol list
		AudioPayloadMappingsClear ();

		SdpAudioPayloadsAdd (pMediaDescriptor, true);

		//
		// Add directionality for audio
		//
		DirectionalityAdd (m_audioStream.inboundSupported (),
				m_audioStream.outboundSupported (),
				pMediaDescriptor);

		if (includeEncryptionAttributes ())
		{
			callLeg->encryptionAttributesAdd (pMediaDescriptor, estiMEDIA_TYPE_AUDIO);
		}
	}

	//
	// Add all the video codecs we support
	//
	if (m_videoStream.supported ())
	{
		// Add all the video codecs we support
		RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorAdd(RV_SDPMEDIATYPE_VIDEO, nVideoRtpPort, rtpProtocol);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);

		rvSdpMediaDescrAddBandwidth (pMediaDescriptor, "AS", nMaxVideoRate);

		VideoPayloadMappingsClear ();

		SdpVideoPayloadsAdd (pMediaDescriptor, true, estiOffer);

		//
		// Add directionality for video
		//
		DirectionalityAdd (m_videoStream.inboundSupported () && m_videoStream.isActive (),
				m_videoStream.outboundSupported () && m_videoStream.isActive(),
				pMediaDescriptor);
		
		// If we have signaling for only Sorenson devices set a flag to send a Re-Invite
		// if the remote device is a Sorenson only device.
		if (m_stConferenceParams.rtcpFeedbackTmmbrEnabled == SignalingSupport::SorensonOnly ||
			m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::SorensonOnly)
		{
			callLeg->m_sendNewOfferToSorenson = true;
		}

		if (includeEncryptionAttributes ())
		{
			callLeg->encryptionAttributesAdd (pMediaDescriptor, estiMEDIA_TYPE_VIDEO);
		}
	}

	// Add all the text codecs we support
	if (m_textStream.supported ()
		&& !bConvertedToSIP)
	{
		RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorAdd(RV_SDPMEDIATYPE_UNKNOWN, nTextRtpPort, rtpProtocol);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);
		
		// Radvision doesn't have a TEXT media type.  We added it as unknown and now will manually change it to "text".
		rvSdpMediaDescrSetMediaTypeStr (pMediaDescriptor, "text");

		// Clear the existing protocol list
		TextPayloadMappingsClear ();
		
		SdpTextPayloadsAdd (pMediaDescriptor, true);
		
		//
		// Add directionality for text
		//
		DirectionalityAdd (m_textStream.inboundSupported (),
				m_textStream.outboundSupported (),
				pMediaDescriptor);

		if (includeEncryptionAttributes ())
		{
			callLeg->encryptionAttributesAdd (pMediaDescriptor, estiMEDIA_TYPE_TEXT);
		}
	}

	//
	// We have successfully created the sdp message.
	// Pass it back to the caller.
	//
	*pSdp = Sdp;

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::RemoteDisplayNameDetermine
//
// Description: Retrieve the remote SIP Header information
//
// Abstract:
//
// Returns:
//	This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiSipCall::RemoteDisplayNameDetermine (
	RvSipCallLegHandle hCallLeg)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string RemoteDisplayName;
	EstiBool bNameObtained = estiFALSE;
	char *pszBuf = nullptr;

	// See if we can find one in the From or To headers.
	RvSipPartyHeaderHandle hPartyHeader = nullptr;
	EstiDirection eDirection = CstiCallGet ()->DirectionGet ();
	if (estiINCOMING == eDirection)
	{
		// Retrieve the "From" message header
		RvSipCallLegGetFromHeader (hCallLeg, &hPartyHeader);
	}
	else
	{
		// For outgoing calls,
		// Can we find it in the "To" header?
		RvSipCallLegGetToHeader (hCallLeg, &hPartyHeader);
	}

	// Did we get the header successfully?
	if (nullptr != hPartyHeader)
	{
		// Get the display name
		unsigned int unLength =
			RvSipPartyHeaderGetStringLength (hPartyHeader,
						RVSIP_PARTY_DISPLAY_NAME);
		pszBuf = new char [unLength + 1];
		stiTESTCOND (pszBuf, stiRESULT_ERROR);

		RvStatus rvStatus = RvSipPartyHeaderGetDisplayName (
			hPartyHeader, pszBuf, unLength + 1, &unLength);
		if (RV_OK == rvStatus)
		{
			pszBuf[unLength - 1] = '\0'; // Make sure the string is terminated

			UnescapeDisplayName (pszBuf, &RemoteDisplayName);
			if (!RemoteDisplayName.empty ())
			{
				bNameObtained = estiTRUE;
			}
		} // end else if

		// If it failed to find it for any other reason than
		// it simply wasn't there, set the result to error.
		else if (RV_NotFound != rvStatus)
		{
			stiTHROW (stiRESULT_ERROR);
		} // end else if
	} // end if

	if (!bNameObtained)
	{
		// Do we have the name from SInfo?
		CstiCallGet ()->RemoteDisplayNameGet (&RemoteDisplayName);
		if (!RemoteDisplayName.empty ())
		{
			// Keep name that was already set via SInfo.
			bNameObtained = estiTRUE;
		}
	} // end if

	// 
	// Store the display name if we actually received one.
	//
	if (bNameObtained)
	{
		// Set display name and print an automation trace for it
		RemoteDisplayNameSet (RemoteDisplayName.c_str ());
	}

STI_BAIL:

	if (pszBuf)
	{
		delete [] pszBuf;
		pszBuf = nullptr;
	}

	return (hResult);
} // end CstiSipCall::RemoteDisplayNameDetermine


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::Hold
//
// Description: Hold or resume a call (local hold/resume)
//
// Abstract:
//
// Returns:
//	This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiSipCall::Hold (
	bool bHoldState ///< true=hold false=resume
)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Notify the app -- very first thing.
	if (HoldStateSet (true, bHoldState)) // If there was a change
	{
		if (bHoldState)
		{
			SIP_DBG_EVENT_SEPARATORS("Application turned hold ON");

			// Stop the media immediately so messages travel faster.
			mediaUpdate (false);

			// We only want to call ReInviteSend when not going through the gateway,
			// this is due to issues with reinvites with 3rd party the gateways.
			if (!m_bGatewayCall)
			{
				// Notify remote side.
				hResult = ReInviteSend (false, 0);
				stiTESTRESULT ();
			}
			else
			{
				HoldStateComplete(true);
			}

			if (m_spCallBridge)
			{
				m_spCallBridge->Hold();
			}
		}
		else
		{
			SIP_DBG_EVENT_SEPARATORS("Application turned hold OFF");

			// Resume playback before telling remote side, so no data is lost.
			IncomingMediaUpdate ();

			// If we are coming off of hold with a gateway call, complete the hold and call OutgoingMediaOpen.
			if (!m_bGatewayCall)
			{
				// Notify remote side.
				hResult = ReInviteSend (false, 0);
				stiTESTRESULT ();
			}
			else
			{
				HoldStateComplete(true);
				OutgoingMediaOpen(false);
			}

			if (m_spCallBridge)
			{
				m_spCallBridge->Resume();
			}
			// Do not resume record until a response is received.
			//OutgoingMediaOpen ();
		}
	}

STI_BAIL:

	return hResult;
} // end CstiSipCall::Hold


stiHResult CstiSipCall::mediaUpdate (bool isReply)
{
	auto hResult = IncomingMediaUpdate ();
	stiASSERTRESULT (hResult);

	hResult = OutgoingMediaOpen (isReply);
	stiASSERTRESULT (hResult);

	// Setup the audio bridge
	if (!m_BridgeURI.empty ())
	{
		stiRemoteLogEventSend("EventType=BridgeCreate Reason=MediaUpdate Called With URI=%s", m_BridgeURI.c_str());
		hResult = BridgeCreate (m_BridgeURI);
		if (stiIS_ERROR (hResult))
		{
			stiASSERTMSG (false, "Failed to Create Bridge");
		}
		else
		{
			// Clear the URI if we succeeded
			m_BridgeURI.clear ();
		}
	}

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::MediaClose
//
// Description: Close all media
//
// Abstract: MUST BE CALLED FROM A CstiSipManager EVENT!
//
// Returns:
//	This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiSipCall::MediaClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	Lock ();

	CstiCallGet ()->collectTrace (std::string (__func__));

	m_periodicKeyframeTimer.Stop ();

	// Now, for each channel object, tell it to close operations
	if (m_audioRecord)
	{
		m_audioRecord->Close ();
	}
	if (m_textRecord)
	{
		m_textRecord->Close ();
	}
	if (m_videoRecord)
	{
		m_videoRecord->Close ();
	}
	if (m_audioPlayback)
	{
		m_audioPlayback->Close ();
	}
	if (m_textPlayback)
	{
		m_textPlayback->Close ();
	}
	if (m_videoPlayback)
	{
		m_videoPlayback->Close ();
	}

	// Now that the channels are closed, close the rtp/rtcp sessions
	if (m_audioSession)
	{
		m_audioSession->close ();
	}
	if (m_textSession)
	{
		m_textSession->close ();
	}
	if (m_videoSession)
	{
		m_videoSession->close ();
	}

	//
	// If there is an ICE session then shut it down.
	//
	IceSessionsEnd();

	Unlock ();

	return hResult;
}


/*!
 * \brief Start the connectionKeepaliveTimer
 */
void CstiSipCall::ConnectionKeepaliveStart ()
{
	if (!m_sendSipKeepAlives)
	{
		return;
	}

	Lock ();

	if (m_hCallLeg)
	{
		m_connectionKeepaliveTimer.TypeSet(CstiTimer::Type::Repeat);
		m_connectionKeepaliveTimer.TimeoutSet (PERIODIC_SIP_KEEP_ALIVE);
		m_connectionKeepaliveTimer.Start ();
	}

	Unlock ();
}


/*!\brief Cancels the call keep alive timer.
 * 
 */
void CstiSipCall::ConnectionKeepaliveStop ()
{
	m_connectionKeepaliveTimer.Stop ();
	
	// If we have a keep alive transmitter terminate it now.
	if (m_hKeepAliveTransmitter)
	{
		RvSipTransmitterTerminate (m_hKeepAliveTransmitter);
		m_hKeepAliveTransmitter = nullptr;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::CallEnd
//
// Description: Helper function to end the given call regardless of what state
//	it is in.
//
// Abstract:
//
// Returns: Nothing  (This function never fails.)
//
void CstiSipCall::CallEnd (
	EstiSubState newSubState, ///<! The DISCONNECTING substate to go to
	SipResponse sipRejectCode) ///<! Sip reject code to send
{
	Lock ();

	SIP_DBG_EVENT_SEPARATORS("CallEnd %p", this);

	CstiSipManager *poSipManager = SipManagerGet ();
	auto call = CstiCallGet ();
	
	call->collectTrace (formattedString ("%s: newSubState %#010x, sipRejectCode %d", __func__, newSubState, sipRejectCode));

	//
	// Cancel the lost connection timer
	//
	LostConnectionTimerCancel ();

	// If we have set a timer for sending a re-invite, we need to cancel it and release the reference to the call object.
	ReinviteTimerCancel ();
	
	ReinviteCancelTimerCancel ();
	
	//
	// If we have a subscription then unsubscribe from it.
	//
	if (m_hSubscription && TransferDirectionGet() == estiOUTGOING)
	{
		RvSipSubsUnsubscribe (m_hSubscription);
		m_hSubscription = nullptr;
	}
	
	// Stop any keyframe watches
	m_periodicKeyframeTimer.Stop ();

	ConnectionKeepaliveStop ();

	if (m_spCallBridge)
	{
		if (m_bIsBridgedCall)
		{
			// We are the bridged call so hang up ourselves
			BridgeRemoved (); // Send event
		}
		else
		{
			// Hang up the bridged call
			m_spCallBridge->HangUp();
		}
		
		m_poAudioPlaybackBridge = nullptr;
		m_spCallBridge = nullptr;
	}

	{
		auto dhviCall = m_dhviCall;
		// If the main call is closing then disconnect the MCU call.
		if (dhviCall && dhviCall->m_isDhviMcuCall)
		{
			dhviCall->HangUp ();
			m_dhviCall = nullptr;
		}
	}
	
	//
	// Shut down any media
	//
	MediaClose ();

	// Must transition through disconnecting state.
	if (esmiCS_DISCONNECTING != call->StateGet () && esmiCS_DISCONNECTED != call->StateGet ())
	{
		switch (newSubState)
		{
			case estiSUBSTATE_UNREACHABLE:
			    if (esmiCS_CONNECTING == call->StateGet () || esmiCS_IDLE == call->StateGet ())

				{
					// If there was no established call, a lost invite means the dialed phone is unreachable
					call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
				}
				else
				{
					// If we were in a call, a lost invite means that the network connection has been broken
					call->ResultSet (estiRESULT_LOST_CONNECTION);
					newSubState = estiSUBSTATE_ERROR_OCCURRED;
				}
				break;
				
			case estiSUBSTATE_ERROR_OCCURRED:
				
			    call->ResultSet (estiRESULT_LOST_CONNECTION);
				break;

			default:
				break;
		}

		poSipManager->NextStateSet (call, esmiCS_DISCONNECTING, newSubState);
		
		// If it was a LOOKUP then we never got our redirected notification,
		// therefore send an error up to the app.

		// If the call was never valid, it will not have been automatically removed
		// from call storage.  We must do it manually now.
		if (!CallValidGet ())
		{
			poSipManager->NextStateSet (call, esmiCS_DISCONNECTED, estiSUBSTATE_NONE);
			poSipManager->CallObjectRemove (call);
		}
		else
		{
			if (call->LeaveMessageGet()   // Any non-zero value is valid here.
			    && (call->ResultGet() == estiRESULT_REMOTE_SYSTEM_UNREACHABLE
			            || call->ResultGet() == estiRESULT_REMOTE_SYSTEM_BUSY
			            || call->ResultGet() == estiRESULT_REMOTE_SYSTEM_REJECTED
			            || call->ResultGet() == estiRESULT_HANGUP_AND_LEAVE_MESSAGE))
			{
				call->SignMailInitiatedSet(true);

				// Go to the disconnected state.
				poSipManager->NextStateSet (call, esmiCS_DISCONNECTED, estiSUBSTATE_LEAVE_MESSAGE);
			}
			else if (call->SubstateGet () == estiSUBSTATE_CREATE_VRS_CALL)
			{
				// Do not set this call as disconnected. It will be turned into a VRS call.
			}
			else
			{
				poSipManager->NextStateSet (call, esmiCS_DISCONNECTED, estiSUBSTATE_NONE);
			}
		}
		
		if (m_hCallLeg)
		{
			auto callLeg = CallLegGet(m_hCallLeg);
			RvSipCallLegState eCallLegState;
			RvStatus eStatus = RvSipCallLegGetCurrentState (m_hCallLeg, &eCallLegState);
			
			stiASSERT (RV_OK == eStatus);
			
			if (RV_OK == eStatus)
			{
				switch (eCallLegState)
				{
					case RVSIP_CALL_LEG_STATE_OFFERING:
					{
						if (sipRejectCode == SIPRESP_NONE)
						{
							stiASSERT (false);
							sipRejectCode = SIPRESP_INTERNAL_ERROR;
						}
						else if (sipRejectCode == SIPRESP_NOT_ACCEPTABLE_HERE)
						{
							// When rejecting with NOT_ACCEPTABLE_HERE, add an SDP body
							// to indicate what this endpoint accepts.
							CstiSdp sdp;
							auto outboundMessage = callLeg->outboundMessageGet();

							InitialSdpOfferCreate (&sdp, false);

							auto hResult = outboundMessage.bodySet(sdp);
							stiASSERT(!stiIS_ERROR(hResult));
						}

						RvSipCallLegReject (m_hCallLeg, sipRejectCode);
						
						break;
					}
					
					case RVSIP_CALL_LEG_STATE_IDLE:
					case RVSIP_CALL_LEG_STATE_INVITING:
					{
						RvSipCallLegDisconnect (m_hCallLeg);
						
						break;
					}
					
					case RVSIP_CALL_LEG_STATE_DISCONNECTING:
					case RVSIP_CALL_LEG_STATE_DISCONNECTED:
					case RVSIP_CALL_LEG_STATE_TERMINATED:
						
						break;
						
					default:
					{
						RvSipCallLegDisconnect (m_hCallLeg);
					}
				}
			}
		}

		ForkedCallLegsTerminate ();
	}

	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::HangUp
//
// Description: Hangup a call
//
// Abstract:
//
// Returns:
//	This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiSipCall::HangUp (
    SipResponse sipRejectCode)
{
	CstiSipManager *sipManager = SipManagerGet ();
	return sipManager->CallHangup (CstiCallGet(), sipRejectCode);
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::KeyframeSend
//
// Description: Send a keyframe now.
//
// Abstract:
//
// Returns:
//  stiRESULT_ERROR on failure
//
stiHResult CstiSipCall::KeyframeSend (
	EstiBool bWasRequestedByRemote) ///< Turn off failsafe if we are successfully receiving keyframe requests
{
	auto hResult = stiRESULT_SUCCESS;

	if (estiTRUE == bWasRequestedByRemote)
	{
		Lock ();
		m_stConferenceParams.nAutoKeyframeSend = 0; // Never restart for this call!
		m_periodicKeyframeTimer.Stop ();
		Unlock ();
	}

	stiTESTCOND (m_videoRecord, stiRESULT_ERROR);

	hResult = m_videoRecord->RequestKeyFrame ();
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}


/*!
* \brief Sends a keyframe request to the remote system via XML SIP message
*/
void CstiSipCall::eventVideoPlaybackKeyframeRequest ()
{
	stiLOG_ENTRY_NAME (CstiSipCall::eventVideoPlaybackKeyframeRequest);

	auto hResult = stiRESULT_SUCCESS;

	// Never request a keyframe if we are muted.  That makes no sense!
	if (m_hCallLeg && m_videoPlayback && estiOFF == m_videoPlayback->PrivacyModeGet())
	{
		// Only ask for keyframes when the call is connected.
		RvSipCallLegState callLegState;
		auto rvStatus = RvSipCallLegGetCurrentState (m_hCallLeg, &callLegState);
		stiTESTRVSTATUS ();

		if (RVSIP_CALL_LEG_STATE_CONNECTED == callLegState)
		{
			auto callLeg = CallLegGet (m_hCallLeg);
			stiTESTCOND(callLeg, stiRESULT_ERROR);

			auto transaction = callLeg->createTransaction();

			auto outboundMessage = transaction.outboundMessageGet();

			hResult = outboundMessage.bodySet(MEDIA_CONTROL_MIME, KEYFRAME_REQUEST_MESSAGE);
			stiTESTRESULT ();

			hResult = transaction.requestSend(INFO_METHOD);
			stiTESTRESULT ();

			DBG_MSG ("Call-leg %p XML keyframe request sent", m_hCallLeg);
		}
	}

STI_BAIL:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::InboundCallTimerStart
//
// Description: Start (or restart) the timer to hang up inbound call after so many seconds
//
// Returns:
//  nothing
//
void CstiSipCall::InboundCallTimerStart ()
{
	Lock ();
	m_inboundCallTimer.Start ();
	Unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::RemoteTextSupportedSet
//
// Description: Sets wether or not we can send text to the remote endpoint.
//
// Abstract: This will be set to true only if both the local endpoint has text enabled
// and if the remote endpoint has signaled text capabilities.
//
// Returns:
//  nothing
//
void CstiSipCall::RemoteTextSupportedSet(EstiBool bRemoteTextSupported)
{
	if (m_textStream.outboundSupported ())
	{
		m_bRemoteTextSupported = bRemoteTextSupported;
	}
	else
	{
		m_bRemoteTextSupported = estiFALSE;
	}
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::TextSupportedGet
//
// Description: Doese the remote endpoint support text and do we have text enabled?
//
// Abstract:
//
// Returns:
//  estiTRUE if we do, estiFALSE otherwise
//
EstiBool CstiSipCall::TextSupportedGet () const
{
	EstiBool bRet = estiFALSE;
	
	Lock ();

	bRet = m_bRemoteTextSupported;

	Unlock ();

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::DtmfSupportedGet
//
// Description: Is the remote end point able to receive DTMF?
//
// Abstract:
//
// Returns:
//  estiTRUE if supported, estiFALSE otherwise
//
EstiBool CstiSipCall::DtmfSupportedGet () const
{
	EstiBool bRet = estiFALSE;
	
	Lock ();
	
	if (m_audioRecord)
	{
		bRet = m_audioRecord->DtmfSupportedGet ();
	}
	
	Unlock ();
	
	return bRet;
}


/*!
 * \brief Helper method to cancel the lost connection timer
 */
void CstiSipCall::LostConnectionTimerCancel ()
{
	Lock ();
	m_lostConnectionTimer.Stop ();
	m_bDetectingLostConnection = false;
	Unlock ();
}


/*!
 * \brief Helper method to cancel reinvite timer and release ref to call object
 */
void CstiSipCall::ReinviteTimerCancel ()
{
	Lock ();
	m_reinviteTimer.Stop ();
	Unlock ();
}


/*!
 * \brief Helper method to cancel reinvite cancel timer (different than reinvite timer) and release ref to call object
 */
void CstiSipCall::ReinviteCancelTimerCancel ()
{
	Lock ();
	m_reinviteCancelTimer.Stop ();
	Unlock ();
}


/*!\brief Used to find out if lost connection detection is in effect.
 * 
 * \returns True if lost connection detection has been enabled.  False otherwise.
 */
bool CstiSipCall::DetectingLostConnectionGet ()
{
	return m_bDetectingLostConnection;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::CalledNameRetrieve
//
// Description: Sets the Called Name if the information has been sent.
//
// Abstract:
//	This method traverses the parameters from the call, extracts the Called Name
//	and sets the Called Name data member.
//
// Returns:
//	None.
//
void CstiSipCall::CalledNameRetrieve (
	RvSipMsgHandle hMsg)
{
	// Get the name from the "to" header
	RvSipPartyHeaderHandle hTo = RvSipMsgGetToHeader (hMsg);
	RvUint nDisplayNameLen = RvSipPartyHeaderGetStringLength (hTo, RVSIP_PARTY_DISPLAY_NAME);
	auto pszCalledName = new RvChar[nDisplayNameLen + 1];
	if (RV_OK == RvSipPartyHeaderGetDisplayName (hTo, pszCalledName, nDisplayNameLen, &nDisplayNameLen))
	{
		// Store the value
		std::string calledName;
		UnescapeDisplayName (pszCalledName, &calledName);
		CstiCallGet ()->CalledNameSet (calledName.c_str ());
	}
	delete [] pszCalledName;
	pszCalledName = nullptr;
}


/*!\brief Initializes the text playback object
 * 
 */
stiHResult CstiSipCall::IncomingTextInitialize ()
{
	auto hResult = stiRESULT_SUCCESS;
	
	if (m_textStream.inboundSupported ())
	{
		if (m_textPlayback == nullptr)
		{
			// I think this is ignored for text
			uint32_t maxRate = 0;

			m_textPlayback = std::make_shared<CstiTextPlayback> (
				m_textSession,
				maxRate,
				true,
				SipManagerGet ()->MidMgrGet (),
				CallLegGet(m_hCallLeg)->m_nRemoteSIPVersion);

			stiTESTCOND (m_textPlayback, stiRESULT_ERROR);

			m_signalConnections.push_back (m_textPlayback->remoteTextReceivedSignal.Connect (
				[this](uint16_t* textBuffer) {
					auto call = CstiCallGet ();
					m_poSipManager->PostEvent (
						[this, textBuffer, call] {
							eventTextPlaybackRemoteTextReceived (textBuffer);

							// TODO:  Use RAII instead of new/delete
							auto textMsg = new SstiTextMsg;
							textMsg->poCall = call.get ();
							textMsg->pwszMessage = textBuffer;

							m_poSipManager->remoteTextReceivedSignal.Emit (textMsg);
						});
				}));

			hResult = m_textPlayback->Initialize (m_mTextPayloadMappings);
			stiTESTRESULT ();
		}

		if (!m_textSession->rtpGet ())
		{
			hResult = openRtpSession (m_textSession);
			stiTESTRESULT();

			//
			// Log where we will receieve Text
			//
			auto call = CstiCallGet ();
			int localPort = 0;
			m_textSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
			call->TextPlaybackPortSet (localPort);
		}
	}
	
STI_BAIL:
	return hResult;
}


/*!\brief Updates the state of the text playback object
 * 
 */
stiHResult CstiSipCall::IncomingTextUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_textStream.inboundSupported ())
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		stiTESTCOND (callLeg, stiRESULT_ERROR);

		auto sdpStream = callLeg->BestSdpStreamGet (
				estiMEDIA_TYPE_TEXT,
				PreferredTextPlaybackProtocolsGet ());

		if (sdpStream && RvAddressGetIpPort (&sdpStream->RtpAddress) != 0)
		{
			if (!m_textPlayback)
			{
				hResult = IncomingTextInitialize();
				stiTESTRESULT ();
			}

			if (m_textPlayback)
			{
				if (!m_textPlayback->AnsweredGet ())
				{
					// Answer the channel.
					hResult = m_textPlayback->Answer (callLeg && !callLeg->m_bRemoteIsICE);
					stiTESTRESULT ();
				}

				auto call = CstiCallGet ();

				// Reconfigure the media ports if they have changed in the SDP
				m_textSession->remoteAddressSet (&sdpStream->RtpAddress,
												  &sdpStream->RtcpAddress,
												  nullptr);

				// Set (remote) text privacy
				switch (sdpStream->eOfferDirection)
				{
					case RV_SDPCONNECTMODE_INACTIVE:
					    call->TextPlaybackPortSet (0);
						// FALLTHRU
					case RV_SDPCONNECTMODE_RECVONLY:
					    if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL)
						 || call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
						{
							// Privacy transition will not occur while on hold
							SIP_DBG_NEGOTIATION("Hold confirmed.  No TextPlayback (remote) privacy change.");
						}
						else if (estiOFF == m_textPlayback->PrivacyModeGet ())
						{
							SIP_DBG_NEGOTIATION("text playback (remote) privacy on");
							m_textPlayback->PrivacyModeSet (estiON);
						}

						break;

					case RV_SDPCONNECTMODE_NOTSET:
					case RV_SDPCONNECTMODE_SENDONLY:
					case RV_SDPCONNECTMODE_SENDRECV:

						if (estiON == m_textPlayback->PrivacyModeGet ())
						{
							SIP_DBG_NEGOTIATION("text playback (remote) privacy off");
							m_textPlayback->PrivacyModeSet (estiOFF);
						}

						break;
				}

				// Set remote hold to correct state.
				if ((call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) && !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME)) ||
					call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD)
				)
				{
					SIP_DBG_NEGOTIATION("Holding (remote) text playback data");
					m_textPlayback->Hold (estiHOLD_REMOTE);
				}
				else
				{
					SIP_DBG_NEGOTIATION("Resuming (remote) text playback data");
					m_textPlayback->Resume (estiHOLD_REMOTE);
				}

				// Set local hold to correct state.
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
				{
					SIP_DBG_NEGOTIATION("Holding (local) text playback data");
					m_textPlayback->Hold (estiHOLD_LOCAL);
				}
				else if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
				{
					SIP_DBG_NEGOTIATION("Resuming (local) text playback data");
					m_textPlayback->Resume (estiHOLD_LOCAL);
				}
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Initializes the audio playback object
 *
 */
stiHResult CstiSipCall::IncomingAudioInitialize ()
{
	auto hResult = stiRESULT_SUCCESS;

	// Construct audio playback if needed
	if (m_audioStream.inboundSupported ())
	{
		if (m_audioPlayback == nullptr)
		{
			m_audioPlayback = std::make_shared<CstiAudioPlayback> (
				m_audioSession,
				CstiCallGet ()->CallIndexGet (),
				un32AUDIO_RATE,
				true,
				SipManagerGet ()->MidMgrGet (),
				CallLegGet(m_hCallLeg)->m_nRemoteSIPVersion);
			stiTESTCOND (m_audioPlayback, stiRESULT_ERROR);

			hResult = m_audioPlayback->Initialize (m_mAudioPayloadMappings);
			stiTESTRESULT ();

			if (m_bIsBridgedCall || m_spCallBridge)
			{
				AudioBridgeOn ();
			}
		}

		if (!m_audioSession->rtpGet ())
		{
			hResult = openRtpSession (m_audioSession);
			stiTESTRESULT();

			//
			// Log where we will receieve Audio
			//
			auto call = CstiCallGet ();
			int localPort = 0;
			m_audioSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
			call->AudioPlaybackPortSet (localPort);
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Updates the state of the audio playback object
 * 
 */
stiHResult CstiSipCall::IncomingAudioUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (m_audioStream.inboundSupported ())
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		stiTESTCOND(callLeg, stiRESULT_ERROR);

		auto sdpStream = callLeg->BestSdpStreamGet (
				estiMEDIA_TYPE_AUDIO,
				&m_PreferredAudioProtocols);

		if (sdpStream && RvAddressGetIpPort(&sdpStream->RtpAddress) != 0)
		{
			if (!m_audioPlayback)
			{
				hResult = IncomingAudioInitialize();
				stiTESTRESULT ();
			}

			if (m_audioPlayback)
			{
				if (!m_audioPlayback->AnsweredGet ())
				{
					hResult = m_audioPlayback->Answer (callLeg && !callLeg->m_bRemoteIsICE);
					stiTESTRESULT ();
				}

				bool bAudioAllowed = true;
				if ((estiINTERPRETER_MODE == SipManagerGet ()->LocalInterfaceModeGet ()
				 && !m_bIsBridgedCall && !m_spCallBridge
				))
				{
					bAudioAllowed = false;
				}

				if (bAudioAllowed)
				{
					auto call = CstiCallGet();

					// Reconfigure the media ports if they have changed in the SDP
					m_audioSession->remoteAddressSet (&sdpStream->RtpAddress,
													  &sdpStream->RtcpAddress,
													  nullptr);

					// Set (remote) audio privacy
					switch (sdpStream->eOfferDirection)
					{
						case RV_SDPCONNECTMODE_INACTIVE:
							call->AudioPlaybackPortSet (0);
							// FALLTHRU
						case RV_SDPCONNECTMODE_RECVONLY:
							if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL)
							 || call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
							{
								// Privacy transition will not occour while on hold
								SIP_DBG_NEGOTIATION("Call %p Hold confirmed.  No AudioPlayback (remote) privacy change.", call.get());
							}
							else if (estiOFF == m_audioPlayback->PrivacyModeGet ())
							{
								SIP_DBG_NEGOTIATION("Call %p audio playback (remote) privacy on", call.get());
								m_audioPlayback->PrivacyModeSet (estiON);
							}

							break;

						case RV_SDPCONNECTMODE_NOTSET:
						case RV_SDPCONNECTMODE_SENDONLY:
						case RV_SDPCONNECTMODE_SENDRECV:
							if (estiON == m_audioPlayback->PrivacyModeGet ())
							{
								SIP_DBG_NEGOTIATION("Call %p audio playback (remote) privacy off", call.get());
								m_audioPlayback->PrivacyModeSet (estiOFF);
							}

							break;
					}

					// Set remote hold to correct state.
					if ((call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) && !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME)) ||
						call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD)
					)
					{
						SIP_DBG_NEGOTIATION("Call %p Holding (remote) audio playback data", call.get());
						m_audioPlayback->Hold (estiHOLD_REMOTE);
					}
					else
					{
						SIP_DBG_NEGOTIATION("Call %p Resuming (remote) audio playback data", call.get());
						m_audioPlayback->Resume (estiHOLD_REMOTE);
					}

					// Set local hold to correct state.
					if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
					{
						SIP_DBG_NEGOTIATION("Call %p Holding (local) audio playback data", call.get());
						m_audioPlayback->Hold (estiHOLD_LOCAL);
					}
					else if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
					{
						SIP_DBG_NEGOTIATION("Call %p Resuming (local) audio playback data", call.get());
						m_audioPlayback->Resume (estiHOLD_LOCAL);
					}
				}
			}
		}
	}
	
STI_BAIL:
	
	return hResult;
}


/*!\brief Initializes the video playback object
 * 
 */
stiHResult CstiSipCall::IncomingVideoInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipManager *poSipManager = SipManagerGet ();
	
	if (m_videoStream.inboundSupported ())
	{
		if (!m_videoPlayback)
		{
			if (SipManagerGet ()->TunnelActive ())
			{
				m_videoSession->tunneledSet (true);
			}

			auto maxRate = m_stConferenceParams.GetMaxRecvSpeed(estiSIP);

			m_videoPlayback = std::make_shared<CstiVideoPlayback> (
			            m_videoSession,
						CstiCallGet()->CallIndexGet(),
						maxRate,
						m_stConferenceParams.eAutoSpeedSetting,
						true,
						poSipManager->MidMgrGet(),
						CallLegGet(m_hCallLeg)->m_nRemoteSIPVersion);

			stiTESTCOND (m_videoPlayback, stiRESULT_ERROR);

			m_signalConnections.push_back (m_videoPlayback->keyframeRequestSignal.Connect (
			    [this]{
				    auto call = CstiCallGet ();
					m_poSipManager->PostEvent ([this,call]{ eventVideoPlaybackKeyframeRequest(); });
			    }));

			m_signalConnections.push_back (m_videoPlayback->privacyModeChangedSignal.Connect (
			    [this](bool muted){
				    auto call = CstiCallGet ();
					m_poSipManager->PostEvent (
					    [this,muted,call]{
						    eventVideoPlaybackPrivacyModeChanged (muted);

							// This is a temporary change while working with call audio bridging
							// The NotifyAppofCall is set to false for an audio bridge.  That works
							// for all the call state changes but there is currently no check that
							// works on all the other notifications.  The symptom is the non-bridge
							// part of the call gets the video playback muted when the bridge
							// is established.
							if (call->NotifyAppOfCallGet())
							{
								m_poSipManager->videoPlaybackPrivacyModeChangedSignal.Emit (call, muted);
							}
					    });
			    }));

			hResult = m_videoPlayback->Initialize (m_mVideoPayloadMappings);
			stiTESTRESULT ();
		}

		if (!m_videoSession->rtpGet ())
		{
			hResult = openRtpSession (m_videoSession);
			stiTESTRESULT ();

			//
			// Log where we will receieve Video
			//
			auto call = CstiCallGet ();
			int localPort = 0;
			m_videoSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
			call->VideoPlaybackPortSet (localPort);
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Updates the video playback object to reflect the current stream state.
 * 
 */
stiHResult CstiSipCall::IncomingVideoUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_videoStream.inboundSupported ())
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		
		SstiSdpStreamP sdpStream;
		SstiMediaPayloadP bestVideoPlayback;

		stiTESTCOND(callLeg, stiRESULT_ERROR);

		std::tie (sdpStream, bestVideoPlayback) = callLeg->BestSdpStreamMediaGet (
				estiMEDIA_TYPE_VIDEO,
				&m_PreferredVideoPlaybackProtocols);

		if (sdpStream && RvAddressGetIpPort (&sdpStream->RtpAddress) != 0)
		{
			if (!m_videoPlayback)
			{
				hResult = IncomingVideoInitialize ();
				stiTESTRESULT ();
			}

			if (m_videoPlayback  && m_videoStream.isActive ())
			{
				if (!m_videoPlayback->AnsweredGet ())
				{
					hResult = m_videoPlayback->Answer (callLeg && !callLeg->m_bRemoteIsICE);
					stiTESTRESULT ();
				}

				auto call = CstiCallGet();

				// Reconfigure the media ports if they have changed in the SDP
				m_videoSession->remoteAddressSet (&sdpStream->RtpAddress,
												  &sdpStream->RtcpAddress,
												  nullptr);

				// Update RtpSession with rtcp feedback support
				rtcpFeedbackSupportUpdate (bestVideoPlayback->rtcpFeedbackTypes);

				// Save the IP where the remote endpoint wants video sent.
				char addr[stiIPV4_ADDRESS_LENGTH + 1];
				RvAddressIpv4ToString (addr, sizeof(addr), sdpStream->RtpAddress.data.ipv4.ip);
				call->RemoteVideoAddrSet (addr);

				switch (sdpStream->eOfferDirection)
				{
					case RV_SDPCONNECTMODE_INACTIVE:
					    call->VideoPlaybackPortSet (0);
						// FALLTHRU
					case RV_SDPCONNECTMODE_RECVONLY:
						if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL)
						 || call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
						{
							// Privacy transition will not occur while on hold
							SIP_DBG_NEGOTIATION("Call %p Hold confirmed.  No VideoPlayback (remote) privacy change.", call.get());
						}
						else if (estiOFF == m_videoPlayback->PrivacyModeGet ())
						{
							SIP_DBG_NEGOTIATION("Call %p video playback (remote) privacy on", call.get());
							m_videoPlayback->PrivacyModeSet (estiON);
						}
						break;

					case RV_SDPCONNECTMODE_SENDRECV:
					case RV_SDPCONNECTMODE_NOTSET:
					case RV_SDPCONNECTMODE_SENDONLY:
						if (estiON == m_videoPlayback->PrivacyModeGet ())
						{
							SIP_DBG_NEGOTIATION("Call %p video playback (remote) privacy off", call.get());
							m_videoPlayback->PrivacyModeSet (estiOFF);
						}
						break;
				}

				// Set remote hold to correct state.
				if ((call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) && !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME)) ||
					call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD)
				)
				{
					SIP_DBG_NEGOTIATION("Call %p Holding (remote) video playback data", call.get());
					m_videoPlayback->Hold (estiHOLD_REMOTE);
				}
				else
				{
					SIP_DBG_NEGOTIATION("Call %p Resuming (remote) video playback data", call.get());
					m_videoPlayback->Resume (estiHOLD_REMOTE);
				}

				// Set local hold to correct state.
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
				{
					SIP_DBG_NEGOTIATION("Call %p Holding (local) video playback data", call.get());
					m_videoPlayback->Hold (estiHOLD_LOCAL);
				}
				else if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
				{
					SIP_DBG_NEGOTIATION("Call %p Resuming (local) video playback data", call.get());
					m_videoPlayback->Resume (estiHOLD_LOCAL);
				}
			}
		}
	}

STI_BAIL:

	return hResult;
}


/*!\brief Initializes each of the incoming media streams.
 *
 */
stiHResult CstiSipCall::IncomingMediaInitialize ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = IncomingAudioInitialize ();
	stiTESTRESULT ();

	hResult = IncomingTextInitialize ();
	stiTESTRESULT ();

	hResult = IncomingVideoInitialize ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*!\brief Updates each of the incoming media streams.
 *
 */
stiHResult CstiSipCall::IncomingMediaUpdate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiCallGet ()->collectTrace (std::string (__func__));

	hResult = IncomingAudioUpdate ();

	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "Playback audio update FAILED");
	}

	hResult = IncomingTextUpdate ();

	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "Playback text update FAILED");
	}

	hResult = IncomingVideoUpdate ();

	if (stiIS_ERROR (hResult))
	{
		stiASSERTMSG (estiFALSE, "Playback video update FAILED");
	}

	return hResult;
}


/*!\brief Opens or updates the audio record data task if the call is in the correct state
 *
 */
void CstiSipCall::audioRecordOpen (
	const SstiSdpStream &sdpStream,
	const SstiMediaPayload &bestMediaPayload)
{
	auto call = CstiCallGet ();

	if (call->mediaSendAllowed ())
	{
		if (bestMediaPayload.bandwidth > 0)
		{
			m_audioRecord->FlowControlRateSet (bestMediaPayload.bandwidth * 1000);
		}

		m_audioRecord->Open (sdpStream, bestMediaPayload);

		// Log where we will send Audio.
		call->AudioRemotePortSet (RvAddressGetIpPort (&sdpStream.RtpAddress));
	}
}


/*!\brief Opens or updates the video record data task if the call is in the correct state
 *
 */
void CstiSipCall::videoRecordOpen (
	const SstiSdpStream &sdpStream,
	const SstiMediaPayload &bestMediaPayload)
{
	auto call = CstiCallGet ();
	auto callLeg = CallLegGet (m_hCallLeg);

	// Ensure that the video stream is active as well. Fixes bug#30506
	if (call->mediaSendAllowed () && m_videoStream.isActive ())
	{
		if (bestMediaPayload.bandwidth > 0)
		{
			int nRate = bestMediaPayload.bandwidth * 1000;

			// Adjust the data rate down if the audio channel is active and the sum of the AR bandwidth and
			// the VR bandwidth would exceed either our maximum send speed or the remote system's call rate.
			if (m_audioRecord &&
				estiOFF == m_audioRecord->PrivacyModeGet ())
			{
				if (callLeg)
				{
					int nRemoteUACallBandwidth = callLeg->RemoteUACallBandwidthGet() * 1000;

					// If we didn't get a remote bandwidth signalled, assume it can handle our maximum send speed.
					if (nRemoteUACallBandwidth <= 0)
					{
						nRemoteUACallBandwidth = m_stConferenceParams.GetMaxSendSpeed(estiSIP);
						stiDEBUG_TOOL(g_stiRateControlDebug,
									  stiTrace("\tThe Remote Call BandwidthGet was not signalled.  Setting it to MaxSendSpeed [%d]\n", nRemoteUACallBandwidth);
						);
					}

					if ((m_audioRecord->MaxChannelRateGet() + nRate) >
					   std::min(m_stConferenceParams.GetMaxSendSpeed(estiSIP), nRemoteUACallBandwidth))
					{
						nRate = std::min(m_stConferenceParams.GetMaxSendSpeed(estiSIP), nRemoteUACallBandwidth) -
							m_audioRecord->MaxChannelRateGet();

						stiDEBUG_TOOL(g_stiRateControlDebug,
									  stiTrace("<CstiSipCall::OutgoingMediaOpen> Location 3.  New Rate = %d\n", nRate);
						);
					}
				}
			}

			m_videoRecord->FlowControlRateSet (nRate);
		}

		if (callLeg)
		{
			m_videoRecord->remoteSipVersionSet (callLeg->m_nRemoteSIPVersion);
		}
		m_videoRecord->Open (sdpStream, bestMediaPayload);
		m_videoRecord->RtcpFeedbackEventsSet ();
		if (m_videoPlayback)
		{
			m_videoPlayback->flowControlReset ();
		}

		// Log where we will send Video.
		call->VideoRemotePortSet (RvAddressGetIpPort (&sdpStream.RtpAddress));
	}
}


/*!\brief Opens or updates the text record data task if the call is in the correct state
 *
 */
void CstiSipCall::textRecordOpen (
	const SstiSdpStream &sdpStream,
	const SstiMediaPayload &bestMediaPayload)
{
	auto call = CstiCallGet ();

	if (call->mediaSendAllowed ())
	{
		if (bestMediaPayload.bandwidth > 0)
		{
			m_textRecord->FlowControlRateSet (bestMediaPayload.bandwidth * 1000);
		}

		// If this is a redundant T.140 channel, we need to pass in both the RTP payload type as well as the redundant payload type.
		// todo: why can't this happen during SDP parsing and attach the RED payload number to the media payload?
		int8_t n8RedPayloadTypeNbr = -1;
		if (estiT140_RED_TEXT == bestMediaPayload.eCodec)
		{
			const SstiMediaPayload *pstT140BasicOffer = T140BasicMediaGet (sdpStream);
			if (pstT140BasicOffer)
			{
				n8RedPayloadTypeNbr = pstT140BasicOffer->n8PayloadTypeNbr;
			}
		}

		m_textRecord->Open (sdpStream, bestMediaPayload, n8RedPayloadTypeNbr);

		// Log where we will send Text.
		call->TextRemotePortSet (RvAddressGetIpPort (&sdpStream.RtpAddress));
	}
}


////////////////////////////////////////////////////////////////////////////////
//; PacketizationSchemeDetermine
//
// Description: Determines the packetization scheme for a given SstiMediaPayload
//
// Returns:
//	the appropriate packetization scheme
//
EstiPacketizationScheme PacketizationSchemeDetermine (const SstiMediaPayload &offer)
{
	EstiPacketizationScheme ePacketizationScheme = estiPktUnknown;
	switch (offer.eCodec)
	{
		case estiH263_1998_VIDEO:
		case estiH263_VIDEO:
			if (RV_RTP_PAYLOAD_H263 == offer.n8PayloadTypeNbr)
			{
				ePacketizationScheme = estiH263_RFC_2190;
			}
			else
			{
				ePacketizationScheme = estiH263_RFC_2429; // 1998
			}
			break;
		case estiH264_VIDEO:
		{
			ePacketizationScheme = offer.ePacketizationScheme;
			break;
		}
		default:
			break;
	}

	return ePacketizationScheme;
}


stiHResult CstiSipCall::OutgoingMediaOpen (
	bool bIsReply)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto callLeg = CallLegGet (m_hCallLeg);
	bool bAudioAllowed = true;
	auto call = CstiCallGet ();
	auto maxRateDefault = m_stConferenceParams.GetMaxSendSpeed (estiSIP);

	CstiSipManager *poSipManager = SipManagerGet ();

	call->collectTrace (std::string (__func__));

	if (callLeg && m_audioStream.outboundSupported () &&
	    // If the remote device is a SVRS Hold Server, don't open audio record
		!callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER) &&
		// If local device is the Hold Server don't open an audio record
		APPLICATION != APP_HOLDSERVER)
	{
		if (SipManagerGet ()->TunnelActive ())
		{
			m_audioSession->tunneledSet (true);
		}

		if (estiINTERPRETER_MODE == poSipManager->LocalInterfaceModeGet ()
			&& !m_bIsBridgedCall && !m_spCallBridge)
		{
			bAudioAllowed = false;
		}

		SstiSdpStreamP sdpStream;
		SstiMediaPayloadP bestAudioRecord;

		std::tie (sdpStream, bestAudioRecord) = callLeg->BestSdpStreamMediaGet (
				estiMEDIA_TYPE_AUDIO,
				&m_PreferredAudioProtocols);

		// Set up the channel object.
		if (nullptr == m_audioRecord)
		{
			if (bestAudioRecord
			 && RvAddressGetIpPort (&sdpStream->RtpAddress) != 0
			 && (sdpStream->eOfferDirection == RV_SDPCONNECTMODE_SENDRECV
				 || sdpStream->eOfferDirection == RV_SDPCONNECTMODE_RECVONLY))
			{
				if (!m_audioSession->rtpGet ())
				{
					hResult = openRtpSession (m_audioSession);
					stiTESTRESULT();

					//
					// Log where we will receieve Audio
					//
					int localPort = 0;
					m_audioSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
					call->AudioPlaybackPortSet (localPort);
				}

				auto maxRate = std::min (
					maxRateDefault,
					static_cast<int> (bestAudioRecord->bandwidth * 1000));

				m_audioRecord = std::make_shared<CstiAudioRecord>(
					m_audioSession,
					maxRate,
					bestAudioRecord->eCodec,
					m_poSipManager->MidMgrGet());

				m_signalConnections.push_back (m_audioRecord->privacyModeChangedSignal.Connect (
					[this](bool muted){
						auto call = CstiCallGet ();
						m_poSipManager->PostEvent ([this,muted,call]{ eventAudioRecordPrivacyModeChanged (muted); });
					}));

				hResult = m_audioRecord->Initialize ();
				stiTESTRESULT ();

				if (m_bIsBridgedCall || m_spCallBridge)
				{
					AudioBridgeOn ();
				}

				audioRecordOpen(*sdpStream, *bestAudioRecord);

				if (!bAudioAllowed)
				{
					m_audioRecord->PrivacyModeSet (estiON);
				}

				stiDEBUG_TOOL (g_stiSipMgrDebug,
							RvRtpSession hRtp = m_audioSession->rtpGet ();
					        stiTrace ("Call %p Started Audio Record port = %d\n",
					                  call.get(), !hRtp ? 0 : RvRtpGetPort (hRtp));
				);
			}
		}
		else
		{
			if (m_spCallBridge)
			{
				AudioBridgeOn ();
			}
			else
			{
				// Set remote hold to correct state.
				if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) ||
					call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD) ||
					!bestAudioRecord // If the media stream is inactive or sendonly then we are effectively on hold for this media stream.
				)
				{
					SIP_DBG_NEGOTIATION("Call %p Holding (remote) audio record data", call.get());
					m_audioRecord->Hold (estiHOLD_REMOTE);
				}
				else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_LCL))
				{
					SIP_DBG_NEGOTIATION("Call %p Resuming (remote) audio record data", call.get());
					m_audioRecord->Resume (estiHOLD_REMOTE);
				}

				// Set local hold to correct state.
				if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL) ||
					call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
				{
					SIP_DBG_NEGOTIATION("Call %p Holding (local) audio record data", call.get());
					m_audioRecord->Hold (estiHOLD_LOCAL);
				}
				else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_RMT))
				{
					SIP_DBG_NEGOTIATION("Call %p Resume (local) audio record data", call.get());
					m_audioRecord->Resume (estiHOLD_LOCAL);
				}

				if (sdpStream && bestAudioRecord)
				{
					audioRecordOpen (*sdpStream, *bestAudioRecord);
				}
			}
		}
	}

	if (m_textStream.outboundSupported () &&
	    // If the remote device is a SVRS Hold Server, don't open text record
		!RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER))
	{
		SstiSdpStreamP sdpStream;
		SstiMediaPayloadP bestTextRecord;
		std::tie (sdpStream, bestTextRecord) = callLeg->BestSdpStreamMediaGet (
				estiMEDIA_TYPE_TEXT,
				PreferredTextRecordProtocolsGet ());

		// Set up the channel object.
		if (nullptr == m_textRecord)
		{
			if (bestTextRecord
			 && RvAddressGetIpPort (&sdpStream->RtpAddress) != 0
			 && (sdpStream->eOfferDirection == RV_SDPCONNECTMODE_SENDRECV
			 || sdpStream->eOfferDirection == RV_SDPCONNECTMODE_RECVONLY))
			{
				if (!m_textSession->rtpGet ())
				{
					hResult = openRtpSession (m_textSession);
					stiTESTRESULT();

					//
					// Log where we will receieve Text
					//
					int localPort = 0;
					m_textSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
					call->TextPlaybackPortSet (localPort);
				}

				m_textRecord = std::make_shared<CstiTextRecord>(
					m_textSession,
					maxRateDefault,
					bestTextRecord->eCodec,
					m_poSipManager->MidMgrGet());

				stiTESTCOND (m_textRecord, stiRESULT_ERROR);

				m_signalConnections.push_back (m_textRecord->privacyModeChangedSignal.Connect (
					[this](bool muted){
						auto call = CstiCallGet ();
						m_poSipManager->PostEvent ([this,muted,call]{ eventTextRecordPrivacyModeChanged(muted); });
					}));

				hResult = m_textRecord->Initialize ();
				stiTESTRESULT ();

				textRecordOpen (*sdpStream, *bestTextRecord);

				stiDEBUG_TOOL (g_stiSipMgrDebug,
							RvRtpSession hRtp = m_textSession->rtpGet ();
							stiTrace ("Started Text Record port = %d\n", !hRtp ? 0 : RvRtpGetPort (hRtp));
				);
			}
		}
		else
		{
			// Set remote hold to correct state.
			if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) ||
			    call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD) ||
				!bestTextRecord // If the media stream is inactive or sendonly then we are effectively on hold for this media stream.
			)
			{
				SIP_DBG_NEGOTIATION("Holding (remote) text record data");
				m_textRecord->Hold (estiHOLD_REMOTE);
			}
			else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_LCL))
			{
				SIP_DBG_NEGOTIATION("Resuming (remote) text record data");
				m_textRecord->Resume (estiHOLD_REMOTE);
			}

			// Set local hold to correct state.
			if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL) ||
			    call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
			{
				SIP_DBG_NEGOTIATION("Holding (local) text record data");
				m_textRecord->Hold (estiHOLD_LOCAL);
			}
			else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_RMT))
			{
				SIP_DBG_NEGOTIATION("Resume (local) text record data");
				m_textRecord->Resume (estiHOLD_LOCAL);
			}

			if (sdpStream && bestTextRecord)
			{
				textRecordOpen (*sdpStream, *bestTextRecord);
			}
		}
	}

	if (callLeg && 
		m_videoStream.outboundSupported () &&

	    // If the remote device is an SVRS Hold Server, don't open video record
		!callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_HOLD_SERVER))
	{
		SstiSdpStreamP sdpStream;
		SstiMediaPayloadP bestMediaTypeVideo;
		std::tie (sdpStream, bestMediaTypeVideo) = callLeg->BestSdpStreamMediaGet (
				estiMEDIA_TYPE_VIDEO,
				&m_PreferredVideoRecordProtocols);

		if (bestMediaTypeVideo)
		{
			// Update RtpSession with rtcp feedback support
			rtcpFeedbackSupportUpdate (bestMediaTypeVideo->rtcpFeedbackTypes);
		}
		else
		{
			// If we dont have a best media send an empty list to ensure remote RTCP
			// feedback is set to false.
			rtcpFeedbackSupportUpdate ({});
		}

		// Set up the channel object.
		if (nullptr == m_videoRecord)
		{
			if (bestMediaTypeVideo)
			{
				if (!m_videoSession->rtpGet ())
				{
					hResult = openRtpSession (m_videoSession);
					stiTESTRESULT();

					//
					// Log where we will receieve Video
					//
					int localPort = 0;
					m_videoSession->LocalAddressGet (nullptr, &localPort, nullptr, nullptr);
					call->VideoPlaybackPortSet (localPort);
				}
			}
			
			if (bestMediaTypeVideo
			 && RvAddressGetIpPort (&sdpStream->RtpAddress) != 0
			 && (sdpStream->eOfferDirection == RV_SDPCONNECTMODE_SENDRECV
			 || sdpStream->eOfferDirection == RV_SDPCONNECTMODE_RECVONLY))
			{

				auto maxRate =
					std::min (
						maxRateDefault,
						static_cast<int> (bestMediaTypeVideo->bandwidth * 1000));
		
				uint32_t audioBitRateUsed = 0;

				if (m_audioRecord && !m_audioRecord->PrivacyGet ())
				{
					audioBitRateUsed = static_cast<uint32_t> (m_audioRecord->MaxChannelRateGet ());
				}

				// Get call id to pass to Video Record Channel
				std::string CallId;
				call->CallIdGet(&CallId);

				RTCPFlowcontrol::Settings flowControlSettings;

				flowControlSettings.autoSpeedSetting = m_stConferenceParams.eAutoSpeedSetting;
				flowControlSettings.autoSpeedMinStartSpeed = m_stConferenceParams.un32AutoSpeedMinStartSpeed;
				flowControlSettings.publicIPv4 = m_stConferenceParams.PublicIPv4;
				flowControlSettings.remoteFlowControlLogging = m_stConferenceParams.bRemoteFlowControlLogging;

				m_videoRecord = std::make_shared<CstiVideoRecord>(
									m_videoSession,
									call->CallIndexGet (),
									maxRate,
									bestMediaTypeVideo->eCodec,
									PacketizationSchemeDetermine (*bestMediaTypeVideo),
									m_poSipManager->MidMgrGet(),
									flowControlSettings,
									CallId,
									m_poSipManager->LocalInterfaceModeGet(),
									audioBitRateUsed);
				stiTESTCOND (m_videoRecord, stiRESULT_ERROR);

				// Handle signals from videoRecord
				m_signalConnections.push_back (m_videoRecord->privacyModeChangedSignal.Connect (
					[this](bool muted){
						auto call = CstiCallGet ();
						m_poSipManager->PostEvent ([this,muted,call]{ eventVideoRecordPrivacyModeChanged (muted); });
					}));

				m_signalConnections.push_back (m_videoRecord->holdserverVideoFileCompletedSignal.Connect (
					[this]() {
						auto call = CstiCallGet ();
						m_poSipManager->PostEvent ([this, call] { eventHSVideoFileCompleted (); });
					}));

				hResult = m_videoRecord->Initialize (
							RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE),
							call->ConnectedWithMcuGet(estiMCU_ANY));

				videoRecordInitialized.Emit();
				stiTESTRESULT ();

				unsigned int remoteDisplayWidth = 0;
				unsigned int remoteDisplayHeight = 0;
				callLeg->RemoteDisplaySizeGet (&remoteDisplayWidth, &remoteDisplayHeight);
				m_videoRecord->RemoteDisplaySizeSet (remoteDisplayWidth, remoteDisplayHeight);

				videoRecordOpen (*sdpStream, *bestMediaTypeVideo);

				m_videoPrivacyMode = m_videoRecord->PrivacyModeGet();

				stiDEBUG_TOOL (g_stiSipMgrDebug,
					RvRtpSession hRtp = m_videoSession->rtpGet ();
					stiTrace ("Started Video Record port = %d\n", !hRtp ? 0 : RvRtpGetPort (hRtp));
				);
			}
		}
		else
		{
			// Set remote hold to correct state.
			if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT) ||
			    call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD) ||
				!bestMediaTypeVideo // If the media stream is inactive or sendonly then we are effectively on hold for this media stream.
			)
			{
				SIP_DBG_NEGOTIATION("Holding (remote) video record data");
				m_videoRecord->Hold (estiHOLD_REMOTE);
			}
			// TODO: Why were we requiring that is was a reply?  With this in place only one direction
			// of a resumed call would start sending video again.
			else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_LCL))
			{
				SIP_DBG_NEGOTIATION("Resuming (remote) video record data");
				m_videoRecord->Resume (estiHOLD_REMOTE);
			}

			// Set local hold to correct state.
			if (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL) ||
			    call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
			{
				SIP_DBG_NEGOTIATION("Holding (local) video record data");
				m_videoRecord->Hold (estiHOLD_LOCAL);
			}
			else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_RMT))
			{
				SIP_DBG_NEGOTIATION("Resume (local) video record data");
				m_videoRecord->Resume (estiHOLD_LOCAL);
			}

			// If this is an OK and we were transitioning out of video mute
			// then we need to complete the transition and resume media flow.
			if (bIsReply)
			{
				m_videoRecord->LocalPrivacyComplete ();
			}

			if (sdpStream && bestMediaTypeVideo)
			{
				videoRecordOpen (*sdpStream, *bestMediaTypeVideo);
			}
		}

		// Start keyframe timer if it isn't already going.
		// Moved keyframe timer to only start if we are actually sending video.
		if (m_stConferenceParams.nAutoKeyframeSend > 0)
		{
			m_periodicKeyframeTimer.TimeoutSet (m_stConferenceParams.nAutoKeyframeSend);
			m_periodicKeyframeTimer.Start ();
		}
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::SorensonCustomInfoParse
//
// Description: Parse a custom Sorenson INFO message and act upon it
//
// Returns:
//   estiOK upon success
//
stiHResult CstiSipCall::SorensonCustomInfoParse (
	RvSipCallLegHandle hCallLeg,
	RvSipMsgHandle hMsg,
	bool bIsReply ///< This will be set to true if it this message is a result of our request
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	char *pszSorensonMsg = nullptr;
	EstiSorensonMessage eMessage = estiSM_UNKNOWN;
	RvUint nStringLen = 0;
	RvUint nBufLen = 0;
	auto callLeg = CallLegGet (hCallLeg);

	// if remote is not a sorenson videophone, automatically reject
	if (callLeg && 
		!callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		DBG_MSG ("Sorenson info message from non-SVRS_VIDEOPHONE device.");
		stiTESTCOND_NOLOG (0, stiRESULT_ERROR);
	}

	// How big is the INFO message?
	nStringLen = RvSipMsgGetStringLength (hMsg, RVSIP_MSG_BODY);
	if (nStringLen <= 0)
	{
		stiTESTCOND (0, stiRESULT_ERROR);
	}
	if (nStringLen >= strlen (SORENSON_CUSTOM_MIME))
	{
		nBufLen = nStringLen + 1;
	}
	else
	{
		nBufLen = strlen (SORENSON_CUSTOM_MIME) + 1;
	}

	// Allocate a buffer sufficient for the message
	pszSorensonMsg = new char [nBufLen + 1];
	if (nullptr == pszSorensonMsg)
	{
		// Failed to allocate memory.  Log an error and exit
		stiTHROW (stiRESULT_ERROR);
	}

	{
		// Retrieve the content type header
		auto contentType = vpe::sip::Message(hMsg).contentTypeHeaderGet();

		// Was it a Sorenson header?
		if (contentType != SORENSON_CUSTOM_MIME)
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}

	// It is an sorenson message.  Retrieve the message body
	rvStatus = RvSipMsgGetBody (hMsg, pszSorensonMsg, nStringLen, &nStringLen);
	stiTESTRVSTATUS ();

	// Retrieve the message id and value
	// Expected: Zero-padded four digit message type, followed
	// immediately by optional zero-padded four digit message value.
	if (nStringLen >= 4)
	{
		pszSorensonMsg[4] = '\0';
		eMessage = (EstiSorensonMessage)atoi (pszSorensonMsg);
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

	// Now take action based on the message!
	switch (eMessage)
	{
		case estiSM_CLEAR_TEXTSHARE:
		{
			CstiSipManager *pSipMgr = SipManagerGet ();

			pSipMgr->removeTextSignal.Emit (m_poCstiCall.lock());

			if (!bIsReply)
			{
				RvSipTranscHandle hTransaction;

				rvStatus = RvSipCallLegGetTranscByMsg (hCallLeg, hMsg, true, &hTransaction);
				stiTESTRVSTATUS ();

				{
					auto outboundMessage = vpe::sip::Transaction(hTransaction).outboundMessageGet();

					hResult = SorensonCustomInfoSend (hCallLeg, eMessage, 0, outboundMessage);
					stiTESTRESULT ();
				}
			}

			break;
		}

		case estiSM_UNKNOWN:
		case estiSM_IS_HOLDABLE_MESSAGE:
		case estiSM_VIDEO_COMPLETE_INDICATION:
		case estiSM_FLASH_LIGHT_RING_COMMAND:
		case estiSM_HOLD_COMMAND:
		case estiSM_RESUME_COMMAND:
		case estiSM_IS_PTZABLE_MESSAGE:
		case estiSM_IS_TRANSFERABLE_MESSAGE:
		case estiSM_CALL_TRANSFER_COMMAND:
		case estiSM_DELAYED_SINFO:
		case estiSM_CLEAR_DTMF:
		case estiSM_CLEAR_REMOTETEXTSHARE:
		case estiSM_CLEAR_LOCALTEXTSHARE:
		case estiSM_ZZZZ_NO_MORE:

			// We do not need to process these messages.

			break;
	}

STI_BAIL:

	if (nullptr != pszSorensonMsg)
	{
		delete [] pszSorensonMsg;
		pszSorensonMsg = nullptr;
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::SorensonCustomInfoSend
//
// Description: Send a custom Sorenson INFO message
//
// Returns:
//   estiOK upon success
//
#define MAX_MSG_LEN 8
stiHResult CstiSipCall::SorensonCustomInfoSend (
	RvSipCallLegHandle hCallLeg,
	EstiSorensonMessage eMessage, ///< The message id to send
	unsigned int nValue, ///< Optional parameter for the message
	RvSipMsgHandle hAddToMsg ///< sip message to attach this custom message to (if null, create and send a new message)
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	RvSipMsgHandle hOutboundMsg = nullptr;
	RvSipTranscHandle hTransaction = nullptr;
	auto callLeg = CallLegGet (hCallLeg);

	//
	// Don't send if the remote endpoint is not a Sorenson Videophone.
	//
	if (callLeg && 
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_VIDEOPHONE))
	{
		// Convert the message/value to a string
		char szBody[MAX_MSG_LEN + 1];
		snprintf (szBody, MAX_MSG_LEN + 1, "%04d%04d", eMessage, nValue);

		// Create a new message if necessary
		if (nullptr == hAddToMsg)
		{
			// Create a new message
			rvStatus = RvSipCallLegTranscCreate (hCallLeg, nullptr, &hTransaction);
			stiTESTRVSTATUS ();

			rvStatus = RvSipTransactionGetOutboundMsg (hTransaction, &hOutboundMsg);
			stiTESTRVSTATUS ();
		}
		else
		{
			// Use the message provided
			hOutboundMsg = hAddToMsg;
		}

		// Attach custom Sorenson data to the message
		rvStatus = RvSipMsgSetContentTypeHeader (hOutboundMsg, (char*)SORENSON_CUSTOM_MIME);
		stiTESTRVSTATUS ();

		// Attach the string
		rvStatus = RvSipMsgSetBody (hOutboundMsg, (RvChar*)szBody);
		stiTESTRVSTATUS ();

		// Automatically send the message now if we created it.
		if (nullptr == hAddToMsg)
		{
			rvStatus = RvSipCallLegTranscRequest (hCallLeg, (RvChar*)INFO_METHOD, &hTransaction);
			stiTESTRVSTATUS ();
		}
	}

STI_BAIL:

	return hResult;
}
#undef MAX_MSG_LEN


////////////////////////////////////////////////////////////////////////////////
//; HoldStateSet
//
// Description: Helper to transition to a new hold state and notify app if necessary.
//	This does not change the media flow.  You must call MediaOpen (for each direction)
//	after HoldStateSet to have new hold state represented in media streams.
//
// Returns:
//	true if hold state changed, false otherwise
//
bool CstiSipCall::HoldStateSet (
	bool bLocal, /// < Set this flag
	bool bChangeTo) ///< To this value
{
	bool bChanged = true;
	CstiSipManager *poSipManager = SipManagerGet ();
	auto call = CstiCallGet ();
	EsmiCallState eCallState = call->StateGet ();


	if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT))
	{
		if (!bLocal) // Remote hold state has changed
		{
			if (!bChangeTo && call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT)) // remote hold went off
			{
				// Are we already negotiating local resume?
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
				{
					SIP_DBG_NEGOTIATION ("Remote hold went off while negotiating local resume");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_RMT_RESUME | estiSUBSTATE_NEGOTIATING_LCL_RESUME);
				}
				else
				{
					SIP_DBG_NEGOTIATION ("Remote hold went off");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_RMT_RESUME);
				}
			}
			else if (bChangeTo && !call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_RMT)) // remote hold went on
			{
				// Are we already negotiating local hold?
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
				{
					SIP_DBG_NEGOTIATION ("Remote hold went on while negotiating a local hold");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_RMT_HOLD | estiSUBSTATE_NEGOTIATING_LCL_HOLD);
				}
				else
				{
					SIP_DBG_NEGOTIATION ("Remote hold went on");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_RMT_HOLD);
				}
			}
			else
			{
				SIP_DBG_NEGOTIATION ("No change to current Remote hold state");
				bChanged = false;
			}
		}
		else  // Local hold state has changed
		{
			if (!bChangeTo && call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL)) // local hold went off
			{
				// Are we already negotiating remote resume?
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME))
				{
					SIP_DBG_NEGOTIATION ("Local hold went off while negotiating a remote resume");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_LCL_RESUME | estiSUBSTATE_NEGOTIATING_RMT_RESUME);
				}
				else
				{
					SIP_DBG_NEGOTIATION ("Local hold went off");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_LCL_RESUME);
				}
			}
			else if (bChangeTo && !call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL)) // local hold went on
			{
				// Are we already negotiating remote hold?
				if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD))
				{
					SIP_DBG_NEGOTIATION ("Local hold went on while negotiating a remote hold");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_LCL_HOLD | estiSUBSTATE_NEGOTIATING_RMT_HOLD);
				}
				else
				{
					SIP_DBG_NEGOTIATION ("Local hold went on");
					poSipManager->NextStateSet (call, eCallState, estiSUBSTATE_NEGOTIATING_LCL_HOLD);
				}
			}
			else
			{
				SIP_DBG_NEGOTIATION ("No change to current Local hold state");
				bChanged = false;
			}
		}
	}
	else if (!call->StateValidate (esmiCS_CONNECTING | esmiCS_IDLE))
	{
		// Although we don't want to change the hold state for a connecting
		// call, we also don't want to print a misleading error message.
		DBG_MSG ("Unable to change hold status for current call state %d", eCallState);
		bChanged = false;
	}

	return bChanged;
}


////////////////////////////////////////////////////////////////////////////////
//; HoldStateComplete
//
// Description: Helper to transition from negotiating a hold state, to that hold state.
//
// Returns:
//	none
//
void CstiSipCall::HoldStateComplete (bool bLocal)
{
	CstiSipManager *poSipManager = SipManagerGet ();
	auto call = CstiCallGet ();

	if (bLocal)
	{
		if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD))
		{
			if (call->StateValidate (esmiCS_HOLD_RMT) ||
				
				// If we happened to be negotiating both at the same time, we also transition to esmiCS_HOLD_BOTH
			    (call->StateValidate (esmiCS_CONNECTED) &&
			     call->StateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD) &&
			     call->StateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD))) // both are now held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_RMT => esmiCS_HOLD_BOTH");
				poSipManager->NextStateSet (call, esmiCS_HOLD_BOTH, estiSUBSTATE_HELD);
			}
			else // only local is held
			{
				DBG_MSG ("HoldStateComplete esmiCS_CONNECTED => esmiCS_HOLD_LCL");
				poSipManager->NextStateSet (call, esmiCS_HOLD_LCL, estiSUBSTATE_HELD);
			}

			// Completing a local hold can be a step in the transfer procedure.  If this is the
			// case, we need to continue on with the next step.
#ifdef HOLD_TRANSFEREE
			ReferSend ();
#endif
		}
		else if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME))
		{
			if (call->StateValidate (esmiCS_HOLD_BOTH) &&
			  
				// If we happened to be negotiating resume of both at the same time, we need to go
				// back to the connected state.  Check that this isn't the case.
			    !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME)) // only remote is now held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_BOTH => esmiCS_HOLD_RMT");
				poSipManager->NextStateSet (call, esmiCS_HOLD_RMT, estiSUBSTATE_HELD);
			}
			else // nothing is held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_LCL => esmiCS_CONNECTED");
				poSipManager->NextStateSet (call, esmiCS_CONNECTED, estiSUBSTATE_CONFERENCING);
			}
		}
	}
	else
	{
		if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD))
		{
			if (call->StateValidate (esmiCS_HOLD_LCL) ||

				// If we happened to be negotiating both at the same time, we also transition to esmiCS_HOLD_BOTH
			    (call->StateValidate (esmiCS_CONNECTED) &&
			     call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD)
				 )) // both are now held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_LCL => esmiCS_HOLD_BOTH");
				poSipManager->NextStateSet (call, esmiCS_HOLD_BOTH, estiSUBSTATE_HELD);
			}
			else // only local is held
			{
				DBG_MSG ("HoldStateComplete esmiCS_CONNECTED => esmiCS_HOLD_RMT");
				poSipManager->NextStateSet (call, esmiCS_HOLD_RMT, estiSUBSTATE_HELD);
			}
		}
		else if (call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME))
		{
			if (call->StateValidate (esmiCS_HOLD_BOTH) &&
			  
				// If we happened to be negotiating resume of both at the same time, we need to go
				// back to the connected state.  Check that this isn't the case.
			    !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME)) // only local is now held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_BOTH => esmiCS_HOLD_LCL");
				poSipManager->NextStateSet (call, esmiCS_HOLD_LCL, estiSUBSTATE_HELD);
			}
			else // nothing is held
			{
				DBG_MSG ("HoldStateComplete esmiCS_HOLD_RMT => esmiCS_CONNECTED");
				poSipManager->NextStateSet (call, esmiCS_CONNECTED, estiSUBSTATE_CONFERENCING);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; MaxMBPSfromSipLevelGet
//
// Description: Find the Max Macroblocks for a given level in SIP terms
//
// Returns:
//	The max macroblocks for the level or 0 if bad input.
//
static int MaxMBPSfromSipLevelGet(
	unsigned int eLevel)
{
	switch (eLevel)
	{
		case estiH264_LEVEL_1:
			return nMAX_MBPS_1;
		case estiH264_LEVEL_1_1:
			return nMAX_MBPS_1_1;
		case estiH264_LEVEL_1_2:
			return nMAX_MBPS_1_2;
		case estiH264_LEVEL_1_3:
			return nMAX_MBPS_1_3;
		case estiH264_LEVEL_2:
			return nMAX_MBPS_2;
		case estiH264_LEVEL_2_1:
			return nMAX_MBPS_2_1;
		case estiH264_LEVEL_2_2:
			return nMAX_MBPS_2_2;
		case estiH264_LEVEL_3:
			return nMAX_MBPS_3;
		case estiH264_LEVEL_3_1:
			return nMAX_MBPS_3_1;
		case estiH264_LEVEL_3_2:
			return nMAX_MBPS_3_2;
		case estiH264_LEVEL_4:
			return nMAX_MBPS_4;
		case estiH264_LEVEL_4_1:
			return nMAX_MBPS_4_1;
		case estiH264_LEVEL_4_2:
			return nMAX_MBPS_4_2;
		case estiH264_LEVEL_5:
			return nMAX_MBPS_5;
		case estiH264_LEVEL_5_1:
			return nMAX_MBPS_5_1;
		default:
			stiASSERT("Unhandled level in MaxMBPSGet");
			return 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; MaxFSfromSipLevelGet
//
// Description: Find the Max Frame Size for a given level in SIP terms
//
// Returns:
//	The max framesize for the level or 0 if bad input.
//
static int MaxFSfromSipLevelGet(
	unsigned int eLevel)
{
	switch (eLevel)
	{
		case estiH264_LEVEL_1:
			return nMAX_FS_1;
		case estiH264_LEVEL_1_1:
			return nMAX_FS_1_1;
		case estiH264_LEVEL_1_2:
			return nMAX_FS_1_2;
		case estiH264_LEVEL_1_3:
			return nMAX_FS_1_3;
		case estiH264_LEVEL_2:
			return nMAX_FS_2;
		case estiH264_LEVEL_2_1:
			return nMAX_FS_2_1;
		case estiH264_LEVEL_2_2:
			return nMAX_FS_2_2;
		case estiH264_LEVEL_3:
			return nMAX_FS_3;
		case estiH264_LEVEL_3_1:
			return nMAX_FS_3_1;
		case estiH264_LEVEL_3_2:
			return nMAX_FS_3_2;
		case estiH264_LEVEL_4:
			return nMAX_FS_4;
		case estiH264_LEVEL_4_1:
			return nMAX_FS_4_1;
		case estiH264_LEVEL_4_2:
			return nMAX_FS_4_2;
		case estiH264_LEVEL_5:
			return nMAX_FS_5;
		case estiH264_LEVEL_5_1:
			return nMAX_FS_5_1;
		default:
			stiASSERT("Unhandled level in MaxFSGet");
			return 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; FmtpValueGet
//
// Description: Helper to return a value from a pFmtp map based on its name.
//
// Returns:
//	nothing
//
void FmtpValueGet (
	std::map<std::string, std::string> *pFmtp, ///< Pointer to an fmtp map (as found via SdpFmtpValuesGet)
	const char *pszName, ///< The name of the value to get
	std::string *psValue) ///< A std::string to be populated with the value (or empty string if not found)
{
	auto it = pFmtp->find (pszName);
	if (it != pFmtp->end ())
	{
		*psValue = it->second;
	}
	else
	{
		psValue->clear ();
	}
}


/*!
 * \brief Parses FMTP and updates map with payloads for RTX.
 *
 * \param fmtp The fmtp value retrieved from the SDP
 */
stiHResult CstiSipCall::RtxVideoPayloadsSet (
	std::string fmtp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int rtxPayload = INVALID_DYNAMIC_PAYLOAD_TYPE;
	int associatedPaylod = INVALID_DYNAMIC_PAYLOAD_TYPE;
	auto payloadMap = m_mVideoPayloadMappings.begin ();
	std::vector<std::string> results;
	auto rtxFmtpRegex = sci::make_unique<CstiRegEx>("^([0-9]{1,3}) apt=([0-9]{1,3})$");
	
	// The fmtp should be in the format "<rtx payload> apt=<associated payload>" Example: "100 apt=96"
	stiTESTCONDMSG (
		(!fmtp.empty () && rtxFmtpRegex->Match(fmtp, results)),
		stiRESULT_INVALID_PARAMETER,
		"RtxPayloadsSet fmtp passed in is invalid fmtp %s", fmtp.c_str());
	
	// The second value in the results will be the rtx payload.
	stiTESTCONDMSG (
		boost::conversion::try_lexical_convert(results[1], rtxPayload),
		stiRESULT_ERROR,
		"RtxPayloadsSet invalid RTX payload type in fmtp %s.", fmtp.c_str());
	
	// The third value in the results will be the associated payload
	stiTESTCONDMSG (
		boost::conversion::try_lexical_convert(results[2], associatedPaylod),
		stiRESULT_ERROR,
		"RtxPayloadsSet invalid assoctiated payload type in fmtp %s.", fmtp.c_str());
	
	// Verify we got two different payload types
	stiTESTCONDMSG ((rtxPayload != associatedPaylod), stiRESULT_ERROR,
		"RtxPayloadsSet RTX and APT are the same fmtp = %s", fmtp.c_str());
	
	// Last check make sure the associated payload is one we know about.
	payloadMap = m_mVideoPayloadMappings.find (associatedPaylod);
	stiTESTCONDMSG (
		(payloadMap != m_mVideoPayloadMappings.end ()),
		stiRESULT_ERROR,
		"RtxPayloadsSet associated payload not found in map fmtp = %s", fmtp.c_str());

	payloadMap->second.rtxPayloadType = rtxPayload;
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; SdpFmtpValuesGet
//
// Description: Finds all fmtp settings for a given payload id on a given media
//	descriptor.
//
// Returns:
//	Nothing
//
void CstiSipCall::SdpFmtpValuesGet (
	std::map<std::string, std::string> *mpRetVal, ///< Buffer to return result in
	const RvSdpMediaDescr *pMedia, ///< The media descriptor we are searching
	int nPayloadID, ///< The payload number whose fmtp we're looking for
	ECodec eCodec) ///< The protocol this codec is (h263, etc...)
{
	RvSdpListIter hIterator;
	RvSdpAttribute *pFmtp = rvSdpMediaDescrGetFirstFmtp (const_cast<RvSdpMediaDescr *>(pMedia), &hIterator);
	bool bFoundH264Packetization = false;
	bool bFoundH264Profile = false;
	while (nullptr != pFmtp)
	{
		const char *pszValue = rvSdpAttributeGetValue (pFmtp);
		if ((nullptr != pszValue) && (atoi (pszValue) == nPayloadID))
		{
			auto copySize = strlen (pszValue) + 1;
			auto szValueMem = new char[copySize];
			char *szValue = szValueMem;
			memcpy (szValue, pszValue, copySize);

			for (; szValue[0] == ' '; ++szValue) // If the fmtp begins with spaces skip over them
			{
			}

			for (; szValue[0] && szValue[0] != ' '; ++szValue) // find first space after this word
			{
			}

			for (; szValue[0] == ' '; ++szValue) // find next word
			{
			}

			// If we are on a T.140 Redundant Text codec, the data will be loaded a bit differently.
			if (estiT140_RED_TEXT == eCodec)
			{
				const char *pszKey = "RED_PTs";  // Store the payload type for the redundant data.

				mpRetVal->insert (std::pair<std::string, std::string>(pszKey, szValue));
			}
			else if (estiRTX == eCodec)
			{
				// Currently only implementing support RTX with video.
				if (rvSdpMediaTypeTxt2Val(pMedia->iMediaTypeStr) == RV_SDPMEDIATYPE_VIDEO)
				{
					RtxVideoPayloadsSet (pszValue);
				}
			}
			else
			{
				while (szValue[0])
				{
					const char *pszKey = szValue;
					for (; szValue[0] && szValue[0] != ' ' && szValue[0] != '='; szValue++) // find '=' or space
					{
					}

					if (!szValue[0])
					{
						break;
					}

					szValue[0] = '\0';
					++szValue;
					for (; szValue[0] == ' ' || szValue[0] == '='; ++szValue) // find next word
					{
					}

					pszValue = szValue;
					for (; szValue[0] && szValue[0] != ' ' && szValue[0] != ';'; szValue++) // find next space or ;
					{
					}

					if (szValue[0])
					{
						szValue[0] = '\0';
						++szValue;
						for (; szValue[0] == ' ' || szValue[0] == ';'; szValue++) // find next word
						{
						}
					}
					mpRetVal->insert (std::pair<std::string, std::string>(pszKey, pszValue));

					if (eCodec == estiH264_VIDEO)
					{
						if (g_fmtpPacketizationMode == pszKey)
						{
							bFoundH264Packetization = true;
						}

						if (g_fmtpProfileLevelID == pszKey)
						{
							bFoundH264Profile = true;
						}
					}
				}
			}

			delete [] szValueMem;
			szValueMem = nullptr;
		}
		pFmtp = rvSdpMediaDescrGetNextFmtp (&hIterator);
	}

	// Capabilities check
	switch (eCodec)
	{
		case estiH263_1998_VIDEO:
		{
			(*mpRetVal)[g_fmtpProfile] = "0";
			break;
		}
		case estiH264_VIDEO:
		{
			if (!bFoundH264Packetization)
			{
				const std::string modeZero = "0";

				// Since the packetization mode was not signaled, set it to mode 0.
				mpRetVal->insert (std::make_pair(g_fmtpPacketizationMode, modeZero));
			}

			if (!bFoundH264Profile)
			{
				const std::string baselineLevelOneThree = "42000D";

				// Since the profile and level were not signaled, set them to baseline (0x42) and level 1.3 (0x0d).
				mpRetVal->insert (std::make_pair(g_fmtpProfileLevelID, baselineLevelOneThree));
			}

			// Get a copy of our record capabilities.  We will compare what we can send to what
			// the remote side can receive.
			SstiH264Capabilities stH264Caps;
			ProtocolManagerGet ()->RecordCodecCapabilitiesGet (estiVIDEO_H264, &stH264Caps);
			
			// profile-level-id
			std::string value = (*mpRetVal)[g_fmtpProfileLevelID];

			unsigned int unLevel = estiH264_LEVEL_1;
			int nMaxMBPS = MaxMBPSfromSipLevelGet(unLevel);
			int nMaxFS = MaxFSfromSipLevelGet(unLevel);

			if (value.length () == 6)
			{
				unsigned int unConstraint;
				unsigned int unProfileTheirs;
				unsigned int unLevelTheirs = estiH264_LEVEL_1;
				char szTmp[7];

				sscanf (value.c_str (), "%02x%02x%02x", &unProfileTheirs, &unConstraint, &unLevelTheirs);

				// Get the profile, level and constraints
				unLevel = std::min (static_cast<int> (unLevelTheirs), static_cast<int> (stH264Caps.eLevel));
				snprintf (szTmp, sizeof (szTmp), "%02x%02x%02x", unProfileTheirs, unConstraint, unLevel);
				(*mpRetVal)[g_fmtpProfileLevelID] = szTmp;

				nMaxMBPS = MaxMBPSfromSipLevelGet(unLevelTheirs);
				nMaxFS = MaxFSfromSipLevelGet(unLevelTheirs);
			}

			// Remove any values that we aren't explicitly supporting at the moment.
			// We support:
			// "profile-level-id", "packetization-mode", "max-mbps", "max-fs"
			for (auto it = mpRetVal->begin (); it != mpRetVal->end ();)
			{
				if (g_fmtpProfileLevelID != it->first &&
					g_fmtpPacketizationMode != it->first &&
					g_fmtpMaxMBPS != it->first &&
					g_fmtpMaxFS != it->first)
				{
					it = mpRetVal->erase (it);
				}
				else
				{
					++it;
				}
			}

			// If their level is higher than ours but we have a maxMBPS, then we need to set
			// max-mbps to the min of our max-mbps and their max-mbps
			auto it = mpRetVal->find(g_fmtpMaxMBPS);
			if (it != mpRetVal->end())
			{
				sscanf(it->second.c_str(), "%d", &nMaxMBPS);
			}

			int nMyMaxMBPS = 0;
			if (stH264Caps.unCustomMaxMBPS > 0)
			{
				nMyMaxMBPS = stH264Caps.unCustomMaxMBPS;
			}
			else
			{
				nMyMaxMBPS = MaxMBPSfromSipLevelGet(stH264Caps.eLevel);
			}

			int nMinMax = std::min<int>(nMyMaxMBPS, nMaxMBPS);

			int nLevelMBPS = MaxMBPSfromSipLevelGet(unLevel);
			if (nMinMax > nLevelMBPS)
			{
				(*mpRetVal)[g_fmtpMaxMBPS] = std::to_string(nMinMax);
			}
			else
			{
				mpRetVal->erase(g_fmtpMaxMBPS); // use level defaults
			}

			// If their level is higher than ours but we have a maxFS, then we need to set
			// max-fs to the min of our max-fs and their max-fs
			it = mpRetVal->find(g_fmtpMaxFS);
			if (it != mpRetVal->end())
			{
				sscanf(it->second.c_str(), "%d", &nMaxFS);
			}

			int nMyMaxFS = 0;
			if (stH264Caps.unCustomMaxFS > 0)
			{
				nMyMaxFS = stH264Caps.unCustomMaxFS;
			} 
			else
			{
				nMyMaxFS = MaxFSfromSipLevelGet(stH264Caps.eLevel);
			}

			nMinMax = std::min<int>(nMyMaxFS, nMaxFS);

			int nLevelFS = MaxFSfromSipLevelGet(unLevel);
			if (nMinMax > nLevelFS)
			{
				(*mpRetVal)[g_fmtpMaxFS] = std::to_string(nMinMax);
			}
			else
			{
				mpRetVal->erase(g_fmtpMaxFS); // level defaults.
			}

			break;
		}

		default:
			// No FMTP settings for this type
			break;
	}

#if defined(stiHOLDSERVER) || defined(stiMEDIASERVER)
	// For the HoldServer, we don't currently have videos that support SIF; remove it.
	mpRetVal->erase ("SIF");
#else
	// If we have SIF, we need to remove CIF from the list, to let the remote side know we'd rather do SIF.
#if !defined(stiMVRS_APP)
	if (mpRetVal->find ("SIF") != mpRetVal->end ())
	{
		mpRetVal->erase ("CIF");
	}
#endif
#endif

	// Remove unsupported resolutions
	mpRetVal->erase ("QQVGA");
	mpRetVal->erase ("QVGA");
	mpRetVal->erase ("VGA");
}


////////////////////////////////////////////////////////////////////////////////
//;	CstiSipCall::StackCallHandleSet
//
// Description: Sets up the stack call leg handle to work with this call object
//
// Side-Effects:
//	Will set up for SDP call negotiation.
//
// Returns:
//  estiOK - Success
//	estiERROR = Failure
//
void CstiSipCall::StackCallHandleSet (RvSipCallLegHandle hCallLeg)
{
	m_hCallLeg = hCallLeg;
	ConnectionKeepaliveStart();
}  // End CstiSipCall::StackCallHandleSet


EstiBool SipTokenValidate (
	IN const char* szString)
{
	const char* szSIP_TOKENS = "-.!%*_+~";
	EstiBool bIsToken = estiTRUE;
	int i;
	if (nullptr != szString)
	{
		int nStrLen = strlen (szString);
		for (i = 0; i < nStrLen && bIsToken; i++)
		{
			if (isalpha (szString[i]) ||
				isdigit (szString[i]))
			{
				continue;
			} // end if

			unsigned int j;
			unsigned int nLen = strlen (szSIP_TOKENS);
			bIsToken = estiFALSE;
			for (j = 0; j < nLen; j++)
			{
				if (szString[i] == szSIP_TOKENS[j])
				{
					// Found the current character amongst the tokens,
					// drop out of this inner loop
					bIsToken = estiTRUE;
					break;
				} // end if
			} // end for
		} // end for
	} // end if
	else
	{
		bIsToken = estiFALSE;
	} // end else

	return (bIsToken);
} // end SipTokenValidate


void CstiSipCall::RemoteProductNameGet (
	std::string *pRemoteProductName) const
{
	//
	// m_hCallleg is set if we just have one call.  If we multiple calls then
	// we cannot determine the product name of the remote endpoint.
	//
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->RemoteProductNameGet (pRemoteProductName);
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}
}


void CstiSipCall::RemoteProductVersionGet (
	std::string *pRemoteProductVersion) const
{
	//
	// m_hCallleg is set if we just have one call.  If we multiple calls then
	// we cannot determine the product version of the remote endpoint.
	//
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->RemoteProductVersionGet (pRemoteProductVersion);
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}
}


void CstiSipCall::RemoteAutoSpeedSettingGet (
	EstiAutoSpeedMode *peAutoSpeedSetting) const
{
	//
	// m_hCallleg is set if we just have one call.  If we have multiple calls then
	// we cannot determine the auto speed setting of the remote endpoint.
	//
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->RemoteAutoSpeedSettingGet(peAutoSpeedSetting);
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}
}


/*!\brief Returns a boolean indicating whether or not the call be transferred.
 * 
 * Note: this call is only valid once the remote endpoint is connected.
 */
EstiBool CstiSipCall::IsTransferableGet () const
{
	EstiBool bResult = estiFALSE;

	//
	// m_hCallleg is set if we just have one call.  If we multiple calls then
	// we cannot determine the product version of the remote endpoint.
	//
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg->IsTransferableGet ())
		{
			bResult = estiTRUE;
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}

	return (bResult);
};


/*!\brief Is the remote device type of the type passed in the parameter list?
 *
 */
EstiBool CstiSipCall::RemoteDeviceTypeIs (
	int nDeviceType) const
{
	EstiBool bCheck = estiFALSE;

	//
	// m_hCallleg is set if we just have one call.  If we multiple calls then
	// we cannot determine the device type.
	//
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			bCheck = (callLeg->RemoteDeviceTypeIs (nDeviceType) ? estiTRUE : estiFALSE);
		}
		else
		{
			stiASSERT (estiFALSE);
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}

	return bCheck;
}


////////////////////////////////////////////////////////////////////////////////
//; RemotePtzCapsGet
//
// Description: Return the remote PTZ capabilities
//
// Abstract:
//
// Returns:
//	The remote PTZ capability bitmap (or 0 if we don't have a channel object)
//
int CstiSipCall::RemotePtzCapsGet () const
{
	int nRet = 0;

	if (m_videoRecord)
	{
		nRet = m_videoRecord->RemotePtzCapsGet ();
	}

	return nRet;
}


/*!
* \brief Add a payload type to the allowed list
* \return nothing
*/
void CstiSipCall::VideoPayloadMappingAdd (
	int8_t n8PayloadTypeNbr, ///< The codec's id number
	EstiVideoCodec eVideoCodec, ///< The corresponding vp codec
	EstiVideoProfile eProfile, ///< The video codec profile to be used with this mapping
	EstiPacketizationScheme ePacketizationScheme, ///< The packetization scheme of the codec
	std::list<RtcpFeedbackType> rtcpFeedbackTypes)
{
	VideoPayloadAttributes payloadAttrs;
	payloadAttrs.eCodec = eVideoCodec;
	payloadAttrs.eProfile = eProfile;
	payloadAttrs.ePacketizationScheme = ePacketizationScheme;
	payloadAttrs.rtcpFeedbackTypes = rtcpFeedbackTypes;
	m_mVideoPayloadMappings[n8PayloadTypeNbr] = payloadAttrs;
	if (m_videoPlayback)
	{
		m_videoPlayback->PayloadMapSet (m_mVideoPayloadMappings);
	}
}


/*!\brief Retreives the payload ID that has been mapped to the provided codec and packetization scheme.
 *
 * \param eVideoCodec The video codec for which the payload number is to be retrieved.
 * \param EstiVideoProfile The video codec profile to be used with this mapping.
 * \param ePacketizationScheme The packetization scheme for which the payload number is to be retrieved.
 * \param pnPayloadTypeNbr The payload number found for the codec and packeization scheme.  This will be -1 if no mapping was found.
 */
void CstiSipCall::VideoPayloadMapGet (
	EstiVideoCodec eVideoCodec,
	EstiVideoProfile eProfile,
	EstiPacketizationScheme ePacketizationScheme,
	int *pnPayloadTypeNbr)
{
	int nResult = -1;

	for (const auto &payloadMapping: m_mVideoPayloadMappings)
	{
		if (payloadMapping.second.eCodec == eVideoCodec &&
			payloadMapping.second.eProfile == eProfile &&
			payloadMapping.second.ePacketizationScheme == ePacketizationScheme)
		{
			nResult = payloadMapping.first;
			break;
		}
	}

	*pnPayloadTypeNbr = nResult;
}


/*!\brief Determines if the a video payload ID is already in use
 *
 */
bool CstiSipCall::VideoPayloadIDUsed (
	int nPayloadTypeNbr) const
{
	auto i = m_mVideoPayloadMappings.find(nPayloadTypeNbr);

	return (i != m_mVideoPayloadMappings.end ());
}


stiHResult CstiSipCall::VideoPayloadDynamicIDGet (
	int *pnPayloadTypeNbr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nFirstPayloadTypeNbr = NextDynamicPayloadTypeGet ();
	int nPayloadTypeNbr = nFirstPayloadTypeNbr;

	for (;;)
	{
		//
		// Check to see if this payload ID is already in use.
		//
		auto i = m_mVideoPayloadMappings.find(nPayloadTypeNbr);

		//
		// If this payload ID does not exist in the mappings
		// then we found an available ID.  Break out of the loop.
		//
		if (i == m_mVideoPayloadMappings.end ())
		{
			break;
		}

		nPayloadTypeNbr = NextDynamicPayloadTypeGet ();

		//
		// We should never see the first payload number we retreived. If we
		// do that means the dynamic payload IDs have been exhausted.
		//
		stiTESTCOND (nPayloadTypeNbr != nFirstPayloadTypeNbr, stiRESULT_ERROR);
	}

	*pnPayloadTypeNbr = nPayloadTypeNbr;

STI_BAIL:

	return hResult;
}


/*!
* \brief Clear the allowed codec list
* \return nothing
*/
void CstiSipCall::VideoPayloadMappingsClear ()
{
	m_mVideoPayloadMappings.clear ();
	
	if (m_videoPlayback)
	{
		m_videoPlayback->PayloadMapSet (m_mVideoPayloadMappings);
	}
}


/*!
* \brief Add a payload type to the allowed list
* \return nothing
*/
void CstiSipCall::AudioPayloadMappingAdd (
	int8_t n8PayloadTypeNbr, ///< The codec's id number
	EstiAudioCodec eAudioCodec, ///< The corresponding ap codec
	EstiPacketizationScheme ePacketizationScheme, ///< The packetization scheme of the codec
	uint32_t clockRate) ///< The clock rate being used
{
	AudioPayloadAttributes payloadAttrs{};
	payloadAttrs.eCodec = eAudioCodec;
	payloadAttrs.ePacketizationScheme = ePacketizationScheme;
	payloadAttrs.clockRate = clockRate;
	m_mAudioPayloadMappings[n8PayloadTypeNbr] = payloadAttrs;
	if (m_audioPlayback)
	{
		m_audioPlayback->PayloadMapSet (m_mAudioPayloadMappings);
	}
}


/*!\brief Retreives the payload ID that has been mapped to the provided codec and packetization scheme.
 *
 * \param eAudioCodec The audio codec for which the payload number is to be retrieved.
 * \param ePacketizationScheme The packetization scheme for which the payload number is to be retrieved.
 * \param pnPayloadTypeNbr The payload number found for the codec and packeization scheme.  This will be -1 if no mapping was found.
 */
void CstiSipCall::AudioPayloadMapGet (
	EstiAudioCodec eAudioCodec,
	EstiPacketizationScheme ePacketizationScheme,
	uint32_t clockRate,
	int *pnPayloadTypeNbr)
{
	int nResult = -1;

	for (const auto &payloadMapping: m_mAudioPayloadMappings)
	{
		if (payloadMapping.second.eCodec == eAudioCodec && payloadMapping.second.ePacketizationScheme == ePacketizationScheme && payloadMapping.second.clockRate == clockRate)
		{
			nResult = payloadMapping.first;
			break;
		}
	}

	*pnPayloadTypeNbr = nResult;
}


/*!\brief Determines if the a audio payload ID is already in use
 *
 */
bool CstiSipCall::AudioPayloadIDUsed (
	int nPayloadTypeNbr) const
{
	auto i = m_mAudioPayloadMappings.find(nPayloadTypeNbr);

	return (i != m_mAudioPayloadMappings.end ());
}


stiHResult CstiSipCall::AudioPayloadDynamicIDGet (
	int *pnPayloadTypeNbr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nFirstPayloadTypeNbr = NextDynamicPayloadTypeGet ();
	int nPayloadTypeNbr = nFirstPayloadTypeNbr;

	for (;;)
	{
		//
		// Check to see if this payload ID is already in use.
		//
		auto i = m_mAudioPayloadMappings.find(nPayloadTypeNbr);

		//
		// If this payload ID does not exist in the mappings
		// then we found an available ID.  Break out of the loop.
		//
		if (i == m_mAudioPayloadMappings.end ())
		{
			break;
		}

		nPayloadTypeNbr = NextDynamicPayloadTypeGet ();

		//
		// We should never see the first payload number we retreived. If we
		// do that means the dynamic payload IDs have been exhausted.
		//
		stiTESTCOND (nPayloadTypeNbr == nFirstPayloadTypeNbr, stiRESULT_ERROR);
	}

	*pnPayloadTypeNbr = nPayloadTypeNbr;

STI_BAIL:

	return hResult;
}


/*!
* \brief Add a payload type to the allowed list
* \return nothing
*/
void CstiSipCall::TextPayloadMappingAdd (
	int8_t n8PayloadTypeNbr, ///< The codec's id number
	EstiTextCodec eTextCodec, ///< The corresponding tp codec
	EstiPacketizationScheme ePacketizationScheme) ///< The packetization scheme of the codec
{
	TextPayloadAttributes payloadAttrs{};
	payloadAttrs.eCodec = eTextCodec;
	payloadAttrs.ePacketizationScheme = ePacketizationScheme;
	m_mTextPayloadMappings[n8PayloadTypeNbr] = payloadAttrs;
	if (m_textPlayback)
	{
		m_textPlayback->PayloadMapSet (m_mTextPayloadMappings);
	}
}


/*!\brief Retreives the payload ID that has been mapped to the provided codec and packetization scheme.
 *
 * \param eTextCodec The text codec for which the payload number is to be retrieved.
 * \param ePacketizationScheme The packetization scheme for which the payload number is to be retrieved.
 * \param pnPayloadTypeNbr The payload number found for the codec and packeization scheme.  This will be -1 if no mapping was found.
 */
void CstiSipCall::TextPayloadMapGet (
	EstiTextCodec eTextCodec,
	EstiPacketizationScheme ePacketizationScheme,
	int *pnPayloadTypeNbr)
{
	int nResult = -1;

	for (const auto &payloadMapping: m_mTextPayloadMappings)
	{
		if (payloadMapping.second.eCodec == eTextCodec && payloadMapping.second.ePacketizationScheme == ePacketizationScheme)
		{
			nResult = payloadMapping.first;
			break;
		}
	}

	*pnPayloadTypeNbr = nResult;
}


/*!\brief Determines if the a text payload ID is already in use
 *
 */
bool CstiSipCall::TextPayloadIDUsed (
	int nPayloadTypeNbr) const
{
	auto i = m_mTextPayloadMappings.find(nPayloadTypeNbr);

	return (i != m_mTextPayloadMappings.end ());
}


stiHResult CstiSipCall::TextPayloadDynamicIDGet (
	int *pnPayloadTypeNbr)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nFirstPayloadTypeNbr = NextDynamicPayloadTypeGet ();
	int nPayloadTypeNbr = nFirstPayloadTypeNbr;

	for (;;)
	{
		//
		// Check to see if this payload ID is already in use.
		//
		auto i = m_mTextPayloadMappings.find(nPayloadTypeNbr);

		//
		// If this payload ID does not exist in the mappings
		// then we found an available ID.  Break out of the loop.
		//
		if (i == m_mTextPayloadMappings.end ())
		{
			break;
		}

		nPayloadTypeNbr = NextDynamicPayloadTypeGet ();

		//
		// We should never see the first payload number we retreived. If we
		// do that means the dynamic payload IDs have been exhausted.
		//
		stiTESTCOND (nPayloadTypeNbr == nFirstPayloadTypeNbr, stiRESULT_ERROR);
	}

	*pnPayloadTypeNbr = nPayloadTypeNbr;

STI_BAIL:

	return hResult;
}


/*!
* \brief Clear the allowed codec list
* \return nothing
*/
void CstiSipCall::AudioPayloadMappingsClear ()
{
	m_mAudioPayloadMappings.clear ();
	if (m_audioPlayback)
	{
		m_audioPlayback->PayloadMapSet (m_mAudioPayloadMappings);
	}
}


/*!
* \brief Clear the allowed codec list
* \return nothing
*/
void CstiSipCall::TextPayloadMappingsClear ()
{
	m_mTextPayloadMappings.clear ();
	if (m_textPlayback)
	{
		m_textPlayback->PayloadMapSet (m_mTextPayloadMappings);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::LostConnectionCheck
//
// Description: Check for a lost connection
//
// Abstract:
//  This method begins the check to see if the remote device is still out there.
//
// Returns:
//  nothing
//
void CstiSipCall::LostConnectionCheck (bool bDisconnectTimerStart)
{
	// Don't expect to have a media if the call isn't even connected.
	if (CstiCallGet ()->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT))
	{
		// Send a ping.
		DBG_MSG ("Checking for lost connection...");
		
		m_poSipManager->ReInviteSend (CstiCallGet (), 0, bDisconnectTimerStart);
	}
} // end CstiSipCall::LostConnectionCheck


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::FlowControlSend
//
// Description: Send a flow control message for the video playback channel
//
// Abstract:
//
// Returns:
//	none
//
void CstiSipCall::FlowControlSend (
	std::shared_ptr<CstiVideoPlayback> videoPlayback,
	int nNewRate)
{
	//
	// Send to the SIP manager so that it comes through the SIP thread.
	//
	m_poSipManager->VideoFlowControlSend (shared_from_this(), videoPlayback, nNewRate);
}


/*!\brief This method handles a video flow control sent from the SIP manager.
 * 
 * This method is in place to avoid a deadlock in the Radvision stack
 * 
 * \param videoPlayback The video playback data task to set the rate on.
 * \param nNewRate The new rate to set the playback channel to (in 100kbps units).
 */
stiHResult CstiSipCall::VideoFlowControlSendHandle (
	std::shared_ptr<CstiVideoPlayback> videoPlayback,
	int nNewRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (videoPlayback == m_videoPlayback)
	{
		DBG_MSG ("Sending video bandwidth flow control...");
		m_videoPlayback->FlowControlRateSet (nNewRate);
		DBG_MSG ("FCS m_videoPlayback rate=%d", nNewRate / 1000);
		
		// If this is a gateway call do not send a reinvite.
		// Reinvites through the gateway cause call stability issues with 3rd party gateways.
		if (!m_bGatewayCall)
		{
			ReInviteSend (false, 0);
		}
	}
	else
	{
		stiASSERTMSG (estiFALSE, "Cannot set flow control for unknown channel %p", videoPlayback.get());
	}
	
	return hResult;
}

void CstiSipCall::TextShareRemoveRemote()
{
	m_poSipManager->PostEvent(
		[this]
		{
			stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
			std::stringstream stream;

			stiTESTCOND (m_hCallLeg, stiRESULT_ERROR);

			{
				auto callLeg = CallLegGet (m_hCallLeg);
				stiTESTCOND(callLeg, stiRESULT_ERROR);

				stream << std::setfill('0') << std::setw(4) << estiSM_CLEAR_TEXTSHARE << std::setw(4) << 0;

				{
					auto transaction = callLeg->createTransaction();

					auto outboundMessage = transaction.outboundMessageGet();

					outboundMessage.bodySet(SORENSON_CUSTOM_MIME, stream.str());

					hResult = transaction.requestSend(INFO_METHOD);
					stiTESTRESULT();
				}

				DBG_MSG ("Sent TextShareRemoveRemote message");

				// Need to notify ui/vrcl
				{
					auto sipCall = callLeg->m_sipCallWeak.lock ();
					auto call = sipCall->CstiCallGet ();

					m_poSipManager->removeTextSignal.Emit (call);
				}

			}

		STI_BAIL:;
		}
	);
}

#ifdef stiHOLDSERVER
stiHResult CstiSipCall::HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_videoRecord)
	{
		hResult = m_videoRecord->HSEndService(eMediaServiceSupportState);
	}
	else
	{
		m_signalConnections.push_back (videoRecordInitialized.Connect ([this, eMediaServiceSupportState] () {
			m_videoRecord->HSServiceStartStateSet (eMediaServiceSupportState);
		}));
	}

	return hResult;
}
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::InviteInsideOfReInviteCheck
//
// Description: Determine if we have an invite inside of an invite.
//
// Abstract:
// If detected, we will appropriately handle the situation.
//
// Returns:
//	none
//
void CstiSipCall::InviteInsideOfReInviteCheck ()
{
	auto call = CstiCallGet ();

	// If true, rfc3261 says that a timer with value of T should be set as follows:
	//
	// - If the UAC is the owner of the Call-ID (the call initiator), T has a randomly chosen value
	// between 2.1 and 4 seconds in units of 10 ms.
	//
	// - If the UAC is NOT the owner of the Call-ID, T has a randomly chosen value of between 0 and 2
	// seconds in units of 10 ms.
	int nRandom = 0;

	// Seed the random generator
	srand(stiOSTickGet ());

	if (estiINCOMING == call->DirectionGet ())
	{
		// Need a random number between 0 and 2 seconds
		nRandom = rand () % 201;  // There will be 201 possible 10ms choices in this range
		nRandom *= 10; // Convert to ms from units of 10 ms
	}
	else
	{
		// Need a random number between 2.1 and 4 seconds
		nRandom = rand () % 191;  // There will be 200 possible 10ms choices in this range
		nRandom *= 10; // Convert to ms from units of 10 ms
		nRandom += 2100;	// Move it to be in the proper window (by adding 2100 ms)
	}

	DBG_MSG ("SIP RE-INVITE: Timing out in %d milli-seconds", nRandom);

	m_reinviteTimer.TimeoutSet (nRandom);
	m_reinviteTimer.Start ();
}


void CstiSipCall::AlertUserCountDownSet (
	int nCount)
{
	if (nCount == 0)
	{
#if 0
		AlertLocalUserSet(true);
#else
		AlertLocalUser();
#endif
	}
	
	m_nAlertUserCountDown = nCount;
}


void CstiSipCall::AlertUserCountDownDecr ()
{
	AlertUserCountDownSet(m_nAlertUserCountDown - 1);
}


void CstiSipCall::AlertLocalUserSet (
	bool bAlertLocalUser)
{
	m_bAlertLocalUser = bAlertLocalUser;
}


bool CstiSipCall::AlertLocalUserGet ()
{
	return m_bAlertLocalUser;
}


void CstiSipCall::AlertLocalUser ()
{
	//
	// If we have not yet alerted the user
	// then do so now.
	//
	if (!m_bLocalUserAlerted)
	{
		m_bLocalUserAlerted = true;

		CallValidSet ();
		SipManagerGet()->NextStateSet (CstiCallGet (), esmiCS_CONNECTING, estiSUBSTATE_WAITING_FOR_USER_RESP);
		InboundCallTimerStart ();
	}
}


void CstiSipCall::VideoRecordDataStart ()
{
	if (m_videoRecord)
	{
		m_videoRecord->DataStart ();
	}
}


/**
 * Gets the CallId
 */
void CstiSipCall::CallIdGet (std::string *pCallId)
{
	pCallId->assign(m_CallId);
}


/**
 * Sets the CallId
 */
void CstiSipCall::CallIdSet (const char *pCallId)
{
	auto call = CstiCallGet();
	if (call)
	{
		call->collectTrace(formattedString("%s: %s", __func__, pCallId));
	}
	m_CallId.assign(pCallId);
}


/**
 * Gets the Original Call ID
 */
void CstiSipCall::OriginalCallIDGet (std::string *pCallId)
{
	pCallId->assign(m_OriginalCallID);
}


/*!\brief Creates a call leg object associated to the call leg handle passed in
 * 
 * \param hCallLeg The Radvision to create a call leg object for
 */
CstiSipCallLegSP CstiSipCall::CallLegAdd (
	RvSipCallLegHandle hCallLeg)
{
	// First check to see if this call leg has already been added.
	// If it hasn't then create the call leg and add it to the map.
	auto callLeg = CallLegGet (hCallLeg);
	if (!callLeg)
	{
		callLeg = std::make_shared<CstiSipCallLeg>();
		callLeg->m_sipCallWeak = shared_from_this();
		callLeg->m_hCallLeg = hCallLeg;
		m_CallLeg[hCallLeg] = callLeg;
		callLeg->vrsFocusedRoutingSentSet (m_vrsFocusedRoutingSent);
	}

	CstiCallGet ()->callLegCreatedSet(true);
	
	return callLeg;
}


/*!\brief Returns the call leg object associated with the Radvision call leg handle
 *
 * \param hCallLeg The Radvision call leg handle for which to retrieve the call leg object
 */
CstiSipCallLegSP CstiSipCall::CallLegGet (
	RvSipCallLegHandle hCallLeg) const
{
	CstiSipCallLegSP callLeg = nullptr;
	auto it = m_CallLeg.begin ();
	
	// If a call leg handle was passed in then try to 
	// retrieve the call leg from the map.
	// If NULL was passed in then just get the 
	// first call leg from the map.
	if (hCallLeg)
	{
		it = m_CallLeg.find (hCallLeg);
	}
	
	if (it != m_CallLeg.end ())
	{
		callLeg = it->second;
	}
	
	return callLeg;
}


/*!\brief Ends the ICE sessions for all call legs.
 * 
 */
void CstiSipCall::IceSessionsEnd ()
{
	for (auto &kv : m_CallLeg)
	{
		if (kv.second->m_pIceSession)
		{
			kv.second->m_pIceSession->SessionEnd ();
			kv.second->m_pIceSession = nullptr;
		}
	}
}


/*!\brief Terminates all forked call legs (call legs that don't match m_hCallLeg)
 * 
 */
stiHResult CstiSipCall::ForkedCallLegsTerminate ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Terminate the forked call legs
	for (auto &kv : m_CallLeg)
	{
		if (kv.first != m_hCallLeg)
		{
			RvSipCallLegTerminate (kv.first);
		}
	}
	
	return hResult;
}

/*!\brief Checks to see if all the current call legs have failed ICE nominations
 *
 * @return True if all call legs have failed ICE nominations.  Otherwise, returns false.
 */
bool CstiSipCall::AllLegsFailedICENominations ()
{
	bool allFailed = true;
	for (auto &kv : m_CallLeg)
	{
		if (kv.second->m_eCallLegIceState != estiFailed)
		{
			allFailed = false;
			break;
		}
	}

	return allFailed;
}


////////////////////////////////////////////////////////////////////////////////
//; ContactSerialize
//
// Description: Serializes a contact struct
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR
//
stiHResult ContactSerialize (
	const SstiSharedContact &contact, ///! The contact struct you wish to serialize
	std::string *pResultBuffer) ///! The buffer to write the serialized contact to
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::stringstream ss;
	
	stiTESTCOND (pResultBuffer, stiRESULT_ERROR);

	ss << CONTACT_NAME_INDICATOR << contact.Name.c_str () << "\n";
	ss << CONTACT_COMPANY_NAME_INDICATOR << contact.CompanyName.c_str () << "\n";
	ss << CONTACT_DIAL_STRING_INDICATOR << contact.DialString.c_str () << "\n";

	// If the number type is unknown then default it to Home.
	if (contact.eNumberType == ePHONE_UNKNOWN)
	{
		ss << CONTACT_NUMBER_TYPE_INDICATOR << ePHONE_HOME << "\n";
	}
	else
	{
		ss << CONTACT_NUMBER_TYPE_INDICATOR << contact.eNumberType << "\n";
	}
	ss << "END\n"; // required
	
	*pResultBuffer = ss.str ();
	
STI_BAIL:
	
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; ContactToText
//
// Description: Formats a contact struct as human-readable text
//		The format will be a two-line text message like:
//			Name: Jake Smith; 
//			Cell: (555) 555-5555;
//		(Where the number type will be omitted if unknown.)
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR
//
stiHResult ContactToText (
	const SstiSharedContact &contact, ///! The contact struct you wish to serialize
	std::string *pResultBuffer) ///! The buffer to write the serialized contact to
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::stringstream ss;
	int nPhoneNumberFormat = 0;
	
	stiTESTCOND (pResultBuffer, stiRESULT_ERROR);

	// Adding a new line in case text has already been shared.
	ss << "\r\n";

	if (!contact.Name.empty ())
	{
		ss << "Name: " << contact.Name << "\r\n";
	}
	if (!contact.CompanyName.empty ())
	{
		ss << "Company: " << contact.CompanyName << "\r\n";
	}
	switch (contact.eNumberType)
	{
		case ePHONE_HOME:
			ss << "Home: ";
			break;
		case ePHONE_WORK:
			ss << "Work: ";
			break;
		case ePHONE_CELL:
			ss << "Cell: ";
			break;
		case ePHONE_UNKNOWN:
		case ePHONE_MAX:
			break;
		// default: Commented out for type safety
	}

	// decide what we have
	if (contact.DialString.size() == 11)
	{
		nPhoneNumberFormat = 11;
	}
	else if (contact.DialString.size() == 7)
	{
		nPhoneNumberFormat = 7;
	}
	else if (contact.DialString.size() == 10)
	{
		nPhoneNumberFormat = 10;
	}
	
	if (nPhoneNumberFormat)
	{
		for (const auto &character: contact.DialString)
		{
			if (!isdigit (character))
			{
				nPhoneNumberFormat = 0;
				break;
			}
		}
	}

	// format the phone number
	if (nPhoneNumberFormat == 11)
	{
		ss << "(" << contact.DialString.substr (1, 3) << ") " << contact.DialString.substr (4, 3) << '-' << contact.DialString.substr (7, 4);
	}
	else if (nPhoneNumberFormat == 10)
	{
		ss << '(' << contact.DialString.substr (0, 3) << ") " << contact.DialString.substr (3, 3) << '-' << contact.DialString.substr (6, 4);
	}
	else if (nPhoneNumberFormat == 7)
	{
		ss << contact.DialString.substr (0, 3) << '-' << contact.DialString.substr (3, 4);
	}
	else
	{
		ss << contact.DialString;
	}

	// If the user types more text after sharing a text contact, this 
	// space will ensure the text isn't butted together.
	ss << " ";
	
	*pResultBuffer = ss.str ();
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; ContactDeserialize
//
// Description: Deserializes a string into a contact struct
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR
//
stiHResult ContactDeserialize (
	const char *pszBuffer, ///! The buffer containing the contact information
	SstiSharedContact *contact) ///! The contact struct to fill out with the results
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char *szTokenizedBuffer = nullptr;
	char *pszLast;
	
	stiTESTCOND (pszBuffer, stiRESULT_ERROR);
	
	stiTESTCOND (contact, stiRESULT_ERROR);
	szTokenizedBuffer = new char[strlen (pszBuffer) + 1];
	pszLast = szTokenizedBuffer;
	
	for (int i = 0; pszBuffer[i]; i++)
	{
		if (pszBuffer[i] == '\n') // we just completed a line
		{
			szTokenizedBuffer[i] = '\0';
			if (pszLast == &szTokenizedBuffer[i])
			{
				// skip over empty lines
			}
			else if (0 == strncmp (pszLast, CONTACT_NAME_INDICATOR, sizeof (CONTACT_NAME_INDICATOR)-1))
			{
				contact->Name =  &pszLast[sizeof (CONTACT_NAME_INDICATOR) - 1];
			}
			else if (0 == strncmp (pszLast, CONTACT_COMPANY_NAME_INDICATOR, sizeof (CONTACT_COMPANY_NAME_INDICATOR)-1))
			{
				contact->CompanyName =  &pszLast[sizeof (CONTACT_COMPANY_NAME_INDICATOR) - 1];
			}
			else if (0 == strncmp (pszLast, CONTACT_DIAL_STRING_INDICATOR, sizeof (CONTACT_DIAL_STRING_INDICATOR)-1))
			{
				contact->DialString =  &pszLast[sizeof (CONTACT_DIAL_STRING_INDICATOR) - 1];
			}
			else if (0 == strncmp (pszLast, CONTACT_NUMBER_TYPE_INDICATOR, sizeof (CONTACT_NUMBER_TYPE_INDICATOR)-1))
			{
				contact->eNumberType = (EPhoneNumberType)atoi (&pszLast[sizeof (CONTACT_NUMBER_TYPE_INDICATOR) - 1]);
			}

			pszLast = &szTokenizedBuffer[i + 1];
		}
		else
		{
			szTokenizedBuffer[i] = pszBuffer[i];
		}
	}
	
STI_BAIL:

	if (szTokenizedBuffer)
	{
			delete [] szTokenizedBuffer;
			szTokenizedBuffer = nullptr;
	}
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::ShareContact
//
// Description: Send a contact to the remote device (if supported)
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR (ORed with an error code).
//
stiHResult CstiSipCall::ContactShare (const SstiSharedContact &contact)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallLegSP callLeg = nullptr;
	std::string encodedContactBuffer;
	
	// Ensure that we can send text according to our side.
	stiTESTCOND (m_poSipManager->LocalShareContactSupportedGet (), stiRESULT_ERROR);
	
	// Get the call leg in question.
	if (m_hCallLeg)
	{
		callLeg = CallLegGet (m_hCallLeg);
	}
	
	// Determine how to send the contact
	if (callLeg && 
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE) && 
		callLeg->ShareContactSupportedGet ())
	{
		// Send using the share contact feature
		hResult = ContactSerialize (contact, &encodedContactBuffer);
		stiTESTRESULT ();
		
		SipInfoSend(CONTACT_MIME, INFO_PACKAGE, CONTACT_PACKAGE, encodedContactBuffer);

	}
	else if (TextSupportedGet ())
	{
		// Send using the share text feature
		std::vector<uint16_t> textMessage;
		
		hResult = ContactToText (contact, &encodedContactBuffer);
		stiTESTRESULT ();
		
		for (const auto &character: encodedContactBuffer)
		{
			textMessage.push_back (character);
		}
		textMessage.push_back ('\0');
		
		TextSend (textMessage.data (), estiSTS_SAVED_TEXT);
	}
	else
	{
		// We have no way of sending a contact
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::ShareContactSupportedGet
//
// Description: Determine if sending contacts is allowed for this call
//
// Returns:
//	true or false
//
bool CstiSipCall::ShareContactSupportedGet ()
{
	bool bAllowed = false;
	
	if (m_poSipManager->LocalShareContactSupportedGet ())
	{
		CstiSipCallLegSP callLeg = nullptr;
		
		if (m_hCallLeg)
		{
			callLeg = CallLegGet (m_hCallLeg);
		}
		
		if (callLeg && callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
		{
			// We can send using either the share contact mechanism or a plain text message.
			bAllowed = callLeg->ShareContactSupportedGet () || TextSupportedGet ();
		}
	}
	
	return bAllowed;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredDisplaySizeSend
//
// Description: Send the devices preferred display size to the remote endpoint
//
// Returns:
//	Success or failure.
//
stiHResult CstiSipCall::PreferredDisplaySizeSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallLegSP callLeg = nullptr;

	// Get the call leg in question.
	if (m_hCallLeg)
	{
		callLeg = CallLegGet (m_hCallLeg);
	}
	
	unsigned int preferredDisplayWidth = 0;
	unsigned int preferredDisplayHeight = 0;
	IstiVideoOutput::InstanceGet ()->PreferredDisplaySizeGet(
		&preferredDisplayWidth,
		&preferredDisplayHeight);
	auto call = CstiCallGet ();
	
	if (call)
	{
		call->PreferredDisplaySizeChangeLog ();
	}

	// Send the preferred display size.
	if (callLeg &&
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE) &&
		preferredDisplayWidth != 0 &&
		preferredDisplayHeight != 0)
	{
		// Attach the data
		std::stringstream displaySizeStream;
		displaySizeStream << preferredDisplayWidth << ":" << preferredDisplayHeight;

		SipInfoSend(DISPLAY_SIZE_MIME, INFO_PACKAGE, DISPLAY_SIZE_PACKAGE, displaySizeStream.str ());
	}

	return hResult;
}


/*!\brief Sends a SIP Info message with geolocation
 *
 */
stiHResult CstiSipCall::GeolocationSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Send the geolocation information.
	if (!m_poSipManager->m_geolocation.empty())
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		stiTESTCOND(callLeg, stiRESULT_ERROR);

		auto transaction = callLeg->createTransaction();

		auto outboundMessage = transaction.outboundMessageGet();
		
		// Attach the geolocation information to the message
		hResult = outboundMessage.geolocationAdd ();
		stiTESTRESULT ();
		
		hResult = transaction.requestSend(INFO_METHOD);
		stiTESTRESULT ();
	}
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredAudioProtocolsGet
//
// Description: Gets a list of all supported audio protocols in preferred order.
//
// Returns:
//	the list
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::PreferredAudioProtocolsGet () const
{
	return &m_PreferredAudioProtocols;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::AudioFeatureProtocolsGet
//
// Description: Gets a list of all audio feature protocols.
//
// Returns:
//	the list
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::AudioFeatureProtocolsGet () const
{
	return &m_AudioFeatureProtocols;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredTextPlaybackProtocolsGet
//
// Description: Gets a list of all supported text protocols in preferred order.
//
// NOTE: Currently this is the same thing as PreferredTextRecordProtocolsGet()
//	but if we start having devices with different capabilities for sending
//	and recieving then this must be implemented.
//
// Returns:
//	the list if text is enabled, otherwise NULL
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::PreferredTextPlaybackProtocolsGet () const
{
	if (m_textStream.supported ())
	{
		return &m_PreferredTextProtocols;
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredTextRecordProtocolsGet
//
// Description: Gets a list of all supported text protocols in preferred order.
//
// Returns:
//	the list if text is enabled, otherwise NULL
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::PreferredTextRecordProtocolsGet () const
{
	if (m_textStream.outboundSupported ())
	{
		return &m_PreferredTextProtocols;
	}

	return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredVideoPlaybackProtocolsGet
//
// Description: Gets a list of all supported video protocols in preferred order.
//
// Returns:
//	the list
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::PreferredVideoPlaybackProtocolsGet () const
{
	return &m_PreferredVideoPlaybackProtocols;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::PreferredVideoRecordProtocolsGet
//
// Description: Gets a list of all supported video protocols in preferred order.
//
// Returns:
//	the list
//
const std::list<SstiPreferredMediaMap> *CstiSipCall::PreferredVideoRecordProtocolsGet () const
{
	return &m_PreferredVideoRecordProtocols;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::SipInfoSend
//
// Description: Send a SIP info message to a connected call.
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR (ORed with an error code).
//
stiHResult CstiSipCall::SipInfoSend (
	const std::string &contentType,
	const std::string &headerName,
	const std::string &headerValue,
	const std::string &msgBody)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto callLeg = CallLegGet (m_hCallLeg);
	stiTESTCOND(callLeg, stiRESULT_ERROR);

	{
		auto transaction = callLeg->createTransaction();

		auto outboundMessage = transaction.outboundMessageGet();

		hResult = outboundMessage.headerAdd(headerName, headerValue);
		stiTESTRESULT ();

		hResult = outboundMessage.bodySet(contentType, msgBody);
		stiTESTRESULT ();

		// Automatically send the message now if we created it.
		hResult = transaction.requestSend(INFO_METHOD);
		stiTESTRESULT();
	}
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::HearingStatusSend
//
// Description: Send the HearingStatus via SIP info message to a connected call.
//
// Returns:
//	nothing.
//
void CstiSipCall::HearingStatusSend (EstiHearingCallStatus eHearingStatus)
{
	CstiSipCallLegSP callLeg = nullptr;
	
	// Get the call leg in question.
	if (m_hCallLeg)
	{
		callLeg = CallLegGet (m_hCallLeg);
	}
	
	// Convert HearingStatus to string
	std::string HearingStatus;
	if (estiHearingCallDisconnected == eHearingStatus)
	{
		HearingStatus = g_szHearingStatusDisconnected;
		CstiCallGet ()->dhviStateSet (IstiCall::DhviState::NotAvailable);
	}
	else if (estiHearingCallConnecting == eHearingStatus)
	{
		HearingStatus = g_szHearingStatusConnecting;
	}
	else // if (estiHearingCallConnected == eHearingStatus)
	{
		HearingStatus = g_szHearingStatusConnected;
	}
	
	// We are sending this only to a Sorenson endpoint
	if (callLeg &&
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		SipInfoSend(HEARING_STATUS_MIME, INFO_PACKAGE, HEARING_STATUS_PACKAGE, HearingStatus);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::NewCallReadyStatusSend
//
// Description: Send the NewCallReadyStatus via SIP info message to a connected call.
//
// Returns:
//	nothing.
//
void CstiSipCall::NewCallReadyStatusSend (bool bNewCallReadyStatus)
{
	CstiSipCallLegSP callLeg = nullptr;
	
	// Get the call leg in question.
	if (m_hCallLeg)
	{
		callLeg = CallLegGet (m_hCallLeg);
	}
	
	// Convert NewCallReadyStatus to string
	std::string NewCallReadyStatus = (bNewCallReadyStatus ? "True" : "False");
	
	// We are sending this only to a Sorenson endpoint
	if (callLeg &&
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		SipInfoSend(NEW_CALL_READY_MIME, INFO_PACKAGE, NEW_CALL_READY_PACKAGE, NewCallReadyStatus);
	}
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::SpawnCall
//
// Description: Send the call info to the interpreter phone to placea new call.
//
// Returns:
//	nothing.
//
void CstiSipCall::SpawnCallSend (const SstiSharedContact &spawnCallInfo)
{
	CstiSipCallLegSP callLeg = nullptr;
	std::string EncodedContactBuffer;
	
	// Get the call leg in question.
	if (m_hCallLeg)
	{
		callLeg = CallLegGet (m_hCallLeg);
	}
	
	// We are sending this only to a Sorenson endpoint
	if (callLeg &&
		callLeg->RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
	{
		ContactSerialize (spawnCallInfo, &EncodedContactBuffer);
		
		SipInfoSend(SPAWN_CALL_MIME, INFO_PACKAGE, HEARING_CALL_SPAWN_PACKAGE, EncodedContactBuffer);
	}
}


/*!\brief Forces the call to failover for VRS.
 *	Sends a cancel for the current call.
 *  Sets useVRSFailoverServer to true causing a call using the VRS failover 
 *  server to be placed from CallLegStateChangedCB.
 * \return Succes or Failure.
 */
stiHResult CstiSipCall::VRSFailOverForce ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	auto call = CstiCallGet ();
	
	call->FailureTimersStop ();
	
	//
	// Shut down any media
	//
	MediaClose ();
	
	rvStatus = RvSipCallLegDisconnect (m_hCallLeg);
	stiTESTCONDMSG(rvStatus == RV_OK, stiRESULT_ERROR,
		"CstiSipCall::VRSFailOverForce Failed to disconnect call leg rvStatus = %d", rvStatus);
	
STI_BAIL:
	
	return hResult;
}


/*!\brief Configures the routing address to use the VRS failover server.
 *
 * \return Succes or Failure.
 */
stiHResult CstiSipCall::VRSFailOverConfigure ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto call = CstiCallGet ();
	auto interpreterNumberLength = 12U;
	
	// Since we are failing over ensure the failover timers are stopped.
	// They could have not fired if we failed over due to an error from the server.
	call->FailureTimersStop ();
	
	std::string user = call->RoutingAddressGet ().userGet ();
	std::string uri;
	
	// If we failed to get a number from the previous routing address use the original dialstring.
	// This fixes bug# 26957.
	// Or if we are transferring to a registered interepreter also get the original dialstring.
	if (user.empty() || (user.size() == interpreterNumberLength))
	{
		call->OriginalDialStringGet(&user);
		stiASSERTMSG(!user.empty (), "VRSFailover failed to obtain user information for CallId=%s", m_CallId.c_str ());
	}
	
	SipUriCreate (user, call->VRSFailOverServerGet (), &uri);
	CstiRoutingAddress RoutingAddress (uri);
	call->RoutingAddressSet (RoutingAddress);
	call->VRSFailOverServerSet ("");
	m_useVRSFailoverServer = true;
	call->ForcedVRSFailoverSet (true);
	
	return hResult;
}


/*!
 * \brief Updates RtpSession with the current state of TMMBR/FIR/PLI support
 *        This is used to determine whether or not VideoRecord should expect
 *        to receive TMMBR/FIR/PLI requests.
 */
void CstiSipCall::rtcpFeedbackSupportUpdate (
	const std::list<RtcpFeedbackType> &rtcpFeedbackTypes)
{
	// Initialize all feedback mechanism support to false
	auto firNewState = false;
	auto pliNewState = false;
	auto afbNewState = false;
	auto tmmbrNewState = false;
	auto nackRtxSupport = false;

	auto call = CstiCallGet ();

	// Enable only if not disabled by configuration
	for (auto fbType : rtcpFeedbackTypes)
	{
		switch (fbType)
		{
			case RtcpFeedbackType::FIR:
				firNewState = m_stConferenceParams.rtcpFeedbackFirEnabled;
				break;
				
			case RtcpFeedbackType::PLI:
				pliNewState = m_stConferenceParams.rtcpFeedbackPliEnabled;
				break;
				
			case RtcpFeedbackType::TMMBR:
				tmmbrNewState = (m_stConferenceParams.rtcpFeedbackTmmbrEnabled == SignalingSupport::Everyone ||
								 (m_stConferenceParams.rtcpFeedbackTmmbrEnabled == SignalingSupport::SorensonOnly &&
									RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE)));
				break;
				
			case RtcpFeedbackType::NACKRTX:
				nackRtxSupport = (m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::Everyone ||
								  (m_stConferenceParams.rtcpFeedbackNackRtxSupport == SignalingSupport::SorensonOnly &&
								   RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE)));
				break;
				
			case RtcpFeedbackType::UNKNOWN:
				break;
		}
	}

	// AFB is not negotiated. Enabled based on conference params to listen for AFB (ie REMB)
	afbNewState = m_stConferenceParams.rtcpFeedbackAfbEnabled;

	// Update the states in the video session
	m_videoSession->rtcpFeedbackFirEnabledSet (firNewState);
	m_videoSession->rtcpFeedbackPliEnabledSet (pliNewState);
	m_videoSession->rtcpFeedbackAfbEnabledSet (afbNewState);
	m_videoSession->rtcpFeedbackTmmbrEnabledSet (tmmbrNewState);
	m_videoSession->rtcpFeedbackNackRtxEnabledSet (nackRtxSupport);

	// Update negotiated states (used for call stats)
	call->FirNegotiatedSet (firNewState);
	call->PliNegotiatedSet (pliNewState);
	call->TmmbrNegotiatedSet (tmmbrNewState);
	call->NackRtxNegotiatedSet (nackRtxSupport);
}


bool CstiSipCall::includeEncryptionAttributes ()
{
	bool includeEncryption = false;
	EstiSecureCallMode secureCallMode (m_stConferenceParams.eSecureCallMode);
	auto call = CstiCallGet ();
	auto uri = call->RoutingAddressGet().UriGet();
	const std::string vpcc = "vcs-c01.sorenson.com";
	
	if (secureCallMode != estiSECURE_CALL_NOT_PREFERRED
		&& uri.find(vpcc) == std::string::npos) // We don't want to do encryption with Cisco VPCC
	{
		includeEncryption = true;
	}
	
	return includeEncryption;
}


vpe::EncryptionState CstiSipCall::encryptionStateGet () const
{
	auto ret = vpe::EncryptionState::AES_256;

	bool anySessionOpen {false};

	// Go through each of channels, and if any marked as not
	// encrypted, return as such

	for (auto const& session : { m_videoSession, m_audioSession, m_textSession })
	{
		if (session && session->isOpen())
		{
			anySessionOpen = true;
			if (session->encryptionStateGet () < ret)
			{
				ret = session->encryptionStateGet ();
			}
		}
	}

	if (!anySessionOpen)
	{
		ret = vpe::EncryptionState::NONE;
	}

	return ret;
}

////////////////////////////////////////////////////////////////////////////////
//; CstiSipCall::SignMailSkipToRecord()
//
// Description: Hangs up the call (if in the connecting state) and goes directly
// 				to recording a SignMail, skipping the greeting.
//
// Abstract:
//
// Returns:
//	This function returns estiOK (success), or estiERROR (failure).
//
stiHResult CstiSipCall::SignMailSkipToRecord()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiSipManager *sipManager = SipManagerGet ();
	
	hResult = sipManager->SignMailSkipToRecord (CstiCallGet ());
	
	stiTESTRESULT();

STI_BAIL:

	return hResult;
}


/*!
 * \brief Returns true if we should use ICE for calls.
 *
 */
bool CstiSipCall::UseIceForCalls ()
{
	return (SipManagerGet ()->m_iceManager
	&& m_eTransportProtocol != estiTransportUDP
	&& !SipManagerGet()->TunnelActive ()
		&& !IsBridgedCall());
}


/*!
 * \brief Sets the conferenced state for the call leg.
 *
 */
void CstiSipCall::ConferencedSet (bool conferenced)
{
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->ConferencedSet (conferenced);
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}
}


/*!
 * \brief Returns if the active callLeg reached a conferenced state.
 *
 */
bool CstiSipCall::ConferencedGet () const
{
	bool conferenced = false;
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			conferenced = callLeg->ConferencedGet ();
		}
	}
	
	return conferenced;
}


/*!
 * \brief Sets the time we start connecting.
 *
 */
void CstiSipCall::connectingTimeStart ()
{
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->connectingTimeStart ();
		}
	}
	else
	{
		stiASSERT (false);
	}
}


/*!
 * \brief Sets the time we stop connecting.
 *
 */
void CstiSipCall::connectingTimeStop ()
{
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->connectingTimeStop ();
		}
	}
	else
	{
		stiASSERT (false);
	}
}


/*!
 * \brief Returns the time time spent connecting the all in milliseconds.
 *
 */
std::chrono::milliseconds CstiSipCall::connectingDurationGet () const
{
	std::chrono::milliseconds connectingDuration = {};
	
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			connectingDuration = callLeg->connectingDurationGet ();
		}
	}
	else
	{
		stiASSERT (false);
	}
	
	return connectingDuration;
}


/*!
 * \brief Sets the time we become connected.
 *
 */
void CstiSipCall::connectedTimeStart ()
{
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->connectedTimeStart ();
		}
	}
	else
	{
		stiASSERT (false);
	}
}

/*!
 * \brief Sets the time we are ending the call.
 *
 */
void CstiSipCall::connectedTimeStop ()
{
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			callLeg->connectedTimeStop ();
		}
	}
	else
	{
		stiASSERT (false);
	}
}


/*!
 * \brief Returns the call duration in milliseconds.
 *
 */
std::chrono::milliseconds  CstiSipCall::connectedDurationGet () const
{
	std::chrono::milliseconds connectedDuration = {};
	
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			connectedDuration = callLeg->connectedDurationGet ();
		}
	}
	else
	{
		stiASSERT (false);
	}
	
	return connectedDuration;
}


/*!
 * \brief Returns the time since a call was ended in seconds.
 *
 */
std::chrono::seconds CstiSipCall::secondsSinceCallEndGet () const
{
	std::chrono::seconds timeSinceCallEnded = {};
	
	if (m_hCallLeg)
	{
		auto callLeg = CallLegGet (m_hCallLeg);
		if (callLeg)
		{
			timeSinceCallEnded = callLeg->SecondsSinceCallEndedGet ();
		}
	}
	else
	{
		stiASSERT (false);
	}
	
	return timeSinceCallEnded;
}


/*!\brief Switches active video stream between  a vrs call and its background call.
*
*/
stiHResult CstiSipCall::switchActiveCall (bool sendReinvite)
{
	auto newState = !m_videoStream.isActive ();
	auto call = m_dhviCall;
	stiHResult hResult = stiRESULT_SUCCESS;
	
	if (call)
	{
		auto sipCall = std::static_pointer_cast<CstiSipCall>(call->ProtocolCallGet());
		stiTESTCOND (sipCall != nullptr, stiERROR);
		
		sipCall->m_videoStream.setActive (m_videoStream.isActive ());
		m_videoStream.setActive (newState);
		
		if (m_videoStream.isActive ())
		{
			if (sipCall->m_videoRecord)
			{
				sipCall->m_videoRecord->Hold (estiHOLD_DHV);
			}
			
			if (sipCall->m_videoPlayback)
			{
				sipCall->m_videoPlayback->Hold (estiHOLD_DHV);
			}
			
			if (m_videoRecord)
			{
				m_videoRecord->Resume (estiHOLD_DHV);
			}
			
			if (m_videoPlayback)
			{
				m_videoPlayback->Resume (estiHOLD_DHV);
			}
		}
		else
		{
			if (m_videoRecord)
			{
				m_videoRecord->Hold (estiHOLD_DHV);
			}
			
			if (m_videoPlayback)
			{
				m_videoPlayback->Hold (estiHOLD_DHV);
			}
			
			if (sipCall->m_videoRecord)
			{
				sipCall->m_videoRecord->Resume (estiHOLD_DHV);
			}
			
			if (sipCall->m_videoPlayback)
			{
				sipCall->m_videoPlayback->Resume (estiHOLD_DHV);
			}
		}
		
		switchedActiveCallsSet (true);
		
		// We need to always send the reinvite to the other call leg (MCU) so media is flowing as intended.
		m_poSipManager->ReInviteSend (call, 0, false);
		
		if (sendReinvite)
		{
			m_poSipManager->ReInviteSend (CstiCallGet(), 0, false);
		}
	}

STI_BAIL:

	return hResult;
	
}


void CstiSipCall::sendDhviCreateMsg (const std::string &destAddress)
{
	auto dhviCommand = g_dhviCreate + " " + destAddress;
	SipInfoSend (DHVI_MIME, INFO_PACKAGE, DHVI_PACKAGE, dhviCommand);
}


void CstiSipCall::sendDhviSwitchMsg ()
{
	SipInfoSend (DHVI_MIME, INFO_PACKAGE, DHVI_PACKAGE, g_dhviSwitch);
}


void CstiSipCall::sendDhviCapabilityCheckMsg (const std::string &hearingNumber)
{
	auto message = g_dhviCapabilityCheck + " " + hearingNumber;
	SipInfoSend (DHVI_MIME, INFO_PACKAGE, DHVI_PACKAGE, message);
}


void CstiSipCall::sendDhviCapableMsg (bool capable)
{
	std::stringstream message;
	message << g_dhviCapable << " " << std::boolalpha << capable;
	SipInfoSend (DHVI_MIME, INFO_PACKAGE, DHVI_PACKAGE, message.str());
}


void CstiSipCall::sendDhviDisconnectMsg ()
{
	SipInfoSend (DHVI_MIME, INFO_PACKAGE, DHVI_PACKAGE, g_dhviDisconnect);
}


void CstiSipCall::logDhviDeafDisconnect ()
{
	auto dhviCall = m_dhviCall;
	if (dhviCall)
	{
		auto uri = dhviCall->RoutingAddressGet ().ResolvedURIGet ();
		std::string callID;
		CallIdGet (&callID);
		
		stiRemoteLogEventSend ("EventType=DHVI Reason=EndDHVI CallID=%s Room=%s", callID.c_str (), uri.c_str ());
	}
}


/*!\brief Sets if a lost connection is needed useful when we have to use a timer to resend the lost connection re-Invite.
 *
*/
void CstiSipCall::lostConnectionCheckNeededSet (bool lostConnectionCheckNeeded)
{
	m_lostConnectionCheckNeeded = lostConnectionCheckNeeded;
}


/*!\brief Gets if a lost connection is needed useful when we have to use a timer to resend the lost connection re-Invite.
 *
*/
bool CstiSipCall::lostConnectionCheckNeededGet () const
{
	return m_lostConnectionCheckNeeded;
}


std::string CstiSipCall::localIpAddressGet () const
{
	if (CstiCallGet())
	{
		return CstiCallGet()->localIpAddressGet();
	}

	return IstiNetwork::InstanceGet()->localIpAddressGet(estiTYPE_IPV4, {});
}

/*!\brief Starts DTLS key exchange for each session.  Once key exchange is complete
 *        for each session, the supplied callback will be called to continue setting 
 *        up media channels.
*/
void CstiSipCall::dtlsBegin (bool isReply)
{
	auto onReady = [this, isReply]()
	{
		auto audioReady = m_audioSession == nullptr || m_audioSession->rtpGet () || m_audioSession->keyExchangeMethodGet() != vpe::KeyExchangeMethod::DTLS;
		auto videoReady = m_videoSession == nullptr || m_videoSession->rtpGet () || m_videoSession->keyExchangeMethodGet() != vpe::KeyExchangeMethod::DTLS;
		auto textReady = m_textSession == nullptr || m_textSession->rtpGet () || m_textSession->keyExchangeMethodGet() != vpe::KeyExchangeMethod::DTLS || !TextSupportedGet();
		if (audioReady && videoReady && textReady)
		{
			auto callLeg = CallLegGet (m_hCallLeg);
			auto hr = callLeg->ConfigureMediaChannels (isReply);
			stiASSERTRESULT (hr);
		}
	};

	onReady ();

	if (m_audioSession != nullptr)
	{
		m_audioSession->onDtlsCompleteCallbackSet (onReady);
		m_audioSession->dtlsBegin ();
	}

	if (m_videoSession != nullptr)
	{
		m_videoSession->onDtlsCompleteCallbackSet (onReady);
		m_videoSession->dtlsBegin ();
	}

	if (m_textSession != nullptr && TextSupportedGet())
	{
		m_textSession->onDtlsCompleteCallbackSet (onReady);
		m_textSession->dtlsBegin ();
	}
}


/*!\brief Determine if we should disable DTLS for this call.
 *
 * NOTE: Any calls that we know anchor media to USM or DTLS fails for should ensure this returns true for to disable DTLS.
 * This can be removed when we no longer have an issue with DTLS not working for anchored media or other calls.
 *
 * \return Returns true if we are either Tunneled or we are not registered.
 */
bool CstiSipCall::disableDtlsGet () const
{
	if (m_stConferenceParams.alwaysEnableDTLS)
	{
		// The Hold Server is unregistered, but should not be anchored and thus can safely use DTLS
		return false;
	}

	bool usesTunneling = !m_stConferenceParams.TunneledIpAddr.empty ();
	bool isUnregistered = !m_poSipManager->IsRegistered ();
	return usesTunneling || isUnregistered;
}

void CstiSipCall::transportsCallTrace (
	const char* tag,
	const CstiMediaTransports* audioTransports,
	const CstiMediaTransports* textTransports,
	const CstiMediaTransports* videoTransports)
{
	auto call = CstiCallGet ();

	std::string videoLocalRtp{ "" }, videoLocalRtcp{ "" }, videoRemoteRtp{ "" }, videoRemoteRtcp{ "" };
	int videoLocalRtpPort{ 0 }, videoLocalRtcpPort{ 0 }, videoRemoteRtpPort{ 0 }, videoRemoteRtcpPort{ 0 };
	videoTransports->LocalAddressesGet (&videoLocalRtp, &videoLocalRtpPort, &videoLocalRtcp, &videoLocalRtcpPort);
	videoTransports->RemoteAddressesGet (&videoRemoteRtp, &videoRemoteRtpPort, &videoRemoteRtcp, &videoRemoteRtcpPort);
	call->collectTrace (formattedString ("%s (Video): local(%s:%d, %s:%d), remote(%s:%d, %s:%d)",
		tag, videoLocalRtp.c_str (), videoLocalRtpPort, videoLocalRtcp.c_str (), videoLocalRtcpPort,
		videoRemoteRtp.c_str (), videoRemoteRtpPort, videoRemoteRtcp.c_str (), videoRemoteRtcpPort));

	std::string audioLocalRtp{ "" }, audioLocalRtcp{ "" }, audioRemoteRtp{ "" }, audioRemoteRtcp{ "" };
	int audioLocalRtpPort{ 0 }, audioLocalRtcpPort{ 0 }, audioRemoteRtpPort{ 0 }, audioRemoteRtcpPort{ 0 };
	audioTransports->LocalAddressesGet (&audioLocalRtp, &audioLocalRtpPort, &audioLocalRtcp, &audioLocalRtcpPort);
	audioTransports->RemoteAddressesGet (&audioRemoteRtp, &audioRemoteRtpPort, &audioRemoteRtcp, &audioRemoteRtcpPort);
	call->collectTrace (formattedString ("%s (Audio): local(%s:%d, %s:%d), remote(%s:%d, %s:%d)",
		tag, audioLocalRtp.c_str (), audioLocalRtpPort, audioLocalRtcp.c_str (), audioLocalRtcpPort,
		audioRemoteRtp.c_str (), audioRemoteRtpPort, audioRemoteRtcp.c_str (), audioRemoteRtcpPort));

	std::string textLocalRtp{ "" }, textLocalRtcp{ "" }, textRemoteRtp{ "" }, textRemoteRtcp{ "" };
	int textLocalRtpPort{ 0 }, textLocalRtcpPort{ 0 }, textRemoteRtpPort{ 0 }, textRemoteRtcpPort{ 0 };
	textTransports->LocalAddressesGet (&textLocalRtp, &textLocalRtpPort, &textLocalRtcp, &textLocalRtcpPort);
	textTransports->RemoteAddressesGet (&textRemoteRtp, &textRemoteRtpPort, &textRemoteRtcp, &textRemoteRtcpPort);
	call->collectTrace (formattedString ("%s (Text): local(%s:%d, %s:%d), remote(%s:%d, %s:%d)",
		tag, textLocalRtp.c_str (), textLocalRtpPort, textLocalRtcp.c_str (), textLocalRtcpPort,
		textRemoteRtp.c_str (), textRemoteRtpPort, textRemoteRtcp.c_str (), textRemoteRtcpPort));
}

// end file CstiSipCall.cpp
