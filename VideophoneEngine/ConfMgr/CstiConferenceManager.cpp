/*!
* \file CstiConferenceManager.cpp
* \brief TODO:  Need up-to-date description
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include <algorithm>
#include "CstiConferenceManager.h"
#include "CstiSipManager.h"
#include "CstiSipCall.h"
#include "stiTaskInfo.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "CstiCallStorage.h"
#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiVideoOutput.h"
#ifdef stiTUNNEL_MANAGER
	#include "CstiTunnelManager.h"
#endif
#include "rvrtplogger.h"
#include "rtcp.h"

#include "stiOS.h"          // OS Abstraction definitions

//SLB#undef stiLOG_ENTRY_NAME
//SLB#define stiLOG_ENTRY_NAME(name) stiTrace ("-CMGR: %s\n", #name);

//
// Constants
//
const unsigned int unSECONDS_PER_RING = 6000; // in milliseconds
const unsigned int unSTATS_COLLECTION_INTERVAL = 1000; // in milliseconds


/*!
 * \brief Constructor
 */
CstiConferenceManager::CstiConferenceManager ()
:
	CstiEventQueue (stiCM_TASK_NAME),
    m_statsCollectTimer (unSTATS_COLLECTION_INTERVAL, this),
    m_remoteRingTimer (unSECONDS_PER_RING, this),
    m_localRingTimer (unSECONDS_PER_RING, this),
    m_purgeStaleCallObjectsTimer (nSTALE_OBJECT_CHECK_DELAY_MS, this)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CstiConferenceManager);

	//
	// Set the call counter in the CstiCall object to zero.
	//
	CstiCall::m_nCallCount = 0;

	// First thing to run in the event loop
	m_signalConnections.push_back (startedSignal.Connect (
		[this]{
			SubtasksStartup();
			startupCompletedSignal.Emit ();
		}));

	// Last thing to run in the event loop
	m_signalConnections.push_back (shutdownSignal.Connect (
		[this]{
			SubtasksShutdown();
		}));

	m_signalConnections.push_back (m_remoteRingTimer.timeoutSignal.Connect (
		[this]{
		    m_nRingCount++;
			remoteRingCountSignal.Emit (m_nRingCount);
			m_remoteRingTimer.restart ();
		}));

	m_signalConnections.push_back (m_localRingTimer.timeoutSignal.Connect (
		[this]{
		    m_un32LocalRingCount++;
			localRingCountSignal.Emit (m_un32LocalRingCount);
			m_localRingTimer.restart ();
		}));

	m_signalConnections.push_back (m_statsCollectTimer.timeoutSignal.Connect (
		[this]{
		    m_pCallStorage->collectStats ();
			m_statsCollectTimer.restart ();
		}));

	m_signalConnections.push_back (m_purgeStaleCallObjectsTimer.timeoutSignal.Connect (
		[this]{
		    m_pCallStorage->removeStaleObjects();
		}));
}


/*!
 * \brief Destructor
 */
CstiConferenceManager::~CstiConferenceManager ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::~CstiConferenceManager);

	Shutdown ();

	if (m_poSipManager)
	{
		delete m_poSipManager;
		m_poSipManager = nullptr;
	}
#ifdef stiTUNNEL_MANAGER
	if (m_pTunnelManager)
	{
		delete m_pTunnelManager;
		m_pTunnelManager = nullptr;
	}
#endif

	if (m_pCallStorage)
	{
		delete m_pCallStorage;
		m_pCallStorage = nullptr;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::Initialize
//
// Description: Initialization method for the class
//
// Abstract:
//  This method will call the method that spawns the CM task and also will
//  create the message queue that is used to receive messages sent to this
//  task by other tasks.
//
// Returns:
//  estiOK for success or estiERROR when an error occurs
//
stiHResult CstiConferenceManager::Initialize (
	IstiBlockListManager2 *pBlockListManager,
	ProductType productType,
	const std::string &version,
	const SstiConferenceParams *pConferenceParams,
	const SstiConferencePorts *pConferencePorts)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::Initialize);

	stiHResult hResult = stiRESULT_SUCCESS;
	SstiSipConferencePorts stSipPorts{};

	m_pCallStorage = new CstiCallStorage;

	// Create protocol managers
#ifdef stiTUNNEL_MANAGER
	m_pTunnelManager = new CstiTunnelManager ();


	m_signalConnections.push_back (m_pTunnelManager->tunnelTerminatedSignal.Connect (
			[this]{ tunnelTerminatedSignal.Emit (); }));

	m_signalConnections.push_back (m_pTunnelManager->tunnelEstablishedSignal.Connect (
			[this](std::string ipAddress){ TunnelIPResolved (&ipAddress); }));

	m_signalConnections.push_back (m_pTunnelManager->publicIpResolvedSignal.Connect (
			[this](std::string ipAddress){
                publicIpResolvedSignal.Emit (ipAddress); }));

	m_signalConnections.push_back (m_pTunnelManager->failedToEstablishTunnelSignal.Connect (
			[this]{ failedToEstablishTunnelSignal.Emit (); }));
#endif

	m_poSipManager = new CstiSipManager (m_pCallStorage, pBlockListManager);

	// Connect to the signals of subtasks
	signalsConnect ();

	// Send the message to the SIP manager
	m_poSipManager->MaxCallsSet (m_maxCalls);

	ConferenceParamsSet (pConferenceParams);

	// Send the message to the SIP manager
	stSipPorts.Set(pConferencePorts);
	hResult = m_poSipManager->Initialize (productType, version, &stSipPorts);
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);
}


/*!
 * \brief Helper method for connecting to subtask signals
 */
void CstiConferenceManager::signalsConnect ()
{
	m_signalConnections.push_back (m_poSipManager->callStateChangedSignal.Connect (
			[this](const SstiStateChange &stateChange){ CallStateChangeNotify(stateChange); }));

	// Inform SipManager to re-register, telling it if there are any active calls.
	m_signalConnections.push_back (m_poSipManager->sipRegisterNeededSignal.Connect (
			[this]{ m_poSipManager->ReRegisterNeeded (m_bActiveCalls); }));
	
	m_signalConnections.push_back(m_poSipManager->preIncomingCallSignal.Connect(
			[this](const CstiCallSP &call){ preIncomingCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->sipMessageConfAddressChangedSignal.Connect (
			[this](const SstiConferenceAddresses &addresses) { sipMessageConfAddressChangedSignal.Emit (addresses); }));

	m_signalConnections.push_back (m_poSipManager->sipRegistrationRegisteringSignal.Connect (
			[this]{ sipRegistrationRegisteringSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->sipRegistrationConfirmedSignal.Connect (
			[this](std::string proxyIpAddress){ sipRegistrationConfirmedSignal.Emit (proxyIpAddress); }));

	m_signalConnections.push_back (m_poSipManager->publicIpResolvedSignal.Connect (
			[this](std::string ipAddress){
                publicIpResolvedSignal.Emit (ipAddress); }));

	m_signalConnections.push_back (m_poSipManager->preCompleteCallSignal.Connect (
			[this](CstiCallSP call){ preCompleteCallSignal.Emit (call); }));  // unused?

	m_signalConnections.push_back (m_poSipManager->currentTimeSetSignal.Connect (
			[this](time_t currentTime){ currentTimeSetSignal.Emit (currentTime); }));

	m_signalConnections.push_back (m_poSipManager->directoryResolveSignal.Connect (
			[this](CstiCallSP call){ directoryResolveSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->callTransferringSignal.Connect (
			[this](CstiCallSP call){ callTransferringSignal.Emit (call); }));
	
	m_signalConnections.push_back (m_poSipManager->geolocationUpdateSignal.Connect (
			[this](CstiCallSP call){ geolocationUpdateSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->hearingCallSpawnSignal.Connect (
			[this](const SstiSharedContact &contact){ hearingCallSpawnSignal.Emit (contact); }));

	m_signalConnections.push_back (m_poSipManager->contactReceivedSignal.Connect (
			[this](const SstiSharedContact &contact){ contactReceivedSignal.Emit (contact); }));

	m_signalConnections.push_back (m_poSipManager->hearingStatusChangedSignal.Connect (
			[this](CstiCallSP call) { hearingStatusChangedSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->hearingStatusSentSignal.Connect (
			[this]{ hearingStatusSentSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->newCallReadySentSignal.Connect (
			[this]{ newCallReadySentSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->flashLightRingSignal.Connect (
			[this]{ flashLightRingSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->conferencingWithMcuSignal.Connect (
			[this](CstiCallSP call){ conferencingWithMcuSignal.Emit(call); }));

	m_signalConnections.push_back (m_poSipManager->heartbeatRequestSignal.Connect (
			[this]{ heartbeatRequestSignal.Emit (); }));

#ifdef stiTUNNEL_MANAGER
	m_signalConnections.push_back (m_poSipManager->rvTcpConnectionStateChangedSignal.Connect (
			[this](RvSipTransportConnectionState state){ rvTcpConnectionStateChangedSignal.Emit (state); }));
#endif

	m_signalConnections.push_back (m_poSipManager->conferencePortsSetSignal.Connect (
		[this](bool success){
			PostEvent (
				std::bind(&CstiConferenceManager::EventSecondListeningPortChanged, this, success));
		}));

	m_signalConnections.push_back (m_poSipManager->removeTextSignal.Connect (
			[this](CstiCallSP call){ removeTextSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->preHoldCallSignal.Connect (
			[this](CstiCallSP call){ preHoldCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->preResumeCallSignal.Connect (
			[this](CstiCallSP call){ preResumeCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->missedCallSignal.Connect (
			[this](CstiCallSP call){ missedCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->callBridgeStateChangedSignal.Connect (
			[this](CstiCallSP call){ callBridgeStateChangedSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->preAnsweringCallSignal.Connect (
			[this](CstiCallSP call){ preAnsweringCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->callTransferFailedSignal.Connect (
			[this](CstiCallSP call){ callTransferFailedSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->callInformationChangedSignal.Connect (
			[this](CstiCallSP call){ callInformationChangedSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->preHangupCallSignal.Connect (
			[this](CstiCallSP call){ preHangupCallSignal.Emit (call); }));

	m_signalConnections.push_back (m_poSipManager->videoPlaybackPrivacyModeChangedSignal.Connect (
			[this](CstiCallSP call, bool muted){ videoPlaybackPrivacyModeChangedSignal.Emit (call, muted); }));

	m_signalConnections.push_back (m_poSipManager->remoteTextReceivedSignal.Connect (
			[this](SstiTextMsg *textMsg){ remoteTextReceivedSignal.Emit (textMsg); }));

	m_signalConnections.push_back (m_poSipManager->pleaseWaitSignal.Connect (
			[this]{ pleaseWaitSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->vrsServerDomainResolveSignal.Connect (
			[this]{ vrsServerDomainResolveSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->signMailSkipToRecordSignal.Connect (
			[this]{ signMailSkipToRecordSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->remoteGeolocationChangedSignal.Connect (
			[this](const std::string& geolocation){ remoteGeolocationChangedSignal.Emit (geolocation); }));

	m_signalConnections.push_back (m_poSipManager->createVrsCallSignal.Connect (
			[this](CstiCallSP call) { createVrsCallSignal.Emit (call); }));
	
	m_signalConnections.push_back (m_poSipManager->logCallSignal.Connect(
			[this](CstiCallSP call, CstiSipCallLegSP callLeg) {logCallSignal.Emit(call, callLeg);}));

	m_signalConnections.push_back (m_poSipManager->dhviCapableSignal.Connect(
			[this](bool capable) 
			{
				dhviCapableSignal.Emit(capable);
			}));

	m_signalConnections.push_back (m_poSipManager->dhviConnectingSignal.Connect(
			[this](const std::string &conferenceURI, const std::string &initiator, const std::string &profileId) 
			{
				dhviConnectingSignal.Emit(conferenceURI, initiator, profileId);
			}));

	m_signalConnections.push_back (m_poSipManager->dhviConnectedSignal.Connect(
			[this]
			{
				dhviConnectedSignal.Emit();
			}));

	m_signalConnections.push_back (m_poSipManager->dhviDisconnectedSignal.Connect(
			[this](bool hearingDisconnected) 
			{
				dhviDisconnectedSignal.Emit(hearingDisconnected);
			}));

	m_signalConnections.push_back (m_poSipManager->dhviConnectionFailedSignal.Connect(
			[this]
			{
				dhviConnectionFailedSignal.Emit();
			}));
	m_signalConnections.push_back (m_poSipManager->dhviHearingNumberReceived.Connect(
			[this](CstiCallSP call, const std::string hearingNumber)
			{
				dhviHearingNumberReceived.Emit(call, hearingNumber);
			}));
	m_signalConnections.push_back (m_poSipManager->dhviStartCallReceived.Connect(
			[this](CstiCallSP call, const std::string &toNumber, const std::string &fromNumber, const std::string &displayName, const std::string &mcuAddress)
			{
				dhviStartCallReceived.Emit(call, toNumber, fromNumber, displayName, mcuAddress);
			}));
	m_signalConnections.push_back (m_poSipManager->encryptionStateChangedSignal.Connect (
		[this](CstiCallSP call) {
			conferenceEncryptionStateChangedSignal.Emit (call);
		}
	));
	m_signalConnections.push_back(m_poSipManager->sipStackStartupSucceededSignal.Connect(
		[this] () { sipStackStartupSucceededSignal.Emit (); }));

	m_signalConnections.push_back (m_poSipManager->sipStackStartupFailedSignal.Connect (
		[this] () { sipStackStartupFailedSignal.Emit (); }));
}


/*!
 * \brief Starts the event loop
 * \return stiHResult
 */
stiHResult CstiConferenceManager::Startup ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::Startup);
	StartEventLoop ();
	return stiRESULT_SUCCESS;
}


/*!
 * \brief Shuts down the conference manager and all subtasks
 * \return stiHResult
 */
stiHResult CstiConferenceManager::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::Shutdown);
	StopEventLoop ();
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::SubtasksStartup
//
// Description: Start the task.
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::SubtasksStartup ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::SubtasksStartup);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Initialize RTP and RTCP sessions
	if (!m_bRtpInitialized && (0 > RvRtpInit () ||
		(0 > RvRtcpInit ())))
	{
		// We errored in creating the RTP or RTCP session
		stiASSERT (estiFALSE);
	} // end if
	m_bRtpInitialized = true;
	
	RvRtpSetPortRange(RV_PORTRANGE_DEFAULT_START, RV_PORTRANGE_DEFAULT_FINISH);

#ifdef stiTUNNEL_MANAGER
	hResult = m_pTunnelManager->Startup();
//	stiTESTRESULT();
#endif
	// Send the message to the SIP manager
	hResult = m_poSipManager->Startup ();
	stiTESTRESULT ();

STI_BAIL:

	return (hResult);

} // end CstiConferenceManager::SubtasksStartup


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::SubtasksShutdown ()
//
// Description:
//
// Abstract:
//
// Returns:
//
stiHResult CstiConferenceManager::SubtasksShutdown ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::SubtasksShutdown);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Shut down the shared rtp layer
	RvRtpEnd ();
	RvRtcpEnd ();

	m_bRtpInitialized = false;

	// Send the message to the SIP manager
	hResult = m_poSipManager->Shutdown ();
	stiTESTRESULT ();

#ifdef stiTUNNEL_MANAGER
	m_pTunnelManager->Shutdown();
#endif
STI_BAIL:

	return (hResult);

} // end CstiConferenceManager::SubtasksShutdown



//:-----------------------------------------------------------------------------
//:
//:              Event Handler Methods
//:
//:-----------------------------------------------------------------------------

/*!
 * \brief Event handler for when certain state changes occur, which notifies the
 *        application layer of the change
 * \param stateChange
 */
void CstiConferenceManager::EventCallStateChangeNotify (SstiStateChange stateChange)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::EventCallStateChangeNotify);

	// Notify the application of the state change
	CallStateChangeNotifyApp (
	    stateChange.call,
		stateChange.ePrevState,
		stateChange.un32PrevSubStates,
		stateChange.eNewState,
		stateChange.un32NewSubStates);

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	DebugTools::instanceGet()->DebugOutput("vpEngine::CstiConferenceManager::CallStateChangeNotifyHandle() - Done"); //...
#endif
}


/*!
 * \brief Tallies successes/failures for second listening port and notifies CCI
 * \param msg
 */
void CstiConferenceManager::EventSecondListeningPortChanged (
	bool success)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::EventSecondListeningPortChanged);

	if (success)
	{
		m_nSuccessfulPortsSets++;
	}
	else
	{
		m_nFailedPortsSets++;
	}

	if (m_nSuccessfulPortsSets + m_nFailedPortsSets == m_nExpectedPortsSets)
	{
		if (m_nFailedPortsSets == 0)
		{
			conferencePortsSetSignal.Emit (m_stConferencePorts);
		}

		m_nSuccessfulPortsSets = 0;
		m_nFailedPortsSets = 0;
		m_nExpectedPortsSets = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectListGet
//
// Description: Get the call list.
//
// Abstract:
//
// Returns:
//  \retval The call list.
//
void CstiConferenceManager::CallObjectListGet (
    std::list<CstiCallSP> *callList)
{
	*callList = m_pCallStorage->listGet();
}


#ifdef stiHOLDSERVER
////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::NthCallObjectGet
//
// Description: Get the Nth call object from the call object array.
//
// Abstract:
//  This searches the array of call objects and gets the one that is identified
//  as the Nth call.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::NthCallObjectGet (uint32_t un32NthCallNumber)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::NthCallObjectGet);

	return m_pCallStorage->nthCallGet (un32NthCallNumber);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectGetByLocalPhoneNbr
//
// Description: Get the call object identified by the local phone number.
//
// Abstract:
//  This searches the array of call objects and gets the one that is identified
//  by the passed in local phone number.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::CallObjectGetByLocalPhoneNbr (const char *pszPhone)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectGetByLocalPhoneNbr);

	return m_pCallStorage->CallObjectGetByLocalPhoneNbr (pszPhone);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectGetByHSCallId
//
// Description: Get the call object identified by the HoldServer Call Id.
//
// Abstract:
//  This searches the array of call objects and gets the one that is identified
//  by the passed in HoldServer call identifier.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::CallObjectGetByHSCallId (__int64 n64CallID)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectGetByHSCallId);

	return m_pCallStorage->CallObjectGetByHSCallId (n64CallID);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::HangupAllCalls
//
// Description: Hangup all open calls.
//
// Abstract:
//  This function hangs up all open calls on the Hold Server.
//
//
void CstiConferenceManager::HangupAllCalls ()
{
	m_pCallStorage->hangupAllCalls ();
}
#endif // stiHOLDSERVER


/*!
 * \brief Removes the shared call object from call storage that points
 *        to the same call object as the one passed in
 */
stiHResult CstiConferenceManager::CallObjectRemove (
    CstiCall *call)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectRemove);

	stiHResult hResult = m_pCallStorage->remove (call);
	m_poSipManager->CallObjectRemoved ();
	
	return hResult;
}

void CstiConferenceManager::allCallObjectsRemove ()
{
	m_pCallStorage->removeAll();
}

CstiCallSP CstiConferenceManager::callObjectGetByCallIndex (int callIndex)
{
	return m_pCallStorage->getByCallIndex (callIndex);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectHeadGet
//
// Description: Get the head call object in the list
//
// Returns:
//  A pointer to call object or NULL if it wasn't found or the index was invalid
//
CstiCallSP CstiConferenceManager::CallObjectHeadGet ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectHeadGet);

	return (m_pCallStorage->headGet());

} // end CstiConferenceManager::CallObjectHeadGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectsCountGet
//
// Description: Return the number of call objects stored
//
// Abstract:
//
// Returns:
//  The number of call objects stored
//
unsigned int CstiConferenceManager::CallObjectsCountGet ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectsCountGet);

	return (m_pCallStorage->countGet ());
} // end CstiConferenceManager::CallObjectsCountGet

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectsCountGet
//
// Description: Determine the number of calls there are in the given state(s).
//
// Abstract:
//
// Returns:
//  \retval The number of call objects in the given state(s).
//
unsigned int CstiConferenceManager::CallObjectsCountGet (
	uint32_t un32StateMask)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectsCountGet);

	return m_pCallStorage->countGet (un32StateMask);
} // end CstiConferenceManager::CallObjectsCountGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectGet
//
// Description: Get the first call object identified by the passed in call
// state(s).
//
// Abstract:
//  This method searches the call object array and returns a pointer to the
//  first call object that is in one of the passed in call-states.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::CallObjectGet (
	uint32_t un32StateMask)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectGet);

	return m_pCallStorage->get (un32StateMask);
} // end CstiConferenceManager::CallObjectGet


/*!
 * \brief Gets the shared_ptr from call storage that points to the same call
 *        object as the given raw pointer.  This is to map raw call pointers
 *        back to shared pointers, as is needed when the UI passes calls back
 *        down for some methods
 */
CstiCallSP CstiConferenceManager::CallObjectLookup (
    IstiCall *call) const
{
	return m_pCallStorage->lookup (call);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectGetByAppData
//
// Description: Get the call object identified by the passed in AppData.
//
// Abstract:
//  This method searches the call object array and returns a pointer to the
//  call object that is identified by the passed in Application Data.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::CallObjectGetByAppData (
	size_t AppData)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectGetByAppData);

	return m_pCallStorage->getByAppData (AppData);
} // end CstiConferenceManager::CallObjectGetByAppData


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallObjectIncomingGet
//
// Description: Get the call object that is "Offering" in the call object array.
//
// Abstract:
//  This searches the array of call objects and gets the one that is incoming
//  and waiting to be answered locally.
//
// Returns:
//	A pointer to call object or NULL if it wasn't found
//
CstiCallSP CstiConferenceManager::CallObjectIncomingGet ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallObjectIncomingGet);

	return m_pCallStorage->incomingGet ();
} // end CstiConferenceManager::CallObjectIncomingGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallStateChangeNotifyApp
//
// Description: Send notifications to the application of state changes
//
// Abstract:
//  This method is used to send State-change notifications to the
//  application.  When a state change occurs, the Conference Manager will call
//  this method which will inform the application.  Not all state changes are
//  being reported to the application here.  All state and substate changes
//  will be reported to the application via the CCI when synchronization of the
//  CstiConfigData object occurs.  The notifications herein are specific ones
//  that the application needs to know about for UI updates.
//
// Returns:
//  none
//
void CstiConferenceManager::CallStateChangeNotifyApp (
    CstiCallSP call,	// A pointer to the call object
	EsmiCallState ePrevState,		// Previous state
	uint32_t un32PrevSubState,		// Previous substate
	EsmiCallState eNewState,			// New state
	uint32_t un32NewSubstate)		// New substate
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallStateChangeNotifyApp);

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	char *vsPrevState = "";
	switch (ePrevState)
	{
		case esmiCS_UNKNOWN:
			vsPrevState = "UNKNOWN";
			break;
		case esmiCS_IDLE:
			vsPrevState = "IDLE";
			break;
		case esmiCS_CONNECTING:
			vsPrevState = "CONNECTING";
			break;
		case esmiCS_CONNECTED:
			vsPrevState = "CONNECTED";
			break;
		case esmiCS_HOLD_LCL:
			vsPrevState = "HOLD_LCL";
			break;
		case esmiCS_HOLD_RMT:
			vsPrevState = "HOLD_RMT";
			break;
		case esmiCS_HOLD_BOTH:
			vsPrevState = "HOLD_BOTH";
			break;
		case esmiCS_DISCONNECTING:
			vsPrevState = "DISCONNECTING";
			break;
		case esmiCS_DISCONNECTED:
			vsPrevState = "DISCONNECTED";
			break;
		case esmiCS_CRITICAL_ERROR:
			vsPrevState = "CRITICAL_ERROR";
			break;
		case esmiCS_INIT_TRANSFER:
			vsPrevState = "INIT_TRANSFER";
			break;
		default:
			sprintf(vsPrevState, "***UNKNOWN*** State: %d", ePrevState);
			break;
	} // EsmiCallState
	char* vsNewState = "";
	switch (eNewState)
	{
		case esmiCS_UNKNOWN:
			vsNewState = "esmiCS_UNKNOWN";
			break;
		case esmiCS_IDLE:
			vsNewState = "esmiCS_IDLE";
			break;
		case esmiCS_CONNECTING:
			vsNewState = "esmiCS_CONNECTING";
			break;
		case esmiCS_CONNECTED:
			vsNewState = "esmiCS_CONNECTED";
			break;
		case esmiCS_HOLD_LCL:
			vsNewState = "esmiCS_HOLD_LCL";
			break;
		case esmiCS_HOLD_RMT:
			vsNewState = "esmiCS_HOLD_RMT";
			break;
		case esmiCS_HOLD_BOTH:
			vsNewState = "esmiCS_HOLD_BOTH";
			break;
		case esmiCS_DISCONNECTING:
			vsNewState = "esmiCS_DISCONNECTING";
			break;
		case esmiCS_DISCONNECTED:
			vsNewState = "esmiCS_DISCONNECTED";
			break;
		case esmiCS_CRITICAL_ERROR:
			vsNewState = "esmiCS_CRITICAL_ERROR";
			break;
		default:
			sprintf(vsNewState, "***UNKNOWN*** eNewState: 0x%x", eNewState);
			break;
	}; // EsmiCallState;
	static char szMessage[256];
	sprintf(szMessage, "vpEngine::CstiConferenceManager::CallStateChangeNotifyApp() - State: %s   eNewState: %s", vsPrevState, vsNewState);
	DebugTools::instanceGet()->DebugOutput(szMessage); //...
#endif
	// Notify the application that our state has changed
	if (call->NotifyAppOfIncomingCallGet ()
	 && ePrevState != eNewState)
	{
		if (call->NotifyAppOfCallGet ())
		{
			callStateChangedSignal.Emit (call);
		}
	} // end if

	// What is the new state?
	switch (eNewState)
	{
		case esmiCS_IDLE:
			break;

		case esmiCS_CONNECTING:

			if (ePrevState == esmiCS_IDLE
			 && call->DirectionGet() == estiOUTGOING)
			{
				outgoingCallSignal.Emit (call);
			}

			// What substate have we transitioned to?
			switch (un32NewSubstate)
			{
				case estiSUBSTATE_CALLING:
					// Notify the app that we are now dialing.
				    if (call->NotifyAppOfCallGet ())
					{
						dialingSignal.Emit (call);
					}
					break;

				case estiSUBSTATE_ANSWERING:
					// Notify the app that we are now answering a call
				    if (call->NotifyAppOfCallGet ())
					{
						answeringCallSignal.Emit (call);
					}
					break;

				case estiSUBSTATE_RESOLVE_NAME:
					// Notify the app that we are waiting for the Directory
					// Service to resolve the name given.
				    if (call->NotifyAppOfCallGet ())
					{
						resolvingNameSignal.Emit (call);
					}
					break;

				case estiSUBSTATE_WAITING_FOR_USER_RESP:
					// If we have already notified the user of this incoming
					// call, don't do it again.
				    if (call)
					{
						if (call->NotifyAppOfCallGet()
						 /*&& !call->AppNotifiedOfIncomingGet ()*/)
						{
							// Notify the app of the incoming call.
							incomingCallSignal.Emit (call);

							// Start the LocalRingCount timer. This will be used to disconnect the call.
							m_un32LocalRingCount = 1;
							m_pRingingIncomingCall = call;
							localRingCountSignal.Emit (m_un32LocalRingCount);
							m_localRingTimer.restart ();
							
							/*call->AppNotifiedOfIncomingSet (estiTRUE);*/
						} // end if
					} // end if
					break;

				case estiSUBSTATE_WAITING_FOR_REMOTE_RESP:
				{
				    if (call->NotifyAppOfCallGet())
					{
						// Notify the app that the remote system is ringing
						ringingSignal.Emit (call);

						// Start the RemoteRing watchdog timer.

						// Notify the app that the ring count is now 1.
						m_nRingCount = 1;
						remoteRingCountSignal.Emit (m_nRingCount);

						m_remoteRingTimer.restart ();
					}

					break;
				}

				default:
					break;
			} // end switch

			// Do we have any active calls?
			if (!m_bActiveCalls)
			{
				// No.  Store that we do now and inform the application layer.
				m_bActiveCalls = true;

				// Notify the application of an active call.
				activeCallsSignal.Emit (true);
			}
			break;

		case esmiCS_CONNECTED:
			// What substate have we transitioned to?
			switch (un32NewSubstate)
			{
				case estiSUBSTATE_ESTABLISHING:
				    if (call->NotifyAppOfCallGet() && !call->m_isDhviMcuCall)
					{
						// Notify the app that we are connected to the remote
						// endpoint and are now establishing the conference.
						establishingConferenceSignal.Emit (call);
					}
					
					break;

				case estiSUBSTATE_CONFERENCING:
					// Cancel the ring timer.
				    m_remoteRingTimer.stop ();

					if (call == m_pRingingIncomingCall)
					{
						// Cancel the local ring timer.
						m_localRingTimer.stop ();
						m_pRingingIncomingCall = nullptr;
					}

					// If we have already notified the user of this incoming
					// call, don't do it again.
					if (call)
					{
						//
						// Indicate that the call has reached the conferencing state.
						//
						call->ConferencedSet (true);
						
						if (call->NotifyAppOfCallGet()
						 && !call->AppNotifiedOfConferencingGet ())
						{
							// Notify the app that the conference has been established
							// and that we are now conferencing.  Channels have now
							// been requested.
							conferencingSignal.Emit (call);
							
							// NOTE: it was decided to make this a signal to handle the case
							// where a call may go from encrypted to decrypted,
							// even though it currently is only emitted when we reach
							// this conference state
							// NOTE: it would be nice to also emit the encrypted state, but
							// the current AppNotify appears to only take one argument
							conferenceEncryptionStateChangedSignal.Emit (call);

							// Increment the number of call objects that are connected/conferencing
							m_nStatsCounter++;
							call->AppNotifiedOfConferencingSet (estiTRUE);
						} // end if

						// Cause the call stats to be cleared out and prepared to start accumulating.
						call->StatsClear ();

						// Do we need to start the timer for stats collection?
						if (1 == m_nStatsCounter)
						{
							// There are no other calls currently conferencing so the timer needs
							// to be started now.
							m_statsCollectTimer.restart ();
						}
					} // end if

					// Is this the transition from a held state?
					if (0 < (estiSUBSTATE_NEGOTIATING_LCL_RESUME & un32PrevSubState))
					{
						// The call hold that was initiated locally has now been resumed.
						// Notify the app that the call is now active.
						if (call->NotifyAppOfCallGet())
						{
							resumedCallLocalSignal.Emit (call);
						}
					} // end if
					else if (0 < (estiSUBSTATE_NEGOTIATING_RMT_RESUME & un32PrevSubState))
					{
						// The call hold that was initiated remotely has now been resumed.
						// Notify the app that the call is now active.
						if (call->NotifyAppOfCallGet())
						{
							resumedCallRemoteSignal.Emit (call);
						}
					} // end else if
					
					break;
			}  // end switch
			break;

		case esmiCS_DISCONNECTING:
		{
			// If this was for an incoming call and we were set to automatically
			// reject it, we didn't notify the application of the incoming call
			// and therefore, don't want to notify them now either.
			if (esmiCS_CONNECTING == ePrevState &&
				estiSUBSTATE_ANSWERING == un32PrevSubState &&
				m_bAutoReject)
			{
				// Do nothing
			}  // end if
			else if (call->NotifyAppOfIncomingCallGet ())
			{
				// Notify the app that we are disconnecting the call.
				if (call->NotifyAppOfCallGet())
				{
					disconnectingSignal.Emit (call);
				}
			}

			// Removed for iOS due to a race condition with receiving a frame at the end of a call causing a crash.
#if APPLICATION != APP_NTOUCH_IOS
			// If there are no other active calls, restore the Video Record and Video Playback Mute setting
			if ( 0 >= m_pCallStorage->countGet (esmiCS_CONNECTED | esmiCS_HOLD_LCL |
						esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
			{
				IstiVideoOutput::InstanceGet ()->RemoteViewPrivacySet (estiFALSE);
			}
#endif
			
			if (call->AppNotifiedOfConferencingGet ())
			{
				// Cancel stats collection if needed
				StatsCancel (call);
			}
			
			break;
		} // end case esmiCS_DISCONNECTING

		case esmiCS_DISCONNECTED:
		    if (call)
			{
				bool bStartCallStorageCleanupTimer = true;

				if (call->DirectionGet() == estiOUTGOING)
				{
					//
					// Lock the call storage object and make sure there are no other outgoing calls
					// before canceling the ring timer.
					//
					m_pCallStorage->lock ();
					auto outgoingCall = m_pCallStorage->outgoingGet();
					m_pCallStorage->unlock ();

					if (!outgoingCall)
					{
						// Cancel the ring timer.
						m_remoteRingTimer.stop ();
					}
				}
				else if (call == m_pRingingIncomingCall)
				{
					// Cancel the local ring timer.
					m_localRingTimer.stop ();
					m_pRingingIncomingCall = nullptr;
				}

				// send the port changes to the CCI and its helpers
				// If we are in a call, let cmainwin know so that it can perform the update once it completes the call.
				if (m_bSetPortsWhenIdle
				 && 0 < CallObjectsCountGet (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL
											 | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
				{
					ProtocolMgrPortsSet ();
					m_bSetPortsWhenIdle = false;
				}

				if (un32NewSubstate == estiSUBSTATE_LEAVE_MESSAGE)
				{
					// Notify the app that the call has been disconnected, but a message
					// can be left.	The app will be	responsibe to free the call object.
					if (call->NotifyAppOfCallGet() && un32PrevSubState != estiSUBSTATE_MESSAGE_COMPLETE)
					{
						leaveMessageSignal.Emit (call);
						stiDEBUG_TOOL (g_stiCMCallObjectDebug,
						    stiTrace ("<ConfMgr::CallStateChangeNotifyApp> Notify app of \"Leave Message\" call state. [%p]\n", call.get());
						);
					}

					bStartCallStorageCleanupTimer = false;
				}
				else if (un32NewSubstate != estiSUBSTATE_MESSAGE_COMPLETE &&
						 call->NotifyAppOfIncomingCallGet ())
				{
					// Notify the app that the call has been disconnected.
					// If the opaque data hasn't all been freed, it will be
					// the responsibility of the call object to initiate this
					// notification when the last one is freed.
					if (call->NotifyAppOfCallGet())
					{
						disconnectedSignal.Emit (call);
						stiDEBUG_TOOL (g_stiCMCallObjectDebug,
						    stiTrace ("<ConfMgr::CallStateChangeNotifyApp> Notify app of \"Disconnected\" call state. [%p]\n", call.get());
						);
					}
				}

				if (bStartCallStorageCleanupTimer)
				{
					m_purgeStaleCallObjectsTimer.restart ();
				}
			} // end if

			// Do we have any active calls?
			if (0 == CallObjectsCountGet (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH | esmiCS_DISCONNECTING | esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING))
			{
				// No.  Store that we don't now and inform the application layer.
				m_bActiveCalls = false;

				// Notify the application layer of no active calls.
				activeCallsSignal.Emit (false);

				Lock ();

				// Do we have an event that was waiting for calls to complete before we handled it?
				if (m_bClientReregisterNeeded)
				{
					if (!stiIS_ERROR (m_poSipManager->ClientReregister ()))
					{
						m_bClientReregisterNeeded = false;
					}
				}

				Unlock ();
			}
			break;

		case esmiCS_HOLD_LCL:
		    if (call->NotifyAppOfHoldChangeGet ())
			{
				// Is the State to the HELD substate?
				if (estiSUBSTATE_HELD == un32NewSubstate)
				{
					// Are we coming from a held_both state
					if (0 != (estiSUBSTATE_NEGOTIATING_RMT_RESUME & un32PrevSubState))
					{
						// The call is resumed that was initiated remotely.
						// Notify the app that the call has now been resumed remotely.
						if (call->NotifyAppOfCallGet())
						{
							resumedCallRemoteSignal.Emit (call);
						}
					} // end if
					else if (0 != (estiSUBSTATE_NEGOTIATING_LCL_HOLD & un32PrevSubState))
					{
						// The call is held and was initiated locally.
						// Notify the app that the call is now held.
						if (call->NotifyAppOfCallGet())
						{
							heldCallLocalSignal.Emit (call);
						}
					} // end if
				} // end if
			} // end if

			break;

		case esmiCS_HOLD_RMT:
		    if (call->NotifyAppOfHoldChangeGet ())
			{
				// Is the State to the HELD substate?
				if (estiSUBSTATE_HELD == un32NewSubstate)
				{
					// Are we coming from a held_both state
					if (0 != (estiSUBSTATE_NEGOTIATING_LCL_RESUME & un32PrevSubState))
					{
						// The call is resumed that was initiated locally.
						// Notify the app that the call has now been resumed locally.
						if (call->NotifyAppOfCallGet ())
						{
							resumedCallLocalSignal.Emit (call);
						}
					} // end if
					else if (0 != (estiSUBSTATE_NEGOTIATING_RMT_HOLD & un32PrevSubState))
					{
						// The call is held and was initiated remotely.
						// Notify the app that the call is now held.
						if (call->NotifyAppOfCallGet ())
						{
							heldCallRemoteSignal.Emit (call);
						}
					} // end if
				} // end if
			} // end if

			break;

		case esmiCS_HOLD_BOTH:
		    if (call->NotifyAppOfHoldChangeGet ())
			{
				if (estiSUBSTATE_HELD == un32NewSubstate)
				{
					if (0 != (estiSUBSTATE_NEGOTIATING_LCL_HOLD & un32PrevSubState))
					{
						// The call is held and was initiated locally.
						// Notify the app that the call is now held.
						if (call->NotifyAppOfCallGet())
						{
							heldCallLocalSignal.Emit (call);
						}
					} // end if
					else if (0 != (estiSUBSTATE_NEGOTIATING_RMT_HOLD & un32PrevSubState))
					{
						// The call is held and was initiated remotely.
						// Notify the app that the call is now held.
						if (call->NotifyAppOfCallGet ())
						{
							heldCallRemoteSignal.Emit (call);
						}
					} // end else if
				} // end if
			} // end if

			break;

		case esmiCS_CRITICAL_ERROR:
			// Notify the app that we've had a critical error.
		    if (call->NotifyAppOfCallGet ())
			{
				criticalError.Emit (call);
			}
			break;

		default:
			// Do nothing.
			break;
	}  // end switch
} // end CstiConferenceManager::CallStateChangeNotifyApp

void CstiConferenceManager::StatsCancel (
    CstiCallSP /*call*/)
{
	if (0 < m_nStatsCounter)
	{
		// Decrement the stats counter
		m_nStatsCounter--;

		// Are there any conferencing calls?
		if (0 == m_nStatsCounter)
		{
			// Stop the watch dog timer
			m_statsCollectTimer.stop ();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallStateChangeNotify
//
// Description: Inform the Conference Manager of a state change.
//
// Abstract:
//  This method posts a message to the CM task informing it that we have reached
//  a new call state.
//
// Returns:
//  estiOK when successful, otherwise estiERROR
//
stiHResult CstiConferenceManager::CallStateChangeNotify (
	const SstiStateChange &stateChange)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CallStateChangeNotify);

	PostEvent (
		std::bind(&CstiConferenceManager::EventCallStateChangeNotify, this, stateChange));

	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::ErrorReport
//
// Description: Report an error to the Conference Manager task
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::ErrorReport (
	const SstiErrorLog* pstErrorLog)	// Pointer to the structure containing
										// the error data.
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::ErrorReport);
	errorReportSignal.Emit (pstErrorLog);
	return stiRESULT_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::AutoRejectSet
//
// Description: Sets the auto reject switch in config data (on/off)
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::AutoRejectSet (
	bool bAutoReject)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::AutoRejectSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_bAutoReject = bAutoReject;

	// Send the message to the SIP manager
	m_poSipManager->AutoRejectSet (bAutoReject);

	return (hResult);
} // end CstiConferenceManager::AutoRejectSet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::AutoRejectGet
//
// Description: Gets the auto reject state
//
// Abstract:
//
// Returns:
//	true if auto reject is enabled, false otherwise
//
bool CstiConferenceManager::AutoRejectGet () const
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::AutoRejectGet);

	return (m_bAutoReject);
} // end CstiConferenceManager::AutoRejectGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::MaxCallsSet
//
// Description: Sets the maximum calls allowed concurrently.
//
// Abstract:
//
// Returns:
//	stiReSULT_SUCCESS upon success
//
stiHResult CstiConferenceManager::MaxCallsSet (
	unsigned int maxCalls)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::MaxCallsSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_maxCalls = maxCalls;

	// Send the message to the SIP manager
	m_poSipManager->MaxCallsSet (maxCalls);

	return (hResult);
} // end CstiConferenceManager::MaxCallsSet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::MaxCallsGet
//
// Description: Gets the number of concurrent calls allowed.
//
// Abstract:
//
// Returns:
//	The number of  concurrent calls allowed.
//
unsigned int CstiConferenceManager::MaxCallsGet () const
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::MaxCallsGet);

	return (m_maxCalls);
} // end CstiConferenceManager::MaxCallsGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallDial
//
// Description: Post a message to the protocol task to dial a call
//
// Abstract:
//	This method simply calls a function that posts a message to the appropriate
//	protocol object task informing it to dail a call.
//
// Returns:
//	A pointer to a CstiCall object or NULL upon failure
//
CstiCallSP CstiConferenceManager::CallDial (
	EstiDialMethod eDialMethod,
	const CstiRoutingAddress &DialString,
	const std::string &originalDialString,
	const OptString &fromNameOverride,
	const OptString &callListName,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	return CallDial (eDialMethod, DialString, originalDialString, fromNameOverride, callListName, nullptr, 0, additionalHeaders);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::CallDial
//
// Description: Post a message to the protocol task to dial a call
//
// Abstract:
//	This method simply calls a function that posts a message to the appropriate
//	protocol object task informing it to dail a call.
//
// Returns:
//	A pointer to a CstiCall object or NULL upon failure
//
CstiCallSP CstiConferenceManager::CallDial (
	EstiDialMethod eDialMethod,
	const CstiRoutingAddress &DialString,
	const std::string &originalDialString,
	const OptString &fromNameOverride,
	const OptString &callListName,
	const OptString &relayLanguage,
	int nRelayLanguageId,
	const std::vector<SstiSipHeader>& additionalHeaders)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	DebugTools::instanceGet()->DebugOutput("vpEngine::CstiConferenceManager::CallDial() - Start"); //...
#endif

	// Make sure there are no incoming calls before creating our outgoing call
	m_pCallStorage->lock();
	auto incomingCall = m_pCallStorage->incomingGet();
	m_pCallStorage->unlock();

	if (incomingCall)
	{
		return nullptr;
	}

	return m_poSipManager->CallDial (
	            eDialMethod, DialString, originalDialString, fromNameOverride,
	            callListName, relayLanguage, nRelayLanguageId, nullptr, nullptr, additionalHeaders);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::ConferenceJoin
//
// Description: Post a message to the protocol task to join a conference room
//
// Abstract:
//	This method simply calls a function that posts a message to the appropriate
//	protocol object task informing it to dial into a conference room.
//
// Returns:
//	An stiHResult.
//
stiHResult CstiConferenceManager::ConferenceJoin (
    CstiCallSP call1,
    CstiCallSP call2,
	const std::string &DialString)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	CstiRoutingAddress oAddress (DialString);

	if (oAddress.ProtocolMatch (estiSIP))
	{
		// Send the message to the SIP manager
		hResult = m_poSipManager->ConferenceJoin (call1, call2, DialString);
	}
	else
	{
		stiASSERT (estiFALSE);
		hResult = stiRESULT_ERROR;
	}

	return hResult;
}


/*!
 * \brief Construct a new outgoing CstiCall object, and place it in call storage
 *        This method can fail if an incoming call is in progress
 */
CstiCallSP CstiConferenceManager::outgoingCallConstruct (
    const std::string &dialString,
    EstiSubState connectingSubState,
	bool enableEncryption)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);

	CstiSipCallSP sipCall = nullptr;
	CstiCallSP call = nullptr;

	std::string userId;
	std::string groupUserId;
	std::string displayName;
	std::string alternateName;
	std::string vrsCallId;
	std::string returnCallDialString;
	EstiDialMethod returnCallDialMethod = estiDM_UNKNOWN;
	CstiProtocolManager *protocolManager = m_poSipManager;
	auto conferenceParameters = m_stConferenceParams;

	m_pCallStorage->lock ();

	// Ensure there are no incoming calls
	auto incomingCall = m_pCallStorage->incomingGet ();
	stiTESTCOND_NOLOG (!incomingCall, stiRESULT_ERROR);

	protocolManager->UserIdGet (&userId);
	protocolManager->GroupUserIdGet (&groupUserId);
	protocolManager->LocalDisplayNameGet (&displayName);
	m_poSipManager->localAlternateNameGet (&alternateName);
	m_poSipManager->vrsCallIdGet (&vrsCallId);

	if (enableEncryption)
	{
		conferenceParameters.eSecureCallMode = estiSECURE_CALL_PREFERRED;
	}

	sipCall = std::make_shared<CstiSipCall> (&conferenceParameters, m_poSipManager, m_poSipManager->vrsFocusedRoutingGet ());

	call = std::make_shared<CstiCall>(
	            m_pCallStorage, *protocolManager->UserPhoneNumbersGet(), userId,
	            groupUserId, displayName, alternateName, vrsCallId,
				&conferenceParameters);

	stiTESTCOND (call, stiRESULT_ERROR);

	call->ProtocolCallSet (sipCall);
	sipCall->CstiCallSet (call);

	stiDEBUG_TOOL (g_stiCMCallObjectDebug,
	    stiTrace ("CstiConferenceManager::outgoingCallConstruct New call object [%p]\n", call.get());
	);

	call->disallowDestruction ();
	call->RemoteDialStringSet (dialString);
	call->DirectionSet (estiOUTGOING);

	// Set the local return call info so the dialed system knows where we want them to call back to.
	protocolManager->LocalReturnCallInfoGet (&returnCallDialMethod, &returnCallDialString);
	call->LocalReturnCallInfoSet (returnCallDialMethod, returnCallDialString);

	// Add to call storage.  (This will also keep track of the max allowed call count.)
	m_pCallStorage->store (call);

	// Advance the state machine for the call
	protocolManager->NextStateSet (call, esmiCS_CONNECTING, connectingSubState);

STI_BAIL:
	m_pCallStorage->unlock();
	return call;
}


/*!
 * \brief Asynchronous method to update conference ports by posting an event
 * \param pstConferencePorts
 * \return stiHResult
 */
stiHResult CstiConferenceManager::ConferencePortsSet (
	const SstiConferencePorts *pstConferencePorts)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::ConferencePortsSet);

	PostEvent (
		std::bind(&CstiConferenceManager::EventConferencePortsSet, this, *pstConferencePorts));

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Sets the ports to be used through the firewall, used for data channels
 *        conference control, and second listen port
 * \param conferencePorts
 */
void CstiConferenceManager::EventConferencePortsSet (
	SstiConferencePorts conferencePorts)
{
	m_stConferencePorts = conferencePorts;

	if (0 < CallObjectsCountGet (esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
	{
		m_bSetPortsWhenIdle = true;
	}
	else
	{
		m_bSetPortsWhenIdle = false;
		ProtocolMgrPortsSet ();
	}
}


stiHResult CstiConferenceManager::ProtocolMgrPortsSet ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	m_nExpectedPortsSets += 1;

	SstiSipConferencePorts sipPorts{};

	// Send the message to the SIP manager
	sipPorts.Set(&m_stConferencePorts);
	hResult = m_poSipManager->ConferencePortsSet (sipPorts);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::localAlternateNameSet
//
// Description: Sets the local alternate name in the configuration data object
//
// Abstract: Sets the name that will be sent in the SInfo transmissions.
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::localAlternateNameSet (
	const std::string &alternateName)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::localAlternateNameSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Send the message to the SIP manager
	hResult = m_poSipManager->localAlternateNameSet (alternateName);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end CstiConferenceManager::localAlternateNameSet

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::vrsCallIdSet
//
// Description: Sets the vrsCallId
//
// Abstract: Sets the vrsCallId that will be sent in the SInfo transmissions.
//
// Returns:
//	stiRESULT_SUCCESS (Success) or stiRESULT_ERROR (Failure)
//
stiHResult CstiConferenceManager::vrsCallIdSet (
	const std::string &callId)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::vrsCallIdSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Send the message to the SIP manager
	hResult = m_poSipManager->vrsCallIdSet (callId);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end CstiConferenceManager::vrsCallIdSet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::LocalDisplayNameSet
//
// Description: Sets the local display name in the configuration data object
//
// Abstract:  Sets the name that will be sent in the Q.931 DisplayName field
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::LocalDisplayNameSet (
	const std::string &displayName)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::LocalDisplayNameSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Send the message to the SIP manager
	hResult = m_poSipManager->LocalDisplayNameSet (displayName);
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
} // end CstiConferenceManager::LocalDisplayNameSet


/*!\brief Sets the return info for callback purposes
 *
 *  This function is used to set the information needed by the remote system
 *  to call back to the desired location (not always the originating system).
 *  This data will be passed in SInfo to the remote system.
 *
 */
stiHResult CstiConferenceManager::LocalReturnCallInfoSet (
	EstiDialMethod eMethod, // The dial method to use for the return call
    std::string dialString)	// The dial string to dial on the return call
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::LocalReturnCallInfoSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Send the message to the SIP manager
	hResult = m_poSipManager->LocalReturnCallInfoSet (eMethod, dialString);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end CstiConferenceManager::LocalReturnCallInfoSet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::UserPhoneNumbersSet
//
// Description: Sets the device phone numbers in the configuration data object
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::UserPhoneNumbersSet (
	const SstiUserPhoneNumbers *psUserPhoneNumbers)	// UserPhoneNumber structure
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::UserPhoneNumbersSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!stiIS_ERROR (hResult))
	{
		// Send the message to the SIP manager
		hResult = m_poSipManager->UserPhoneNumbersSet (psUserPhoneNumbers);
	}

	return hResult;
} // end CstiConferenceManager::UserPhoneNumbersSet


/*!\brief Sets the m_corePublicIp in SipManager
 *
 * \return stiRESULT_SUCCESS or stiRESULT_INVALID_PARAMETER
 */
stiHResult CstiConferenceManager::corePublicIpSet(
	const std::string &publicIp)
{
	stiLOG_ENTRY_NAME(CstiConferenceManager::corePublicIpSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_poSipManager)
	{
		hResult = m_poSipManager->corePublicIpSet(publicIp);
	}

	return hResult;
} // end CstiConferenceManager::corePublicIpSet


stiHResult CstiConferenceManager::ClientReregister ()
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::ClientReregister);
	stiHResult hResult = stiRESULT_SUCCESS;

	// If there are any calls currently active, hold on to this event
	// and submit it when we get to the idle state.

	Lock ();

	if (m_bActiveCalls)
	{
		m_bClientReregisterNeeded = true;
	}
	else
	{
		// Send the message to the SIP Protocol Manager
		hResult = m_poSipManager->ClientReregister ();
	}

	Unlock ();

	return hResult;
} // end CstiConferenceManager::ClientReregister

void CstiConferenceManager::SipStackRestart ()
{
	Lock ();
	
	m_poSipManager->StackRestart();
	
	Unlock ();
}


// If we are logged in and SIP NAT is not disabled check that our numbers are still registered.
stiHResult CstiConferenceManager::RegistrationStatusCheck (int nStackRestartDelta)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::CheckRegistrationStatus);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	Lock ();
	
	hResult = m_poSipManager->RegistrationStatusCheck (nStackRestartDelta);
	
	Unlock ();

	return hResult;
}


bool CstiConferenceManager::AvailableGet ()
{
	return nullptr != m_poSipManager ? m_poSipManager->AvailableGet () : false;
}


void CstiConferenceManager::AvailableSet (bool isAvailable)
{
	if (nullptr != m_poSipManager)
	{
		m_poSipManager->AvailableSet (isAvailable);
	}
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::NetworkChangedNotify
//
// Description: Notify network parameter has changed
//
// Abstract:
//
//
void CstiConferenceManager::NetworkChangedNotify()
{
	m_poSipManager->NetworkChangeNotify();
}


/*!\brief Sets if we should be using the proxy keep alive code or not.
 *
 */
void CstiConferenceManager::useProxyKeepAliveSet (bool useProxyKeepAlive)
{
	m_poSipManager->useProxyKeepAliveSet (useProxyKeepAlive);
}


void CstiConferenceManager::ConferenceParamsLock () const
{
	m_conferenceParamsLockMutex.lock ();
}


void CstiConferenceManager::ConferenceParamsUnlock () const
{
	m_conferenceParamsLockMutex.unlock ();
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::ConferenceParamsGet
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::ConferenceParamsGet (
	SstiConferenceParams *pstConferenceParams) const
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::ConferenceParamsGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	ConferenceParamsLock ();

	*pstConferenceParams = m_stConferenceParams;

	ConferenceParamsUnlock ();

	return hResult;
} // end CstiConferenceManager::ConferenceParamsGet

void CstiConferenceManager::LoggedInSet (
	bool bLoggedIn)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::LoggedInSet);
	m_poSipManager->LoggedInSet(bLoggedIn);
}

////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::ConferenceParamsSet
//
// Description:
//
// Abstract:
//
// Returns:
//	estiOK (Success) or estiERROR (Failure)
//
stiHResult CstiConferenceManager::ConferenceParamsSet (
	const SstiConferenceParams *pstConferenceParams)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::ConferenceParamsSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	bool bConferenceParamsLocked = false;

#ifdef stiTUNNEL_MANAGER

	bool bNATTraversalChanged = false;

#endif

	ConferenceParamsLock ();

	bConferenceParamsLocked = true;
	
	//
	// Check the parameters found in the structure to make sure they are valid.
	//
	stiTESTCOND (nMINIMUM_NET_SPEED <= pstConferenceParams->nMaxRecvSpeed
				 && nMINIMUM_NET_SPEED <= pstConferenceParams->nMaxSendSpeed, stiRESULT_ERROR);
	
	stiTESTCOND (AUTO_SPEED_MIN_START_SPEED_MINIMUM <= pstConferenceParams->un32AutoSpeedMinStartSpeed
				 && AUTO_SPEED_MIN_START_SPEED_MAXIMUM >= pstConferenceParams->un32AutoSpeedMinStartSpeed,
				 stiRESULT_INVALID_PARAMETER);
	
#ifdef stiTUNNEL_MANAGER

	if (m_stConferenceParams.eNatTraversalSIP != pstConferenceParams->eNatTraversalSIP)
	{
		bNATTraversalChanged = true;
	}

#endif

	m_stConferenceParams = *pstConferenceParams;

	//
	// Override and disable inbound or outbound media base on the debug tools
	//
	if (g_stiDisableOutboundAudio)
	{
		m_stConferenceParams.stMediaDirection.bOutboundAudio = false;
	}

	if (g_stiDisableInboundAudio)
	{
		m_stConferenceParams.stMediaDirection.bInboundAudio = false;
	}

	if (g_stiDisableOutboundVideo)
	{
		m_stConferenceParams.stMediaDirection.bOutboundVideo = false;
	}

	if (g_stiDisableInboundVideo)
	{
		m_stConferenceParams.stMediaDirection.bInboundVideo = false;
	}

	if (g_stiDisableOutboundText)
	{
		m_stConferenceParams.stMediaDirection.bOutboundText = false;
	}

	if (g_stiDisableInboundText)
	{
		m_stConferenceParams.stMediaDirection.bInboundText = false;
	}

	// If NatTraversal was disabled clear out the tunnel address.
	if ((m_stConferenceParams.eNatTraversalSIP != estiSIPNATEnabledUseTunneling))
	{
		m_stConferenceParams.TunneledIpAddr.clear();
	}

#ifndef stiENABLE_SIPS_DIALING
	// make sure that the mode is always disabled
	m_stConferenceParams.eSecureCallMode = estiSECURE_CALL_NOT_PREFERRED;
#endif

	// Send the message to the SIP Protocol Manager
	hResult = m_poSipManager->ConferenceParamsSet (&m_stConferenceParams);
	stiTESTRESULT ();

#ifdef stiTUNNEL_MANAGER
	m_pTunnelManager->ServerSet (&m_stConferenceParams.TunnelServer);

	if (bNATTraversalChanged)
	{
		if (m_stConferenceParams.eNatTraversalSIP == estiSIPNATEnabledUseTunneling)
		{
			m_pTunnelManager->TunnelingEnabledSet (true);
		}
		else
		{
			m_pTunnelManager->TunnelingEnabledSet (false);
		}
	}
#endif

STI_BAIL:

	if (bConferenceParamsLocked)
	{
		ConferenceParamsUnlock ();
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::UserIdsSet
//
// Description: Sets the currently authenticated user id
//
// Abstract:
//
// Returns:
//	estiOK when successful, otherwise estiERROR
//
stiHResult CstiConferenceManager::UserIdsSet (
	const std::string &userId,
	const std::string &groupUserId)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Send the message to the SIP Protocol Manager
	hResult = m_poSipManager->UserIdsSet (userId, groupUserId);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
} // end CstiConferenceManager::UserIdsSet


/*!\brief Sets the default provider agreement status
 *
 *  \return nothing
 */
void CstiConferenceManager::DefaultProviderAgreementSet (
	bool bAgreed)
{
	// Send the message to the SIP Protocol Manager
	m_poSipManager->DefaultProviderAgreementSet (bAgreed);
}


void CstiConferenceManager::LocalInterfaceModeSet (
	EstiInterfaceMode eLocalInterfaceMode)
{
	bool bEnforceAuthorizedPhoneNumber = false;

	ConferenceParamsLock ();

	if (m_stConferenceParams.bCheckForAuthorizedNumber
	 && (estiSTANDARD_MODE == eLocalInterfaceMode || estiPUBLIC_MODE == eLocalInterfaceMode))
	{
		bEnforceAuthorizedPhoneNumber = true;
	}

	ConferenceParamsUnlock ();

	// Send the message to the SIP Protocol Manager
	m_poSipManager->ConfigurationSet (eLocalInterfaceMode,
									  bEnforceAuthorizedPhoneNumber);
}


#ifdef stiTUNNEL_MANAGER
IstiTunnelManager* CstiConferenceManager::TunnelManagerGet ()
{
	return m_pTunnelManager;
}



stiHResult CstiConferenceManager::TunnelIPResolved (
	const std::string *pTunneledIP) //!< Tunneled IP address
{
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiConferenceParams stConferenceParams;
	if (pTunneledIP && !pTunneledIP->empty ())
	{
		hResult = ConferenceParamsGet (&stConferenceParams);
		
		if (!stiIS_ERROR (hResult))
		{
			if (stConferenceParams.TunneledIpAddr != *pTunneledIP)
			{
				stConferenceParams.TunneledIpAddr = *pTunneledIP;
				ConferenceParamsSet (&stConferenceParams);
			}
			else
			{
#ifdef stiTUNNEL_MANAGER
				// There are problems with network changes with tunneling.
				// We call NetworkChangeNotify to resolve these issues and ensure
				// the endpoint is in a state to place and receive calls.
				m_pTunnelManager->NetworkChangeNotify ();
#endif
			}
		}
	} // end if
	
	return hResult;
} // end
#endif


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::AllowIncomingCallsModeSet
//
// Description: Sets the AllowIncomingCalls Mode
//
// Abstract:
//
// Returns:
//
void CstiConferenceManager::AllowIncomingCallsModeSet (
	bool bAllow)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::AllowIncomingCallsModeSet);

	// Send the message to the SIP manager
	m_poSipManager->AllowIncomingCallsModeSet (bAllow);

} // end CstiConferenceManager::AllowIncomingCallsModeSet


void CstiConferenceManager::RvSipStatsGet (
	SstiRvSipStats *pstStats)
{
	// Send the message to the SIP manager
	m_poSipManager->RvStatsGet (pstStats);
}


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::localAlternateNameGet
//
// Description: Return the LocalAlternateName
//
// Abstract:
//	This method simply returns the LocalAlternateName.  When in interpreter
//	interface mode, it will be the interpreter ID; otherwise, it will just
//	be the name of the user logged into the phone.
//
// Returns:
//	an stiHResult which will be stiRESULT_SUCCESS if successful.
//
stiHResult CstiConferenceManager::localAlternateNameGet (
		std::string *pAlternateName)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::localAlternateNameGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_poSipManager->localAlternateNameGet (pAlternateName);

	return hResult;
} // end CstiConferenceManager::localAlternateNameGet


////////////////////////////////////////////////////////////////////////////////
//; CstiConferenceManager::vrsCallIdGet
//
// Description: Return the VRS Call ID
//
// Abstract:
//	This method simply returns the VRS Call ID.  If set and in interpreter
//	interface mode, it will be the VRS Call ID; otherwise, it will be empty.
//
// Returns:
//	an stiHResult which will be stiRESULT_SUCCESS if successful.
//
stiHResult CstiConferenceManager::vrsCallIdGet (
		std::string *callId)
{
	stiLOG_ENTRY_NAME (CstiConferenceManager::vrsCallIdGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	m_poSipManager->vrsCallIdGet(callId);

	return hResult;
} // end CstiConferenceManager::vrsCallIdGet

void CstiConferenceManager::vrsFocusedRoutingSet (const std::string &vrsFocusedRouting)
{
	m_poSipManager->vrsFocusedRoutingSet(vrsFocusedRouting);
}

// end file CstiConferenceManager.cpp

