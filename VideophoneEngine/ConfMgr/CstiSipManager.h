/*!
* \file CstiSipManager.h
* \brief See CstiSipManager.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once


// Enable this to allow a timer before reporting a 404/not found error.
//#define USE_USER_NOT_FOUND_DELAY 1

// Turn on some more verbose debugging
// DO NOT CHECK IN WITH THESE ENABLED!
//#define SIP_DBG_NEGOTIATION 1 // Debug the SDP negotiation process
//#define SIP_DBG_EVENT_SEPARATORS 1 // Print separators to indicate when SIP messages occour

// implement the debug messages flagged above
#ifdef SIP_DBG_NEGOTIATION
#undef SIP_DBG_NEGOTIATION
#define SIP_DBG_NEGOTIATION(fmt,...) stiTrace (fmt "\n", ##__VA_ARGS__);
#else
#define SIP_DBG_NEGOTIATION(...)
#endif
#ifdef SIP_DBG_EVENT_SEPARATORS
#undef SIP_DBG_EVENT_SEPARATORS
#define SIP_DBG_EVENT_SEPARATORS(fmt,...) stiTrace ("============== " fmt " ==============\n", ##__VA_ARGS__);
#define SIP_DBG_LESSER_EVENT_SEPARATORS(fmt,...) stiTrace ("---------- " fmt " ----------\n", ##__VA_ARGS__);
#else
#define SIP_DBG_EVENT_SEPARATORS(...)
#define SIP_DBG_LESSER_EVENT_SEPARATORS(...)
#endif


//
// Includes
//
#include "CstiProtocolManager.h"
#include "stiRtpChannelDefs.h"
#include "CstiIceManager.h"
#include "CstiSipRegistration.h"
#include "CstiSipResolver.h"
#include "stiSipStatistics.h"
#include "CstiRegEx.h"
#include "CstiSdp.h"
#include "stiSipConstants.h"
#include "IstiPlatform.h"  // RestartSystem
#include "OptString.h"
#include "Sip/Transaction.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Sip
#include "RvSipMid.h"
#include "RvSipStackTypes.h"
#include "RvSipStack.h"
#include "RvSipContactHeader.h"

//
// Constants
//

#define SIP_OUTBOUND_PROXY_PORT			5070
const char g_211UriRegex[] = "^\\+?1?(-|\\()?211";

//
// Typedefs
//
struct SstiSipConferencePorts
{
	uint16_t nListenPort;
	uint16_t nTLSListenPort;

	void Set (const SstiConferencePorts *pstConferencePorts)
	{
		nListenPort = pstConferencePorts->nSIPListenPort;
		nTLSListenPort = pstConferencePorts->nSIPTLSListenPort;
	}
};

struct SstiPreferredMediaMap
{
	// Provide a default, but also a constant to use for lookups
	static const int8_t PREFERRED_AUDIO_FEATURE_DYNAMIC_PAYLOAD_TYPE_NONE = -1;

	int nCodec{0}; // EstiVideoCodec, EstiAudio, or EstiTextCodec
	ECodec eCodec {estiCODEC_UNKNOWN};
	int8_t n8PayloadTypeNbr{0}; // Either the constant payload id for this media type or a flag indicating it is dynamic
	// This is a dynamic payload type, but is preferred to use this ID
	// Set this to NONE by default to make it an "opt-in" feature
	// NOTE: any time this is used, be sure to adjust NextDynamicPayloadTypeGet()
	// to skip it to not hand it out for another dynamic profile
	// NOTE: it's only used for audio feature payload ids, and it's usage is deprecated
	int8_t preferredAudioFeatureDynamicPayloadType = PREFERRED_AUDIO_FEATURE_DYNAMIC_PAYLOAD_TYPE_NONE;
	std::string protocolName;
	std::list<EstiPacketizationScheme> PacketizationSchemes;
	std::list<SstiProfileAndConstraint> Profiles;
	std::list<RtcpFeedbackType> rtcpFeedbackTypes;
	uint32_t audioClockRate {8000};
};


//
// Forward Declarations
//
class CstiIceManager;
class CstiIceSession;
class CstiSipCall;
class CstiSipCallLeg;
class CstiSipResolver;
namespace vpe
{
	namespace sip
	{
		class Message;
	}
}


//
// Globals
//

//
// Class Declaration
//

class CstiSipManager : public CstiProtocolManager
{
public:

	CstiSipManager (
	        CstiCallStorage *pCallStorage,
	        IstiBlockListManager2 *pBlockListManager);

	CstiSipManager (const CstiSipManager &other);
	CstiSipManager (CstiSipManager &&other);
	CstiSipManager &operator= (const CstiSipManager &other);
	CstiSipManager &operator= (CstiSipManager &&other);

	~CstiSipManager () override;

	stiHResult Initialize (
	        ProductType productType,
	        const std::string &version,
	        const SstiSipConferencePorts *pConferencePorts);

	stiHResult Startup ();

	stiHResult Shutdown ();

	stiHResult ConferencePortsSet (
	        const SstiSipConferencePorts& conferencePorts);

	stiHResult UserPhoneNumbersSet (
	        const SstiUserPhoneNumbers *psUserPhoneNumbers) override;

	void LoggedInSet(bool bLoggedIn);

	stiHResult ConferenceParamsSet (
	        const SstiConferenceParams *pstConferenceParams) override;

	void AllowIncomingCallsModeSet(bool bAllow);

	stiHResult NetworkChangeNotify();
	
	void useProxyKeepAliveSet (bool useProxyKeepAlive);

	CstiCallSP CallDial (
	        EstiDialMethod eDialMethod,
	        const CstiRoutingAddress &DialString,
	        const std::string &originalDialString,
	        const OptString &fromNameOverride,
	        const OptString &callListName,
	        CstiSipCallSP bridgedSipCall = nullptr,
	        CstiSipCallSP backgroundCall = nullptr,
	        const std::vector<SstiSipHeader>& additionalHeaders = {});
	
	CstiCallSP CallDial (
			EstiDialMethod eDialMethod,
			const CstiRoutingAddress &DialString,
			const std::string &originalDialString,
			const OptString &fromNameOverride,
			const OptString &callListName,
			const OptString &relayLanguage,
			int nRelayLanguageId,
			CstiSipCallSP bridgedSipCall = nullptr,
			CstiSipCallSP backgroundCall = nullptr,
			const std::vector<SstiSipHeader>& additionalHeaders = {});

	stiHResult ConferenceJoin (
	        CstiCallSP call1,
	        CstiCallSP call2,
	        const std::string &DialString);

	void CallObjectRemoved ();

	stiHResult BridgeCreate (
	        const std::string &uri,
	        CstiCallSP call);
	
	void dhviCreate (
	        const std::string &uri,
	        const std::string &conferenceId,
	        CstiCallSP call,
	        bool sendInfo);

	stiHResult ReRegisterNeeded (bool bActiveCalls);

	void RvStatsGet (SstiRvSipStats *pstRvStats);

	CstiIceSession::Protocol IceProtocolGet(const CstiSipCallLegSP &callLeg);
	stiHResult IceSessionStart (CstiIceSession::EstiIceRole eRole, const CstiSdp &Sdp, IceSupport *iceSupport, CstiSipCallLegSP callLeg);

	bool privateGeolocationHeaderCheck (const vpe::sip::Message &inboundMsg, CstiCallSP poCall);

	// Responds 503 service not available to invite/options when not true
	bool AvailableGet ();
	void AvailableSet (bool isAvailable);

	bool IsRegistered ();
	stiHResult TransportConnectionCheck (bool performNetworkChange);

	stiHResult ClientReregister ();
	void StackRestart (); // Restarts the SIP Stack

	stiHResult RegistrationStatusCheck (int nStackRestartDelta);

	stiHResult VRSFailoverCallDisconnect (
	    CstiCallSP call) override;
	
	bool TunnelActive ();

	void localAlternateNameGet (std::string *alternateName) const;
	stiHResult localAlternateNameSet (
		const std::string &alternateName);

	void vrsCallIdGet (std::string *callId) const;
	stiHResult vrsCallIdSet (
		const std::string &callId);
	
	void vrsFocusedRoutingSet (const std::string &vrsFocusedRouting)
	{
		m_vrsFocusedRouting = vrsFocusedRouting;
	}
	
	std::string vrsFocusedRoutingGet ()
	{
		return m_vrsFocusedRouting;
	}
	
	void dhvSipMgrMcuDisconnectCall (CstiCallSP call);

	stiHResult corePublicIpSet (const std::string &publicIp);

public:
	CstiSignal<> sipRegisterNeededSignal;
	CstiSignal<const SstiConferenceAddresses &> sipMessageConfAddressChangedSignal;
	CstiSignal<> sipRegistrationRegisteringSignal;
	CstiSignal<std::string> sipRegistrationConfirmedSignal;
	CstiSignal<std::string> publicIpResolvedSignal;
	CstiSignal<CstiCallSP> preCompleteCallSignal;  // dsNameResolved
	CstiSignal<time_t> currentTimeSetSignal;
	CstiSignal<CstiCallSP> directoryResolveSignal;
	CstiSignal<CstiCallSP> callTransferringSignal;
	CstiSignal<const SstiSharedContact&> hearingCallSpawnSignal;  // Send phone# to interpreter to place new call
	CstiSignal<const SstiSharedContact&> contactReceivedSignal;
	CstiSignal<CstiCallSP> geolocationUpdateSignal;
	CstiSignal<CstiCallSP> hearingStatusChangedSignal;
	CstiSignal<> hearingStatusSentSignal;
	CstiSignal<> newCallReadySentSignal;
	CstiSignal<> flashLightRingSignal;
	CstiSignal<CstiCallSP> preIncomingCallSignal;
	CstiSignal<CstiCallSP> conferencingWithMcuSignal;
	CstiSignal<> heartbeatRequestSignal;
	CstiSignal<RvSipTransportConnectionState> rvTcpConnectionStateChangedSignal;
	CstiSignal<bool> conferencePortsSetSignal;  // Successfully set the Conference Ports
	CstiSignal<CstiCallSP> removeTextSignal;
	CstiSignal<CstiCallSP> preHoldCallSignal;  // unused?
	CstiSignal<CstiCallSP> preResumeCallSignal;  // unused?
	CstiSignal<CstiCallSP> missedCallSignal;
	CstiSignal<CstiCallSP> callBridgeStateChangedSignal;
	CstiSignal<CstiCallSP> preAnsweringCallSignal;
	CstiSignal<CstiCallSP> callTransferFailedSignal;
	CstiSignal<CstiCallSP, bool> videoPlaybackPrivacyModeChangedSignal;  // true if muted, false if unmuted
	CstiSignal<SstiTextMsg*> remoteTextReceivedSignal;
	CstiSignal<> vrsServerDomainResolveSignal;
	CstiSignal<> signMailSkipToRecordSignal;
	CstiSignal<const std::string&> remoteGeolocationChangedSignal;
	CstiSignal<CstiCallSP> createVrsCallSignal;
	CstiSignal<CstiCallSP, CstiSipCallLegSP> logCallSignal;
	CstiSignal<bool> dhviCapableSignal;
	CstiSignal<const std::string&, const std::string&, const std::string&> dhviConnectingSignal;
	CstiSignal<> dhviConnectedSignal;
	CstiSignal<bool> dhviDisconnectedSignal; // true indicates that the hearing party initiated the disconnect.
	CstiSignal<> dhviConnectionFailedSignal;
	CstiSignal<CstiCallSP, const std::string&> dhviHearingNumberReceived;
	CstiSignal<CstiCallSP, const std::string&, const std::string&, const std::string&, const std::string&> dhviStartCallReceived;
	CstiSignal<CstiCallSP> encryptionStateChangedSignal;
	CstiSignal<> sipStackStartupFailedSignal;
	CstiSignal<> sipStackStartupSucceededSignal;

private:
	//Constants
	

	void eventLoop () override;

	// API
	inline HRPOOL AppPoolGet ()
	{ return m_hAppPool; }

	stiHResult Register (const char *pszNumberToRegister);
	stiHResult Unregister (const char *pszNumberToUnregister);
	stiHResult RegisterFailureRetry (const char *pszPhoneNumber);

	const SstiSipConferencePorts *ConferencePortsGet ()
	{ return &m_stConferencePorts; }

	inline RvSipSubsMgrHandle SubscriptionManagerGet ()
	{ return m_hSubscriptionMgr; }

	stiHResult TCPConnectionCreate ();
	void TCPConnectionDestroy ();

	stiHResult CallAnswer (CstiCallSP call) override;
	stiHResult CallHangup (CstiCallSP call, SipResponse sipRejectCode);
	stiHResult CallHold (CstiCallSP call) override;
	stiHResult CallReject (CstiCallSP call) override;
	stiHResult CallResume (CstiCallSP call) override;
	stiHResult CallTransfer (CstiCallSP call, std::string dialString) override;
	stiHResult CallSendDtmf (CstiCallSP call, int nDtmfCode);
	stiHResult DSNameResolved (CstiCallSP call) override;
	stiHResult ConnectedHeaderParse(vpe::sip::Message inboundMsg, CstiSipCallSP sipCall);

	stiHResult InviteRequirementsAdd (
		RvSipMsgHandle hMsg,
		bool bIsBridgedCall,
		bool geolocationRequested);
	
	stiHResult geolocationAdd (
		RvSipMsgHandle msgHandle);

	stiHResult InviteSend (
		CstiSipCallSP sipCall,
		const CstiSdp *pSdp);

	stiHResult OutboundDetailsSet (
		CstiSipCallSP sipCall,
		CstiSipCallLegSP callLeg,
		std::string *pProxy,
		EstiTransport *peTransport);
	inline EstiInterfaceMode LocalInterfaceModeGet ()
		{ return m_eLocalInterfaceMode; }

	bool IsLocalAgentMode ();

	stiHResult PreferredVideoPlaybackProtocolsCreate (
			std::list<SstiPreferredMediaMap> *pProtocols);
	stiHResult PreferredVideoRecordProtocolsCreate (
			std::list<SstiPreferredMediaMap> *pProtocols);
	stiHResult PreferredAudioProtocolsCreate (
			std::list<SstiPreferredMediaMap> *pProtocols,
			std::list<SstiPreferredMediaMap> *pFeatures);
	stiHResult PreferredTextProtocolsCreate (
			std::list<SstiPreferredMediaMap> *pProtocols);

	void ReInviteSend (CstiCallSP call, int nCurrentRetryCount, bool bLostConnectionDetection);
	stiHResult RemoteLightRingFlash (CstiCallSP call) override;
	int ConnectionKeepaliveTimeoutGet ()
		{ return m_proxyConnectionKeepaliveTimer.TimeoutGet (); }
	stiHResult ConnectionKeepalive ();

	bool IsCallOnRegisteredProxyConnection (const CstiCallSP &call);
	bool ProxyConnectionReflexiveIPPortGet (std::string *ipAddress, uint16_t *port, EstiTransport *transport);
	bool IsUsingOutboundProxy (CstiCallSP call);
	bool UseInboundProxy () const;

	const SstiUserPhoneNumbers *UserPhoneNumbersGet ()
	{
		return &m_UserPhoneNumbers;
	}

	void ProxyRegistrationLookupRestart ();

	std::string ProxyDomainGet ();
	std::string ProxyDomainAltGet ();
	stiHResult ProxyDomainResolve ();
	void ProxyDomainResolverDelete ();

	stiHResult ProxyRegistrarLookup ();

	void ProxyRegistrarLookupTimerStart ();

	RvSipTranscMgrHandle TransactionMgrGet ()
	{
		return m_hTransactionMgr;
	}

	RvSipMidMgrHandle MidMgrGet ()
	{
		return m_hMidMgr;
	}

	/*!\brief This method sends a video flow control to the sip call object.
	 *
	 * This method is in place to avoid a deadlock in the Radvision stack.
	 *
	 * \param poSipCall The call object for which the flow control is needed.
	 * \param pIChannel The video playback channel on which the control is needed.
	 * \param nNewRate The new data rate (in 100kbps units).
	 */
	stiHResult VideoFlowControlSend (
		CstiSipCallSP sipCall,
		std::shared_ptr<CstiVideoPlayback> videoPlayback,
		int nNewRate);

	stiHResult CallObjectRemove (
	    CstiCallSP call);

	stiHResult SInfoGenerate (
		std::string *systemInfo,
		CstiSipCallSP sipCall);

	stiHResult SignMailSkipToRecord(CstiCallSP call);

private:
	bool m_bLoggedIn = false;

	// Task management params
	EstiBool m_bShutdown = estiFALSE;			// Should this task shut down now?
	EstiBool m_bRunning = estiFALSE;			// Is this task running?

	// Stack management handles
	RvSipStackHandle m_hStackMgr = nullptr;
	RvSipAuthenticatorHandle m_hAuthenticatorMgr = nullptr;
	RvSipRegClientMgrHandle m_hRegClientMgr = nullptr;
	RV_LOG_Handle m_hLog = nullptr;
	HRPOOL m_hAppPool = nullptr;
	RvSipCallLegMgrHandle m_hCallLegMgr = nullptr;
	RvSipMidMgrHandle m_hMidMgr = nullptr;
	RvSipTransportMgrHandle m_hTransportMgr = nullptr;
	RvSipSubsMgrHandle m_hSubscriptionMgr = nullptr;
	RvSipTranscMgrHandle m_hTransactionMgr = nullptr;
	RvSipResolverMgrHandle m_hResolverMgr = nullptr;

	CstiSipResolverSharedPtr ResolverAllocate(CstiSipResolver::EResolverType type);
	void ResolverDeleter (CstiSipResolver *resolver); // Custom deleter for the resolvers
	// Hold on to resolver event connections
	// NOTE: use raw pointer because it's used in the custom deleter
	std::map<CstiSipResolver *, CstiSignalConnection::Collection> m_resolverConnections;

	stiHResult ResolverRemove(
		const CstiSipResolverSharedPtr &resolver);

	RvSipCallLegHandle m_hLookupCallLeg = nullptr;
#ifdef stiENABLE_TLS
	/*Handle to the server TLS engine. that engine holds the key and will provide a
	 matching certificate on demand */
	RvSipTransportTlsEngineHandle m_hTlsServerEngine = nullptr;
	/*Handle to the client TLS engine. that engine will be loaded with a certificate
	 authority certificate and will be responsible to verify any incoming certificate */
	RvSipTransportTlsEngineHandle m_hTlsClientEngine = nullptr;
#endif // stiENABLE_TLS


	// Normal params
	CstiTimer m_proxyWatchdogTimer;
	CstiTimer m_proxyConnectionKeepaliveTimer;
	CstiTimer m_stackStartupRetryTimer; // Used if the Stack Startup fails so that we retry again later
	CstiTimer m_resourceCheckTimer;  // Used to check the stack resources periodically
	CstiTimer m_restartProxyRegistrarLookupTimer;
	CstiTimer m_proxyRegisterRetryTimer;
	int m_nConnectRetryAttemptCount = 0;

	CstiSignalConnection::Collection m_signalConnections;

	SstiCMInitData m_CMInitData; // Data that describes this phone
	std::map<std::string,CstiSipRegistration*> m_Registrations;

	SstiSipConferencePorts m_stConferencePorts{};

	RvSipTransportConnectionHandle m_hPersistentProxyConnection = nullptr;

	SstiConferenceAddresses m_ConferenceAddresses;

	std::string m_ReflexiveAddress;
	
	std::string m_TunneledIpAddr;

	std::string m_proxyDomain;
	std::string m_proxyDomainIpAddress;
	uint16_t m_proxyDomainPort = 0;

	std::string m_PublicDomainIpAddress;
	uint16_t m_un16PublicDomainPort = nDEFAULT_SIP_LISTEN_PORT;

#ifdef stiMVRS_APP
	std::string m_LastIpAddress;
#endif
	
	bool m_bSwitchProtocols = false;

	std::unique_ptr<CstiIceManager> m_iceManager;
	CstiSignalConnection::Collection m_iceConnections;

	CstiCallSP m_originalCall = nullptr;
	CstiSipCallSP m_originalSipCall = nullptr;
	EstiDirection m_eOriginalDirection = estiUNKNOWN;
	CstiCallSP m_transferCall = nullptr;
	CstiSipCallSP m_transferSipCall = nullptr;

	CstiRegEx * m_GvcUri = nullptr;
	CstiRegEx m_regex211Uri{ g_211UriRegex };
	
	unsigned int m_unLastNonceCount = 0;

	unsigned int m_unLastOptionsRequestSequence = 0;
	time_t m_LastOptionsRequestTime = 0;

	std::string m_alternateName;
	std::string m_vrsCallId;
	mutable std::recursive_mutex m_vrsInfoMutex;

	void ConferenceAddressSet ();
	
	// Task management functions
	void StackShutdown ();

#ifdef stiENABLE_TLS
	stiHResult InitTlsSecurity ();
#endif
	// Utility functions
	stiHResult AddrUrlGetHost (RvSipAddressHandle hAddress, std::string *host);
	RvUint32 AddrUrlGetPortNum (RvSipAddressHandle hAddress);
	
	stiHResult CallDial (CstiCallSP call);
	stiHResult CallLegContactAttach (RvSipCallLegHandle hCallLeg);
	
	void PreferredVideoProtocolsGet (
			std::list<SstiPreferredMediaMap> *pProtocols,
			const std::list <EstiPacketizationScheme> &pktSchemes,
			bool bIsPlayback);
	stiHResult RegistrarSettingsDetermine (const char *pszPhoneNumber, CstiSipRegistration::SstiRegistrarSettings *pRegSettings);
	bool PhoneNumberVerify (const char *pszPhoneNumber);
	void PrintMessageContents (IN RvSipMsgHandle hMsg);
	
	stiHResult RegistrationsCreate ();
	
	stiHResult RegistrationCreate (
		const char *pszNumberToRegister,
		int nRegistrationID);
	
	void RegistrationsDelete ();
	
	stiHResult CallLegCreate (
		CstiSipCallSP sipCall);

	stiHResult InvitePrepare (
		CstiSipCallSP sipCall);
	
	stiHResult OfferingCallProcess (
		RvSipCallLegHandle hCallLeg,
		vpe::sip::Message &inboundMessage,
		CstiSipCallSP sipCall);

	stiHResult ConnectedCallProcess (
		CstiSipCallLegSP callLeg,
		RvSipCallLegDirection eDirection);
	
	stiHResult DisconnectedCallProcess (
		RvSipCallLegHandle hCallLeg,
		CstiSipCallSP sipCall,
		RvSipCallLegStateChangeReason eReason);
	
	stiHResult TransportsGather (
		CstiSipCallLegSP callLeg);

	void Redirect (CstiCallSP call, RvSipMsgHandle hMsg, RvSipCallLegHandle hCallLeg);

	stiHResult AddSInfoToMessage (RvSipMsgHandle hOutboundMsg, CstiSipCallSP sipCall);

	stiHResult TransferDial (CstiSipCallSP originalSipCall, RvSipSubsHandle hSubscription, std::string host);
	void ContinueTransfer (CstiCallSP call);
	static stiHResult RegistrationCB (void *pUserData, CstiSipRegistration *pRegistration, EstiRegistrationEvent eEvent, size_t EventParam);
	stiHResult ProxyConnectionKeepaliveStart ();

	uint16_t LocalListenPortGet ();

	stiHResult UserAgentHeaderAdd (
		RvSipMsgHandle hOutboundMessage);
	
	stiHResult AddSLPToDynamicListenAddresses (
		RvSipTransportMgrHandle hTransportMgr,
		uint16_t un16SipListenPort);
		
	bool LocalShareContactSupportedGet ();

	stiHResult OptionsRequestProcess (
		const vpe::sip::Transaction &transaction);

	bool AudioProtocolAllowed (const std::string &protocol);
	


	stiHResult SIPResolverIPv4DNSServersSet ();

	static CstiSipCallLegSP callLegLookup (size_t callLegKey);
	static void callLegInsert (size_t callLegKey, CstiSipCallLegSP callLeg);

	void updateVrsCallInfo(CstiSipCallSP sipCall);

	// Radvision callback functions
	
#ifdef stiENABLE_TLS
	static void RVCALLCONV TransportConnectionTlsSequenceStartedCB (
		IN  RvSipTransportConnectionHandle            hConn,
		INOUT RvSipTransportConnectionAppHandle*   phAppConn);

	static RvStatus RVCALLCONV TransportConnectionTlsStateChangedCB (
		IN  RvSipTransportConnectionHandle hConnection,
		IN  RvSipTransportConnectionAppHandle hAppConnection,
		IN  RvSipTransportConnectionTlsState eState,
		IN  RvSipTransportConnectionStateChangedReason eReason);
	static void RVCALLCONV TransportConnectionTlsPostConnectionAssertionEvCB (
		IN  RvSipTransportConnectionHandle hConnection,
		IN  RvSipTransportConnectionAppHandle hAppConnection,
		IN  RvChar* strHostName,
		IN  RvSipMsgHandle hMsg,
		OUT RvBool* pbAsserted);
	static RvInt32 TransportVerifyCertificateEvCB (
		IN  RvInt32 prevError,
		IN  RvSipTransportTlsCertificate  certificate);
#endif
	static void RVCALLCONV CallLegCreatedCB (
		IN RvSipCallLegHandle hCallLeg,
		OUT RvSipAppCallLegHandle *phAppCallLeg);
	static void RVCALLCONV CallLegStateChangeCB (
		IN  RvSipCallLegHandle hCallLeg,
		IN  RvSipAppCallLegHandle hAppCallLeg,
		IN  RvSipCallLegState eState,
		IN  RvSipCallLegStateChangeReason eReason);
	static void RVCALLCONV AuthenticationLoginCB (
		IN RvSipAuthenticatorHandle hAuthenticator,
		IN RvSipAppAuthenticatorHandle hAppAuthenticator,
		IN void* hObject,
		IN void* peObjectType,
		IN RPOOL_Ptr *pRpoolRealm,
		OUT RPOOL_Ptr *pRpoolUserName,
		OUT RPOOL_Ptr *pRpoolPassword);
	static void RVCALLCONV AuthenticationMD5CB (
		IN RvSipAuthenticatorHandle hAuthenticator,
		IN RvSipAppAuthenticatorHandle hAppAuthenticator,
		IN RPOOL_Ptr *pRpoolMD5Input,
		IN RvUint32 length,
		OUT RPOOL_Ptr *pRpoolMD5Output);
	static void RVCALLCONV SipMgrPipeReadCB (
		IN RvInt fd,
		IN RvSipMidSelectEvent event,
		IN RvBool error,
		IN void* ctx);
	static RvStatus RVCALLCONV TransportParseErrorCB (
		IN RvSipTransportMgrHandle hTransportMgr,
		IN RvSipAppTransportMgrHandle hAppTransportMgr,
		IN RvSipMsgHandle hMsgReceived,
		OUT RvSipTransportBsAction *peAction);
	static void RVCALLCONV StackLogCB (
		IN void* context,
		IN RvSipLogFilters filter,
		IN const RvChar *formattedText);
	static void RVCALLCONV CallForkDetectedCB (
		IN RvSipCallLegHandle hNewCallLeg,
		OUT RvSipAppCallLegHandle *phNewAppCallLeg,
		OUT RvBool *pbTerminate);
	static RvStatus RVCALLCONV CallLegMessageCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipMsgHandle hMsg);
	static void RVCALLCONV CallLegTransactionStateChangeCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipTranscHandle hTransc,
		IN RvSipAppTranscHandle hAppTransc,
		IN RvSipCallLegTranscState eTranscState,
		IN RvSipTransactionStateChangeReason eReason);
	static void RVCALLCONV CallLegPrackStateCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipCallLegPrackState eState,
		IN RvSipCallLegStateChangeReason eReason,
		IN RvInt16 prackResponseCode);
	static void RVCALLCONV CallLegProvisionalResponseReceivedCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipTranscHandle hTransc,
		IN RvSipAppTranscHandle hAppTransc,
		IN RvSipCallLegInviteHandle hCallInvite,
		IN RvSipAppCallLegInviteHandle hAppCallInvite,
		IN RvSipMsgHandle hRcvdMsg);
	static void RVCALLCONV CallLegTransactionCreatedCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipTranscHandle hTransaction,
		OUT RvSipAppTranscHandle *hAppTransaction,
		OUT RvBool *bAppHandleTransaction);
	static void RVCALLCONV CallLegReInviteCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipCallLegInviteHandle hReInvite,
		IN RvSipAppCallLegInviteHandle hAppReInvite,
		IN RvSipCallLegModifyState eModifyState,
		IN RvSipCallLegStateChangeReason eReason);
	static void RVCALLCONV SubscriptionStateChangeCB (
		IN RvSipSubsHandle hSubscription,
		IN RvSipAppSubsHandle hAppSubs,
		IN RvSipSubsState eState,
		IN RvSipSubsStateChangeReason eReason);
	static void RVCALLCONV SubscriptionCreatedCB (
		RvSipSubsHandle hSubs,
		RvSipCallLegHandle hCallLeg,
		RvSipAppCallLegHandle hAppCallLeg,
		RvSipAppSubsHandle *phAppSubs);
	static void RVCALLCONV ReferNotifyReadyCB (
		IN RvSipSubsHandle hSubscription,
		IN RvSipAppSubsHandle hAppSubs,
		IN RvSipSubsReferNotifyReadyReason eReason,
		IN RvInt16 nResponseCode,
		IN RvSipMsgHandle hResponseMsg);
	static void RVCALLCONV SubscriptionNotifyCB (
		IN RvSipSubsHandle hSubs,
		IN RvSipAppSubsHandle hAppSubs,
		IN RvSipNotifyHandle hNotification,
		IN RvSipAppNotifyHandle hAppNotification,
		IN RvSipSubsNotifyStatus eNotifyStatus,
		IN RvSipSubsNotifyReason eNotifyReason,
		IN RvSipMsgHandle hNotifyMsg);
	static void RVCALLCONV TransactionCreatedCB (
		IN RvSipTranscHandle hTransaction,
		IN void *context,
		OUT RvSipTranscOwnerHandle *phAppTransaction,
		OUT RvBool *pbHandleTransaction);
	static RvStatus RVCALLCONV TransactionMessageSendCB (
		IN RvSipTranscHandle hTransaction,
		IN RvSipTranscOwnerHandle hTransactionOwner,
		IN RvSipMsgHandle hMsgToSend);
	static void RVCALLCONV TransactionStateChangedCB (
		IN RvSipTranscHandle hTransaction,
		IN RvSipTranscOwnerHandle hAppTransaction,
		IN RvSipTransactionState eState,
		IN RvSipTransactionStateChangeReason eReason);
	static RvStatus RVCALLCONV CallLegMessageSendCB (
		IN RvSipCallLegHandle hCallLeg,
		IN RvSipAppCallLegHandle hAppCallLeg,
		IN RvSipMsgHandle hMsgToSend);
	static RvStatus RVCALLCONV TCPConnectionStateCB (
		RvSipTransportConnectionHandle hConn,
		RvSipTransportConnectionOwnerHandle hObject,
		RvSipTransportConnectionState eState,
		RvSipTransportConnectionStateChangedReason eReason);
	static RvStatus TCPConnectionStatusCB (
		RvSipTransportConnectionHandle hConn,
		RvSipTransportConnectionOwnerHandle hOwner,
		RvSipTransportConnectionStatus eStatus,
		void *pInfo);
	static void RVCALLCONV ConnectionKeepaliveStateChangeCB (
		RvSipTransmitterHandle hTrx,
		RvSipAppTransmitterHandle hAppTrx,
		RvSipTransmitterState eState,
		RvSipTransmitterReason eReason,
		RvSipMsgHandle hMsg,
		void* pExtraInfo);
	static void RVCALLCONV NonceCountIncrementor (
		RvSipAuthenticatorHandle hAuthenticator,
		RvSipAppAuthenticatorHandle hAppAuthenticator,
		void *hObject,
		void *peObjectType,
		RvSipAuthenticationHeaderHandle hAuthenticationHeader,
		RvInt32 *pNonceCount);
	static RvStatus RVCALLCONV CallLegFinalDestResolvedCB (
		RvSipCallLegHandle hCallLeg,
		RvSipAppCallLegHandle hAppCallLeg,
		RvSipTranscHandle hTransc,
		RvSipAppTranscHandle hAppTransc,
		RvSipMsgHandle hMsgToSend);
	static void RVCALLCONV TransportConnectionServerReuseCB (
		RvSipTransportMgrHandle hTransportMgr,
		RvSipAppTransportMgrHandle hAppTransportMgr,
		RvSipTransportConnectionHandle hConn,
		RvSipTransportConnectionAppHandle hAppConn);
	static RvStatus RVCALLCONV NewConnectionCB (
		RvSipCallLegHandle hCallLeg,
		RvSipAppCallLegHandle hAppCallLeg,
		RvSipTransportConnectionHandle hConn,
		RvBool bNewConnCreated);

	static void RVCALLCONV TransportConnectionCreatedCB(
		RvSipTransportMgrHandle hTransportMgr,
		RvSipAppTransportMgrHandle hAppTransportMgr,
		RvSipTransportConnectionHandle hConn,
		RvSipTransportConnectionAppHandle *phAppConn,
		RvBool *pbDrop);

	// Event handlers
	void eventCallAnswer (CstiCallSP call);
	void eventCallDial (CstiCallSP call);
	void eventConferenceJoin (CstiCallSP currCall, CstiCallSP heldCall, std::string dialStr);
	void eventCallHangup (CstiCallSP call, SipResponse rejectCode);
	void eventCallObjectRemoved ();
	void eventCallResume (CstiCallSP call);
	void eventCallTransfer (CstiCallSP call, std::string dialString);
	void eventDsNameResolved (CstiCallSP call);
	void eventConferencePortsSet (const SstiSipConferencePorts &ports);
	void eventPhoneNumberLookup (CstiCallSP call, std::string phoneNumber);
	void eventRegister (std::string phoneNumber);
	void eventUnregister (std::string phoneNumber);
	void eventRegisterFailureRetry (std::string phoneNumber);

	void eventStackResourcesCheck ();
	void eventStackStartup ();
	void eventNetworkChangeNotify ();
	void eventIceGatheringTryingAnother (size_t callLegKey);

	void eventProxyWatchdog ();
	void eventConnectionKeepalive ();
	void eventResolverRetry (const CstiSipResolverSharedPtr &sipResolver);
	void eventNatTraversalConfigure ();

	void eventRegistrationCallback (
		std::string phoneNumber,
		EstiRegistrationEvent event,
		size_t eventParam);

	void eventReRegisterNeeded (bool activeCalls);
	void eventStackRestart ();

	void eventResolverCallback (
		const CstiSipResolverSharedPtr &sipResolver,
		EstiResolverEvent resolverEvent);

	void eventClientReregister ();
	void eventRegistrationStatusCheck (int stackRestartDelta);
	void eventPreferredDisplaySizeChanged ();
	void eventVrsFailoverCallDisconnect (CstiSipCallSP sipCall);
	void eventSignMailSkipToRecord (CstiCallSP call);
	
	void eventGeolocationChanged (const std::string& geolocation);
	void eventGeolocationClear ();

	const std::string publicIpGet ();

	// Not yet implemented
	stiHResult StateConnectedProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateConnectingProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateCriticalErrorProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateDisconnectingProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateDisconnectedProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateHoldLclProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateHoldRmtProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;
	stiHResult StateHoldBothProcess(CstiCallSP call, EsmiCallState eNewState, uint32_t un32NewSubState) override;


	friend class CstiSipResolver;
	friend class CstiSipCall;
	friend class CstiSipCallLeg;
	friend class vpe::sip::Message;

	bool newProxyDomainResolverRequired ();

	CstiSipResolverSharedPtr m_proxyDomainResolver;
	std::list <CstiSipResolverSharedPtr> m_PublicDomainResolvers;
	std::string m_LastResolvedPublicDomain;
	
	EstiTransport m_eLastNATTransport = estiTransportUnknown;
	bool m_bDisallowIncomingCalls = false;
	RvSipTransmitterHandle m_hKeepAliveTransmitter = nullptr;
	
	uint32_t m_un32StackStartupTime = 0;

	CstiSignalConnection m_preferredDisplaySizeListener;
	
	CstiSignalConnection m_geolocationChangedListener;
	CstiSignalConnection m_geolocationClearListener;
	std::string m_geolocation;

	// Because Radvision C API doesn't let us pass shared_ptrs through their
	// SIP/ICE stacks we pass a unique integer representing the call leg, which
	// can be used to lookup a weak pointer to a call leg instance.  This is
	// safer than passing around raw pointers because we won't ever get dangling
	// pointers if the object is deleted prematurely.  Instead we'll get a nullptr,
	// and crash with a SEGFAULT, making it easy to track down the problem.
	// These are static because we can't easily get a "this" pointer to access them
	// in the Radvision callbacks without first having a call leg instance.  It's a
	// chicken and egg problem.  The alternative was to pass around a heap-allocated
	// structure that has a pointer to the sipmanager.  The problem with that approach
	// is it's unclear how the entry and exit points match up (where you should delete it)
	// Some tools instantiate multiple SipManagers, so we also need to synchronize access.
	static std::map<size_t,CstiSipCallLegWP> m_callLegMap;
	static std::recursive_mutex m_callLegMapMutex;
	
	bool m_useProxyKeepAlive = true;
	std::atomic<bool> m_isAvailable;
	std::string m_vrsFocusedRouting;
	
	std::chrono::steady_clock::time_point m_lastKeepAliveTime = {};
	std::chrono::seconds m_timeBetweenKeepAlive = std::chrono::seconds::zero ();

	std::string m_corePublicIp;
};
