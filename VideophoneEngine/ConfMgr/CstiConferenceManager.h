/*!
* \file CstiConferenceManager.h
* \brief See CstiConferenceManager.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include <list>
#include <mutex>
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiCall.h"
#include "CstiProtocolManager.h"
#include "CstiRoutingAddress.h"
#include "stiSipStatistics.h"
#include "OptString.h"

#ifdef stiTUNNEL_MANAGER
#include "RvSipTransportTypes.h"
#include "IstiTunnelManager.h"
#endif

//
// Constants
//
#ifdef stiDEBUGGING_TOOLS
const uint8_t un8MAX_EVENT_NAME = 48;  // The maximum length of the "Text"
										// name of an event name.

const int nMAX_STATE_NAME = 128;		// The maximum length of the
										// "Text" name of a state name.
#endif


//
// Forward Declarations
//
class CstiCallStorage;
class CstiSipManager;
#ifdef stiTUNNEL_MANAGER
class CstiTunnelManager;
#endif
struct SstiConferencePorts;


//
// Class Declaration
//
class CstiConferenceManager : public CstiEventQueue
{
public:
	CstiConferenceManager ();
	CstiConferenceManager (const CstiConferenceManager &other) = delete;
	CstiConferenceManager (CstiConferenceManager &&other) = delete;
	CstiConferenceManager &operator= (const CstiConferenceManager &other) = delete;
	CstiConferenceManager &operator= (CstiConferenceManager &&other) = delete;

	~CstiConferenceManager () override;

	stiHResult Initialize (
		IstiBlockListManager2 *pBlockListManager,
		ProductType productType,
		const std::string &version,
		const SstiConferenceParams *pstConferenceParams,
		const SstiConferencePorts *pstConferencePorts);

	void LocalInterfaceModeSet (
		EstiInterfaceMode eLocalInterfaceMode);

	void ConferenceParamsLock () const;

	void ConferenceParamsUnlock () const;

	void LoggedInSet(bool bLoggedIn);
	
	stiHResult ConferenceParamsGet (
		SstiConferenceParams *pstConferenceParams) const;

	stiHResult ConferenceParamsSet (
		const SstiConferenceParams *pstConferenceParams);

	stiHResult Startup ();
	stiHResult Shutdown ();

	stiHResult AutoRejectSet (
		bool bAutoReject);

	bool AutoRejectGet () const;

	stiHResult MaxCallsSet (
		unsigned int maxCalls);
	
	unsigned int MaxCallsGet () const;
	
	CstiCallSP CallDial (
		EstiDialMethod eDialMethod,
		const CstiRoutingAddress &DialString,
		const std::string &originalDialString,
		const OptString &fromNameOverride,
		const OptString &callListName,
		const std::vector<SstiSipHeader>& additionalHeaders = {});
	
	CstiCallSP CallDial (
		EstiDialMethod eDialMethod,
		const CstiRoutingAddress &DialString,
		const std::string &originalDialString,
		const OptString &fromNameOverride,
		const OptString &callListName,
		const OptString &relayLanguage,
		int nRelayLanguageId,
		const std::vector<SstiSipHeader>& additionalHeaders = {});

	unsigned int CallObjectsCountGet ();

	unsigned int CallObjectsCountGet (
		uint32_t un32StateMask);

	CstiCallSP CallObjectHeadGet ();

	CstiCallSP CallObjectGet (
		uint32_t un32StateMask);

	CstiCallSP CallObjectLookup (
	    IstiCall *call) const;

	CstiCallSP CallObjectGetByAppData (
		size_t AppData);

	CstiCallSP CallObjectIncomingGet ();

	stiHResult CallObjectRemove (
	    CstiCall *poCall);

	CstiCallSP callObjectGetByCallIndex (int callIndex);

	stiHResult ConferenceJoin (
	    CstiCallSP poCall1,
	    CstiCallSP poCall2,
		const std::string &DialString);


	void CallObjectListGet (std::list<CstiCallSP> *callList);

#ifdef stiHOLDSERVER
	CstiCallSP CallObjectGetByLocalPhoneNbr (const char *pszPhone);
	CstiCallSP CallObjectGetByHSCallId (__int64 n64CallID);
	CstiCallSP NthCallObjectGet (
		uint32_t un32NthCallNumber);
	void HangupAllCalls ();
#endif

	stiHResult ConferencePortsSet (
		const SstiConferencePorts *pstConferencePorts);

	void DefaultProviderAgreementSet (bool bAgreed);

	CstiCallSP outgoingCallConstruct (
	    const std::string &dialString,
	    EstiSubState connectingSubState,
		bool enableEncryption);

	stiHResult LocalDisplayNameSet (
		const std::string &displayName);

	stiHResult localAlternateNameSet (
		const std::string &alternateName);

	stiHResult vrsCallIdSet (
		const std::string &callId);

	stiHResult LocalReturnCallInfoSet (
		EstiDialMethod eMethod,
	    std::string dialString);

	stiHResult UserPhoneNumbersSet (
		const SstiUserPhoneNumbers *psUserPhoneNumbers);

	stiHResult UserIdsSet (
		const std::string &userId,
		const std::string &groupUserId);

	stiHResult corePublicIpSet (const std::string &publicIp);

	//
	// Methods called by objects within the Conference Manager task
	//

	void NetworkChangedNotify();
	
	void useProxyKeepAliveSet (bool useProxyKeepAlive);

#ifdef stiTUNNEL_MANAGER
	IstiTunnelManager* TunnelManagerGet ();
#endif

	stiHResult TunnelIPResolved (
		const std::string *pTunneledIP);

	void AllowIncomingCallsModeSet (
		bool bAllow);

	void RvSipStatsGet (
		SstiRvSipStats *pstStats);

	stiHResult localAlternateNameGet (
		std::string *pAlternateName);

	stiHResult vrsCallIdGet (
		std::string *callId);

	stiHResult ClientReregister ();
	void SipStackRestart ();
	
	stiHResult RegistrationStatusCheck (
		int nStackRestartDelta);

	// Responds 503 service not available to invite/options when not true
	bool AvailableGet ();
	void AvailableSet (bool isAvailable);
	
	void vrsFocusedRoutingSet (const std::string &vrsFocusedRouting);
	void allCallObjectsRemove ();

public:
	CstiSignal<> startupCompletedSignal;   // notify ready to make/receive calls
	CstiSignal<int> remoteRingCountSignal;
	CstiSignal<uint32_t> localRingCountSignal;
	CstiSignal<const SstiConferenceAddresses &> sipMessageConfAddressChangedSignal;
	CstiSignal<> sipRegistrationRegisteringSignal;
	CstiSignal<std::string> sipRegistrationConfirmedSignal;
	CstiSignal<std::string> publicIpResolvedSignal;
	CstiSignal<CstiCallSP> preCompleteCallSignal;
	CstiSignal<time_t> currentTimeSetSignal;
	CstiSignal<CstiCallSP> directoryResolveSignal;
	CstiSignal<CstiCallSP> callTransferringSignal;
	CstiSignal<const SstiSharedContact&> hearingCallSpawnSignal;
	CstiSignal<const SstiSharedContact&> contactReceivedSignal;
	CstiSignal<CstiCallSP> geolocationUpdateSignal;
	CstiSignal<CstiCallSP> hearingStatusChangedSignal;
	CstiSignal<> hearingStatusSentSignal;
	CstiSignal<> newCallReadySentSignal;
	CstiSignal<> flashLightRingSignal;
	CstiSignal<CstiCallSP> conferencingWithMcuSignal;
	CstiSignal<> heartbeatRequestSignal;
#ifdef stiTUNNEL_MANAGER
	CstiSignal<RvSipTransportConnectionState> rvTcpConnectionStateChangedSignal;
#endif
	CstiSignal<const SstiConferencePorts &> conferencePortsSetSignal;
	CstiSignal<CstiCallSP> removeTextSignal;
	CstiSignal<CstiCallSP> preHoldCallSignal;   // unused?
	CstiSignal<CstiCallSP> preResumeCallSignal; // unused?
	CstiSignal<CstiCallSP> missedCallSignal;
	CstiSignal<CstiCallSP> callBridgeStateChangedSignal;
	CstiSignal<CstiCallSP> preAnsweringCallSignal;
	CstiSignal<CstiCallSP> callTransferFailedSignal;
	CstiSignal<CstiCallSP> callInformationChangedSignal;
	CstiSignal<CstiCallSP> preHangupCallSignal;
	CstiSignal<CstiCallSP, bool> videoPlaybackPrivacyModeChangedSignal;
	CstiSignal<SstiTextMsg*> remoteTextReceivedSignal;
	CstiSignal<CstiCallSP> callStateChangedSignal;
	CstiSignal<CstiCallSP> outgoingCallSignal;  // unused?
	CstiSignal<CstiCallSP> dialingSignal;
	CstiSignal<CstiCallSP> answeringCallSignal;
	CstiSignal<CstiCallSP> resolvingNameSignal;
	CstiSignal<CstiCallSP> preIncomingCallSignal;
	CstiSignal<CstiCallSP> incomingCallSignal;
	CstiSignal<CstiCallSP> ringingSignal;
	CstiSignal<bool> activeCallsSignal;
	CstiSignal<CstiCallSP> establishingConferenceSignal;
	CstiSignal<CstiCallSP> conferencingSignal;
	CstiSignal<CstiCallSP> conferenceEncryptionStateChangedSignal;
	CstiSignal<CstiCallSP> resumedCallLocalSignal;
	CstiSignal<CstiCallSP> resumedCallRemoteSignal;
	CstiSignal<CstiCallSP> disconnectingSignal;
	CstiSignal<CstiCallSP> leaveMessageSignal;
	CstiSignal<CstiCallSP> createVrsCallSignal;
	CstiSignal<CstiCallSP> disconnectedSignal;
	CstiSignal<CstiCallSP> heldCallLocalSignal;
	CstiSignal<CstiCallSP> heldCallRemoteSignal;
	CstiSignal<CstiCallSP> criticalError;
	CstiSignal<CstiCallSP, CstiSipCallLegSP> logCallSignal;
	CstiSignal<const SstiErrorLog*> errorReportSignal;
	CstiSignal<> tunnelTerminatedSignal;  // unused?
	CstiSignal<> failedToEstablishTunnelSignal;  // unused?
	CstiSignal<> pleaseWaitSignal;
	CstiSignal<> vrsServerDomainResolveSignal;
	CstiSignal<> signMailSkipToRecordSignal;
	CstiSignal<const std::string&> remoteGeolocationChangedSignal;
	CstiSignal<bool> dhviCapableSignal;
	CstiSignal<const std::string&, const std::string&, const std::string&> dhviConnectingSignal;
	CstiSignal<> dhviConnectedSignal;
	CstiSignal<bool> dhviDisconnectedSignal; // true indicates that the hearing party initiated the disconnect.
	CstiSignal<> dhviConnectionFailedSignal;
	CstiSignal<CstiCallSP, const std::string&> dhviHearingNumberReceived;
	CstiSignal<CstiCallSP, const std::string&, const std::string&, const std::string&, const std::string&> dhviStartCallReceived;
	CstiSignal<> sipStackStartupFailedSignal;
	CstiSignal<> sipStackStartupSucceededSignal;

private:

	stiHResult ProtocolMgrPortsSet ();

	stiHResult CallStateChangeNotify (
		const SstiStateChange &stateChange);

	stiHResult SubtasksStartup ();
	stiHResult SubtasksShutdown ();
	void signalsConnect ();

	// Registered Event handler methods
	void EventCallStateChangeNotify (
		SstiStateChange stateChange);

	void EventSecondListeningPortChanged (
		bool success);

	void EventConferencePortsSet (
		SstiConferencePorts conferencePorts);

	void CallStateChangeNotifyApp (
	    CstiCallSP call,
		EsmiCallState ePrevState,
		uint32_t un32PrevSubState,
		EsmiCallState eNewState,
		uint32_t un32NewSubstate);

	void StatsCancel (
	    CstiCallSP call);

	stiHResult ErrorReport (
	    const SstiErrorLog* pstErrorLog);

private:

	SstiConferenceParams m_stConferenceParams = {};
	mutable std::mutex m_conferenceParamsLockMutex;
	SstiConferencePorts m_stConferencePorts = {};
	bool m_bSetPortsWhenIdle = false;
	int m_nSuccessfulPortsSets = 0;
	int m_nFailedPortsSets = 0;
	int m_nExpectedPortsSets = 0;

	CstiCallStorage *m_pCallStorage = nullptr;
	CstiSipManager *m_poSipManager = nullptr;
#ifdef stiTUNNEL_MANAGER
	CstiTunnelManager *m_pTunnelManager = nullptr;
#endif
	vpe::EventTimer m_statsCollectTimer; // Used to collect statistics
	int m_nStatsCounter = 0;
	vpe::EventTimer m_remoteRingTimer; 		// Used to notify CCI of a ring.
	vpe::EventTimer m_localRingTimer;		// Used to notify CCI of local ringing.
	vpe::EventTimer m_purgeStaleCallObjectsTimer;
	int m_nRingCount = 0;
	uint32_t m_un32LocalRingCount = 0;
	CstiCallSP m_pRingingIncomingCall = nullptr;

	bool m_bAutoReject = false;
	
	unsigned int m_maxCalls {MAX_CALLS};

#ifdef stiRTP_LOGGING
	void *m_RvRtpLogger = nullptr;
#endif

	bool m_bRtpInitialized = false;
	bool m_bActiveCalls = false;
	bool m_bClientReregisterNeeded = false;

	CstiSignalConnection::Collection m_signalConnections;
};
