////////////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  stiConfDefs.h
//  Created on:      23-Nov-2009
////////////////////////////////////////////////////////////////////////////////////////
#ifndef STICONFDEFS_H
#define STICONFDEFS_H

#include "stiSVX.h"
#include "CstiServerInfo.h"
#include <map>
#include <string>
#include <vector>


const int nDEFAULT_SIP_LISTEN_PORT = STANDARD_SIP_PORT;
const int nDEFAULT_TLS_SIP_LISTEN_PORT = STANDARD_SIP_TLS_PORT;

enum EstiTransport
{
	estiTransportUnknown = 0,
	estiTransportTCP = (1 << 0),
	estiTransportTLS = (1 << 1),
	estiTransportUDP = (1 << 2),
};

//
// This value is used to indicate any of the transports are valid.  The value can be stored in
// the SipNatTransport setting and is used mainliy by CstiSipResolver to determine
// what transports it is allowed to consider.  The value is a bit mask and it is set
// to all one bits even though, currently, only the three lower bits are used (TCP, TLS and UDP).
//
#define stiTRANSPORT_ANY	0xFF

//
// Structure used to pass the various ports and port ranges to use in the protocol stacks
//
struct SstiConferencePorts
{
	SstiConferencePorts () = default;

	int nSIPListenPort{0};
	int nSIPTLSListenPort{0};
};


struct SstiRegistrationInfo
{
	SstiRegistrationInfo () = default;
	
	//
	// Copy constructor
	//
	SstiRegistrationInfo (
		const SstiRegistrationInfo &Other) = default;
	
	SstiRegistrationInfo &operator= (
		const SstiRegistrationInfo &Other) = default;
	
	std::string PublicDomain;
	std::string PublicDomainAlt;
	std::string PrivateDomain;
	std::string PrivateDomainAlt;
	std::string AgentDomain;
	std::string AgentDomainAlt;
	std::map<std::string, std::string> PasswordMap;
	int nRestartProxyMaxLookupTime = CLIENT_REREGISTER_MAX_LOOKUP_TIME_DEFAULT;	// The maximum number of seconds to wait before forcing a complete DNS lookup and re-registration.
	int nRestartProxyMinLookupTime = CLIENT_REREGISTER_MIN_LOOKUP_TIME_DEFAULT;	// The minimum number of seconds to wait before forcing a complete DNS lookup and re-registration.
};


// *** Note *** if adding any new members to the structure, make sure that they are properly
// initialized using C++11-style initialization
struct SstiConferenceParams
{
public:

	int SpeedClamp(EstiProtocol eProtocol, int nSpeed) const
	{
		stiUNUSED_ARG (eProtocol);
	 	return nSpeed;
	}

	int GetMaxSendSpeed(EstiProtocol eProtocol) const
	{
		return SpeedClamp(eProtocol, nMaxSendSpeed);
	}

	void SetMaxSendSpeed(int maxSendSpeed)
	{
		nMaxSendSpeed = maxSendSpeed;
	}

	int GetMaxRecvSpeed(EstiProtocol eProtocol) const
	{
		return SpeedClamp(eProtocol, nMaxRecvSpeed);
	}

	void SetMaxRecvSpeed(int maxRecvSpeed)
	{
		nMaxRecvSpeed = maxRecvSpeed;
	}
	
public:
	unsigned int unLowFrameRateThreshold263 = 128000;
	unsigned int unLowFrameRateThreshold264 = 180000;

	int nRingsBeforeGreeting = stiRINGS_BEFORE_GREETING_DEFAULT;

	EstiAudioCodec ePreferredAudioCodec = estiAUDIO_NONE;
	EstiTextCodec ePreferredTextCodec = estiTEXT_NONE;
	EstiVideoCodec ePreferredVideoCodec = estiVIDEO_NONE;
	EstiVideoCodec2 eAllowedVideoEncoders = estiVIDEOCODEC_ALL;

	EstiSIPNATTraversal eNatTraversalSIP = estiSIPNATDisabled;
	
	EstiPublicIPAssignMethod eIPAssignMethod = estiIP_ASSIGN_AUTO;
	std::string PublicIPv4;
	std::string PublicIPv6;

	std::string AutoDetectedPublicIp;

	EstiVcoType ePreferredVCOType = stiVCO_PREFERRED_TYPE_DEFAULT;
	std::string vcoNumber;

	struct MediaDirection
	{
		bool bInboundAudio = true;
		bool bOutboundAudio = true;
		bool bInboundText = true;
		bool bOutboundText = true;
		bool bInboundVideo = true;
		bool bOutboundVideo = true;
	};

	MediaDirection stMediaDirection;

	int nAutoKeyframeSend = 5000;
	unsigned int unLostConnectionDelay = 30;
	bool bVerifyAddress = true;
	bool bCheckForAuthorizedNumber = true;
	EstiTransport eSIPDefaultTransport = estiTransportTCP;	// This determines the transport in the target URI when a transport is not specified.
	int nAllowedProxyTransports = stiTRANSPORT_ANY;		// This determines the transports allowed to use for the connection to the proxy when NAT Traversal is enabled.
	bool bIPv6Enabled = false;					// This determines whether to use IPv6 for the connection to the proxy when NAT Traversal is enabled.
	EstiSecureCallMode eSecureCallMode = stiSECURE_CALL_MODE_DEFAULT; // Use sips uris and encrypted media
	
	CstiServerInfo TunnelServer;
	CstiServerInfo TurnServer;
	CstiServerInfo TurnServerAlt;
	
	SstiRegistrationInfo SIPRegistrationInfo;
	
	std::string TunneledIpAddr;

	bool bDTMFEnabled = false;  // Enable DTMF
	std::vector<std::string> AllowedAudioCodecs; // Specifically define which audio codecs to support (if hardware supported)
	
	std::string SipInstanceGUID;

	int nMaxSendSpeed = 256000;
	int nMaxRecvSpeed = 256000;
	
	EstiAutoSpeedMode eAutoSpeedSetting = stiAUTO_SPEED_MODE_DEFAULT;
	uint32_t un32AutoSpeedMinStartSpeed = stiAUTO_SPEED_MIN_START_SPEED_DEFAULT;
	bool bBlockCallerID = false;
	bool bBlockAnonymousCallers = false;
	bool bRemoteFlowControlLogging = false;
	
	bool rtcpFeedbackFirEnabled = false;
	bool rtcpFeedbackPliEnabled = false;
	bool rtcpFeedbackAfbEnabled = false;
	SignalingSupport rtcpFeedbackTmmbrEnabled = SignalingSupport::Disabled;
	SignalingSupport rtcpFeedbackNackRtxSupport = SignalingSupport::Disabled;
	
	int vrsFailoverTimeout = stiVRS_FAILOVER_TIMEOUT_DEFAULT;

	bool useIceLiteMode = false; //Verify ICE Candidates are sent to you but don't gather your own
	bool alwaysEnableDTLS = false;
	bool rejectHostOnlyIceRequests = false; //Reject calls that request ICE but only have "host" candidates with IP's in the private range
	int maxIceSessions = 10; // Default in the Radvision ICE stack
	
	// Prioritize privacy over video quality/latency. Remove public IP addresses from the
	// SDP by removing non-relay candidates. This doesn't currently affect SIP headers.
	bool prioritizePrivacy = false;

	bool sendSipKeepAlive {true}; //Whether to send a periodic SIP info message to keep the TCP connection alive
};

// struct to hold name and body for Sip Header
struct SstiSipHeader {
	std::string name{};
	std::string body{};
};

#endif // #ifndef STICONFDEFS_H
