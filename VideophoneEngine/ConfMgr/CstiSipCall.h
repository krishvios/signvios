////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipCall
//
//  File Name:  CstiSipCall.h
//
//  Abstract:
//	See CstiSipCall.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

//
// Forward Declarations
//

//
// Includes
//
#include "stiSipConstants.h"
#include "stiSVX.h"
#include "stiSystemInfo.h"
#include "RvSipCallLeg.h"
#include "RvSipStackTypes.h"
#include "rvsdpmsg.h"
#include "CstiProtocolCall.h"
#include "CstiSipManager.h"
#include "CstiSipCallLeg.h"
#include "CstiTimer.h"
#include <map>
#include <list>
#include <string>

#include "CstiDataTaskCommonP.h"
#include "CstiAudioBridge.h"
#include "CstiRtpSession.h"
#include "MediaStream.h"

#include "SDESKey.h"

//
// Constants
//
#define MAX_SESSIONID_DIGITS 10

extern const char g_szHearingStatusConnecting[];
extern const char g_szHearingStatusConnected[];
extern const char g_szHearingStatusDisconnected[];

extern const std::string g_dhviSwitch;
extern const std::string g_dhviCapabilityCheck;
extern const std::string g_dhviCapable;
extern const std::string g_dhviCreate;
extern const std::string g_dhviDisconnect;

//
// Typedefs
//

//
// Forward Declarations
//
struct SstiPreferredMediaMap;

//
// Globals
//
stiHResult ContactSerialize (const SstiSharedContact &contact, std::string *pResultBuffer);
stiHResult ContactToText (const SstiSharedContact &contact, std::string *pResultBuffer);
stiHResult ContactDeserialize (const char *pszBuffer, SstiSharedContact *contact);


//
// Class Declaration
//
class CstiSipCall : public CstiProtocolCall, public std::enable_shared_from_this<CstiSipCall>
{
public:

	CstiSipCall (
		const SstiConferenceParams *pstConferenceParams,
		CstiSipManager *sipManager,
		const std::string &vrsFocusedRouting,
		CstiSipCallSP bridgedCall = nullptr,
		CstiSipCallSP dhviCall = nullptr);

	CstiSipCall (const CstiSipCall &other) = delete;
	CstiSipCall (CstiSipCall &&other) = delete;
	CstiSipCall &operator= (const CstiSipCall &other) = delete;
	CstiSipCall &operator= (CstiSipCall &&other) = delete;

	~CstiSipCall () override;
	
	stiHResult Accept () override;
	
	void CalledNameRetrieve (
		RvSipMsgHandle hInboundMsg);
	
	void CallEnd (EstiSubState nNewSubState, SipResponse sipRejectCode);
	
	CstiSipCallLegSP CallLegAdd (
		RvSipCallLegHandle hCallLeg);
	
	CstiSipCallLegSP CallLegGet (
		RvSipCallLegHandle hCallLeg) const;

	void IceSessionsEnd ();
	
	bool CallValidGet ()
	{
		return m_bCallValid;
	}
	
	void CallValidSet (bool bValid = true)
		{ m_bCallValid = bValid; }
	
	stiHResult RemoteDisplayNameDetermine (
		RvSipCallLegHandle hCallLeg);

	stiHResult HangUp () override { return HangUp (SIPRESP_NONE); }
	stiHResult HangUp (SipResponse sipResponseCode);
	stiHResult Hold (bool bHoldState = true);
	void InboundCallTimerStart ();
	void InviteInsideOfReInviteCheck ();
	inline bool IsHeld ()
	{
		auto call = m_poCstiCall.lock ();
		return call ? (call->StateValidate (esmiCS_HOLD_BOTH | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT) == estiTRUE) : false;
	}

	stiHResult mediaUpdate (bool isReply);

	stiHResult MediaClose ();
	
	stiHResult IncomingAudioInitialize ();
	stiHResult IncomingAudioUpdate ();
	
	stiHResult IncomingTextInitialize ();
	stiHResult IncomingTextUpdate ();
	
	stiHResult IncomingVideoInitialize ();
	stiHResult IncomingVideoUpdate ();
	
	stiHResult IncomingMediaInitialize ();
	stiHResult IncomingMediaUpdate ();

	stiHResult OutgoingMediaOpen (
		bool bIsReply);

	inline EstiProtocol ProtocolGet () override
		{ return estiSIP; }

	CstiProtocolManager *ProtocolManagerGet () override
		{ return m_poSipManager; }
	
	inline bool ProtocolModifiedGet () override
		{ return m_bAddSipToH323Header;}
	
	stiHResult ReInviteSend (bool bLostConnectionDetection, int nCurrentRetryCount);
	inline stiHResult Reject (SipResponse sipResponseCode)
	{
		return HangUp (sipResponseCode);
	}
	
	inline RvSipCallLegHandle StackCallHandleGet ()
	{
		return m_hCallLeg;
	}
	
	void StackCallHandleSet (RvSipCallLegHandle hCallLeg);
	inline CstiSipManager *SipManagerGet ()
		{ return m_poSipManager; }
	void SipManagerSet (CstiSipManager *poSipManager)
		{ m_poSipManager = poSipManager; }
		
	stiHResult SdpAudioPayloadsAdd (
		RvSdpMediaDescr *pMediaDescriptor,
		bool streamEnabled);

	stiHResult SdpTextPayloadsAdd (
		RvSdpMediaDescr *pMediaDescriptor,
		bool streamEnabled);

	stiHResult SdpVideoPayloadsAdd (
		RvSdpMediaDescr *pMediaDescriptor,
		bool streamEnabled,
		EstiSDPRole role);

	stiHResult InitialSdpOfferCreate (
 		CstiSdp *pSdp,
		bool bCreateChannels);
	
	stiHResult SorensonCustomInfoParse (
		RvSipCallLegHandle hCallLeg,
		RvSipMsgHandle hMsg, 
		bool bIsReply);
	
	stiHResult SorensonCustomInfoSend (
		RvSipCallLegHandle hCallLeg,
		EstiSorensonMessage eMessage,
		unsigned int nValue = 0,
		RvSipMsgHandle hAddToMsg = nullptr);
	
	RvSipSubsHandle SubscriptionGet ()
		{ return m_hSubscription; }
	void SubscriptionSet (RvSipSubsHandle hSubscription)
		{ m_hSubscription = hSubscription; }
	EstiBool TextSupportedGet () const override;
	void RemoteTextSupportedSet (EstiBool bRemoteTextSupported) override;	
	EstiBool DtmfSupportedGet () const override;
	stiHResult TransferInfoSet (const std::string &destAddress, EstiDirection eDirection);
	EstiDirection TransferDirectionGet () const
	{
		return m_eTransferDirection;
	}
	stiHResult TransferToAddress (const std::string &destAddress) override;
	
	stiHResult BridgeCreate (const std::string &destAddress);
	stiHResult BridgeDisconnect ();
	stiHResult BridgeStateGet (EsmiCallState *peCallState);
	stiHResult BridgeRemoved ();
	bool IsBridgedCall () { return m_bIsBridgedCall; }
	
	stiHResult dhviCreate (const std::string &destAddress, const std::string &conferenceId, bool sendInfo);
	stiHResult dhviMcuDisconnect (bool sendReinvite);
	
	void RemoteTransportAddressesSet (
		CstiMediaTransports *audioTransports,
		CstiMediaTransports *textTransports,
		CstiMediaTransports *videoTransports);

	void RemoteTransportAddressesSet (
		bool inboundMedia,
		EstiMediaType mediaType,
		const std::list<SstiPreferredMediaMap> &preferredMedia,
		CstiMediaTransports *transports);

	void IncomingTransportsSet (
		const CstiMediaTransports &audioTransports,
		const CstiMediaTransports &textTransports,
		const CstiMediaTransports &videoTransports);
	
	void DefaultAddressesSet (
		const RvAddress *pAudioRtpAddress,
		const RvAddress *pAudioRtcpAddress,
		const RvAddress *pTextRtpAddress,
		const RvAddress *pTextRtcpAddress,
		const RvAddress *pVideoRtpAddress,
		const RvAddress *pVideoRtcpAddress);
	
	stiHResult DefaultTransportsApply (
		CstiSipCallLegSP callLeg);

	int RemotePtzCapsGet () const override;
	EstiBool RemoteDeviceTypeIs (int nDeviceType) const override;
	stiHResult KeyframeSend (EstiBool bWasRequestedByRemote = estiFALSE);
	stiHResult RemoteLightRingFlash ();
	stiHResult TextSend (const uint16_t *pwszText, EstiSharedTextSource sharedTextSource) override;
	stiHResult SInfoLoad (const char *pszIncomingCallTag);
	void HoldStateComplete (bool bLocal);
	void LostConnectionTimerCancel ();
	void ReinviteTimerCancel ();
	void ReinviteCancelTimerCancel ();
	bool DetectingLostConnectionGet ();

#ifdef stiHOLDSERVER
	virtual stiHResult HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState);
#endif

	stiHResult VideoFlowControlSendHandle (
		std::shared_ptr<CstiVideoPlayback> videoPlayback,
		int nNewRate);

	void AlertUserCountDownSet (
		int nCount);
	
	void AlertUserCountDownDecr ();
	
	void AlertLocalUserSet (
		bool bAlertLocalUser);

	bool AlertLocalUserGet ();

	void AlertLocalUser ();

	void CallIdGet (std::string *pCallId) override;
	virtual void CallIdSet (const char *pCallId);

	void OriginalCallIDGet (std::string *pCallID) override;

	stiHResult ForkedCallLegsTerminate ();

	bool AllLegsFailedICENominations ();

	void SInfoSet (
		const std::string &systemInfo)
	{
		m_SInfo = systemInfo;
	}

	void RemoteProductNameGet (
		std::string *pRemoteProductName) const override;

	void RemoteProductVersionGet (
		std::string *pRemoteProductVersion) const override;
	
	void RemoteAutoSpeedSettingGet (
		EstiAutoSpeedMode *peAutoSpeedSetting) const override;

	EstiBool IsTransferableGet () const override;
	
	virtual stiHResult ContactShare (const SstiSharedContact &contact);
		
	virtual bool ShareContactSupportedGet ();
	
	stiHResult PreferredDisplaySizeSend ();
	
	stiHResult GeolocationSend ();

//!\todo: make these new data members private
	EstiTransport m_eTransportProtocol = estiTransportUnknown;
	
	bool m_bProxyLoginAttempted = false;
	void TextShareRemoveRemote() override;
	
	friend class CstiSipCallLeg;
	friend class CstiSipManager;
	
	void HearingStatusSend (EstiHearingCallStatus eHearingStatus);
	void NewCallReadyStatusSend (bool bNewCallReadyStatus);
	void SpawnCallSend (const SstiSharedContact &spawnCallInfo);
	
	bool NewCallReadyStatusGet ()
	{
		return m_bRelayNewCallReady;
	}
	
	EstiHearingCallStatus HearingStatusGet ()
	{
		return m_eHearingCallStatus;
	}
	
	stiHResult VRSFailOverForce ();
	stiHResult VRSFailOverConfigure ();
	
	stiHResult SignMailSkipToRecord() override;
	
	bool UseIceForCalls ();
	
	void ConferencedSet (bool conferenced) override;
	bool ConferencedGet () const override;
	
	void connectingTimeStart () override;
	void connectingTimeStop () override;
	std::chrono::milliseconds connectingDurationGet () const override;
	
	void connectedTimeStart () override;
	void connectedTimeStop () override;
	std::chrono::milliseconds  connectedDurationGet () const override;
	
	std::chrono::seconds secondsSinceCallEndGet () const override;
	
	stiHResult switchActiveCall (bool sendReinvite);
	void sendDhviCreateMsg (const std::string &destAddress);
	void sendDhviSwitchMsg ();
	void sendDhviCapabilityCheckMsg (const std::string &hearingNumber);
	void sendDhviCapableMsg (bool capable);
	void sendDhviDisconnectMsg ();
	void logDhviDeafDisconnect ();

	// Event handlers (run from SipMgr thread)
	void eventVideoPlaybackPrivacyModeChanged (bool muted);
	void eventVideoPlaybackKeyframeRequest ();
	void eventVideoRecordPrivacyModeChanged (bool muted);
	void eventHSVideoFileCompleted ();
	void eventAudioRecordPrivacyModeChanged (bool muted);
	void eventTextPlaybackRemoteTextReceived (const uint16_t *textBuffer);
	void eventTextRecordPrivacyModeChanged (bool muted);
	
	std::string vrsFocusedRoutingSentGet ()
	{
		return m_vrsFocusedRoutingSent;
	}
	
	void switchedActiveCallsSet (bool switchedCalls)
	{
		m_switchedActiveCalls = switchedCalls;
	}
	
	bool switchedActiveCallsGet ()
	{
		return m_switchedActiveCalls;
	}
	
	void dhvSwitchAfterHandoffSet (bool dhvSwitchAfterHandoff)
	{
		m_dhvSwitchAfterHandoff = dhvSwitchAfterHandoff;
	}
	
	bool dhvSwitchAfterHandoffGet ()
	{
		return m_dhvSwitchAfterHandoff;
	}
	
	bool isDhvCall ()
	{
		return (m_dhviCall != nullptr);
	}

	CstiCallSP dhvCstiCallGet () const
	{
		return m_dhviCall;
	}

	void bridgeUriSet (const std::string &uri)
	{
		m_BridgeURI = uri;
	}

	vpe::EncryptionState encryptionStateGet () const;
	
	std::string localIpAddressGet () const;

	void dtlsBegin(bool isReply);
	
	bool disableDtlsGet () const;

private:
	void ConnectionKeepaliveStart();
	void ConnectionKeepaliveStop();

	void createRtpSessions ();

	stiHResult openRtpSession (
	    std::shared_ptr<vpe::RtpSession> session);

	void rtcpFeedbackSupportUpdate (
		const std::list<RtcpFeedbackType> &rtcpFeedbackTypes);

	bool includeEncryptionAttributes ();

	void privacyModeChange (
	    std::shared_ptr<CstiDataTaskCommonP> dataTask,
	    bool muted,
	    EstiSwitch *privacyMode);

	void audioRecordOpen (
		const SstiSdpStream &bestSdpStream,
		const SstiMediaPayload &bestMediaPayload);

	void videoRecordOpen (
		const SstiSdpStream &bestSdpStream,
		const SstiMediaPayload &bestMediaPayload);

	void textRecordOpen (
		const SstiSdpStream &bestSdpStream,
		const SstiMediaPayload &bestMediaPayload);

	stiHResult sdpVideoRtxPayloadsAdd (
		int payloadType,
		RvSdpMediaDescr *mediaDescriptor);
	
	void LostConnectionCheck (bool bDisconnectTimerStart) override;
	void lostConnectionCheckNeededSet (bool lostConnectionCheckNeeded);
	bool lostConnectionCheckNeededGet () const;

private:
	RvSipCallLegHandle m_hCallLeg = nullptr; ///< handle to the call-leg
	CstiSipManager *m_poSipManager = nullptr;

	// Shared rtp/rtcp/srtp sessions
	std::shared_ptr<vpe::RtpSession> m_audioSession;
	std::shared_ptr<vpe::RtpSession> m_textSession;
	std::shared_ptr<vpe::RtpSession> m_videoSession;

	TAudioPayloadMap m_mAudioPayloadMappings;
	TTextPayloadMap m_mTextPayloadMappings;
	TVideoPayloadMap m_mVideoPayloadMappings;

	MediaStream m_audioStream;
	MediaStream m_videoStream;
	MediaStream m_textStream;

	CstiTimer m_periodicKeyframeTimer;
	CstiTimer m_inboundCallTimer;
	CstiTimer m_privacyUpdateTimer;
	CstiTimer m_lostConnectionTimer;
	CstiTimer m_reinviteTimer;
	CstiTimer m_reinviteCancelTimer;
	CstiTimer m_connectionKeepaliveTimer;

	CstiSignalConnection::Collection m_signalConnections;

	bool m_bDetectingLostConnection = false;
	bool m_lostConnectionCheckNeeded {false};

	bool m_bCallValid = true;
	std::string m_transferringTo;
	EstiDirection m_eTransferDirection = estiUNKNOWN;
	EstiSwitch m_videoPrivacyMode = estiOFF;
	EstiSwitch m_textPrivacyMode = estiOFF;
	unsigned int m_unLastPrivacyChangeTime = 0;  // TODO:  use chrono
	RvSipSubsHandle m_hSubscription = nullptr;
	EstiBool m_bRemoteTextSupported = estiFALSE;

	std::map<RvSipCallLegHandle, CstiSipCallLegSP> m_CallLeg;
	int m_nAlertUserCountDown = 0;
	bool m_bAlertLocalUser = false;
	bool m_bLocalUserAlerted = false;

	RvAddress m_DefaultAudioRtpAddress{};
	RvAddress m_DefaultAudioRtcpAddress{};
	RvAddress m_DefaultTextRtpAddress{};
	RvAddress m_DefaultTextRtcpAddress{};
	RvAddress m_DefaultVideoRtpAddress{};
	RvAddress m_DefaultVideoRtcpAddress{};
	
	static uint32_t m_un32SdpSessionId;
	char m_szSessionId[MAX_SESSIONID_DIGITS + 1]{};

	std::string m_SInfo;	// Holds a copy of the SInfo string
	std::string m_CallId;
	std::string m_OriginalCallID; // For transferred calls this holds the original call leg
	EstiIpAddressType m_eIpAddressType = estiTYPE_IPV4;
	
	std::list<SstiPreferredMediaMap> m_PreferredVideoPlaybackProtocols;
	std::list<SstiPreferredMediaMap> m_PreferredVideoRecordProtocols;
	std::list<SstiPreferredMediaMap> m_PreferredAudioProtocols;
	std::list<SstiPreferredMediaMap> m_AudioFeatureProtocols;
	std::list<SstiPreferredMediaMap> m_PreferredTextProtocols;

	void AudioBridgeOn ();

	//
	// Audio Payload Mapping methods
	//
	void AudioPayloadMappingAdd (
		int8_t n8PayloadTypeNbr,
		EstiAudioCodec eAudioCodec,
		EstiPacketizationScheme ePacketizationScheme,
		uint32_t clockRate);

	void AudioPayloadMapGet (
		EstiAudioCodec eAudioCodec,
		EstiPacketizationScheme ePacketizationScheme,
		uint32_t clockRate,
		int *pnPayloadTypeNbr);

	void AudioPayloadMappingsClear ();

	bool AudioPayloadIDUsed (
		int nPayloadTypeNbr) const;

	stiHResult AudioPayloadDynamicIDGet (
		int *pnPayloadTypeNbr);

	//
	// Text Payload Mapping methods
	//
	void TextPayloadMappingAdd (
		int8_t n8PayloadTypeNbr,
		EstiTextCodec eTextCodec,
		EstiPacketizationScheme ePacketizationScheme);

	void TextPayloadMapGet (
		EstiTextCodec eTextCodec,
		EstiPacketizationScheme ePacketizationScheme,
		int *pnPayloadTypeNbr);

	void TextPayloadMappingsClear ();

	bool TextPayloadIDUsed (
		int nPayloadTypeNbr) const;

	stiHResult TextPayloadDynamicIDGet (
		int *pnPayloadTypeNbr);

	//
	// Video Payload Mapping methods
	//
	void VideoPayloadMappingAdd (
		int8_t n8PayloadTypeNbr,
		EstiVideoCodec eVideoCodec,
		EstiVideoProfile eProfile,
		EstiPacketizationScheme ePacketizationScheme,
		std::list<RtcpFeedbackType> rtcpFeedbackTypes);

	void VideoPayloadMapGet (
		EstiVideoCodec eVideoCodec,
		EstiVideoProfile eProfile,
		EstiPacketizationScheme ePacketizationScheme,
		int *pnPayloadTypeNbr);

	void VideoPayloadMappingsClear ();

	bool VideoPayloadIDUsed (
		int nPayloadTypeNbr) const;

	stiHResult VideoPayloadDynamicIDGet (
		int *pnPayloadTypeNbr);

	void BandwidthReset ();
	inline bool HoldStateGet (bool bLocal)
	{
		auto call = m_poCstiCall.lock ();
		uint32_t flags = esmiCS_HOLD_BOTH | (bLocal ? esmiCS_HOLD_LCL : esmiCS_HOLD_RMT);
		return call ? call->StateValidate (flags) == estiTRUE : false;
	}
	bool HoldStateSet (bool bLocal, bool bChangeTo);

	stiHResult RtxVideoPayloadsSet (
		std::string fmtp);

	void SdpFmtpValuesGet (
		std::map<std::string, std::string> *mpRetVal,
		const RvSdpMediaDescr *pMedia,
		int nPayloadID,
		ECodec eSorensonCodec);

	stiHResult SdpMessageBodyAdd ();
	stiHResult ReferSend ();

	void FlowControlSend (
		std::shared_ptr<CstiVideoPlayback> videoPlayback, int nNewRate) override;
	void VideoRecordDataStart () override;

	stiHResult MediaOfferCreate (
		std::list<SstiPreferredMediaMap> *pPreference,
		RvSdpMediaType eType);

	void transportsCallTrace (
		const char* tag,
		const CstiMediaTransports* audioTransports,
		const CstiMediaTransports* textTransports,
		const CstiMediaTransports* videoTransports);

	const std::list<SstiPreferredMediaMap> *PreferredAudioProtocolsGet () const;
	const std::list<SstiPreferredMediaMap> *AudioFeatureProtocolsGet () const;
	const std::list<SstiPreferredMediaMap> *PreferredTextRecordProtocolsGet () const;
	const std::list<SstiPreferredMediaMap> *PreferredTextPlaybackProtocolsGet () const;
	const std::list<SstiPreferredMediaMap> *PreferredVideoRecordProtocolsGet () const;
	const std::list<SstiPreferredMediaMap> *PreferredVideoPlaybackProtocolsGet () const;


	// Call Bridge support
	CstiCallSP m_spCallBridge; ///< Call to bridge this call's audio with
	bool m_bIsBridgedCall = false; ///< true if this is the bridge part of the call
	std::shared_ptr<CstiAudioBridge> m_poAudioPlaybackBridge = nullptr; ///< Connects our AudioPlayback to the bridged call's AudioRecord
	
	// DHVI Support
	CstiCallSP m_dhviCall;
	bool m_isDhviMcuCall = false;/// < Indicates that this is the MCU call for the DHVI call pair.

	RvSipTransmitterHandle m_hKeepAliveTransmitter = nullptr;
	
	bool m_bAddSipToH323Header = false; // Used to signal if we need to convert the call from SIP to H323
	bool m_bGatewayCall = false; // Signals if this call is using the gateway to be converted from H323 to SIP

	bool m_bRelayNewCallReady = false; // Used to signal if the relay phone is capable of receiving a new call
	EstiHearingCallStatus m_eHearingCallStatus = estiHearingCallDisconnected; // Used to represent the state of the hearing party
	
	stiHResult SipInfoSend (
		const std::string &pszContentHeader,
		const std::string &pszHeaderName,
		const std::string &pszHeaderValue,
		const std::string &msgBody);

	std::string m_BridgeURI;
	
	bool m_useVRSFailoverServer = false;
	
	std::string m_vrsFocusedRoutingSent;
	
	bool m_switchedActiveCalls {false};
	bool m_dhvSwitchAfterHandoff {false};

	std::list<std::tuple<Poco::Timestamp, int>> m_encryptionStateChangesVideo;
	std::list<std::tuple<Poco::Timestamp, int>> m_encryptionStateChangesAudio;
	std::list<std::tuple<Poco::Timestamp, int>> m_encryptionStateChangesText;

	bool m_sendSipKeepAlives {true};

};  // End class CstiSipCall declaration
