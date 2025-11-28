/*!
 * \file CstiCall.h
 * \brief Declaration of the concrete call object class for maintaining calls in the videophone.
 *
 * Sorenson Communications Inc. Confidential - Do not distribute
 * Copyright 2008-2020 by Sorenson Communications, Inc. All rights reserved.
 *
 */
#pragma once

//
// Includes
//
#include "stiSVX.h"
#include "stiProductCompatibility.h"
#include "IstiCall.h"
#include "IVRCLCall.h"
#include "CstiProtocolCall.h"
#include "stiDelegate.h"
#include "CstiRoutingAddress.h"
#include "CstiTimer.h"
#include "OptString.h"
#include <memory>
#include <atomic>
#include <iomanip>
#include "AgentHistoryItem.h"
#include "CstiSignal.h"
#include "CstiEventTimer.h"
#include "stiTools.h"
#include "stiRemoteLogEvent.h"

//
// Constants
//
const int IP_ADDRESS_LENGTH = 4;
extern const int g_nDEFAULT_RELAY_LANGUAGE;
#define nMAX_RINGS_BEFORE_GREETING 15 //The maximum number of rings allowed before greeting will play.
#define nDEFAULT_SIGNMAIL_DOWNLOAD_SPEED 256000

//
// Helpers
//
#define callTraceRecord collectTrace (__func__)
#define callTraceRecordMessage(format, ...) collectTrace (std::string{__func__} + ": " + formattedString (format, ##__VA_ARGS__))
#ifdef GDPR_COMPLIANCE
#define callTraceRecordMessageGDPR(format, ...) callTraceRecord
#else
#define callTraceRecordMessageGDPR(format, ...) callTraceRecordMessage (format, ##__VA_ARGS__)
#endif

// This structure maintains the info needed to
// leave a P2P Video message.
struct SstiMessageInfo
{
	std::string GreetingURL;
	std::string GreetingText;
	EstiBool bP2PMessageSupported{estiFALSE};
	eSignMailGreetingType eGreetingType{eGREETING_NONE};
	int nMaxNumbOfRings{nMAX_RINGS_BEFORE_GREETING};
	int nMaxRecordSeconds{60};
	int nRemoteDownloadSpeed{nDEFAULT_SIGNMAIL_DOWNLOAD_SPEED};
	EstiBool bMailBoxFull{estiFALSE};
	EstiBool bCountdownEmbedded{estiFALSE};

	int nNumbOfRings{0};
	
	// Only used when hearing to deaf and the Interpreter will be leaving the message
	std::string CallId;  
	std::string TerpId;
	OptString hearingName;
};


struct SstiConferenceRoomStats
{
	std::string ConferencePublicId;
	bool bAddAllowed = false;
	int nRoomActive = 0;
	int nRoomAllowed = 0;
	std::list<SstiGvcParticipant> ParticipantList;
	int peakParticipants = 0;
};

class CallLogBuildup
{
public:
	void collectGeneralTrace (std::string message)
	{
		std::lock_guard<std::recursive_mutex> lock (m_generalTracesMutex);
		m_generalTraces.push_back (std::make_tuple (std::chrono::system_clock::now (), message));
		if (m_generalTraces.size () >= MAX_GENERAL_TRACES)
		{
			sendTrace ();
		}
	}

	void collectNewStateTrace (EsmiCallState state, uint32_t substate)
	{
		std::lock_guard<std::mutex> lock (m_statePathMutex);
		m_statePath.push_back (std::make_tuple (state, substate));
	}

	void sendTrace ()
	{
		std::stringstream build{};
		{
			std::lock_guard<std::recursive_mutex> lock (m_generalTracesMutex);
			if (m_generalTraces.empty ())
			{
				return;
			}
			build << "CallTrace:\n";
			for (const auto& traceItem : m_generalTraces)
			{
				build << '[' << timepointToTimestamp (std::get<0> (traceItem)) << "] " << std::get<1> (traceItem) << '\n';
			}
			m_generalTraces.clear ();
		}
		build << stateTraceGet ();
		auto message = build.str ();
		if (message.back () == '\n')
		{
			message.pop_back (); // remove extra new-line
		}

		stiRemoteLogEventSend (&message);
	}

private:
	std::string stateTraceGet () 
	{
		std::stringstream build{};
		{
			std::lock_guard<std::mutex> lock (m_statePathMutex);
			if (m_statePath.empty ())
			{
				return "";
			}
			build << std::hex << std::setfill ('0');;
			build << "StatePath: ";
			for (const auto& state : m_statePath)
			{
				build << "0x" << std::setw (8) << std::get<0> (state) << "_0x" << std::setw (8) << std::get<1> (state) << ", ";
			}
			m_statePath.clear ();
		}
		auto message = build.str ();
		message.pop_back (); // remove extra space
		message.pop_back (); // remove extra comma
		return message;
	}

	const size_t MAX_GENERAL_TRACES{ 1000 };
	std::vector<std::tuple<std::chrono::system_clock::time_point, std::string>> m_generalTraces{};
	std::recursive_mutex m_generalTracesMutex{};
	std::vector<std::tuple<EsmiCallState, uint32_t>> m_statePath{};
	std::mutex m_statePathMutex{};
};

// This enumeration is used to express when performing an action relative
// to the local system or the remote system (eg. Change "Local" or "Remote").
enum ELocation
{
	eLOCAL,		// Local system
	eREMOTE,	// Remote system
	eBOTH,		// Both the local and the remote system
};

// This enumeration is used to indicate if signmail is to be left or not.
enum EstiLeaveSignMail
{
	estiSM_NONE = 0,  // Don't leave a message
	estiSM_LEAVE_MESSAGE_NORMAL,	// Leave a message.
	estiSM_LEAVE_MESSAGE_SKIP_GREETING, // Leave a message but skip to countdown
	estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD, // Leave a message but skip to recording
};


enum EstiTransferLogType
{
	estiTRANSFER_LOG_TYPE_NONE = 0,
	estiTRANSFER_LOG_TYPE_TRANSFERER = 1,
	estiTRANSFER_LOG_TYPE_TRANSFEREE = 2
};


enum EstiICENominationsResult
{
	eICENominationsUnknown = 0,
	eRemoteNotICE = 1,
	eICENominationsSucceeded = 2,
	eICENominationsFailed = 3
};


enum class DisplayOrientation
{
	Unknown = 0,
	Portrait = 1,
	Landscape = 2
};

// This structure is used to pass the new and old states to the CM upon a state
// change.
struct SstiStateChange
{
	CstiCallSP call {nullptr};
	EsmiCallState ePrevState {esmiCS_UNKNOWN};
	uint32_t un32PrevSubStates {0};
	EsmiCallState eNewState {esmiCS_UNKNOWN};
	uint32_t un32NewSubStates {0};
};

class CstiProtocolManager;
class CstiCallStorage;

//
// Class Declaration
//
class CstiCall: public IstiCall, public IVRCLCall, public std::enable_shared_from_this<CstiCall>
{
public:

	CstiCall () = delete;

	// Public Methods
	CstiCall (
		CstiCallStorage *poCallStorage,
		const SstiUserPhoneNumbers &stUserPhoneNumbers,
		const std::string &userId,
		const std::string &groupUserId,
		const std::string &localDisplayName,
		const std::string &localAlternateName,
		const std::string &vrsCallId,
		const SstiConferenceParams *pstConferenceParams);

	CstiCall (const CstiCall &other) = delete;
	CstiCall (CstiCall &&other) = delete;
	CstiCall &operator= (const CstiCall &other) = delete;
	CstiCall &operator= (CstiCall &&other) = delete;

	~CstiCall () override;

	CstiProtocolManager *ProtocolManagerGet ()
	{
		CstiProtocolManager *poProtocolManager = nullptr;
		if (m_protocolCall)
		{
			 poProtocolManager = m_protocolCall->ProtocolManagerGet ();
		}

		return poProtocolManager;
	}

	std::shared_ptr<IstiCall> sharedPointerGet () override { return shared_from_this(); }
	void disallowDestruction () { m_callCopy = shared_from_this(); }
	void allowDestruction ()    { m_callCopy = nullptr; }

	// pass through with custom step
	stiHResult Accept ();

	void DtmfToneSend (
		EstiDTMFDigit eDTMFDigit) override;

	stiHResult inline TransferToAddress(const std::string& address) override
	{
		callTraceRecordMessageGDPR ("%s", address.c_str ());

		DELEGATE_RET_CODE1(m_protocolCall, stiHResult, TransferToAddress, address);
	}
	
	stiHResult ContactShare (const SstiSharedContact &contact) override;
	bool ShareContactSupportedGet () override;

	EstiSwitch VideoPlaybackMuteGet () override;

	// straight pass through for CCI
	DELEGATE1(m_protocolCall,RemoteDisplayNameGet,std::string *);
	void RemoteProductNameGet (std::string *pRemoteProductName) const override;

#ifdef stiHOLDSERVER
	DELEGATE_RET1(m_protocolCall, stiHResult, HSEndService, EstiMediaServiceSupportStates);
#endif

	// straight pass through for application (req'd by IstiCall)
	// TODO: Some of these maybe should be moved up from the ProtocolCall object.
	EstiBool AllowHangUpGet () const override;
	void AllowHangupSet (EstiBool bAllow);
	bool BridgeCompletionNotifiedGet () override
		{ return m_bBridgeCompletionNotification; }
	void BridgeCompletionNotifiedSet (bool bCompleted) override
		{ m_bBridgeCompletionNotification = bCompleted; }
	EstiBool IsHoldableGet () const override;
	EstiBool IsTransferableGet () const override;
	EstiBool ConnectedWithMcuGet (unsigned int unType) const override;
	stiHResult ConnectedWithMcuSet (EstiMcuType eType);
	stiHResult ConferencePublicIdGet (std::string *pConferenceId) const;
	stiHResult ConferencePublicIdSet (const std::string *ConferenceId);
	stiHResult ConferenceRoomStatsSet (
			bool bAddAllowed,
			int nRoomActive,
			int nRoomAllowed,
			const std::list<SstiGvcParticipant> *pParticipantList);
	bool ConferenceRoomParticipantAddAllowed (bool *pbLackOfResources);

	int ConferenceRoomPeakParticipantsGet () const;

	void RemoteNameGet (std::string *pName) const override;
	void RemoteAlternateNameGet (std::string *pName) const override;
	EstiInterfaceMode RemoteInterfaceModeGet () const override;
	void VideoPlaybackSizeGet (uint32_t *pW, uint32_t *pH) const override;
	void VideoRecordSizeGet (uint32_t *pW, uint32_t *pH) const override;
	stiHResult TextSend(const uint16_t* pwszText, EstiSharedTextSource sharedTextSource) override;
	int RemotePtzCapsGet () const override;
	void StatisticsGet (SstiCallStatistics *pStats) const override;
	void StatsCollect ();
	void StatsClear ();
	EstiBool RemoteDeviceTypeIs (int nType) const override;
	EstiBool TextSupportedGet () const override;
	EstiBool DtmfSupportedGet () const override;

	// straight pass through for VRCL
	inline const char* RemoteMacAddressGet() override
	{
		DELEGATE_RET_CODE(m_protocolCall, const char*, RemoteMacAddressGet);
	}

	inline const char* RemoteVcoCallbackGet() override
	{
		DELEGATE_RET_CODE(m_protocolCall, const char*, RemoteVcoCallbackGet);
	}

	inline stiHResult BandwidthDetectionBegin() override
	{
		DELEGATE_RET_CODE(m_protocolCall, stiHResult, BandwidthDetectionBegin);
	}

	void TextShareRemoveRemote() override;

	void inline RemoteProductVersionGet (std::string *pRemoteProductVersion) override
	{
		if (m_protocolCall)
		{
			m_protocolCall->RemoteProductVersionGet (pRemoteProductVersion);
		}
		else
		{
			pRemoteProductVersion->clear ();
		}
	}

	// TODO: 90% of how this is used (especially in ConferenceManager) is wrong!
	EstiProtocol ProtocolGet () const override;
	bool ProtocolModifiedGet ();

	// standard function
	stiHResult Lock() const;
	void Unlock() const;

	stiHResult Answer () override;
	stiHResult ContinueDial () override;
	inline stiHResult HangUp () override { return HangUp (estiFALSE); } // The application cannot set bResult.
	stiHResult HangUp (EstiBool bUseResult);
	stiHResult Hold () override;
	inline CstiProtocolCallSP ProtocolCallGet () const {return (m_protocolCall); }
	void ProtocolCallSet (CstiProtocolCallSP protocolCall);
	stiHResult Resume () override;
	stiHResult Transfer (const std::string &dialString) override;
	std::string ResultNameGet () const override;

	stiHResult RemoteLightRingFlash () override;

	stiHResult Reject (
		EstiCallResult eCallResult) override; //!< The reason for rejection

	inline EstiBool AppNotifiedOfConferencingGet () const
	{
		Lock ();
		EstiBool bRet = m_bAppNotifiedOfConferencing && !m_isDhviMcuCall ? estiTRUE : estiFALSE;
		Unlock ();

		return (bRet);
	}

	inline void AppNotifiedOfConferencingSet (EstiBool bSet)
	{
		Lock ();
		m_bAppNotifiedOfConferencing = bSet;
		Unlock ();
	}

#if 0
	inline EstiBool AppNotifiedOfIncomingGet () const
	{
		Lock ();
		EstiBool bRet = m_bAppNotifiedOfIncomingCall;
		Unlock ();

		return (bRet);
	}

	inline void AppNotifiedOfIncomingSet (EstiBool bSet)
	{
		Lock ();
		m_bAppNotifiedOfIncomingCall = bSet;
		Unlock ();
	}
#endif
		
	inline EstiBool NotifyAppOfHoldChangeGet () const
	{
		Lock ();
		EstiBool bRet = m_bNotifyAppOfHoldChange && !m_isDhviMcuCall ? estiTRUE : estiFALSE;
		Unlock ();
		return (bRet);
	}

	inline void NotifyAppOfHoldChangeSet (EstiBool bSet)
	{
		Lock ();
		m_bNotifyAppOfHoldChange = bSet;
		Unlock ();
	}

	inline EstiBool NotifyAppOfIncomingCallGet () const
	{
		Lock ();
		EstiBool bRet = m_bNotifyAppOfIncomingCall && !m_isDhviMcuCall ? estiTRUE : estiFALSE;
		Unlock ();
		return (bRet);
	}

	inline void NotifyAppOfIncomingCallSet (EstiBool bSet)
	{
		Lock ();
		m_bNotifyAppOfIncomingCall = bSet;
		Unlock ();
	}
		
	inline EstiBool NotifyAppOfCallGet () const
	{
		Lock ();
		EstiBool bRet = m_bNotifyAppOfCall && !m_isDhviMcuCall ? estiTRUE : estiFALSE;
		Unlock ();
		return (bRet);
	}
	
	inline void NotifyAppOfCallSet (EstiBool bNotifyApp)
	{
		Lock ();
		m_bNotifyAppOfCall = bNotifyApp;
		Unlock ();
	}
	
	void CallInformationChanged ();
	void CalledNameGet (std::string *pCalledName) const override;
	void CalledNameSet (const char *szName);

	void VideoMailServerNumberGet(std::string *pVideoMailNumber) const;
	void VideoMailServerNumberSet(const std::string *pVideoMailNumber);

	bool directSignMailGet () const override;
	void directSignMailSet (bool directSignMail);

	stiHResult DialStringGet (
		EstiDialMethod *peDialMethod,
		std::string *pDialString) const override;

	void NewDialStringGet (
		std::string *pDialString) const override;

	void NewDialStringSet (
	    std::string dialString);

	EstiBool CallBlockedGet()
	{
		Lock ();

		EstiBool bRet = m_bCallBlocked;

		Unlock ();

		return bRet;
	}

	void CallBlockedSet(EstiBool bCallBlocked)
	{
		callTraceRecordMessage ("%d", bCallBlocked);

		Lock ();

		m_bCallBlocked = bCallBlocked;

		Unlock ();
	}

	stiHResult BridgeCreate (const std::string &uri) override;
	stiHResult BridgeDisconnect () override;
	stiHResult BridgeStateGet (EsmiCallState *peCallState) override;
	EstiDialMethod DialMethodGet () const override ;
	void DialMethodSet (EstiDialMethod eMethod);
	void IsInContactsSet (EstiInContacts eIsInContacts);
	EstiInContacts IsInContactsGet () const override;
	CstiItemId ContactIdGet() const override;
	void ContactIdSet(const CstiItemId &Id);
	CstiItemId ContactPhoneNumberIdGet() const override;
	void ContactPhoneNumberIdSet(const CstiItemId &Id);
	
	stiHResult backgroundGroupCallCreate (const std::string &uri, const std::string &conferenceId);

	EstiLeaveSignMail LeaveMessageGet () const
	{
		Lock ();
		EstiLeaveSignMail eRet = m_eLeaveMessage;
		Unlock ();
		return (eRet);
	}

	void LeaveMessageSet (EstiLeaveSignMail eVal)
	{
		Lock ();
		m_eLeaveMessage = eVal;
		Unlock ();
	}

	OptString fromNameOverrideGet () const;
	void fromNameOverrideSet (const OptString &fromNameOverride);
	
	std::string localAlternateNameGet () const;
	void localAlternateNameSet (const std::string &alternateName);
	std::string vrsCallIdGet () const;
	void vrsCallIdSet (const std::string &vrsCallId);
	std::string vrsAgentIdGet () const override;
	void vrsAgentIdSet (const std::string &agentId);

	std::string subscriptionIdGet()
	{
		Lock ();
		std::string subscriptionId = m_subscriptionId;
		Unlock ();
		return subscriptionId;
	}

	void subscriptionIdSet(const std::string& subscriptionId)
	{
		Lock ();
		m_subscriptionId = subscriptionId;
		Unlock ();
	}

	std::string LocalDisplayNameGet () const;
	void LocalDisplayNameSet (const std::string &displayName);
	stiHResult LocalReturnCallInfoSet (EstiDialMethod eMethod, std::string dialString);
	void LocalReturnCallDialStringGet (std::string *returnDialString) const;
	EstiDialMethod LocalReturnCallDialMethodGet () const;

	inline int LocalPreferredLanguageGet ()
	{
		Lock ();
		int nRet = m_nLocalPreferredLanguage;
		Unlock ();
		return nRet;
	}

	void LocalPreferredLanguageGet (OptString *preferredLanguage) const;
	void LocalPreferredLanguageSet (const OptString &preferredLanguage, int nLang);

	stiHResult MessageInfoSet (SstiMessageInfo *pstMessageInfo);
	stiHResult MessageInfoGet (SstiMessageInfo *pstMessageInfo);

	bool IsSignMailAvailableGet () const override;
	stiHResult SignMailSkipToRecord () override;

	eSignMailGreetingType MessageGreetingTypeGet () const override;
	std::string MessageGreetingTextGet() const override;
	std::string MessageGreetingURLGet() const override;

	bool signMailBoxFullGet() const override
	{
		return m_pstMessageInfo ? m_pstMessageInfo->bMailBoxFull == estiTRUE : false;
	}

	bool SignMailInitiatedGet () const override
	{
		Lock ();
		bool bSignMailInitiated = m_bSignMailInitiated;
		Unlock ();

		return bSignMailInitiated;
	}

	void SignMailInitiatedSet (
	        bool bSignMailInitiated)
	{
		callTraceRecordMessage ("%d", bSignMailInitiated);

		Lock ();
		m_bSignMailInitiated = bSignMailInitiated;
		Unlock ();
	}

	uint32_t SignMailDurationGet ()
	{
		Lock ();
		uint32_t un32SignMailDuration = m_un32SignMailDuration;
		Unlock ();

		return un32SignMailDuration;
	}

	void SignMailDurationSet (
	        uint32_t un32SignMailDuration)
	{
		callTraceRecordMessage ("%zu", un32SignMailDuration);

		Lock ();
		m_un32SignMailDuration = un32SignMailDuration;
		Unlock ();
	}

	int MessageRecordTimeGet() override;
	EstiBool MessageCountdownEmbeddedGet();
	ICallInfo *RemoteCallInfoGet () override
	{ return &m_oRemoteCallInfo; }
	void RemoteCallListNameGet (std::string *remoteCallListName) const;
	void RemoteCallListNameSet (const std::string &remoteCallListName);
	void RemoteContactNameGet (std::string *pRemoteContactName) const;
	void RemoteContactNameSet (const char *pszRemoteContactName);
	int RemoteDownloadSpeedGet () const;
	inline const ICallInfo *LocalCallInfoGet ()
	{ return &m_oLocalCallInfo; }

	inline EstiDirection DirectionGet () const override
	{
		Lock ();
		EstiDirection eRet = m_eDirection;
		Unlock ();

		return (eRet);
	}

	inline void DirectionSet (EstiDirection eDirection)
	{
		callTraceRecordMessage ("%d", eDirection);

		Lock ();
		m_eDirection = eDirection;
		Unlock ();
	}

	virtual void OriginalDialStringGet (std::string *pOriginalDialString) const;
	void OriginalDialMethodSet (EstiDialMethod eDialMethod);
	void OriginalDialStringSet (EstiDialMethod eDialMethod, std::string dialString);
	void IntendedDialStringGet (std::string *pszDialString) const override;
	void IntendedDialStringSet (std::string dialString);
	virtual EstiDialMethod RemoteDialMethodGet () const;
	void RemoteDialMethodSet (EstiDialMethod eMethod);
	void RemoteDialStringGet (std::string *pRemoteDialString) const override;
	void RemoteDialStringSet (std::string pszDialString);
	void TransferDialStringGet (std::string *pRemoteDialString) const;
	void TransferDialStringSet (std::string dialString);
	std::string RemoteIpAddressGet () const override;
	void RemoteIpAddressSet (const std::string &remoteIpAddress);

	CstiRoutingAddress RoutingAddressGet () const
	{
		Lock ();
		auto routingAddress = m_RoutingAddress;
		Unlock ();

		return routingAddress;
	}

	void RoutingAddressSet (const CstiRoutingAddress &routingAddress)
	{
		callTraceRecordMessageGDPR ("%s", routingAddress.originalStringGet ().c_str ());

		Lock ();
		m_RoutingAddress = routingAddress;
		Unlock ();
	}

	inline void AppDataSet (size_t data)
	{
		Lock ();
		m_AppData = data;
		Unlock ();
	}

	inline size_t AppDataGet ()
	{
		Lock ();
		size_t ret = m_AppData;
		Unlock ();
		return (ret);
	}

	inline EsmiCallState StateGet () const override
	{
		Lock ();
		EsmiCallState eRet = m_eCallState;
		Unlock ();
		return (eRet);
	}

	stiHResult NextStateSet (
	        EsmiCallState eState,
	        uint32_t un32Substate = estiSUBSTATE_NONE);

	void StateSet (
	        EsmiCallState eState,
	        uint32_t un32Substate = estiSUBSTATE_NONE);

	inline uint32_t SubstateGet () const override
	{
		Lock ();
		uint32_t un32Ret = m_un32Substate;
		Unlock ();
		return (un32Ret);
	}

	EstiBool StateValidate (IN uint32_t un32States) const;
	EstiBool SubstateValidate (IN uint32_t un32Substates) const;
	inline void ConferencedSet (
	       bool conferenced)
	{
		Lock ();
		if (m_protocolCall)
		{
			m_protocolCall->ConferencedSet (conferenced);
		}
		Unlock ();
	}
	inline bool ConferencedGet () const override
	{
		Lock ();
		bool conferenced = false;
		if (m_protocolCall)
		{
			conferenced = m_protocolCall->ConferencedGet ();
		}
		Unlock ();
		return (conferenced);
	}

	EstiBool TransferGet () const
	{
		Lock ();
		EstiBool bRet = m_bTransfer;
		Unlock ();
		return (bRet);
	}
	void TransferSet (EstiBool bVal)
	{
		Lock ();
		m_bTransfer = bVal;
		Unlock ();
	}

	bool RelayTransferGet () const override
	{
		Lock ();
		bool bRet = m_bRelayTransfer;
		Unlock ();
		return (bRet);
	}
	void RelayTransferSet (bool bVal)
	{
		Lock ();
		m_bRelayTransfer = bVal;
		Unlock ();
	}

	void TransferredSet ();

	EstiBool TransferredGet () const override;

	EstiBool VerifyAddressGet () const override;
	void VerifyAddressSet (
	        EstiBool bVerifyAddress);

	inline void ResultSentToVRCLSet (EstiBool bResultSentToVRCL)
	{
		Lock ();
		m_bResultSentToVRCL = bResultSentToVRCL;
		Unlock ();
	}

	inline EstiBool ResultSentToVRCLGet () const override
	{
		Lock ();
		EstiBool bRet = m_bResultSentToVRCL;
		Unlock ();

		return (bRet);
	}

	inline EstiCallResult ResultGet () const override
	{
		Lock ();
		EstiCallResult eResult = m_eResult;
		Unlock ();

		return (eResult);
	}

	inline void ResultSet (EstiCallResult eResult)
	{
		Lock ();

		// If result is in the UNKNOWN or CALL_SUCCESSFUL state and we get an 
		// error we want to change the result, and we only want the first error.
		if (m_eResult == EstiCallResult::estiRESULT_UNKNOWN ||
			m_eResult == EstiCallResult::estiRESULT_CALL_SUCCESSFUL)
		{
			m_eResult = eResult;
		}
		Unlock ();
	}

	inline void AddedToCallListSet (EstiBool bAdded)
	{
		Lock ();
		m_bAddedToCallList = bAdded;
		Unlock ();
	}

	inline EstiBool AddedToCallListGet () const
	{
		Lock ();
		EstiBool bRet = m_bAddedToCallList;
		Unlock ();

		return bRet;
	}

	std::chrono::milliseconds ConnectingDurationGet ();

	double CallDurationGet () override;
	double secondsSinceCallEndGet ();

	void CallIdGet (std::string *pCallId) override;

	void OriginalCallIDGet (std::string *pOriginalCallID);

	int CallIndexGet () override
	{
		return m_nCallIndex;
	}

	inline void DialedOwnRingGroupSet (EstiBool bDialedOwnRingGroup)
	{
		Lock ();
		m_bDialedOwnRingGroup = bDialedOwnRingGroup;
		Unlock ();
	}

	inline EstiBool DialedOwnRingGroupGet () const
	{
		Lock ();
		EstiBool bRet = m_bDialedOwnRingGroup;
		Unlock ();

		return (bRet);
	}

	inline EstiVcoType RemoteVcoTypeGet () override
	{
		Lock ();

		EstiVcoType eVcoType = m_eVcoType;

		Unlock ();

		return eVcoType;
	}

	inline void RemoteVcoTypeSet (EstiVcoType eVcoType)
	{
		callTraceRecordMessage ("%d", eVcoType);

		Lock ();
		m_eVcoType = eVcoType;
		Unlock ();
	}

	inline EstiBool RemoteIsVcoActiveGet () override
	{
		Lock ();

		EstiBool bRemoteIsVcoActive = m_bRemoteIsVcoActive;

		Unlock ();

		return bRemoteIsVcoActive;
	}

	inline void RemoteIsVcoActiveSet (EstiBool bIsVcoActive)
	{
		callTraceRecordMessage ("%d", bIsVcoActive);

		Lock ();
		m_bRemoteIsVcoActive = bIsVcoActive;
		Unlock ();
	}

	inline void LocalIsVcoActiveSet (bool bIsVcoActive) override
	{
		callTraceRecordMessage ("%d", bIsVcoActive);

		Lock ();
		m_bLocalIsVcoActive = bIsVcoActive;
		Unlock ();
	}

	inline bool LocalIsVcoActiveGet ()
	{
		Lock ();

		bool bLocalIsVcoActive = m_bLocalIsVcoActive;

		Unlock ();

		return bLocalIsVcoActive;
	}

	inline void AlwaysAllowSet(EstiBool bAlwaysAllow)
	{
		Lock ();
		m_bAlwaysAllow = bAlwaysAllow;
		Unlock ();
	}

	inline EstiBool AlwaysAllowGet ()
	{
		return m_bAlwaysAllow;
	}

	inline time_t StartTimeGet () const
	{
		Lock ();
		time_t start = m_timeCallStart;
		Unlock ();

		return start;
	}
	
	inline void SIPCallInfoSet (const std::string &sipCallInfo)
	{
		callTraceRecordMessageGDPR ("%s", sipCallInfo.c_str ());

		Lock ();
		
		if (!sipCallInfo.empty ())
		{
			m_sipCallInfo = sipCallInfo;
		}
		
		Unlock ();
	}
	
	inline std::string SIPCallInfoGet () const
	{
		Lock ();
		std::string callInfo = m_sipCallInfo;
		Unlock ();
		
		return callInfo;
	}

	inline ETriState RemoteRegisteredGet ()
	{
		Lock ();
		ETriState eRet = m_eRemoteRegistered;
		Unlock ();

		return eRet;
	}

	inline void RemoteRegisteredSet (ETriState eRegistered)
	{
		callTraceRecordMessage ("%d", eRegistered);

		Lock ();
		m_eRemoteRegistered = eRegistered;
		Unlock ();
	}

	std::string LocalPreferredLanguageGet() const override;
	std::string RemotePreferredLanguageGet () const override;
	void RemotePreferredLanguageSet (const std::string &preferredLanguage);
	inline int RemotePreferredLanguageCodeGet ()
	{
		Lock ();
		int nRet = m_nRemotePreferredLanguage;
		Unlock ();

		return nRet;
	}
	inline void RemotePreferredLanguageSet (int nLang)
	{
		Lock ();
		m_nRemotePreferredLanguage = nLang;
		Unlock ();
	}

#ifdef stiHOLDSERVER
	inline uint32_t NthCallNumberGet ()
	{
		Lock ();
		uint32_t un32Ret = m_un32NthCall;
		Unlock ();

		return un32Ret;
	}
	inline void NthCallNumberSet (uint32_t un32NthCall)
	{
		Lock ();
		m_un32NthCall = un32NthCall;
		Unlock ();
	}
	inline int64_t HSCallIdGet ()
	{
		Lock ();
		int64_t n64Ret = m_n64HSCallId;
		Unlock ();

		return n64Ret;
	}
	inline void HSCallIdSet (int64_t n64HSCallId)
	{
		Lock ();
		m_n64HSCallId = n64HSCallId;
		Unlock ();
	}
	inline EstiBool HSActiveStatusGet ()
	{
		Lock ();
		EstiBool bRet = m_bHSActiveStatus;
		Unlock ();

		return bRet;
	}
	void HSActiveStatusSet (EstiBool bHSActiveStatus);

	inline EstiBool IsCallH323InterworkedGet()
	{
		return m_bIsCallH323Interworked;
	}

	inline void IsCallH323InterworkedSet(EstiBool value)
	{
		m_bIsCallH323Interworked = value;
	}

	static int m_nMaxHoldServerCalls;

	char m_AssertedIdentity[un8stiDIAL_STRING_LENGTH + 1];

	inline void AssertedIdentitySet(const char *pAssertedIdentity)
	{
		m_AssertedIdentity[0] = '\0';
		if (pAssertedIdentity != NULL &&
		    strlen(pAssertedIdentity) > 0)
		{
			strncpy_s(m_AssertedIdentity, pAssertedIdentity, sizeof(m_AssertedIdentity));
			m_AssertedIdentity[un8stiDIAL_STRING_LENGTH] = '\0';
		}
	}

	inline char * AssertedIdentityGet()
	{
		return m_AssertedIdentity;
	}

#endif // stiHOLDSERVER

	std::string m_vriUserInfo{};

	inline void vriUserInfoSet (std::string userInfo)
	{
		m_vriUserInfo = userInfo;
	}

	inline std::string vriUserInfoGet ()
	{
		return m_vriUserInfo;
	}

	std::string m_interpreterSkills{};

	inline void interpreterSkillsSet (std::string interpreterSkills)
	{
		m_interpreterSkills = interpreterSkills;
	}

	inline std::string interpreterSkillsGet ()
	{
		return m_interpreterSkills;
	}

	std::string m_sipGeolocation;

	inline void SIPGeolocationSet(const std::string &sipGeolocation)
	{
		if (!sipGeolocation.empty())
		{
			m_sipGeolocation = sipGeolocation;
		}
	}

	inline const char * SIPGeolocationGet()
	{
		return m_sipGeolocation.c_str();
	}

	static int m_nCallCount;
	static std::mutex m_callCountMutex;

	EstiTransferLogType TransferLogTypeGet () const
	{
		return m_eTransferLogType;
	}

	void TransferLogTypeSet (
	        EstiTransferLogType eTransferLogType)
	{
		m_eTransferLogType = eTransferLogType;
	}

	void TransferFromDialStringGet (
	        std::string *pFromDialString)
	{
		*pFromDialString = m_transferFromDialString;
	}

	void TransferToDialStringGet (
	        std::string *pToDialString)
	{
		*pToDialString= m_RoutingAddress.userGet ();
	}

	void DialSourceSet (
	        EstiDialSource eDialSource)
	{
		callTraceRecordMessage ("%d", eDialSource);

		m_eDialSource = eDialSource;
	}

	EstiDialSource DialSourceGet ()
	{
		return m_eDialSource;
	}

	void MediaTransportTypesSet (
	        EstiMediaTransportType eRtpTransportAudio,
	        EstiMediaTransportType eRtcpTransportAudio,
	        EstiMediaTransportType eRtpTransportText,
	        EstiMediaTransportType eRtcpTransportText,
	        EstiMediaTransportType eRtpTransportVideo,
	        EstiMediaTransportType eRtcpTransportVideo)
	{
		Lock ();

		m_eRtpTransportAudio = eRtpTransportAudio;
		m_eRtcpTransportAudio = eRtcpTransportAudio;
		m_eRtpTransportText = eRtpTransportText;
		m_eRtcpTransportText = eRtcpTransportText;
		m_eRtpTransportVideo = eRtpTransportVideo;
		m_eRtcpTransportVideo = eRtcpTransportVideo;

		Unlock ();

		callTraceRecordMessageGDPR (
			"Video(%d, %d), Audio(%d, %d), Text(%d, %d)", 
			eRtpTransportVideo, eRtcpTransportVideo, eRtpTransportAudio, eRtcpTransportAudio, eRtpTransportText, eRtcpTransportText);
	}

	void MediaTransportTypesGet (
	        EstiMediaTransportType *peRtpTransportAudio,
	        EstiMediaTransportType *peRtcpTransportAudio,
	        EstiMediaTransportType *peRtpTransportText,
	        EstiMediaTransportType *peRtcpTransportText,
	        EstiMediaTransportType *peRtpTransportVideo,
	        EstiMediaTransportType *peRtcpTransportVideo)
	{
		Lock ();

		*peRtpTransportAudio = m_eRtpTransportAudio;
		*peRtcpTransportAudio = m_eRtcpTransportAudio;
		*peRtpTransportText = m_eRtpTransportText;
		*peRtcpTransportText = m_eRtcpTransportText;
		*peRtpTransportVideo = m_eRtpTransportVideo;
		*peRtcpTransportVideo = m_eRtcpTransportVideo;

		Unlock ();
	}

	void RemoteVideoAddrSet (const char *pszAaddr)
	{
		callTraceRecordMessageGDPR ("%s", pszAaddr);
		m_RemoteVideoAddr = pszAaddr;
	}

	std::string RemoteVideoAddrGet () const
	{
		return m_RemoteVideoAddr;
	}

	void LocalVideoAddrSet (const char *pszAaddr)
	{
		callTraceRecordMessageGDPR ("%s", pszAaddr);
		m_LocalVideoAddr = pszAaddr;
	}

	std::string LocalVideoAddrGet () const
	{
		return m_LocalVideoAddr;
	}

	void VideoPlaybackPortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nVideoPlaybackPort = nPort;
	}

	int VideoPlaybackPortGet ()
	{
		return m_nVideoPlaybackPort;
	}

	void VideoRemotePortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nRemoteVideoPort = nPort;
	}

	int VideoRemotePortGet ()
	{
		return m_nRemoteVideoPort;
	}

	void AudioPlaybackPortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nAudioPlaybackPort = nPort;
	}

	int AudioPlaybackPortGet ()
	{
		return m_nAudioPlaybackPort;
	}

	void AudioRemotePortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nRemoteAudioPort = nPort;
	}

	int AudioRemotePortGet ()
	{
		return m_nRemoteAudioPort;
	}

	void TextPlaybackPortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nTextPlaybackPort = nPort;
	}

	int TextPlaybackPortGet ()
	{
		return m_nTextPlaybackPort;
	}

	void TextRemotePortSet (int nPort)
	{
		callTraceRecordMessage ("%d", nPort);
		m_nRemoteTextPort = nPort;
	}

	int TextRemotePortGet ()
	{
		return m_nRemoteTextPort;
	}

	void HearingStatusSend (EstiHearingCallStatus eHearingStatus);
	void NewCallReadyStatusSend (bool bNewCallReadyStatus);
	bool CanSpawnNewRelayCall ();
	EstiHearingCallStatus HearingCallStatusGet () override;
	void SpawnCall (const SstiSharedContact &callInfo);

	void SpawnCallNumberSet (std::string dialString)
	{
		callTraceRecordMessageGDPR ("%s", dialString.c_str ());
		m_SpawnCallNumber = dialString;
	}

	void SpawnCallNumberGet (std::string *pSpawnCallDialString)
	{
		*pSpawnCallDialString = m_SpawnCallNumber;
	}

	bool AddMissedCallGet ()
	{
		return m_bAddMissedCall;
	}

	void AddMissedCallSet (bool bAddMissedCall)
	{
		m_bAddMissedCall = bAddMissedCall;
	}

	bool IsBehindIVVRGet ()
	{
		return m_bIsBehindIVVR;
	}

	void IsBehindIVVRSet (bool bIsBehindIVVR)
	{
		callTraceRecordMessage ("%d", bIsBehindIVVR);
		m_bIsBehindIVVR = bIsBehindIVVR;
	}

	bool RemoteCallerIdBlockedGet ()
	{
		return m_bRemoteCallerIdBlocked;
	}

	void RemoteCallerIdBlockedSet (bool bRemoteCallerIdBlocked)
	{
		m_bRemoteCallerIdBlocked = bRemoteCallerIdBlocked;
	}

	bool LocalCallerIdBlockedGet () const
	{
		return m_bLocalCallerIdBlocked;
	}

	void LocalCallerIdBlockedSet (bool bLocalCallerIdBlocked)
	{
		m_bLocalCallerIdBlocked = bLocalCallerIdBlocked;
	}

	bool RemoteAddMissedCallGet ()
	{
		return m_bRemoteAddMissedCall;
	}

	void RemoteAddMissedCallSet (bool bRemoteAddMissedCall)
	{
		m_bRemoteAddMissedCall = bRemoteAddMissedCall;
	}

	void FirNegotiatedSet (bool bSet) {callTraceRecordMessage ("%d", bSet); m_bFirNegotiated = bSet;};
	bool FirNegotiatedGet () {return m_bFirNegotiated;};

	void PliNegotiatedSet (bool bSet) {callTraceRecordMessage ("%d", bSet); m_pliNegotiated = bSet;};
	bool PliNegotiatedGet () {return m_pliNegotiated;};

	void TmmbrNegotiatedSet (bool bSet) {callTraceRecordMessage ("%d", bSet); m_tmmbrNegotiated = bSet;};
	bool TmmbrNegotiatedGet () {return m_tmmbrNegotiated;};
	
	void NackRtxNegotiatedSet (bool negotiated) {callTraceRecordMessage ("%d", negotiated); m_nackRtxNegotiated = negotiated;};
	bool NackRtxNegotiatedGet () {return m_nackRtxNegotiated;};

	void vrsPromptRequiredSet (bool value) {callTraceRecordMessage ("%d", value); m_vrsPromptRequired = value;};
	bool vrsPromptRequiredGet () { return m_vrsPromptRequired; }

	void InitialInviteAddressSet (
	        std::string InitialInviteAddress)
	{
		callTraceRecordMessageGDPR ("%s", InitialInviteAddress.c_str ());
		m_InitialInviteAddress = InitialInviteAddress;
	}

	std::string InitialInviteAddressGet () const
	{
		return m_InitialInviteAddress;
	}

	void UriSet (
	        const std::string &Uri)
	{
		callTraceRecordMessageGDPR ("%s", Uri.c_str ());
		m_Uri = Uri;
	}

	std::string UriGet () const
	{
		return m_Uri;
	}

	void InitialPreferredDisplaySizeLog ();
	void PreferredDisplaySizeChangeLog ();
	void PreferredDisplaySizeStatsGet (
	        std::string *preferredDisplaySizeStats);


	std::string VRSFailOverServerGet () const
	{
		return m_failOverServer;
	}

	void VRSFailOverServerSet (const std::string &server)
	{
		callTraceRecordMessage ("%s", server.c_str ());
		m_failOverServer = server;
	}
	
	void ForcedVRSFailoverSet (bool forcedVRSFailover)
	{
		callTraceRecordMessage ("%d", forcedVRSFailover);
		m_forcedVRSFailover = forcedVRSFailover;
	}

	bool ForcedVRSFailoverGet () const
	{
		return m_forcedVRSFailover;
	}

	void UseVRSFailoverSet (bool useFailOver)
	{
		m_useVRSFailover = useFailOver;
	}

	bool UseVRSFailoverGet () const
	{
		return m_useVRSFailover;
	}
	
	void FailureTimersStop ();
	
	std::string TrsUserIPGet () const override;

	vpe::EncryptionState encryptionStateGet () const override;

	bool mediaSendAllowed () const;

	bool geolocationRequestedGet () const override;
	void geolocationRequestedSet (bool geolocationRequested) override;
	GeolocationAvailable geolocationAvailableGet () const;
	void geolocationAvailableSet (GeolocationAvailable geolocationAvailable) override;
	
	void unregisteredCallSet (bool unregistered);
	bool unregisteredCallGet () const;
	void remoteDestSet (const std::string &remoteAddr);
	std::string remoteDestGet () const;

	std::string localIpAddressGet() const;
	
	void callLegCreatedSet (bool created);
	bool callLegCreatedGet () const;

	void logCallSet (bool logCall);
	bool logCallGet () const;
	
	void dhviStateSet (DhviState dhviState);
	DhviState dhviStateGet () override;
	void dhviCapabilityCheck (const std::string &phoneNumber);
	
	ISignal<DhviState>& dhviStateChangeSignalGet() override;
	
	bool m_isDhviMcuCall {false};
	
	stiHResult dhviMcuDisconnect () override;
	
	void dhvHearingNumberSet (const std::string &hearingNumber);
	std::string dhvHearingNumberGet () const;

	void dhvConnectingTimerStart (CstiEventQueue* eventQueue);
	void dhvConnectingTimerStop ();

	inline bool LeavingMessageGet() const override
	{
		return SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE | estiSUBSTATE_MESSAGE_COMPLETE);
	}

	inline bool GreetingStartingGet() const override
	{
		return !MessageGreetingURLGet().empty() && LeaveMessageGet() != estiSM_LEAVE_MESSAGE_SKIP_TO_RECORD;
	}

	inline bool SignMailCompletedGet() const override
	{
		return StateValidate(esmiCS_DISCONNECTED) &&
			SubstateValidate(estiSUBSTATE_LEAVE_MESSAGE | estiSUBSTATE_MESSAGE_COMPLETE);
	}

	const std::vector<SstiSipHeader>& additionalHeadersGet() const override;
	void additionalHeadersSet(const std::vector<SstiSipHeader>& additionalHeaders) override;
	void additionalHeadersSet(const std::vector<SstiSipHeader>&& additionalHeaders) override;
	
	void collectTrace (std::string message);
	void sendTrace ();

public:
	static std::string g_szDEFAULT_RELAY_LANGUAGE;

	CstiTimer pleaseWaitTimer;
	CstiTimer vrsFailOverTimer;

private:
	
	void ForceVRSFailover ();
	
	SstiConferenceParams m_stConferenceParams; // Stored copy of the conference params to be passed in to created protocol call(s)

	CstiCallStorage *m_poCallStorage = nullptr;	// Handle to the call storage class
	CstiProtocolCallSP m_protocolCall;
	mutable std::recursive_mutex m_LockMutex; // Used to secure access to this call from different threads

	EsmiCallState m_eCallState = esmiCS_IDLE; // The state of the current call
	uint32_t m_un32Substate = estiSUBSTATE_NONE; // The substate of the current call
	EstiDirection m_eDirection = estiINCOMING; // The direction of the call (Incoming/Outgoing)
	size_t m_AppData = 0;

	/*
	 * Original
	 * 	The number that was originally dialed.
	 * 	Changed to redirected (New) number if present.
	 *
	 * New
	 * 	The number to which the original phone number was changed (only set on outbound calls).
	 * 	This number comes from a core services directory resolve.
	 *
	 * Remote
	 *  On outbound calls this is set to the original dial string.
	 *  On inbound calls this is set to to the user portion of the SIP From URI if the user
	 *  portion is a phone number. Otherwise it is set to the whole SIP From URI.
	 *  This may be replaced by the MyPhone description on an inbound call when
	 *  calling within a MyPhone group.
	 *
	 * Original Remote
	 *  A copy of the remote dial string from the first caller before a transfer.
	 *
	 * Intended
	 * 	The number the remote endpoint tried to reach (only set on inbound calls).
	 * 	This number comes from the user portion of the SIP To URI.
	 *
	 * Transfer
	 *  The number to which the call is being transferred.
	 *
	 * Local Return Call
	 * 	The phone number of the local endpoint when an interface group overrides the local number
	 */
	std::string m_originalDialString;
	std::string m_newDialString;
	std::string m_remoteDialString;
	std::string m_originalRemoteDialString;
	std::string m_intendedDialString;
	std::string m_transferDialString;
	std::string m_localReturnCallDialString;

	EstiDialMethod m_eDialMethod = estiDM_UNKNOWN;
	EstiDialMethod m_eOriginalDialMethod = estiDM_UNKNOWN;
	EstiDialMethod m_eRemoteDialMethod = estiDM_UNKNOWN;
	EstiDialMethod m_eOriginalRemoteDialMethod = estiDM_UNKNOWN;
	EstiDialMethod m_eLocalReturnCallDialMethod = estiDM_UNKNOWN;	// Contains the dial method when an interface group overrides the local dial method

	char m_szCalledName[un8stiNAME_LENGTH + 1]{};
	std::string m_remoteCallListName; // Since this defaults to the dial string, must be able to fit a dial string.
	char m_szRemoteContactName[un8stiNAME_LENGTH + 1]{};
	char m_szOriginalRemoteName[un8stiNAME_LENGTH + 1]{};
	
	CstiRoutingAddress m_RoutingAddress;
	std::string m_localDisplayName;
	std::string m_localAlternateName;
	std::string m_vrsCallId;
	std::string m_vrsAgentId;
	std::string m_subscriptionId;
	OptString m_fromNameOverride;  // When placing a call, if set, this will override the name the remote endpoint sees (empty is valid override value)
	CstiCallInfo m_oLocalCallInfo;
	EstiBool m_bAppNotifiedOfConferencing = estiFALSE;
	EstiBool m_bNotifyAppOfHoldChange = estiTRUE;
	EstiBool m_bNotifyAppOfIncomingCall = estiTRUE;
	EstiBool m_bNotifyAppOfCall = estiTRUE;

	EstiBool m_bCallBlocked = estiFALSE;

	EstiBool m_bTransfer = estiFALSE; // Flag denoting the need to initiate a transfer.
	bool m_bRelayTransfer = false;  // Flag denoting that this is a relay transfer.
	EstiBool m_bTransferred = estiFALSE; // Flag indicating that this call has been transferred.
	EstiMcuType m_eConnectedWithMcuType = estiMCU_NONE; // Indicates what type of MCU (if any) this call is connected with.
	SstiConferenceRoomStats m_stConferenceRoomStats = {}; // When connected with a Group Video Chat room, this contains the room's information.

	EstiInContacts m_eIsInContacts = estiInContactsUnknown; // Indicates whether or not the callee is in the contacts list
	CstiItemId m_ContactId;
	CstiItemId m_ContactPhoneNumberId;

	EstiBool m_bVerifyAddress = estiFALSE;

	SstiMessageInfo *m_pstMessageInfo = nullptr;	// Structrue that holds the info needed to leave a message.
	EstiLeaveSignMail m_eLeaveMessage = estiSM_NONE; // Flag denoting the need leave a message on the disconnect.
	bool m_bSignMailInitiated = false;
	uint32_t m_un32SignMailDuration = 0;

	CstiCallInfo m_oRemoteCallInfo;

	EstiBool m_bResultSentToVRCL = estiFALSE; // The call result (m_eResult) has been sent to the VRCL client

	EstiCallResult m_eResult = estiRESULT_UNKNOWN; // The call result

	EstiBool m_bAddedToCallList = estiFALSE;			// Flag indicating call was already added to call list.

	int m_nLocalPreferredLanguage = 0;	// The preferred language ID to use for interpreting services
	OptString m_localPreferredLanguage; // The preferred language (as a string) to use for interpreting services


	//
	// This data members are set using the wall clock. Do not
	// use the monotonic or any other clock as this value is
	// used in call history.
	//
	time_t m_timeCallStart = 0;         // Keep track of the when the call gets created
	//time_t m_timeCallStop;

	EstiBool m_bDialedOwnRingGroup = estiFALSE;  // Dialed another member of their own ring group
	EstiVcoType m_eVcoType = estiVCO_NONE; // Indicates the type of VCO preferred by the device (None, 1-line, or 2-line)
	EstiBool m_bRemoteIsVcoActive = estiFALSE; // Indicates whether the call is placed with VCO or not
	bool m_bLocalIsVcoActive = false; // Indicates the _local_ Vco is active
	EstiBool m_bAlwaysAllow = estiFALSE; // Indicates the call (e.g. from CIR or 911 callback) should (even if inbound calls are currently disallowed)
	bool m_bBridgeCompletionNotification = false; // Indicates whether the final bridge completion state must be sent.
	
	ETriState m_eRemoteRegistered = TRI_UNKNOWN;  // Has the remote system signed the default provider agreement?

	int m_nRemotePreferredLanguage = 0;           // The preferred language to use for interpreting services.
	std::string m_remotePreferredLanguage{}; // The preferred language (as a string) to use for interpreting services
#ifdef stiHOLDSERVER
	uint32_t m_un32NthCall = 0;
	int64_t m_n64HSCallId = 0;
	EstiBool m_bHSActiveStatus = estiTRUE;
	EstiBool m_bIsVcoActive = estiFALSE;          // Indicates whether the call is placed with VCO or not
	EstiBool m_bIsCallH323Interworked = estiFALSE;
#endif // stiHOLDSERVER

	int m_nCallIndex = 0;
	static int m_nNextCallIndex;

	EstiTransferLogType m_eTransferLogType = estiTRANSFER_LOG_TYPE_NONE;
	std::string m_transferFromDialString;
	std::string m_VideoMailServerNumber;
	bool m_directSignMail = false;
	EstiDialSource m_eDialSource = estiDS_UNKNOWN;

	EstiMediaTransportType m_eRtpTransportAudio = estiMediaTransportUnknown;
	EstiMediaTransportType m_eRtcpTransportAudio = estiMediaTransportUnknown;
	EstiMediaTransportType m_eRtpTransportText = estiMediaTransportUnknown;
	EstiMediaTransportType m_eRtcpTransportText = estiMediaTransportUnknown;
	EstiMediaTransportType m_eRtpTransportVideo = estiMediaTransportUnknown;
	EstiMediaTransportType m_eRtcpTransportVideo = estiMediaTransportUnknown;
	
	std::string m_SpawnCallNumber;
	bool m_bAddMissedCall = false; // If set we will add a missed call.
	bool m_bIsBehindIVVR = false; // Determines if we are behind an IVVR or not.
	bool m_bRemoteCallerIdBlocked = false; // Determines if the remote caller ID is blocked
	bool m_bLocalCallerIdBlocked = false; // Determines if the local caller ID is blocked
	bool m_bRemoteAddMissedCall = false; // If set we will send the SINFO tag estiTAG_ADD_MISSED_CALL
	bool m_bFirNegotiated = false; // Did we negotiate FIR with the remote endpoint?
	bool m_pliNegotiated = false;		// Did we negotiate PLI with the remote endpoint?
	bool m_tmmbrNegotiated = false;	// Did we negotiate TMMBR with the remote endpoint?
	bool m_vrsPromptRequired = false;
	bool m_nackRtxNegotiated = false; // NACK and RTX negotiated
	
	std::string m_InitialInviteAddress;
	std::string m_Uri;

	int m_nVideoPlaybackPort = 0;
	int m_nAudioPlaybackPort = 0;
	int m_nTextPlaybackPort = 0;
	int m_nRemoteVideoPort = 0;
	int m_nRemoteAudioPort = 0;
	int m_nRemoteTextPort = 0;
	
	std::string m_LocalVideoAddr;
	std::string m_RemoteVideoAddr;
	
	uint16_t m_NumberRotations = 0;
	double m_DurationInLandscape = 0;
	double m_DurationInPortrait = 0;
	DisplayOrientation m_initialOrientation = DisplayOrientation::Unknown;
	DisplayOrientation m_currentOrientation = DisplayOrientation::Unknown;
	
	std::string m_sipCallInfo;
	std::string m_failOverServer;
	bool m_forcedVRSFailover = false;
	bool m_useVRSFailover = false;

	// We keep a copy of the shared ptr internally so the call doesn't get destructed prematurely
	// Unfortunately Radvision passes around raw pointers to call objects and the calls were being
	// destructed before the call leg was completetly terminated.  Call allowDestruction() to unset.
	CstiCallSP m_callCopy = nullptr;

	bool m_geolocationRequested = false;
	GeolocationAvailable m_geolocationAvailable = GeolocationAvailable::NotDetermined;

	CstiSignalConnection::Collection m_signalConnections;
	
	bool m_unregisteredCall = false;
	std::string m_remoteDest;

	bool m_CallLegCreated = false;
	bool m_logCall = true;
	
	DhviState m_dhviState {DhviState::NotAvailable};
	
	CstiSignal<IstiCall::DhviState> dhviStateChangeSignal;
	std::string m_dhvHearingNumber;

	CstiTimer m_dhvConnectingTimer;
	CstiSignalConnection m_dhvConnectingTimerSignalConnection;
	
	std::string m_hearingCapabilityCheckNumber;

	std::vector<SstiSipHeader> m_additionalHeaders{};
	
	CallLogBuildup m_logs{};

};	// End class CstiCall declaration

