////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipCallLeg
//
//  File Name:  CstiSipCallLeg.h
//
//  Abstract:
//	See CstiSipCallLeg.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISIPCALLLEG_H
#define CSTISIPCALLLEG_H

#include "stiRtpChannelDefs.h"
#include "CstiIceManager.h"
#include "CstiSdp.h"
#include "DtlsContext.h"
#include "Sip/Message.h"
#include "Sip/Transaction.h"
#include <memory>
#include <list>
#include <map>
#include <chrono>

#include <boost/optional.hpp>

#define stiSDP_MAX_FMTP_SIZE 256
#define stiSDP_MAX_ATTR_SIZE 256

#define SORENSON_SIP_VERSION_STRING "SSV"

// A URI for Group Video Chat will look like
// sip:SVRS-GC-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx@address
// where all x is any hex character (0-9, a-f).  The GUID is that part of the
// string following "sip:" and ending just prior to the '@' character.
#define nMCU_GUID_START_POS_IN_URI 4
#define nMCU_GUID_LEN_IN_URI 44

// This value has no meaning other than it needs to be present and doesn't matter
// what number is used.  42 seemed to be the correct response.  :)
#define stiDUMMY_PAYLOAD_ID 42

enum EstiMediaDirection
{
	estiMDSend = 0,
	estiMDReceive = 1,
	estiMDInactive = 2
};

enum EstiOfferAnswerMessage
{
	estiOfferAnswerNone = -1,
	estiOfferAnswerINVITE = 0,
	estiOfferAnswerINVITEFinalResponse = 1,
	estiOfferAnswerACK = 2,
	estiOfferAnswerReliableResponse = 3,
	estiOfferAnswerPRACK = 4,
	estiOfferAnswerPRACKFinalResponse = 5,
	estiOfferAnswerUPDATE = 6,
	estiOfferAnswerUPDATEFinalResponse = 7
};

enum EstiSDPRole
{
	estiOffer,
	estiAnswer
};

enum EstiIceState
{
	estiUnknown = 0,
	estiFailed = 1,
	estiSucceeded = 2
};


const SstiMediaPayload *T140BasicMediaGet (
	const SstiSdpStream &mediaStream);

class CstiSipCall;
class CstiIceSession;
struct SstiPreferredMediaMap;


class CstiSipCallLeg : public std::enable_shared_from_this<CstiSipCallLeg>
{
public:

	CstiSipCallLeg () = default;

	CstiSipCallLeg (const CstiSipCallLeg &other) = delete;
	CstiSipCallLeg (CstiSipCallLeg &&other) = delete;
	CstiSipCallLeg &operator= (const CstiSipCallLeg &other) = delete;
	CstiSipCallLeg &operator= (CstiSipCallLeg &&other) = delete;

	~CstiSipCallLeg () = default;

	stiHResult SdpParse (
		const CstiSdp &Sdp,
		EstiSDPRole eSDPRole,
		bool *pbRestartICE,
		bool *pbCallInfoChanged);

	stiHResult SdpCreate (
		CstiSdp *pSdp,
		EstiSDPRole eSDPRole);

	stiHResult SdpResponseAdd ();

	stiHResult SdpHeaderAdd (
		CstiSdp *pSdp);

	stiHResult SdpMediaDescriptorAdd (
		const SstiSdpStream &sdpStream,
		const SstiSdpStream &localSdpStream,
		int nIndex,
		CstiSdp *pSdp,
		const std::string &RtpAddress,
		unsigned int unRtpPort,
		const std::string &RtcpAddress,
		unsigned int unRtcpPort,
		RvSdpConnectionMode eDirection,
		int nRate,
		EstiSDPRole role);

	// Various ways to query media streams/payloads

	// NOTE: preference types are a pointer since that argument is
	// often the result of a function call that could be null

	SstiSdpStreamP BestSdpStreamGet (
		EstiMediaType eMediaType,
		const std::list<SstiPreferredMediaMap> *pPreferenceTypes);

	std::tuple<SstiSdpStreamP, SstiMediaPayloadP> BestSdpStreamMediaGet (
		EstiMediaType eMediaType,
		const std::list<SstiPreferredMediaMap> *pPreferenceTypes
		);

	std::tuple<SstiSdpStreamP, boost::optional<int8_t>> BestSdpStreamIdGet (
		EstiMediaType eMediaType,
		const std::list<SstiPreferredMediaMap> *pPreferenceTypes
		);

	SstiMediaPayloadP BestStreamMediaGet (
		SstiSdpStream &sdpStream,
		const std::list<SstiPreferredMediaMap> *pPreferenceTypes);

	stiHResult UnsupportedStreamsDisable (
		CstiSdp *pSdpMessage);

	stiHResult SInfoHeaderRead (const vpe::sip::Message &inboundMessage);
	stiHResult SInfoHeaderRead (const CstiSdp &sdp);
	stiHResult SInfoHeaderRead (const RvSdpAttribute *pAttribute, bool *pbCallInfoChanged);

	stiHResult SInfoHeaderAdd ();

	void RemoteProductNameGet (
		std::string *pRemoteProductName) const;

	void RemoteProductNameSet (
		const char *pszRemoteProductName);

	void RemoteProductVersionGet (
		std::string *pRemoteProductVersion) const;

	void RemoteProductVersionSet (
		const char *pRemoteProductVersion);

	void RemoteSIPVersionSet (
		int nRemoteSIPVersion);

	void RemoteAutoSpeedSettingGet (
		EstiAutoSpeedMode *peAutoSpeedSetting) const;

	void RemoteAutoSpeedSettingSet (
		EstiAutoSpeedMode eRemoteAutoSpeedSetting);

	bool RemoteDeviceTypeIs (
		int nDeviceType) const;

	void remoteUANameSet(
		const vpe::sip::Message &inboundMessage);

	int RemoteUACallBandwidthGet () const
	{
		return m_nRemoteUACallBandwidth;
	}

	stiHResult AllowCapabilitiesDetermine (
		const vpe::sip::Message &message);

	bool IsTransferableGet () const
	{
		return m_bIsTransferable;
	}

	void IsTransferableSet (
		bool bIsTransferable)
	{
		m_bIsTransferable = bIsTransferable;
	}

	bool UpdateAllowedGet () const
	{
		return m_bUpdateAllowed;
	}

	void UpdateAllowedSet (
		bool bUpdateAllowed)
	{
		m_bUpdateAllowed = bUpdateAllowed;
	}

	bool ShareContactSupportedGet () const
	{
		return m_bShareContactSupported;
	}

	void ShareContactSupportedSet (
		bool bShareContactSupported)
	{
		m_bShareContactSupported = bShareContactSupported;
	}

	void RemoteDisplaySizeGet (
		unsigned int *displayWidth,
		unsigned int *displayHeight) const
	{
		if (displayWidth != nullptr)
		{
			*displayWidth = m_remoteDisplayWidth;
		}

		if (displayHeight != nullptr)
		{
			*displayHeight = m_remoteDisplayHeight;
		}
	}

	void RemoteDisplaySizeSet (
		unsigned int displayWidth,
		unsigned int displayHeight)
	{
		m_remoteDisplayWidth = displayWidth;
		m_remoteDisplayHeight = displayHeight;
	}

	stiHResult SdpProcess (
		CstiSdp *pSdp,
		EstiOfferAnswerMessage eMessage);

	stiHResult SdpOfferSend ();

	stiHResult SdpOfferProcess (
		CstiSdp *pSdp,
		EstiOfferAnswerMessage eMessage);

	stiHResult SdpAnswerProcess (
		const CstiSdp &Sdp,
		EstiOfferAnswerMessage eMessage);

	void MediaServerDisconnect ();

	stiHResult IceGatheringComplete ();

	stiHResult IceNominationsComplete (
		bool bSucceeded);

	stiHResult ConfigureMediaChannels (
		bool bIsReply);

	stiHResult MediaStreamNominationsSet (
		EstiMediaType eMediaType, ///< The type of media (audio, text, or video) to search for
		const CstiMediaAddresses &mediaAddresses);

	stiHResult IceNominationsApply ();

	void ReliableResponseSend (
		uint16_t un16SipResponse, ///< The provisional response status code.
		bool *pbSentReliably);

	stiHResult RemoteDisplaySizeProcess (const std::string& remoteDisplaySize);

	bool rtcpFbAttrParse (
		RvSdpAttribute *attribute,
		RvSdpMediaDescr *mediaDescriptor,
		std::map<int8_t, std::list<RtcpFeedbackType>> *feedbackMap,
		EstiSDPRole role);

	size_t uniqueKeyGet () const { return m_uniqueKey; }
	void uniqueKeySet (size_t key) { m_uniqueKey = key; }
	static size_t uniqueKeyGenerate ();

	bool IsBridgedCall ();

	void ConferencedSet (bool conferenced);
	bool ConferencedGet () const;
	
	void callLoggedSet (bool logged);
	bool callLoggedGet () const;
	
	void connectingTimeStart ();
	void connectingTimeStop ();
	std::chrono::milliseconds connectingDurationGet () const;
	
	void connectedTimeStart ();
	void connectedTimeStop ();
	std::chrono::milliseconds  connectedDurationGet () const;
	
	std::chrono::seconds SecondsSinceCallEndedGet () const;
	
	EstiICENominationsResult iceNominationResultGet ();
	
	int rportGet () const;
	void rPortSet (int rPort);
	
	std::string receivedAddrGet () const;
	void receivedAddrSet (const std::string& receivedAddr);

	void localSdpStreamsClear ();

	bool localSdpStreamsAreEmpty () const;

	const SstiSdpStreamUP& localSdpStreamGet (
		EstiMediaType mediaType);

	static std::vector<vpe::SDESKey::Suite> supportedSDESKeySuitesGet();

	static std::vector<vpe::SDESKey> SDESKeysGenerate(const std::vector<vpe::SDESKey::Suite> &suites = supportedSDESKeySuitesGet());

	void localStreamsFromRemoteStreamsUpdate();

	bool selectSDESKeySuite(const SstiSdpStream &sdpEntry, vpe::SDESKey::Suite &suite);

	static bool SDESMediaKeysGet(const SstiSdpStream& remoteStream, const SstiSdpStream& localStream, vpe::SDESKey& encryptKey, vpe::SDESKey& decryptKey);

	void dtlsParametersAdd(RvSdpMediaDescr* sdpMediaDescriptor, const SstiSdpStream& sdpStream);

	bool selectDtls(const SstiSdpStream& sdpEntry);

	bool encryptionRequiredGet ();

	std::shared_ptr<vpe::DtlsContext> dtlsContextGet();
	void dtlsContextSet(std::shared_ptr<vpe::DtlsContext> dtlsContext);

	bool isDtlsNegotiated ();

	CstiSipCallWP m_sipCallWeak;
	RvSipCallLegHandle m_hCallLeg = nullptr;
	CstiSdp m_sdp;
	bool m_bSInfoSent = false;
	bool m_bSInfoRecvd = false;
	std::string m_SInfo;
	CstiIceSession *m_pIceSession = nullptr;
	bool m_bRemoteIsICE = false;
	IceSupport m_remoteIceSupport = IceSupport::Unknown;
	bool m_bAnswerPending = false;
	bool m_bSendNewOfferInPRACK = false;
	bool m_bOfferAnswerComplete = false;
	bool m_bSendOfferWhenReady = false;
	bool m_bUpdateRequestInProgress = false;
	bool m_bPendingOfferUpdate = false;

	bool m_bWaitingForPrackCompletion = false;
	bool m_bCallLegAccepted = false;

	bool m_firOffered = false;
	bool m_pliOffered = false;
	bool m_tmmbrOffered = false;
	bool m_nackOffered = false;
	bool m_sendNewOfferToSorenson = false;

	std::list<RvSipTranscHandle> m_UpdateRequests;

	// these model m-lines, and are populated by parsing the sdp of the far side
	// Offer or answer...
	// TODO: seems like a lot of duplicated information between
	// PayloadAttribute derived classes and SstiPreferredMediaMap
	std::list<SstiSdpStreamUP> m_SdpStreams;

	// This is to partially model our sdp offers
	// (otherwise, we only parse their end, and expect that it is valid.  We would
	//  have no way of knowing unless we compared their sdp message with our raw sdp message)
	// NOTE: right now, this is only used for SDES keys
	std::list<SstiSdpStreamUP> m_localSdpStreams;

	uint32_t m_un32SdpVersionId = 0;

	SstiIceSdpAttributes m_IceSdpAttributes;

	std::string m_RemoteProductName;
	std::string m_RemoteProductVersion;
	bool m_bIsTransferable = false; // Is this call transferable?  (depends on the remote system).
	bool m_bUpdateAllowed = false;
	bool m_bShareContactSupported = false;
	int m_nRemoteSIPVersion = 1;
	int m_nRemoteUACallBandwidth = 0;
	EstiAutoSpeedMode m_eRemoteAutoSpeedSetting = estiAUTO_SPEED_MODE_LEGACY;

	EstiOfferAnswerMessage m_eNextICEMessage = estiOfferAnswerNone;
	EstiIceState m_eCallLegIceState = estiUnknown;

	unsigned int m_remoteDisplayWidth = 0;
	unsigned int m_remoteDisplayHeight = 0;

	// This key is used to lookup the CstiSipCallLeg instance in a map within SipManager
	// which helps us pass call legs through the Radvision stack without passing around raw pointers
	size_t m_uniqueKey = 0;

	// Unique keys are obtained by incrementing this number
	static size_t m_uniqueKeyIncrementor;
	static std::recursive_mutex m_uniqueKeyIncrementorMutex;
	
	void vrsFocusedRoutingSentSet (const std::string &vrsFocusedRoutingSent)
	{
		m_vrsFocusedRoutingSent = vrsFocusedRoutingSent;
	}
	
	std::string vrsFocusedRoutingSentGet ()
	{
		return m_vrsFocusedRoutingSent;
	}
	
	void vrsFocusedRoutingReceivedSet (const std::string &vrsFocusedRoutingReceived)
	{
		m_vrsFocusedRoutingReceived = vrsFocusedRoutingReceived;
	}
	
	std::string vrsFocusedRoutingReceivedGet ()
	{
		return m_vrsFocusedRoutingReceived;
	}

	stiHResult toHeaderRemoteNameGet (
		std::string *RemoteName,
		bool bSvrsPrefixRequired);
	
	stiHResult encryptionAttributesAdd (RvSdpMediaDescr *sdpMediaDescr, EstiMediaType mediaType);

	vpe::EncryptionState encryptionStateVideoGet () const;
	vpe::EncryptionState encryptionStateAudioGet () const;
	vpe::EncryptionState encryptionStateTextGet () const;

	vpe::KeyExchangeMethod keyExchangeMethodVideoGet () const;
	vpe::KeyExchangeMethod keyExchangeMethodAudioGet () const;
	vpe::KeyExchangeMethod keyExchangeMethodTextGet () const;

	std::string encryptionChangesVideoToJsonArrayGet ();
	std::string encryptionChangesAudioToJsonArrayGet ();
	std::string encryptionChangesTextToJsonArrayGet ();

	vpe::sip::Message outboundMessageGet()
	{
		return 	vpe::sip::Message(shared_from_this(), true);
	}

	vpe::sip::Message inboundMessageGet()
	{
		return 	vpe::sip::Message(shared_from_this(), false);
	}

	vpe::sip::Transaction createTransaction()
	{
		return vpe::sip::Transaction(shared_from_this());
	}

private:

	void fallbackVcoSet(bool sdpHasSinfo, bool audioSupported, bool * callInfoChanged);
	
	bool m_conferenced = false;
	bool m_callLogged = false;
	
	int m_rPort = UNDEFINED;
	std::string m_receivedAddr;
	
	std::chrono::steady_clock::time_point m_connectingStartTime = {};
	std::chrono::steady_clock::time_point m_connectingStopTime = {};
	std::chrono::steady_clock::time_point m_connectedStartTime = {};
	std::chrono::steady_clock::time_point m_connectedStopTime = {};
	
	std::string m_vrsFocusedRoutingSent;
	std::string m_vrsFocusedRoutingReceived;
	std::shared_ptr<vpe::DtlsContext> m_dtlsContext;

	static std::string encryptionChangesToJsonGet (const std::list<std::tuple<Poco::Timestamp, int>> &changeList);
};


stiHResult SDESKeysAdd(
	RvSdpMediaDescr * mediaDescriptor,
	const std::vector<vpe::SDESKey> &keys);


#endif // CSTISIPCALLLEG_H
// end file CstiSipCallLeg.h
