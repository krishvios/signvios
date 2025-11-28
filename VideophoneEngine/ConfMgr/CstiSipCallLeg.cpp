////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiSipCallLeg
//
//  File Name:  CstiSipCallLeg.cpp
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
#include "CstiSipCallLeg.h"
#include "stiSipTools.h"
#include "Sip/Message.h"
#include "stiTools.h"
#include <sstream>
#include "RvSipOtherHeader.h"
#include "RvSipViaHeader.h"
#include "payload.h"
#ifdef stiTUNNEL_MANAGER
#include "rvsockettun.h"
#endif
#include "stiSystemInfo.h"
#include "stiRemoteLogEvent.h"
#include "CstiDataTaskCommonP.h"
#include "CstiVideoPlayback.h"
#include "CstiVideoRecord.h"
#include "CstiAudioPlayback.h"
#include "CstiAudioRecord.h"
#include "CstiTextPlayback.h"
#include "CstiTextRecord.h"
#include "IstiNetwork.h"

//
// Constants
//
static const char g_szSdpAttributeIceUfrag[] = "ice-ufrag";
static const char g_szSdpAttributeIcePwd[] = "ice-pwd";


#define DEFAULT_PTIME 20 // NOTE: This is the default ptime defined in rfc3551.

struct SstiRtpMap
{
	int8_t n8PayloadTypeNbr;
	const char *pszName;
	int nClockRate;
	bool bVariableBandwidth; // This codec can use any rate (as opposed to specific data rates).
	int nBandwidth;
};

// There are well-known payload types for some audio/video types.  Below they are listed.
// For more information about well-known mappings, see http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml.
static const SstiRtpMap g_astWellKnownPayloadTypes [] =
{
	{RV_RTP_PAYLOAD_PCMU, "PCMU", 8000, false, 64},		// Valid bandwidths are 56 and 64
	{RV_RTP_PAYLOAD_PCMA, "PCMA", 8000, false, 64},		// Valid bandwidths are 56 and 64
	{RV_RTP_PAYLOAD_GSM, "GSM", 8000, true, 0},
	{RV_RTP_PAYLOAD_G722, "G722", 8000, false, 64},		// Valid bandwidths are 48, 56 and 64
	{RV_RTP_PAYLOAD_G7231, "G723", 8000, false, 6},		// Valid bandwidths are 5.3 and 6.3  (Changed to 6 to fix C++11 compiler error)
	{RV_RTP_PAYLOAD_G728, "G728", 8000, false, 16},		// Valid bandwidths are 16
	{RV_RTP_PAYLOAD_G729, "G729", 8000, false, 8},		// Valid bandwidths are 8
	{RV_RTP_PAYLOAD_DVI4_8K, "DVI4", 8000, false, 0},
	{RV_RTP_PAYLOAD_DVI4_16K, "DVI4", 16000, false, 0},
	{RV_RTP_PAYLOAD_DVI4_11K, "DVI4", 11025, false, 0},
	{RV_RTP_PAYLOAD_DVI4_22K, "DVI4", 22050, false, 0},
	{RV_RTP_PAYLOAD_L16_1, "L16", 44100, false, 0},
	{RV_RTP_PAYLOAD_L16_2, "L16", 44100, false, 0},
	{RV_RTP_PAYLOAD_QCELP, "QCELP", 8000, true, 0},
	{RV_RTP_PAYLOAD_CN, "CN", 8000, true, 0},
	{RV_RTP_PAYLOAD_MPA, "MPA", 90000, true, 0},
	{RV_RTP_PAYLOAD_CELB, "CelB", 90000, true, 0},
	{RV_RTP_PAYLOAD_JPEG, "JPEG", 90000, true, 0},
	{RV_RTP_PAYLOAD_NV, "nv", 90000, true, 0},
	{RV_RTP_PAYLOAD_H261, "H261", 90000, true, 0},
	{RV_RTP_PAYLOAD_MPV, "MPV", 90000, true, 0},
	{RV_RTP_PAYLOAD_MP2T, "MP2T", 90000, true, 0},
	{RV_RTP_PAYLOAD_H263, "H263", 90000, true, 0},
};
static int g_nWellKnownPayloadTypesLength = sizeof (g_astWellKnownPayloadTypes) / sizeof (g_astWellKnownPayloadTypes[0]);

// 	This next bit is a neat trick to get it to print the file reference (Do not "optomize" this or it'll break.)
#define _STA(x) #x
#define _STB(x) _STA(x)
#define HERE __FILE__ ":" _STB(__LINE__)
#define DBG_MSG(fmt,...) stiDEBUG_TOOL (g_stiSipMgrDebug, stiTrace (fmt "\n", ##__VA_ARGS__); )


// See comment in CstiSipManager.h for m_callLegMap for details
size_t CstiSipCallLeg::m_uniqueKeyIncrementor = 0;
std::recursive_mutex CstiSipCallLeg::m_uniqueKeyIncrementorMutex;


/*!
 * \brief add an SDES key to a media descriptor
 */
stiHResult SDESKeysAdd(
	RvSdpMediaDescr * mediaDescriptor,
	const std::vector<vpe::SDESKey> &keys)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// There may be existing keys, account for that
	int index = rvSdpMediaDescrGetNumOfCrypto(mediaDescriptor) + 1;

	for(const auto &key : keys)
	{
		RvSdpCryptoAttr *cryptoAttr = rvSdpMediaDescrAddCrypto(
				mediaDescriptor,
				index,
				vpe::SDESKey::toString(key.suiteGet()).c_str()
				);
		stiTESTCOND(cryptoAttr, stiRESULT_ERROR);

		RvSdpStatus rvStatus = rvSdpCryptoAddKeyParam(
				cryptoAttr,
				key.methodGet().c_str(),
				key.base64KeyParamsGet().c_str()
				);
		// NOTE: don't need to add any session parameters via rvSdpCryptoAddSessionParam()
		stiTESTRVSTATUS();

		++index;
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCallLeg::SdpMediaDescriptorAdd
//
// Description: Add a single "m" line to the given SDP
//
// Returns: The media descriptor
//
stiHResult CstiSipCallLeg::SdpMediaDescriptorAdd (
	const SstiSdpStream &sdpStream,
	const SstiSdpStream &localSdpStream,
	int nEnabledStreamIndex,
	CstiSdp *pSdp, ///< To this
	const std::string &rtpAddress,
	unsigned int unRtpPort, ///< The rtp channel we will be listening on
	const std::string &rtcpAddress,
	unsigned int unRtcpPort, ///< The rtcp channel we will be listening on
	RvSdpConnectionMode eDirection, ///< one of: RV_SDPCONNECTMODE_SENDRECV, RV_SDPCONNECTMODE_RECVONLY, RV_SDPCONNECTMODE_SENDONLY
	int nRate,
	EstiSDPRole role)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	int currentPayloadCount = 0;
	bool streamEnabled = unRtpPort != 0;
	auto sipCall = m_sipCallWeak.lock ();

	//
	// Add "m" line
	//
	RvSdpMediaDescr *pMediaDescriptor;
	pMediaDescriptor = pSdp->mediaDescriptorAdd(RadvisionMediaTypeGet (sdpStream.eType), unRtpPort, sdpStream.MediaProtocol);
	stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);

	// Add SDES keys, if there are any
	SDESKeysAdd(pMediaDescriptor, localSdpStream.sdesKeys);

	if (!sipCall->disableDtlsGet ())
	{
		dtlsParametersAdd(pMediaDescriptor, localSdpStream);
	}

	//
	// If the media type was "unknown" then update the string to match the actual media type.
	// We also have to test for text as Radvision does not know about the text media type.
	//
	if (estiMEDIA_TYPE_TEXT == sdpStream.eType || estiMEDIA_TYPE_UNKNOWN == sdpStream.eType)
	{
		rvSdpMediaDescrSetMediaTypeStr (pMediaDescriptor, sdpStream.MediaTypeString.c_str ());
	}

	//
	// If the media protocol string is unknown then use the media protocol string to set the media protocol.
	//
	if (sdpStream.MediaProtocol == RV_SDPPROTOCOL_UNKNOWN)
	{
		rvSdpMediaDescrSetProtocolStr (pMediaDescriptor, sdpStream.MediaProtocolString.c_str ());
	}

#ifdef stiDEBUG_OUTPUT_CONF_MGR //...
	const char *vasStiMediaType[] =
	{
		"estiMEDIA_TYPE_UNKNOWN",
		"estiMEDIA_TYPE_VIDEO",
		"estiMEDIA_TYPE_AUDIO",
		"estiMEDIA_TYPE_TEXT"
	}; // EstiMediaType
	static char msg[256];
	sprintf(msg, "vpEngine::CstiSipCallLeg::SdpMediaDescriptorAdd() - eMediaType: %s - unRtpPort: %d", vasStiMediaType[sdpStream.eType], unRtpPort);
	DebugTools::instanceGet()->DebugOutput(msg);
#endif

	//
	// Add the payload IDs
	//
	if (sdpStream.eType == estiMEDIA_TYPE_VIDEO)
	{
		sipCall->SdpVideoPayloadsAdd (pMediaDescriptor, streamEnabled, role);
	}
	else if (sdpStream.eType == estiMEDIA_TYPE_TEXT)
	{
		sipCall->SdpTextPayloadsAdd (pMediaDescriptor, streamEnabled);
	}
	else if (sdpStream.eType == estiMEDIA_TYPE_AUDIO)
	{
		sipCall->SdpAudioPayloadsAdd (pMediaDescriptor, streamEnabled);
	}

	// Was this media line disabled?
	// If so, add a dummy payload id, but only if this m-line doesn't have any payload ids
	if (!streamEnabled)
	{
		currentPayloadCount = (RvUint32)rvSdpMediaDescrGetNumOfPayloads (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

		if (0 == currentPayloadCount)
		{
			// Since some devices are very strict in the syntax of the SDP m-line,
			// we need to put at least one payload type into the response; even if we
			// don't support the media stream.
			// TODO: when we parse payload ids, we need to add code to respond correctly
			// when we get a payload id we don't understand, instead of just ignoring it (PayloadInfoGet)
			// IS-25602 exposed this shortcoming in our signaling
			rvSdpMediaDescrAddPayloadNumber (pMediaDescriptor, stiDUMMY_PAYLOAD_ID);
		}
	}
	else
	{
		std::string connectionAddr;
		RvSdpConnection *pConnection = nullptr;

		//
		// If the rtp connection address is different from the sdp connection address then
		// add a "c" line for the rtp address.
		//
		pConnection = pSdp->connectionGet();
		stiTESTCOND (pConnection, stiRESULT_ERROR);

		{
			const char *tmpConnectionAddr = rvSdpConnectionGetAddress (pConnection);
			stiTESTCOND (tmpConnectionAddr != nullptr, stiRESULT_ERROR);

			connectionAddr = tmpConnectionAddr;
		}

		//
		// If the RTP address is not the same as the message scope's connection address then
		// add a c-line to the stream and use the RTP address as the connection address.
		//
		if (connectionAddr != rtpAddress)
		{
			RvSdpAddrType eIpAddressType = IPv6AddressValidate (rtpAddress.c_str ()) ? RV_SDPADDRTYPE_IP6 : RV_SDPADDRTYPE_IP4;
			rvStatus = rvSdpMediaDescrSetConnection (pMediaDescriptor, RV_SDPNETTYPE_IN, eIpAddressType, rtpAddress.c_str ());
			stiTESTRVSTATUS ();

			connectionAddr = rtpAddress;
		}

		if (nRate >= 0)
		{
			rvSdpMediaDescrAddBandwidth (pMediaDescriptor, "AS", nRate);
		}

		rvSdpMediaDescrSetConnectionMode (pMediaDescriptor, eDirection);

		//
		// Add ICE media attributes only if we have attributes and a valid stream index.
		//
		if (!m_IceSdpAttributes.MediaAttributes.empty () && nEnabledStreamIndex >= 0)
		{
			RvSdpListIter ListIterator;
			RvSdpAttribute *pAttribute = nullptr;
			const char *pAttrName = nullptr;
			const char *pAttrValue = nullptr;

			//
			// Add the attributes from the ICE media attributes to the SDP message.
			//
			pAttribute = rvSdpMsgGetFirstAttribute (m_IceSdpAttributes.MediaAttributes[nEnabledStreamIndex], &ListIterator);

			if (pAttribute)
			{
				for (;;)
				{
					pAttrName = rvSdpAttributeGetName (pAttribute);
					pAttrValue = rvSdpAttributeGetValue (pAttribute);

					rvSdpMediaDescrAddAttr (pMediaDescriptor, pAttrName, pAttrValue);

					//
					// Get the next attribute
					//
					pAttribute = rvSdpMediaDescrGetNextAttribute (&ListIterator);

					if (pAttribute == nullptr)
					{
						break;
					}
				}
			}
		}

		//
		// If the rtcp port isn't the next one after the rtp port or
		// if the rtcp address is different from the sdp connection address then add
		// an rtcp port attribute.
		//
		if (unRtcpPort != unRtpPort + 1 || connectionAddr != rtcpAddress)
		{
			rvStatus = rvSdpMediaDescrSetRtcp (pMediaDescriptor, unRtcpPort, RV_SDPNETTYPE_IN, RV_SDPADDRTYPE_IP4, rtcpAddress.c_str ());
			stiTESTRVSTATUS ();
		}
	}

STI_BAIL:

	return hResult;
}

std::string CstiSipCallLeg::encryptionChangesVideoToJsonArrayGet ()
{
	std::string ret;

	auto sipCall = m_sipCallWeak.lock ();
	if (sipCall)
	{
		ret = encryptionChangesToJsonGet (sipCall->m_encryptionStateChangesVideo);
	}
	return ret;
}

std::string CstiSipCallLeg::encryptionChangesAudioToJsonArrayGet ()
{
	std::string ret;

	auto sipCall = m_sipCallWeak.lock ();
	if (sipCall)
	{
		ret = encryptionChangesToJsonGet (sipCall->m_encryptionStateChangesAudio);
	}
	return ret;
}

std::string CstiSipCallLeg::encryptionChangesTextToJsonArrayGet ()
{
	std::string ret;

	auto sipCall = m_sipCallWeak.lock ();
	if (sipCall)
	{
		ret = encryptionChangesToJsonGet (sipCall->m_encryptionStateChangesText);
	}
	return ret;
}

/**
 * Converts a map of the encryption changes into a Json array for reporting.
 *
 * \return std::string A JSON array of encryption changes and the associated timestamps.
 *
 * Example output: [{"Encryption":0,"TimeStamp":"2020-06-17 18:56:14"}, {"Encryption":1,"TimeStamp":"2020-06-17 18:56:14"}]
 * */

std::string CstiSipCallLeg::encryptionChangesToJsonGet (const std::list <std::tuple <Poco::Timestamp, int>> &changeList)
{
	Poco::JSON::Array jsonArray;

	for (auto &it : changeList)
	{
		Poco::Timestamp timestamp;
		int encryption;
		std::tie (timestamp, encryption) = it;

		auto fmt_ts = Poco::DateTimeFormatter::format (timestamp, "%Y-%m-%d %H:%M:%S");

		Poco::JSON::Object jsonObject;
		jsonObject.set ("Encryption", encryption);
		jsonObject.set ("TimeStamp", fmt_ts);
		jsonArray.add (jsonObject);
	}

	std::stringstream jsonStream;
	jsonArray.stringify (jsonStream);
	return jsonStream.str ();
}

stiHResult CstiSipCallLeg::SdpCreate (
	CstiSdp *pSdp,
	EstiSDPRole eSDPRole)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	bool bChoseAudio = false;
	bool bChoseText = false;
	bool bChoseVideo = false;
	int nEnabledStreamIndex = -1;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	CstiSdp Sdp;

	if (!m_SdpStreams.empty ())
	{
		Sdp.initialize (nullptr);

		// We use this IP to tell remote endpoints where to send media when they cannot be placed on hold or do not honor the directionality we choose.
		const char *pszMediaRedirectIP = "127.0.0.1";

		//
		// Increment the SDP version.
		// Note: The SDP version needs to be incremented unless there are no changes in streams or stream parameters
		// Today we will always increment and sometime later choose to only increment when streams or stream parameters change.
		//
		++m_un32SdpVersionId;

		hResult = SdpHeaderAdd (&Sdp);
		stiTESTRESULT ();

		// Add media
		// Loop through all stored "m" lines
		// NOTE: m-lines are re-evaluated each time we issue an offer or answer because m-lines can be added throughout the session
		auto SdpStream = m_SdpStreams.cbegin();
		auto localSdpStream = m_localSdpStreams.cbegin();
		for (;
			 SdpStream != m_SdpStreams.end () && localSdpStream != m_localSdpStreams.end();
			 ++SdpStream, ++localSdpStream)
		{
			SstiMediaPayloadP pMedia;
			std::shared_ptr<CstiDataTaskCommonP> pPlayback;
			std::shared_ptr<CstiDataTaskCommonP> pRecord;
			RvAddress *pDefaultRtpAddress = nullptr;
			RvAddress *pDefaultRtcpAddress = nullptr;
			bool bOutboundMediaEnabled = false;
			bool bInboundMediaEnabled = false;
			bool bAddDisabledMediaBack = false;
			std::shared_ptr<vpe::RtpSession> session;

			// If we are sending an offer we want to add back all of our media capabilities.
			// This is to resolve bug #23704.
			if (eSDPRole == estiOffer)
			{
				bAddDisabledMediaBack = true;
			}

			// Determine what media (if any) this "m" line maps to
			if (estiMEDIA_TYPE_AUDIO == (*SdpStream)->eType)
			{
				if (!bChoseAudio
				 && sipCall->m_audioStream.supported ())
				{
					bOutboundMediaEnabled = sipCall->m_audioStream.outboundSupported ();
					bInboundMediaEnabled = sipCall->m_audioStream.inboundSupported ();

					pMedia = BestStreamMediaGet (
						**SdpStream,
						sipCall->PreferredAudioProtocolsGet ());

					if (pMedia)
					{
						bChoseAudio = true;
						pDefaultRtpAddress = &sipCall->m_DefaultAudioRtpAddress;
						pDefaultRtcpAddress = &sipCall->m_DefaultAudioRtcpAddress;
						pRecord = sipCall->m_audioRecord;
						pPlayback = sipCall->m_audioPlayback;
					}

					// if we are performing call bridging, then, since we represent the hearing party,
					// we must ensure that audio always keeps flowing from our end!
					if (sipCall->m_spCallBridge || sipCall->m_bIsBridgedCall)
					{
						bOutboundMediaEnabled = true;
						bInboundMediaEnabled = true;
						pRecord->PrivacyModeSet (estiOFF);
					}

					session = sipCall->m_audioSession;
				}
				// We've already chosen an m-line for this media type, don't enable any remaining m-lines of this type for offers
				else if (eSDPRole == estiOffer)
				{
					bAddDisabledMediaBack = false;
				}
			}
			else if (estiMEDIA_TYPE_TEXT == (*SdpStream)->eType)
			{
				if (!bChoseText
				 && sipCall->m_textStream.supported ())
				{
					bOutboundMediaEnabled = sipCall->m_textStream.outboundSupported ();
					bInboundMediaEnabled = sipCall->m_textStream.inboundSupported ();

					auto preferredTextProtocol = sipCall->PreferredTextPlaybackProtocolsGet();
					if (preferredTextProtocol)
					{
						pMedia = BestStreamMediaGet(
													**SdpStream,
													preferredTextProtocol);
					}

					if (pMedia)
					{
						bChoseText = true;
						pDefaultRtpAddress = &sipCall->m_DefaultTextRtpAddress;
						pDefaultRtcpAddress = &sipCall->m_DefaultTextRtcpAddress;
						pRecord = sipCall->m_textRecord;
						pPlayback = sipCall->m_textPlayback;
					}

					session = sipCall->m_textSession;
				}
				// We've already chosen an m-line for this media type, don't enable any remaining m-lines of this type for offers
				else if (eSDPRole == estiOffer)
				{
					bAddDisabledMediaBack = false;
				}
			}
			else if (estiMEDIA_TYPE_VIDEO == (*SdpStream)->eType)
			{
				if (!bChoseVideo
				 && sipCall->m_videoStream.supported ())
				{
					if (sipCall->m_videoStream.isActive())
					{
						bOutboundMediaEnabled = sipCall->m_videoStream.outboundSupported ();
						bInboundMediaEnabled = sipCall->m_videoStream.inboundSupported ();
					}
					else
					{
						bOutboundMediaEnabled = false;
						bInboundMediaEnabled = false;
					}

					pMedia = BestStreamMediaGet (
						**SdpStream,
						sipCall->PreferredVideoPlaybackProtocolsGet ());

					if (pMedia)
					{
						bChoseVideo = true;
						pDefaultRtpAddress = &sipCall->m_DefaultVideoRtpAddress;
						pDefaultRtcpAddress = &sipCall->m_DefaultVideoRtcpAddress;
						pRecord = sipCall->m_videoRecord;
						pPlayback = sipCall->m_videoPlayback;
					}

					session = sipCall->m_videoSession;
				}
				// We've already chosen an m-line for this media type, don't enable any remaining m-lines of this type for offers
				else if (eSDPRole == estiOffer)
				{
					bAddDisabledMediaBack = false;
				}
			}
			else
			{
				//
				// We don't support this media type so don't add it back in.
				//
				bAddDisabledMediaBack = false;
			}

			//
			// If inbound or outbound media is disabled then don't
			// add back in disabled media
			//
			if (!bOutboundMediaEnabled || !bInboundMediaEnabled)
			{
				bAddDisabledMediaBack = false;
			}

			int rtpPort = 0;
			int rtcpPort = 0;
			std::string RtpAddress;
			std::string RtcpAddress;
			int nRate = -1;
			RvSdpConnectionMode eDirection = RV_SDPCONNECTMODE_INACTIVE;

			//
			// If this is an offer or if the m-line is not disabled then determine ports, directionality, etc
			// Otherwise, just add a disabled m-line.
			//
			if (eSDPRole == estiOffer
				|| (pMedia && RvAddressGetIpPort (&(*SdpStream)->RtpAddress) != 0))
			{
				//
				// Initialize the addresses to the public IP address of the videophone.
				//
				std::string PublicIpAddr;
				if (sipCall->SipManagerGet ()->TunnelActive ())
				{
					PublicIpAddr.assign (sipCall->m_stConferenceParams.TunneledIpAddr);
				}
				else
				{
					if (sipCall->m_eIpAddressType == estiTYPE_IPV6 && !sipCall->m_stConferenceParams.PublicIPv6.empty () && !sipCall->IsBridgedCall ())
					{
						PublicIpAddr.assign (sipCall->m_stConferenceParams.PublicIPv6);
					}
					else if (!sipCall->m_stConferenceParams.PublicIPv4.empty () && !sipCall->IsBridgedCall ())
					{
						PublicIpAddr.assign (sipCall->m_stConferenceParams.PublicIPv4);
					}
					else
					{
						std::string localIpAddress = call->localIpAddressGet();
						stiTESTCOND (!localIpAddress.empty(), stiRESULT_ERROR);

						PublicIpAddr = localIpAddress;
					}
				}

				RtpAddress = PublicIpAddr;
				RtcpAddress = PublicIpAddr;

				if (pPlayback || pRecord)
				{
					if (pPlayback)
					{
						pPlayback->LocalAddressesGet (&RtpAddress, &rtpPort, &RtcpAddress, &rtcpPort);
					}
					else if (pRecord)
					{
						pRecord->LocalAddressesGet (&RtpAddress, &rtpPort, &RtcpAddress, &rtcpPort);
					}

					//
					// For third party
					// we don't want to signal "recvonly" when privacy is enabled.
					// So, set a boolean that acquires the record
					// privacy state only if the remote endpoint is a
					// Sorenson endpoint.
					//
					// TODO: We may need to also do the same thing for playback and privacy.
					//
					bool bRecordPrivacyEnabled = false;

					if (pRecord && RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
					{
						bRecordPrivacyEnabled = pRecord->PrivacyModeGet ();
					}

					if (eSDPRole == estiAnswer)
					{
						bool bAllowOutboundMedia = false;
						bool bAllowInboundMedia = false;

						//
						// This is an answer so respond with consideration to the
						// directionality of the previous offer.
						//
						if (bOutboundMediaEnabled)
						{
							if ((*SdpStream)->eOfferDirection == RV_SDPCONNECTMODE_RECVONLY
								|| (*SdpStream)->eOfferDirection == RV_SDPCONNECTMODE_SENDRECV)
							{
								bAllowOutboundMedia = true;
							}
						}

						if (bInboundMediaEnabled)
						{
							if ((*SdpStream)->eOfferDirection == RV_SDPCONNECTMODE_SENDONLY
								|| (*SdpStream)->eOfferDirection == RV_SDPCONNECTMODE_SENDRECV)
							{
								bAllowInboundMedia = true;
							}
						}

						//
						// In the checks below we don't consider the state of local privacy for audio
						// when considering directionality.
						//

						if ((call->StateValidate (esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH) && !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_RESUME))
							|| call->SubstateValidate (estiSUBSTATE_NEGOTIATING_RMT_HOLD))
						{
							// Remote hold

							if (!pPlayback || !bAllowInboundMedia || estiON == pPlayback->PrivacyModeGet () || call->StateValidate (esmiCS_HOLD_LCL | esmiCS_HOLD_BOTH)) // if local hold or remote mute
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_RECVONLY;
							}
						}
						else if (!pPlayback || !bAllowInboundMedia || estiON == pPlayback->PrivacyModeGet ()) // if rmt mute
						{
							if (!bAllowOutboundMedia || (bRecordPrivacyEnabled && estiMEDIA_TYPE_AUDIO != (*SdpStream)->eType))
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_SENDONLY;
							}
						}
						else if (!bAllowOutboundMedia || (bRecordPrivacyEnabled && estiMEDIA_TYPE_AUDIO != (*SdpStream)->eType)) // if lcl mute
						{
							if (bAllowInboundMedia)
							{
								eDirection = RV_SDPCONNECTMODE_RECVONLY;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
						}
						else if (call->StateValidate (esmiCS_HOLD_LCL | esmiCS_HOLD_BOTH) &&
								 !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME)
								 ) // if they are still on local hold
						{
							if (bAllowOutboundMedia)
							{
								eDirection = RV_SDPCONNECTMODE_SENDONLY;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
						}
						else
						{
							if (bAllowInboundMedia && bAllowOutboundMedia)
							{
								eDirection = RV_SDPCONNECTMODE_SENDRECV;
							}
							else if (bAllowInboundMedia)
							{
								eDirection = RV_SDPCONNECTMODE_RECVONLY;
							}
							else if (bAllowOutboundMedia)
							{
								eDirection = RV_SDPCONNECTMODE_SENDONLY;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
						}
					}
					else
					{
						// This is not an answer, but an offer. Therefore offer things from our local perspective
						// and let them eliminate things as necessary.
						if ((call->StateValidate (esmiCS_HOLD_LCL | esmiCS_HOLD_BOTH) && !call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_RESUME)) ||
							call->SubstateValidate (estiSUBSTATE_NEGOTIATING_LCL_HOLD)) // lcl hold
						{
							if (!bOutboundMediaEnabled || (bRecordPrivacyEnabled && estiMEDIA_TYPE_AUDIO != (*SdpStream)->eType)) // if lcl mute
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
							else if (call->ConnectedWithMcuGet (estiMCU_ANY) && !RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))  //MCU is treated as a third party so we go into this case. FreeSwitch servers handle direction fine, they advertise as a Sorenson user agent.
							{
								// We need to not set the direction to remain SENDRECV and redirect the media to a new IP.
								// This is due to behavior with the MCU when receiving SENDONLY.
								eDirection = RV_SDPCONNECTMODE_SENDRECV;

								// We only want to redirect the media but we still want to receive RTCP packets (this is the same behavior as a hold)
								// so only change the RTP address.
								RtpAddress = pszMediaRedirectIP;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_SENDONLY;
							}
						}
						else if (!bOutboundMediaEnabled || (bRecordPrivacyEnabled && estiMEDIA_TYPE_AUDIO != (*SdpStream)->eType)) // if lcl mute
						{
							if (bInboundMediaEnabled)
							{
								eDirection = RV_SDPCONNECTMODE_RECVONLY;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
						}
						else
						{
							if (bInboundMediaEnabled && bOutboundMediaEnabled)
							{
								eDirection = RV_SDPCONNECTMODE_SENDRECV;
							}
							else if (bInboundMediaEnabled)
							{
								eDirection = RV_SDPCONNECTMODE_RECVONLY;
							}
							else if (bOutboundMediaEnabled)
							{
								eDirection = RV_SDPCONNECTMODE_SENDONLY;
							}
							else
							{
								eDirection = RV_SDPCONNECTMODE_INACTIVE;
							}
						}
					}
				}
				else
				{
					//
					// We don't have a playback or a record data task. Determine ports
					// and addresses from the defaults.
					//
					RvChar addrTxt1[stiIP_ADDRESS_LENGTH + 1];

					if (pDefaultRtpAddress)
					{
						if (RvAddressGetString (pDefaultRtpAddress, sizeof (addrTxt1), addrTxt1))
						{
							RtpAddress = addrTxt1;
							rtpPort = RvAddressGetIpPort (pDefaultRtpAddress);
						}
						else
						{
							stiASSERT (estiFALSE);
						}
					}

					if (pDefaultRtcpAddress)
					{
						if (RvAddressGetString (pDefaultRtcpAddress, sizeof (addrTxt1), addrTxt1))
						{
							RtcpAddress = addrTxt1;
							rtcpPort = RvAddressGetIpPort (pDefaultRtcpAddress);
						}
						else
						{
							stiASSERT (estiFALSE);
						}
					}

					if (bInboundMediaEnabled && bOutboundMediaEnabled)
					{
						eDirection = RV_SDPCONNECTMODE_SENDRECV;
					}
					else if (bInboundMediaEnabled)
					{
						eDirection = RV_SDPCONNECTMODE_RECVONLY;
					}
					else if (bOutboundMediaEnabled)
					{
						eDirection = RV_SDPCONNECTMODE_SENDONLY;
					}
					else
					{
						eDirection = RV_SDPCONNECTMODE_INACTIVE;
					}
				}

				//
				// If this stream is currently disabled but we want to send it as
				// enabled then get the rtp port from the RTP session associated
				// with the stream.
				//
				if (rtpPort == 0 && bAddDisabledMediaBack)
				{
					if (session)
					{
						session->LocalAddressGet (&RtpAddress, &rtpPort, &RtcpAddress, &rtcpPort);
					}
					else
					{
						// We don't already have the channel created.  Not sure if this should ever happen.
						// for now, assert so that we know we have a problem to fix.
						stiASSERT (estiFALSE);
					}
				}

				// Log if we are disabling media that should be supported.
				if (pMedia && !rtpPort && (bInboundMediaEnabled || bOutboundMediaEnabled))
				{
					stiASSERTMSG(estiFALSE, "Failed to obtain valid port for %s using playback %p, record %p, and default RTP address %p",
						(*SdpStream)->MediaTypeString.c_str (), pPlayback.get(), pRecord.get(), pDefaultRtpAddress);
				}

				// figure out the data rate

				if (eDirection != RV_SDPCONNECTMODE_INACTIVE)
				{
					if (pPlayback)
					{
						if (estiMEDIA_TYPE_VIDEO == (*SdpStream)->eType)
						{
							nRate = pPlayback->FlowControlRateGet () / 1000;
						}
						else if (estiMEDIA_TYPE_AUDIO == (*SdpStream)->eType)
						{
							nRate = pPlayback->MaxChannelRateGet () / 1000;
						}
					}
					else if (pRecord)
					{
						nRate = pRecord->FlowControlRateGet () / 1000;
					}
					else
					{	// If channels have not been set up, default to conference parameters.
						if (estiMEDIA_TYPE_VIDEO == (*SdpStream)->eType)
						{
							nRate = sipCall->m_stConferenceParams.GetMaxRecvSpeed(estiSIP) / 1000;
						}
						else if (estiMEDIA_TYPE_AUDIO == (*SdpStream)->eType)
						{
							nRate = un32AUDIO_RATE / 1000;
						}
					}
				}
			}

			if (pMedia)
			{
				++nEnabledStreamIndex;
			}

			// Add the response
			hResult = SdpMediaDescriptorAdd (**SdpStream, **localSdpStream,
											 nEnabledStreamIndex, &Sdp,
											 RtpAddress, rtpPort,
											 RtcpAddress, rtcpPort,
											 eDirection, nRate, eSDPRole);
			stiTESTRESULT ();
		}

		*pSdp = Sdp;
	}
	else
	{
		// If we do not have any data in m_SdpStreams it is possible we are
		// responding to an offerless INVITE. We need m_sdp to not be empty.
		stiTESTCONDMSG (!m_sdp.empty(),
						stiRESULT_ERROR,
						"Unable to create valid SDP for CallID %s",
						sipCall->m_CallId.c_str());
		*pSdp = m_sdp;
	}

STI_BAIL:

	return hResult;
}


/*!\brief Disable any streams in the SDP passed in that are disabled in the previous offer.
 *
 */
stiHResult CstiSipCallLeg::UnsupportedStreamsDisable (
	CstiSdp *pSdp)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	auto sipCall = m_sipCallWeak.lock ();
	bool bChoseAudio = false;
	bool bChoseText = false;
	bool bChoseVideo = false;
	int nIndex = 0;

	//
	// Loop through the currently stored offered streams to see if we support
	// each one.  For each one we don't support then disable the stream in the SDP
	// that was passed in.
	//
	for (auto SdpStream = m_SdpStreams.begin (); SdpStream != m_SdpStreams.end (); ++SdpStream, ++nIndex)
	{
		SstiMediaPayloadP pMedia;

		// Determine what media (if any) this "m" line maps to
		if (estiMEDIA_TYPE_AUDIO == (*SdpStream)->eType)
		{
			if (!bChoseAudio
			 && sipCall->m_audioStream.supported ())
			{
				pMedia = BestStreamMediaGet (
						**SdpStream,
						sipCall->PreferredAudioProtocolsGet ());

				if (pMedia)
				{
					bChoseAudio = true;
				}
			}
		}
		else if (estiMEDIA_TYPE_TEXT == (*SdpStream)->eType)
		{
			if (!bChoseText
			 && sipCall->m_textStream.supported ())
			{
				pMedia = BestStreamMediaGet (
						**SdpStream,
						sipCall->PreferredTextPlaybackProtocolsGet ());

				if (pMedia)
				{
					bChoseText = true;
				}
			}
		}
		else if (estiMEDIA_TYPE_VIDEO == (*SdpStream)->eType)
		{
			if (!bChoseVideo
			 && sipCall->m_videoStream.supported ())
			{
				pMedia = BestStreamMediaGet (
						**SdpStream,
						sipCall->PreferredVideoPlaybackProtocolsGet ());

				if (pMedia)
				{
					bChoseVideo = true;
				}
			}
		}

		if (!pMedia)
		{
			//
			// For the current stream being offered we didn't find support for it so
			// disable it in the SDP message passed in.
			//
			pSdp->mediaDescriptorPortSet(nIndex, 0);
		}
	}

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCallLeg::SdpResponseAdd
//
// Description: Add an sdp response to the message, telling them what our preferred sdp is
//
stiHResult CstiSipCallLeg::SdpResponseAdd ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSdp Sdp;

	hResult = SdpCreate (&Sdp, estiAnswer);
	stiTESTRESULT ();

	hResult = outboundMessageGet().bodySet(Sdp);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


/*! \brief looks for the best media, and then populates features
 *
 * NOTE: SstiSdpStream param isn't const because the stream is modified
 */
SstiMediaPayloadP CstiSipCallLeg::BestStreamMediaGet (
	SstiSdpStream &SdpStream,
	const std::list<SstiPreferredMediaMap> *pPreferenceTypes)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	SstiMediaPayloadP pBestMedia;
	const std::list<SstiPreferredMediaMap> *pFeatureTypes = nullptr; // types that act as additional features to whatever the media is (ex. DTMF)
	auto sipCall = m_sipCallWeak.lock ();

	stiTESTCOND (pPreferenceTypes, stiRESULT_ERROR);

	if (estiMEDIA_TYPE_AUDIO == SdpStream.eType)
	{
		pFeatureTypes = sipCall->AudioFeatureProtocolsGet ();
	}

	for (auto &mediaPayload : SdpStream.Payloads)
	{
		for (const auto &preferredMediaMap : *pPreferenceTypes)
		{
			if ((mediaPayload.eCodec == preferredMediaMap.eCodec) &&
				mediaPayload.eCodec != estiRTX) // Ignore retransmission codecs.
			{
				// Need to look at the profiles and pick the first profile that we support
				// loop on profiles
				auto ProfileIt = preferredMediaMap.Profiles.cbegin ();
				do
				{
					if (ProfileIt == preferredMediaMap.Profiles.end () || ProfileIt->eProfile == mediaPayload.eVideoProfile)
					{
						if (estiPktUnknown == mediaPayload.ePacketizationScheme)
						{
							pBestMedia = nonstd::make_observer(&mediaPayload);
						}
						else
						{
							for (const auto &packetizationScheme : preferredMediaMap.PacketizationSchemes)
							{
								if (packetizationScheme == mediaPayload.ePacketizationScheme)
								{
									pBestMedia = nonstd::make_observer(&mediaPayload);
									break;
								}
							}
						}
					}

					if (ProfileIt != preferredMediaMap.Profiles.end ())
					{
						ProfileIt++;
					}

				} while (!pBestMedia && ProfileIt != preferredMediaMap.Profiles.end ());


				if (pBestMedia)
				{
					break;
				}
			}
		}

		if (pBestMedia)
		{
			break;
		}
	}

	// Search again.  This time for other media types that add extra features.
	if (pBestMedia && pFeatureTypes)
	{
		pBestMedia->FeatureMediaOffersIds.clear (); // Since we are re-calculating the list, clear it out first.

		for (const auto &featureType : *pFeatureTypes)
		{
			for (const auto &mediaPayload : SdpStream.Payloads)
			{
				if (mediaPayload.eCodec == featureType.eCodec)
				{
					/* Since we aren't currently using pFeatureTypes for video,
					 * there isn't any use to loop on profiles
					 * (none of the other types of media use profiles at this time).
					 */

					if (estiPktUnknown == mediaPayload.ePacketizationScheme)
					{
						pBestMedia->FeatureMediaOffersIds.push_back (mediaPayload.n8PayloadTypeNbr);
					}
					else
					{
						for (const auto packetizationScheme : featureType.PacketizationSchemes)
						{
							if (packetizationScheme == mediaPayload.ePacketizationScheme)
							{
								pBestMedia->FeatureMediaOffersIds.push_back (mediaPayload.n8PayloadTypeNbr);
								break;
							}
						}
					}
				}
			}
		}
	}

STI_BAIL:

	return pBestMedia;
}

/*!
 * \brief helper to query sdp entries for the "best" sdp entry
 */
SstiSdpStreamP CstiSipCallLeg::BestSdpStreamGet (
	EstiMediaType eMediaType,
	const std::list<SstiPreferredMediaMap> *pPreferenceTypes)
{
	SstiSdpStreamP sdpStream;
	SstiMediaPayloadP mediaPayload;
	std::tie (sdpStream, mediaPayload) = BestSdpStreamMediaGet (eMediaType, pPreferenceTypes);

	return sdpStream;
}

/*!
 *  \brief helper to query the stream and payloadId
 */
std::tuple<SstiSdpStreamP, boost::optional<int8_t>> CstiSipCallLeg::BestSdpStreamIdGet (
	EstiMediaType eMediaType,
	const std::list<SstiPreferredMediaMap> *pPreferenceTypes)
{
	SstiSdpStreamP sdpStream;
	SstiMediaPayloadP mediaPayload;
	boost::optional<int8_t> payloadId;
	std::tie (sdpStream, mediaPayload) = BestSdpStreamMediaGet (eMediaType, pPreferenceTypes);

	if (mediaPayload)
	{
		*payloadId = mediaPayload->n8PayloadTypeNbr;
	}

	return std::make_tuple(sdpStream, payloadId);
}

/*!
 * \brief Find the best media type for a certain kind of media
 *
 * This will take the first offered (remotely-preferred) media type that we can accept
 *
 * \return the best sdp/media tuple (values can be nullptr)
 *
 * NOTE: the tuple return type may seem a little strange. Another
 * option could have been to return a struct with the pointers, or,
 * pass in extra out parameters, both of which seemed more odd than
 * using a tuple.
 */
std::tuple<SstiSdpStreamP, SstiMediaPayloadP> CstiSipCallLeg::BestSdpStreamMediaGet (
	EstiMediaType eMediaType, ///< The type of media (audio, text, or video) to search for
	const std::list<SstiPreferredMediaMap> *pPreferenceTypes) ///< The list of media mappings (this could be for playback or record)
{
	stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG (hResult);
	SstiSdpStreamP bestSdpStream;
	SstiMediaPayloadP bestMedia;

	stiTESTCOND (pPreferenceTypes, stiRESULT_ERROR);

	for (const auto &sdpStream : m_SdpStreams)
	{
		if (sdpStream->eType == eMediaType)
		{
			bestMedia = BestStreamMediaGet (*sdpStream, pPreferenceTypes);

			if (bestMedia)
			{
				bestSdpStream = nonstd::make_observer(sdpStream.get ());
				break;
			}
		}
	}

/*	DBG_MSG ("Best %s %s media is: %s",
		(eMediaType==estiMEDIA_TYPE_VIDEO)?"video":(eMediaType==estiMEDIA_TYPE_TEXT)?"text":"audio",
		eMediaDirection == estiMDSend ? "send": eMediaDirection == estiMDReceive ? "receive" : "inactive",
		pBestMedia?pBestMedia->pszPayloadName:"[NONE]");*/

STI_BAIL:

	return std::make_tuple(bestSdpStream, bestMedia);
}


/*! \brief Find the T.140 basic media type
 *
 * This will find our T.140 basic media (if present).
 *
 * \return The T.140 basic media type, or NULL
 */
const SstiMediaPayload *T140BasicMediaGet (
	const SstiSdpStream &mediaStream)
{
	const SstiMediaPayload *pBestMedia = nullptr;

	for (const auto &mediaPayload : mediaStream.Payloads)
	{
		if (estiT140_TEXT == mediaPayload.eCodec)
		{
			pBestMedia = &mediaPayload;
			break;
		}
	}

	return pBestMedia;
}


/*!\brief Retrieves payload information identified by the payload number from the provided media descriptor.
 *
 * \param pMediaDescriptor The media descriptor from which to retrieve the payload information
 * \param nPayloadNumber The payload number for which to retrieve the payload information
 * \param pPayloadName The retrieved payload name
 * \param nStreamSendBandwidthSignaled The signaled "send" bandwidth
 * \param pnBandwidth The retrieved bandwidth for the payload.  On entry this contains the stream bandwidth.
 */
static stiHResult PayloadInfoGet (
	const RvSdpMediaDescr *pMediaDescriptor,
	int nPayloadNumber,
	std::string *pPayloadName,
	int nStreamSendBandwidthSignaled,
	int *pnBandwidth, // On entry this parameter contains the stream bandwidth.
	uint32_t *clockRate)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nNumOfRtpMap = 0;
	bool bFound = false;

	nNumOfRtpMap = rvSdpMediaDescrGetNumOfRtpMap (pMediaDescriptor);

	for (int i = 0; i < nNumOfRtpMap; i++)
	{
		auto pRtpMap = rvSdpMediaDescrGetRtpMap (pMediaDescriptor, i);

		int nRtpMapPayload = rvSdpRtpMapGetPayload (pRtpMap);

		if (nRtpMapPayload == nPayloadNumber)
		{
			//
			// This is the RTP Map we are looking for.
			//
			bFound = true;

			*pPayloadName = rvSdpRtpMapGetEncodingName (pRtpMap);
			
			*clockRate = rvSdpRtpMapGetClockRate (pRtpMap);

			//
			// We don't need to look any further so break out of the loop.
			//
			break;
		}
	}

	if (!bFound)
	{
		//
		// We didn't find an RTP Map.  Check to see if this a standard payload ID.
		// If it is, retrieve the information from the table.
		// For more information about well-known mappings, see http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml.
		//
		for (int i = 0; i < g_nWellKnownPayloadTypesLength; i++)
		{
			if (g_astWellKnownPayloadTypes[i].n8PayloadTypeNbr == nPayloadNumber)
			{
				//
				// We found the payload type we were looking for.
				//
				bFound = true;

				*pPayloadName = g_astWellKnownPayloadTypes[i].pszName;

				//
				// If this payload does not support variable bandwidth (it needs to be a specific bandwidth or one of several specific bandwidths),
				// ...
				if (!g_astWellKnownPayloadTypes[i].bVariableBandwidth)
				{
					// Was the bandwidth signaled in the media descriptor?
					if (nStreamSendBandwidthSignaled)
					{
						*pnBandwidth = nStreamSendBandwidthSignaled;
					}

					// Since it wasn't signaled, set it according to our preferred default for that payload type.
					else
					{
						*pnBandwidth = g_astWellKnownPayloadTypes[i].nBandwidth;
					}
				}

				//
				// We don't need to look any further so break out of the loop.
				//
				break;
			}
		}
	}

	if (!bFound)
	{
		//
		// If it still wasn't found then we have a problem.
		//
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}


/*!\brief Retrieve an attribute from the media stream.
 *
 */
stiHResult MediaAttributeGet (
	const RvSdpMediaDescr *pMediaDescriptor,
	const char *pszRequestedAttribute,
	RvSdpAttribute **ppAttribute)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSdpAttribute *pResult = nullptr;

	for (unsigned int j = 0; j < rvSdpMediaDescrGetNumOfAttr2 (const_cast<RvSdpMediaDescr *>(pMediaDescriptor)); j++)
	{
		auto pAttribute = rvSdpMediaDescrGetAttribute2 (const_cast<RvSdpMediaDescr *>(pMediaDescriptor), j);

		if (pAttribute)
		{
			const char *pszAttributeName = rvSdpAttributeGetName (pAttribute);

			if (0 == strcmp (pszAttributeName, pszRequestedAttribute))
			{
				pResult = pAttribute;

				break;
			}
		}
	}

	*ppAttribute = pResult;

	return hResult;
}


/*!\brief Retrieve the value of an attribute from the media stream
 *
 * An optional default value, presumably from the session level, may be
 * passed in as a parameter. If the attribute is not present in the
 * media stream, the value returned will be the default value.
 */
stiHResult MediaAttributeValueGet (
	const RvSdpMediaDescr *pMediaDescriptor,
	const char *pszRequestedAttribute,
	const std::string *pDefaultValue,
	std::string *pAttributeValue)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvSdpAttribute *pAttribute = nullptr;

	hResult = MediaAttributeGet (pMediaDescriptor, pszRequestedAttribute, &pAttribute);
	stiTESTRESULT ();

	if (pAttribute)
	{
		//
		// The attribute was found. Retrieve the attribute value.
		// If the value is not returned then set the value to an empty string
		//
		const char *pszAttributeValue = rvSdpAttributeGetValue (pAttribute);

		if (pszAttributeValue)
		{
			*pAttributeValue = pszAttributeValue;
		}
		else
		{
			pAttributeValue->clear ();
		}
	}
	else
	{
		//
		// The attribute was not found.
		// If there is a default value then use it.
		// Otherwise, set the value to an empty string.
		//
		if (pDefaultValue)
		{
			*pAttributeValue = *pDefaultValue;
		}
		else
		{
			pAttributeValue->clear ();
		}
	}

STI_BAIL:

	return hResult;
}


/*!
 * \brief get keys from an SDP message
 */
std::vector<vpe::SDESKey> SDESKeysGet(
		const RvSdpMediaDescr * mediaDescriptor)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::vector<vpe::SDESKey> keys;

	size_t numCrypto = rvSdpMediaDescrGetNumOfCrypto(mediaDescriptor);

	for(size_t i = 0; i < numCrypto; ++i)
	{
		auto cryptoAttr = rvSdpMediaDescrGetCrypto(mediaDescriptor, i);

		const char * suite = rvSdpCryptoGetSuite(cryptoAttr);

		size_t numKeyParams = rvSdpCryptoGetNumOfKeyParams(cryptoAttr);

		// Currently can't handle more than one key param
		stiTESTCOND(1 == numKeyParams, stiRESULT_ERROR);

		auto method = rvSdpCryptoGetKeyMethod(cryptoAttr, 0);
		auto info = rvSdpCryptoGetKeyInfo(cryptoAttr, 0);

		vpe::SDESKey key;

		// only handle inline keys
		stiTESTCOND(key.methodGet() == method, stiRESULT_ERROR);

		key = vpe::SDESKey::decode(suite, info);

		if(key.validGet())
		{
			keys.push_back(key);
		}
	}

STI_BAIL:

	stiUNUSED_ARG(hResult);

	return keys;
}


void dtlsParametersSet(
	const SstiSdpStreamUP &sdpStream,
	vpe::HashFuncEnum hashFunc,
	const std::vector<uint8_t> &fingerprint,
	const RvSdpMediaDescr* mediaDescriptor)
{
	sdpStream->FingerprintHashFunction = hashFunc;
	sdpStream->Fingerprint = fingerprint;

	sdpStream->SetupRole = rvSdpMediaDescrGetSetupRole(const_cast<RvSdpMediaDescr*>(mediaDescriptor), RV_TRUE);
}


std::tuple<vpe::HashFuncEnum, std::vector<uint8_t>> dtlsParametersParse(const CstiSdp &sdp)
{
	RvSdpStatus rvSdpStatus{};
	RvSdpCertficateFingerprintData cf{};
	vpe::HashFuncEnum hashFunc {vpe::HashFuncEnum::Undefined};
	std::vector<uint8_t> fingerprint (RV_SDP_MAX_FINGERPRINT_LENGTH);

	rvSdpStatus = rvSdpMsgGetCertFingerprint(
		sdp.sdpMessageGet(),
		&cf,
		false);

	if (rvSdpStatus == RV_SDPSTATUS_OK)
	{
		hashFunc = static_cast<vpe::HashFuncEnum>(cf.hashAlg);
		fingerprint.assign(
			reinterpret_cast<uint8_t *>(cf.fingerprint),
			reinterpret_cast<uint8_t *>(cf.fingerprint + cf.fingerprintLen));
	}

	return std::tuple<vpe::HashFuncEnum, std::vector<uint8_t>>{hashFunc, fingerprint};
}


std::tuple<vpe::HashFuncEnum, std::vector<uint8_t>> dtlsParametersParse(
	const RvSdpMediaDescr* mediaDescriptor)
{
	RvSdpStatus rvSdpStatus{};
	RvHashFuncEnum rvHashFunc{};
	vpe::HashFuncEnum hashFunc {vpe::HashFuncEnum::Undefined};
	auto fingerprintSize = EVP_MAX_MD_SIZE;
	std::vector<uint8_t> fingerprint (fingerprintSize);

	rvSdpStatus = rvSdpMediaGetFingerprint(
		const_cast<RvSdpMediaDescr*>(mediaDescriptor),
		&rvHashFunc, reinterpret_cast<RvChar*>(fingerprint.data()),
		&fingerprintSize);

	if (rvSdpStatus == RV_SDPSTATUS_OK)
	{
		hashFunc = static_cast<vpe::HashFuncEnum>(rvHashFunc);
		fingerprint.resize (fingerprintSize);
	}

	return std::tuple<vpe::HashFuncEnum, std::vector<uint8_t>>{hashFunc, fingerprint};
}


/*!
 * \brief Parse a message's sdp into a useful list of media types
 * \return nothing
 */
stiHResult CstiSipCallLeg::SdpParse (
	const CstiSdp &Sdp,
	EstiSDPRole eSDPRole, ///< indicates whether this SDP is an offer or an answer
	bool *pbRestartICE,
	bool *pbCallInfoChanged)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int nVideoChannels = 0;
	int nAudioChannels = 0;
	int nTextChannels = 0;
	const char *pszRemoteAddress = nullptr;
	bool bAllChannelsHeld = true;
	int nNbrMedia = 0;
	const RvSdpOrigin *pOrigin = nullptr;
	const RvSdpConnection *pConnection = nullptr;
	std::string SessionIceUserFragment;
	std::string SessionIcePassword;
	bool bRestartICE = false;
	bool bHasVideoMediaLine = false;
	EstiBool bTextSupported = estiFALSE;
	bool audioSupported = false;
	int s = 0;
	std::list<SstiSdpStreamUP>::iterator SdpStreamsIter;
	bool sdpHasSinfo = false;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	vpe::HashFuncEnum sessionHashFunc{};
	std::vector<uint8_t> sessionFingerprint;
	std::stringstream traceBuildup{};

	stiDEBUG_TOOL (g_stiSipMgrDebug,
		Sdp.Print();
	);

	//
	// Get the origin address.
	//
	pOrigin = Sdp.originGet ();
	if (pOrigin)
	{
		pszRemoteAddress = pOrigin->iAddress;
	}

	// Determine the remote ip if possible (may not be valid for re-invite's)
	pConnection = Sdp.connectionGet ();

	if (!pConnection || nullptr == pConnection->iAddress || 0 == strcmp (pConnection->iAddress, "0.0.0.0"))
	{
#if 0
		if (pConnection)
		{
			// Detected the old "c=0.0.0.0" method of hold
			DBG_MSG ("Call on hold (c=0.0.0.0)");
			if (eSDPRole == estiOffer || !sipCall->HoldStateGet (true))
			{
				// if this is a remote request or is a reply and we didn't tell them to hold, then they must want us held
				bSetRemoteHoldTo = true;
			}
		}
#endif
	}
	else
	{
		pszRemoteAddress = pConnection->iAddress;
	}

	if (!pszRemoteAddress)
	{
		sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_NOT_ACCEPTABLE_HERE);
		stiTHROW (stiRESULT_ERROR);
	}

	// Get bandwidth for the overall call (if present).  This can be altered in the media descriptors for given media types.
	const RvSdpBandwidth *pRvBandwidth;
	pRvBandwidth = Sdp.bandwidthGet ();

	if (pRvBandwidth)
	{
		m_nRemoteUACallBandwidth = rvSdpBandwidthGetValue (pRvBandwidth);

		if (0 == strcmp ("TIAS", rvSdpBandwidthGetType (pRvBandwidth)))
		{
			m_nRemoteUACallBandwidth /= 1000;
		}

		stiDEBUG_TOOL (g_stiRateControlDebug,
			stiTrace ("<SipCallLeg::SdpParse> Remote Call Bandwidth = %d\n", m_nRemoteUACallBandwidth);
		);
	}

	//
	// Extract the data associated with specific attributes in the SDP header.
	//
	for (size_t i = 0; i < Sdp.numberOfAttributesGet (); i++)
	{
		const RvSdpAttribute *pAttribute = Sdp.attributeGet (i);

		if (pAttribute)
		{
			const char *pszAttributeName = rvSdpAttributeGetName (const_cast<RvSdpAttribute *>(pAttribute));

			if (0 == strcmp (pszAttributeName, g_szSdpAttributeIceUfrag))
			{
				SessionIceUserFragment = rvSdpAttributeGetValue (const_cast<RvSdpAttribute *>(pAttribute));
			}
			else if (0 == strcmp (pszAttributeName, g_szSdpAttributeIcePwd))
			{
				SessionIcePassword = rvSdpAttributeGetValue (const_cast<RvSdpAttribute *>(pAttribute));
			}

			else if (0 == strcmp (pszAttributeName, g_szSDPSVRSSinfoPrivateHeader))
			{
				SInfoHeaderRead (pAttribute, pbCallInfoChanged);
				sdpHasSinfo = true;
			}

			else if (0 == strcmp (pszAttributeName, g_szSDPSVRSMailPrivateHeader))
			{
				std::string EncryptedMailMessage = rvSdpAttributeGetValue (const_cast<RvSdpAttribute *>(pAttribute));
				std::string VideoMailNumber;

				stiHResult hResult = DecryptString((unsigned char *)EncryptedMailMessage.c_str(), EncryptedMailMessage.size(), &VideoMailNumber);

				if (!stiIS_ERROR (hResult) && sipCall->SipManagerGet ()->productGet() != ProductType::MediaServer )
				{
					sipCall->CstiCallGet()->LeaveMessageSet(estiSM_LEAVE_MESSAGE_NORMAL);
					sipCall->CstiCallGet()->VideoMailServerNumberSet(&VideoMailNumber);
				}
			}

			else if (0 == strcmp (pszAttributeName, g_szSDPSVRSGroupChatPrivateHeader))
			{
				// Grab the Group Chat URI and store it in the call object
				std::string GVCUri = rvSdpAttributeGetValue (const_cast<RvSdpAttribute *>(pAttribute));
				sipCall->CstiCallGet()->RoutingAddressSet (GVCUri);

				// Set that we are connected with a GVC room.
				sipCall->CstiCallGet()->ConnectedWithMcuSet (estiMCU_GVC);

				// Extract the Conference ID from the uri and store it with the Call object
				std::string Guid = GVCUri.substr (nMCU_GUID_START_POS_IN_URI, nMCU_GUID_LEN_IN_URI);
				sipCall->CstiCallGet()->ConferencePublicIdSet (&Guid);
			}

			else if (0 == strcmp (pszAttributeName, g_svrsDisplaySize))
			{
				// Get the display size initially signaled and process it.
				std::string displaySize = rvSdpAttributeGetValue (const_cast<RvSdpAttribute *>(pAttribute));

				if (!displaySize.empty ())
				{
					stiHResult hResult2 = stiRESULT_SUCCESS;

					hResult2 = RemoteDisplaySizeProcess (displaySize);

					if (stiIS_ERROR(hResult2))
					{
						stiASSERTMSG(estiFALSE, "Failed to process SDP display size %s", displaySize.c_str ());
					}
				}
				else
				{
					stiASSERTMSG (estiFALSE, "Recieved Display Size Header in SDP with no attribute.");
				}
			}
		}
	}

	std::tie(sessionHashFunc, sessionFingerprint) = dtlsParametersParse(Sdp);

	nNbrMedia = Sdp.numberOfMediaDescriptorsGet ();
	traceBuildup << "Parsing " << nNbrMedia << " media sections";

	for (s = 0, SdpStreamsIter = m_SdpStreams.begin (); s < nNbrMedia; s++, ++SdpStreamsIter) // for each 'm' line
	{
		SstiSdpStreamP pOldSdpStream;
		int nPtime = DEFAULT_PTIME; // If ptime is undefined, we want a low (universally-acceptable) value, NOT the max we are capable of!
		const RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorGet (s);
		stiTESTCOND (pMediaDescriptor, stiRESULT_ERROR);
		EstiMediaType eMediaType;
		const char *pszMediaType = nullptr;
		int nRtpPort = 0;
		int nRtcpPort = 0;

		// Maps payload id to supported rtcp-fb types
		std::map<int8_t, std::list<RtcpFeedbackType>> rtcpFeedbackMap;

		// Initialize the channel bandwidth to that of the call.
		int nStreamSendBandwidth = m_nRemoteUACallBandwidth;
		int nStreamSendBandwidthSignaled = 0;

		pszMediaType = rvSdpMediaDescrGetMediaTypeStr (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

		eMediaType = SorensonMediaTypeGet (rvSdpMediaDescrGetMediaType (const_cast<RvSdpMediaDescr *>(pMediaDescriptor)), pszMediaType);

		RvSdpProtocol MediaProtocol = rvSdpMediaDescrGetProtocol (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));
		const char *pszMediaProtocol = rvSdpMediaDescrGetProtocolStr (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

		SstiSdpStreamUP pSdpStream = sci::make_unique<SstiSdpStream> (eMediaType, pszMediaType, MediaProtocol, pszMediaProtocol);

		traceBuildup << "\n| " << pszMediaType << " ";

		//
		// Retrieve the old SDP entry, if there is one.
		//
		if (SdpStreamsIter != m_SdpStreams.end ())
		{
			pOldSdpStream = nonstd::make_observer(SdpStreamsIter->get());
		}

		if (eMediaType == estiMEDIA_TYPE_VIDEO)
		{
			bHasVideoMediaLine = true;
		}

		//
		// Get the RTP port for this media stream.
		//
		nRtpPort = rvSdpMediaDescrGetPort (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

		//
		// If we don't know this media type or if the stream has been disabled (the RTP port is zero)
		// or if we don't understand the media protocol
		// then don't bother processing the rest of this stream.
		//
		if (eMediaType != estiMEDIA_TYPE_UNKNOWN
		 && nRtpPort != 0
		 // This check is disabled as it fails with endpoints that signal SAVPF (WebRTC)
		 // && (MediaProtocol == RV_SDPPROTOCOL_RTP //|| MediaProtocol == RV_SDPPROTOCOL_RTP_SAVP)
			)
		{
			// Parse out the ptime field if present
			for (unsigned int j = 0; j < rvSdpMediaDescrGetNumOfAttr2 (const_cast<RvSdpMediaDescr *>(pMediaDescriptor)); j++)
			{
				auto pAttribute = rvSdpMediaDescrGetAttribute2 (const_cast<RvSdpMediaDescr *>(pMediaDescriptor), j);
				if (pAttribute)
				{
					const char *pszAttributeName = rvSdpAttributeGetName (pAttribute);
					if (0 == strcmp (pszAttributeName, "ptime"))
					{
						nPtime = sipCall->SipManagerGet ()->G711AudioRecordMaxSampleRateGet ();
						// nPtime = sipCall->SipManagerGet ()->T140TextRecordMaxSampleRateGet (); // TODO: Is there ptime for text?  No, but there is a CPS (chars per second).  It is used when something less that 30cps is desired.
						int nRemotePTime = atoi (rvSdpAttributeGetValue (pAttribute));

						if (nRemotePTime < nPtime)
						{
							nPtime = nRemotePTime;
						}
						break;
					}

					// Does the remote system support any rtcp feedback methods?
					if (0 == strcmp (pszAttributeName, "rtcp-fb") &&
							 eMediaType == estiMEDIA_TYPE_VIDEO)
					{
						// Populates the feedback map by payload, so we know which feedback
						// mechanisms are supported by which payloads
						rtcpFbAttrParse (pAttribute,
						                 const_cast<RvSdpMediaDescr*>(pMediaDescriptor),
						                 &rtcpFeedbackMap,
										 eSDPRole);
					}
				}
			}

			pSdpStream->nPtime = nPtime;

			//
			// Get the offer direction
			//
			RvSdpConnectionMode eOfferDirection = rvSdpMediaDescrGetConnectionMode (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

			// if we didn't get a direction, the standard says it is SENDRECV
			if (eOfferDirection == RV_SDPCONNECTMODE_NOTSET)
			{
				eOfferDirection = RV_SDPCONNECTMODE_SENDRECV;
			}

			pSdpStream->eOfferDirection = eOfferDirection;

			//
			// Get the bandwidth
			//
			{
				auto pBandwidth = rvSdpMediaDescrGetBandwidth (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));
				if (pBandwidth)
				{
					if (0 == strcmp ("AS", rvSdpBandwidthGetType (pBandwidth)))
					{
						nStreamSendBandwidthSignaled = rvSdpBandwidthGetValue (pBandwidth);
						nStreamSendBandwidth = nStreamSendBandwidthSignaled;
					}
					else if (0 == strcmp ("TIAS", rvSdpBandwidthGetType (pBandwidth)))
					{
						nStreamSendBandwidthSignaled = rvSdpBandwidthGetValue (pBandwidth) / 1000;
						nStreamSendBandwidth = nStreamSendBandwidthSignaled;
					}
					else
					{
						stiASSERT (estiFALSE);
					}
				}
				else if (nStreamSendBandwidth == 0)
				{
					nStreamSendBandwidth = sipCall->m_stConferenceParams.GetMaxSendSpeed(estiSIP) / 1000;
				}

				SIP_DBG_NEGOTIATION("Raw bw s=%d", nStreamSendBandwidth);
				if (!sipCall->CstiCallGet()->SubstateValidate(estiSUBSTATE_NEGOTIATING_LCL_HOLD))
				{
					// Make sure we aren't exceeding our local maximum send speed.
					nStreamSendBandwidth = std::min (nStreamSendBandwidth, sipCall->m_stConferenceParams.GetMaxSendSpeed(estiSIP) / 1000);

					switch (eOfferDirection)
					{
						case RV_SDPCONNECTMODE_SENDONLY: // SENDONLY from their perspective
								break;

						case RV_SDPCONNECTMODE_NOTSET:
								DBG_MSG ("No direction specified.  Assuming SENDRECV.");
								// FALLTHRU
						case RV_SDPCONNECTMODE_RECVONLY: // RECVONLY from their perspective
						case RV_SDPCONNECTMODE_SENDRECV:
								bAllChannelsHeld = false;
								break;

						case RV_SDPCONNECTMODE_INACTIVE:
								nStreamSendBandwidth = 0;
								break;
						// default: Not handled for type safety
					}
				}
			}
			SIP_DBG_NEGOTIATION("Direction-correct bw s=%d", nStreamSendBandwidth);

			// Get our matching info for this media stream type

			auto pConnection = rvSdpMediaDescrGetConnection(const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

			std::string pszMediaAddress = pszRemoteAddress;

			if (pConnection)
			{
				const char *pszAddress = rvSdpConnectionGetAddress (pConnection);

				if (pszAddress[0] != '\0')
				{
					pszMediaAddress = pszAddress;
				}
			}

			if (sipCall->m_stConferenceParams.bIPv6Enabled)
			{
				std::string mappedAddress{};
				stiMapIPv4AddressToIPv6 (pszMediaAddress, &mappedAddress);
				stiASSERTMSG(false, "pszMediaAddress = %s mappedAddress = %s", pszMediaAddress.c_str(), mappedAddress.c_str());
				
				pszMediaAddress = mappedAddress.c_str();
			}
					
			int nIPAddressType = IPv6AddressValidate (pszMediaAddress.c_str()) ? RV_ADDRESS_TYPE_IPV6 : RV_ADDRESS_TYPE_IPV4;

			RvAddressConstruct (nIPAddressType, &pSdpStream->RtpAddress);
			RvAddressSetString (pszMediaAddress.c_str(), &pSdpStream->RtpAddress);
			RvAddressSetIpPort (&pSdpStream->RtpAddress, nRtpPort);

			//
			// Get the RTCP port for this media stream.
			// If the RTCP port has been specified specifically then use it
			// otherwise the RTCP port is the port following the RTP port.
			//
			nRtcpPort = nRtpPort + 1;
			const char *pszRtcpAddress = pszMediaAddress.c_str();

			RvSdpRtcp *pRtcpDescriptor = rvSdpMediaDescrGetRtcp (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

			if (pRtcpDescriptor)
			{
				// Is there an IP address in the descriptor?
				const char *pszTempAddress = rvSdpRtcpGetAddress (pRtcpDescriptor);
				if (pszTempAddress)
				{
					// We did get an IP address from the descriptor; use it.
					pszRtcpAddress = pszTempAddress;
					nIPAddressType = IPv6AddressValidate (pszRtcpAddress) ? RV_ADDRESS_TYPE_IPV6 : RV_ADDRESS_TYPE_IPV4;
				}

				nRtcpPort = rvSdpRtcpGetPort (pRtcpDescriptor);
			}

			RvAddressConstruct (nIPAddressType, &pSdpStream->RtcpAddress);
			RvAddressSetString (pszRtcpAddress, &pSdpStream->RtcpAddress);
			RvAddressSetIpPort (&pSdpStream->RtcpAddress, nRtcpPort);

			const std::list<SstiPreferredMediaMap> *pInboundProtocolList = nullptr;
			const std::list<SstiPreferredMediaMap> *pFeaturesList = nullptr;
			const std::list<SstiPreferredMediaMap> *pOutboundProtocolList = nullptr;

			switch (eMediaType)
			{
				case estiMEDIA_TYPE_VIDEO:
				{
					if (nVideoChannels == 0)
					{
						pOutboundProtocolList = sipCall->PreferredVideoRecordProtocolsGet ();
						pInboundProtocolList = sipCall->PreferredVideoPlaybackProtocolsGet ();
					}

					break;
				}

				case estiMEDIA_TYPE_AUDIO:
				{
					audioSupported = true;

					if (nAudioChannels == 0)
					{
						pOutboundProtocolList = sipCall->PreferredAudioProtocolsGet ();
						pFeaturesList = sipCall->AudioFeatureProtocolsGet ();
						pInboundProtocolList = sipCall->PreferredAudioProtocolsGet ();
					}

					break;
				}

				case estiMEDIA_TYPE_TEXT:
				{
					bTextSupported = estiTRUE;

					if (nTextChannels == 0)
					{
						pOutboundProtocolList = sipCall->PreferredTextRecordProtocolsGet ();
						pInboundProtocolList = sipCall->PreferredTextPlaybackProtocolsGet ();
					}

					break;
				}

				case estiMEDIA_TYPE_UNKNOWN:

					break;

			} // end switch

			//
			// For each payload, look at what is offered along with what we support and prefer.
			//
			int nPayloadCount = (RvUint32)rvSdpMediaDescrGetNumOfPayloads (const_cast<RvSdpMediaDescr *>(pMediaDescriptor));

			for (int i = 0; i < nPayloadCount; i++)
			{
				int nPayloadTypeNbr = rvSdpMediaDescrGetPayload (const_cast<RvSdpMediaDescr *>(pMediaDescriptor), i);
				std::string PayloadName;
				int nBandwidth = nStreamSendBandwidth;
				uint32_t clockRate{0};

				hResult = PayloadInfoGet (pMediaDescriptor, nPayloadTypeNbr, &PayloadName,
										  nStreamSendBandwidthSignaled, &nBandwidth, &clockRate);
				stiTESTRESULT ();

				if (i == 0)
				{
					pSdpStream->n8DefaultPayloadTypeNbr = nPayloadTypeNbr;
				}

				// Find payload types
				SstiPreferredMediaMap mapping;
				bool mappingFound = false;

				if (eOfferDirection != RV_SDPCONNECTMODE_SENDONLY)
				{
					if (pInboundProtocolList)
					{
						for (const auto &payloadMap: *pInboundProtocolList)
						{
							if ((INVALID_DYNAMIC_PAYLOAD_TYPE == payloadMap.n8PayloadTypeNbr && 0 == stiStrICmp (PayloadName.c_str (), payloadMap.protocolName.c_str ()))
							 || payloadMap.n8PayloadTypeNbr == nPayloadTypeNbr)
							{
								mapping = payloadMap;
								mappingFound = true;
								break;
							}
						}
					}

					if (pFeaturesList && !mappingFound)
					{
						for (const auto &payloadMap: *pFeaturesList)
						{
							if ((INVALID_DYNAMIC_PAYLOAD_TYPE == payloadMap.n8PayloadTypeNbr && 0 == stiStrICmp (PayloadName.c_str (), payloadMap.protocolName.c_str ()))
							 || payloadMap.n8PayloadTypeNbr == nPayloadTypeNbr)
							{
								mapping = payloadMap;
								mappingFound = true;
								break;
							}
						}
					}
				}

				if (!mappingFound && eOfferDirection != RV_SDPCONNECTMODE_RECVONLY)
				{
					if (pOutboundProtocolList)
					{
						for (const auto &payloadMap: *pOutboundProtocolList)
						{
							if ((INVALID_DYNAMIC_PAYLOAD_TYPE == payloadMap.n8PayloadTypeNbr && 0 == stiStrICmp (PayloadName.c_str (), payloadMap.protocolName.c_str ()))
							 || payloadMap.n8PayloadTypeNbr == nPayloadTypeNbr)
							{
								mapping = payloadMap;
								mappingFound = true;
								break;
							}
						}
					}

					if (pFeaturesList && !mappingFound)
					{
						for (const auto &payloadMap: *pFeaturesList)
						{
							if ((INVALID_DYNAMIC_PAYLOAD_TYPE == payloadMap.n8PayloadTypeNbr && 0 == stiStrICmp (PayloadName.c_str (), payloadMap.protocolName.c_str ()))
							 || payloadMap.n8PayloadTypeNbr == nPayloadTypeNbr)
							{
								mapping = payloadMap;
								mappingFound = true;
								break;
							}
						}
					}
				}

				if (!mappingFound)
				{
					// if its not a codec we support, don't add it.
					//DBG_MSG ("Unknown codec: %d %s", nPayloadTypeNbr, pszPayloadName);
					continue;
				}

				// Get the FMTP attributes
				std::map<std::string, std::string>  Attributes;
				sipCall->SdpFmtpValuesGet (&Attributes, pMediaDescriptor, nPayloadTypeNbr, mapping.eCodec);

				// Determine the packetization scheme
				EstiPacketizationScheme ePacketizationScheme = estiPktUnknown;
				EstiVideoProfile eProfile = estiPROFILE_NONE;

				switch (mapping.eCodec)
				{
					case estiH265_VIDEO:
					{
						// What does the SDP say?
						ePacketizationScheme = estiH265_NON_INTERLEAVED;
						eProfile = estiH265_PROFILE_MAIN;

						auto AttributeIt = Attributes.find("profile-id");
						if (AttributeIt != Attributes.end())
						{
							int nProfile = estiPROFILE_NONE;
							sscanf(AttributeIt->second.c_str(), "%d", &nProfile);

							eProfile = (EstiVideoProfile)nProfile;
						}

						//EstiH265Tier eTier;
						//AttributeIt = Attributes.find("tier-flag");
						//if (AttributeIt != Attributes.end())
						//{
						//	int tier {0};
						//	sscanf(AttributeIt->second.c_str(), "%d", &tier);
						//	eTier = static_cast<EstiH265Tier>(tier);
						//	stiTrace ("H265 Tier: %d\n", eTier);
						//}

						//EstiH265Level eLevel;
						//AttributeIt = Attributes.find("level-id");
						//if (AttributeIt != Attributes.end())
						//{
						//	int level {0};
						//	sscanf(AttributeIt->second.c_str(), "%d", &level);
						//	eLevel = static_cast<EstiH265Level>(level);
						//	stiTrace ("H265 Level: %d\n", eLevel);
						//}
						break;
					}

					case estiH264_VIDEO:
					{
						// What does the SDP say?
						ePacketizationScheme = estiH264_SINGLE_NAL;
						auto AttributeIt = Attributes.find ("packetization-mode");
						if (AttributeIt != Attributes.end ())
						{
							int nIndex = atoi (AttributeIt->second.c_str ()) + estiH264_SINGLE_NAL;
							if (nIndex >= estiH264_SINGLE_NAL && nIndex <= estiH264_INTERLEAVED)
							{
								ePacketizationScheme = (EstiPacketizationScheme)nIndex;
							}
						}
						eProfile = estiH264_BASELINE;
						AttributeIt = Attributes.find ("profile-level-id");
						if (AttributeIt != Attributes.end ())
						{
							unsigned int unProfile;
							unsigned int unConstraint;
							unsigned int unLevel;
							sscanf (AttributeIt->second.c_str (), "%02x%02x%02x", &unProfile, &unConstraint, &unLevel);
							eProfile = (EstiVideoProfile)unProfile;
						}

						break;
					}

					case estiH263_VIDEO:
					case estiH263_1998_VIDEO:
						eProfile = estiH263_ZERO;
						break;

					case estiCODEC_UNKNOWN:
					case estiG711_ALAW_56K_AUDIO:
					case estiG711_ALAW_64K_AUDIO:
					case estiG711_ULAW_56K_AUDIO:
					case estiG711_ULAW_64K_AUDIO:
					case estiG722_48K_AUDIO:
					case estiG722_56K_AUDIO:
					case estiG722_64K_AUDIO:
					case estiT140_TEXT:
					case estiTELEPHONE_EVENT:
					case estiT140_RED_TEXT:
						//
						// There are no alternative packetization schemes for these
						// codecs so there is nothing more to do.
						//

						break;
					case estiRTX:
						// We don't want to do RTX with WebRTC until we have implemented RFC#5576
						if (m_nRemoteSIPVersion < webrtcSSV)
						{
							// If this is a rtx codec find the associated media payload
							// and set the rtx paylod number there.
							for (auto &rtxMap : sipCall->m_mVideoPayloadMappings)
							{
								if (rtxMap.second.rtxPayloadType == nPayloadTypeNbr)
								{
									for (auto &payload : pSdpStream->Payloads)
									{
										if (payload.n8PayloadTypeNbr == rtxMap.first)
										{
											payload.rtxPayloadType = nPayloadTypeNbr;
										}
									}
								}
							}
						}
						
						break;
				}

				// Add outbound media type
				pSdpStream->Payloads.emplace_back (
					nPayloadTypeNbr,
					mapping.protocolName,
					mapping.eCodec,
					nBandwidth,
					Attributes,
					rtcpFeedbackMap[nPayloadTypeNbr],
					eProfile,
					ePacketizationScheme
					);

				DBG_MSG ("Call %p Added %d %s, profile %s, packetization %s (lcl SENDABLE) %dkbps to accepted list",
				        sipCall->CstiCallGet().get(),
						nPayloadTypeNbr,
						mapping.protocolName.c_str (),
						estiH263_ZERO == eProfile ? "estiH263_ZERO" :
						estiH264_BASELINE == eProfile ? "estiH264_BASELINE" :
						estiH264_MAIN == eProfile ? "estiH264_MAIN" :
						estiH264_EXTENDED == eProfile ? "estiH264_EXTENDED" :
						estiH264_HIGH == eProfile ? "estiH264_HIGH" : "estiPROFILE_NONE",
						estiH264_NON_INTERLEAVED == ePacketizationScheme ? "estiH264_NON_INTERLEAVED" :
						estiH264_SINGLE_NAL == ePacketizationScheme ? "estiH264_SINGLE_NAL" :
						estiH263_RFC_2190 == ePacketizationScheme ? "estiH263_RFC_2190" : "???",
						nBandwidth);

				// Map the payload type so that when we generate SDP for what we can support we don't come up with new payload IDs for
				// the same codec and packetization scheme combinations.
				// Also count the media types
				switch (eMediaType)
				{
					case estiMEDIA_TYPE_VIDEO:
					{
						if (!sipCall->VideoPayloadIDUsed (nPayloadTypeNbr))
						{
							// For bug # 22859 (Walker), there are cases where the payload type (PT) for a given video codec changes on us
							// when the transfer occurs.  As a result, we need to add the new payload so that playback will not throw away
							// media coming in on the new PT.  Note that this is not a complete fix with respect to the RFCs.
							// The complete fix is more involved and includes removing former PTs from our map and informing the record channel
							// to change what PT to send.  This gets trickier too since removing PTs creates another issue.  Consider the
							// SDP Answer that contains additional PTs (e.g. an additional codec or packetization that the offer didn't include),
							// simply removing PTs that don't show up in the SDP offer will remove those that we had previously added in the answer
							// too and now when we re-add them, they will get a new PT # due to our code.
							// Since this is more involved, we choose to only do this patch for now.
							sipCall->VideoPayloadMappingAdd (
								nPayloadTypeNbr,
								static_cast<EstiVideoCodec>(mapping.nCodec),
								eProfile,
								ePacketizationScheme,
								rtcpFeedbackMap[nPayloadTypeNbr]);
						}

						++nVideoChannels;
						break;
					}

					case estiMEDIA_TYPE_AUDIO:
					{
						if (!sipCall->AudioPayloadIDUsed (nPayloadTypeNbr))
						{
							int nCurrentPayloadTypeNbr = 0;

							//
							// Check to see if this codec and packetization scheme do not already have an ID.
							// If it does then don't add this to the list as we will want to make sure we use the one we have already assigned.
							//
							sipCall->AudioPayloadMapGet (static_cast<EstiAudioCodec>(mapping.nCodec), ePacketizationScheme, clockRate, &nCurrentPayloadTypeNbr);

							if (nCurrentPayloadTypeNbr == -1)
							{
								sipCall->AudioPayloadMappingAdd (nPayloadTypeNbr, static_cast<EstiAudioCodec>(mapping.nCodec), ePacketizationScheme, clockRate);
							}
						}

						++nAudioChannels;
						break;
					}

					case estiMEDIA_TYPE_TEXT:
					{
						if (!sipCall->TextPayloadIDUsed (nPayloadTypeNbr))
						{
							int nCurrentPayloadTypeNbr = 0;

							//
							// Check to see if this codec and packetization scheme do not already have an ID.
							// If it does then don't add this to the list as we will want to make sure we use the one we have already assigned.
							//
							sipCall->TextPayloadMapGet (static_cast<EstiTextCodec>(mapping.nCodec), ePacketizationScheme, &nCurrentPayloadTypeNbr);

							if (nCurrentPayloadTypeNbr == -1)
							{
								sipCall->TextPayloadMappingAdd (nPayloadTypeNbr, static_cast<EstiTextCodec>(mapping.nCodec), ePacketizationScheme);
							}
						}

						++nTextChannels;
						break;
					}

					case estiMEDIA_TYPE_UNKNOWN:

						break;
				}
			} // end j (SDP 'a' lines)

			//
			// Retrieve the ICE user-frag and password from the media stream (or from the session)
			// and determine if there is a change. If there is a change then indicate that an ICE restart is needed.
			//
			hResult = MediaAttributeValueGet (pMediaDescriptor, g_szSdpAttributeIceUfrag, &SessionIceUserFragment, &pSdpStream->IceUserFragment);
			stiASSERT (!stiIS_ERROR(hResult)); // Log the error but don't break out of the loop.

			hResult = MediaAttributeValueGet (pMediaDescriptor, g_szSdpAttributeIcePwd, &SessionIcePassword, &pSdpStream->IcePassword);
			stiASSERT (!stiIS_ERROR(hResult)); // Log the error but don't break out of the loop.

			if (eSDPRole == estiOffer &&
				sipCall->UseIceForCalls ())
			{
				bool bSDPContainsICE = (!pSdpStream->IceUserFragment.empty () || !pSdpStream->IcePassword.empty ());

				traceBuildup << "[bSDPContainsICE=" << bSDPContainsICE << ']';

				if (!pOldSdpStream)
				{
					// Restart ICE if an ICE user-frag or password is present.
					bRestartICE = bSDPContainsICE;

					if (bRestartICE)
					{
						stiTrace ("Restarting ICE 1\n");
						traceBuildup << "[Restarting ICE 1]";
					}
				}
				else if (m_bRemoteIsICE // If we were doing ICE and something has changed
					|| bSDPContainsICE) // Or we were not ICE but the new Offer contains ICE
				{
					// Indicate that an ICE restart is needed if the ICE user-frag or password changed (bug 22361).
					bRestartICE = (pOldSdpStream->IceUserFragment != pSdpStream->IceUserFragment || pOldSdpStream->IcePassword != pSdpStream->IcePassword);

					if (bRestartICE)
					{
						auto message = formattedString ("Restarting ICE 2: %s == %s, %s == %s",
							pOldSdpStream->IceUserFragment.c_str (), pSdpStream->IceUserFragment.c_str (),
							pOldSdpStream->IcePassword.c_str (), pSdpStream->IcePassword.c_str ());

#ifndef GDPR_COMPLIANCE
						traceBuildup << "[" << message << "]";
#endif

						message.append("\n");
						stiTrace (message.c_str ());
					}
				}
			}

			// Parse any SDES keys, if there are any
			pSdpStream->sdesKeys = SDESKeysGet(pMediaDescriptor);
			traceBuildup << "[has sdes keys: " << (pSdpStream->sdesKeys.empty () ? "no" : "yes") << "]";

			if (!sipCall->disableDtlsGet ())
			{
				vpe::HashFuncEnum streamHashFunc{};
				std::vector<uint8_t> streamFingerprint;

				std::tie(streamHashFunc, streamFingerprint) = dtlsParametersParse(pMediaDescriptor);

				traceBuildup << "[handling dtls]";

				if (streamHashFunc != vpe::HashFuncEnum::Undefined)
				{
					dtlsParametersSet(pSdpStream, streamHashFunc, streamFingerprint, pMediaDescriptor);
					traceBuildup << "[streamHashFunc " << (int)streamHashFunc << "]";
				}
				else if (sessionHashFunc != vpe::HashFuncEnum::Undefined)
				{
					dtlsParametersSet(pSdpStream, sessionHashFunc, sessionFingerprint, pMediaDescriptor);
					traceBuildup << "[sessionHashFunc " << (int)sessionHashFunc << "]";
				}
				else
				{
					traceBuildup << "[unknown hash function]";
					dtlsParametersSet(pSdpStream, vpe::HashFuncEnum::Undefined, {}, pMediaDescriptor);
				}

				if (!pSdpStream->Fingerprint.empty () && pSdpStream->FingerprintHashFunction != vpe::HashFuncEnum::Undefined)
				{
					if (MediaProtocol == RV_SDPPROTOCOL_RTP_SAVP || sipCall->includeEncryptionAttributes ())
					{
						dtlsContextGet ()->remoteFingerprintSet (pSdpStream->Fingerprint);
						dtlsContextGet ()->remoteFingerprintHashFuncSet (pSdpStream->FingerprintHashFunction);
						dtlsContextGet ()->remoteKeyExchangeModeSet (vpe::DtlsContext::setupRoleToKeyExchangeMode (pSdpStream->SetupRole));
					}
				}
			}

			if (encryptionRequiredGet ())
			{
				stiTESTCOND (!pSdpStream->sdesKeys.empty () || !pSdpStream->Fingerprint.empty (), stiRESULT_SECURITY_INADEQUATE);
			}
		}
		else
		{
			pSdpStream->IceUserFragment = SessionIceUserFragment;
			pSdpStream->IcePassword = SessionIcePassword;
		}

		//
		// If we are at the end of the list then add the new SDP entry to the end.
		// If we are not at the end of the list then replace the current entry with the new SDP entry.
		//
		if (SdpStreamsIter == m_SdpStreams.end ())
		{
			//
			// The logic in the loop requires that the iterator remain correct. Using
			// push_back seems to invalidate the iterator. Therefore, we use an insert
			// here instead to keep the iterator updated.
			//
			SdpStreamsIter = m_SdpStreams.insert (SdpStreamsIter, std::move(pSdpStream));
		}
		else
		{
			*SdpStreamsIter = std::move(pSdpStream);
		}
	} // end i (SDP 'm' lines)

	// Now that we have all of their m-lines, make sure our local m-lines are up to date
	localStreamsFromRemoteStreamsUpdate();

	// Only a re-invite (not an ok) is allowed to change the hold state!
	if (eSDPRole == estiOffer)
	{
		sipCall->HoldStateSet (false, bAllChannelsHeld);
		sipCall->HoldStateComplete (false);
	}

	// if this is a bridged call, we can bridge audio
	// only and don't need to test that there is a video
	// channel
	if (!sipCall->m_bIsBridgedCall && !bHasVideoMediaLine)
	{
		stiRemoteLogEventSend ("EventType=SPIT Reason=NoVideo");
		
		// Don't log SPIT.
		if (call)
		{
			call->logCallSet (false);
		}
		
		//
		// Silently return an error since we logged this call as SPIT.
		//
		stiTHROW_NOLOG (stiRESULT_ERROR);
	}

	//
	// If the remote text capabilities have changed update them.
	//
	if (sipCall->TextSupportedGet () != bTextSupported)
	{
		sipCall->RemoteTextSupportedSet(bTextSupported);

		// If the caller wants to know if media capabilities have changed
		// notify them.
		if (pbCallInfoChanged)
		{
			*pbCallInfoChanged = true;
		}
	}

	fallbackVcoSet(sdpHasSinfo, audioSupported, pbCallInfoChanged);

	//
	// If the caller wants to know if ICE should be restarted then
	// pass back the determined value.
	//
	if (pbRestartICE)
	{
		*pbRestartICE = bRestartICE;
	}
	
	if (m_pIceSession)
	{
		m_pIceSession->m_remoteSdp = Sdp;
	}

STI_BAIL:

	traceBuildup << "\nresult " << hResult;
	call->collectTrace (formattedString ("%s: role %d\n%s", __func__, eSDPRole, traceBuildup.str ().c_str ()));

	return hResult;
}

/*!
 * \brief helper method to possibly set the VCO preference when SINFO SDP attribute not found
 *
 * \return whether VCO preference was set
 */
void CstiSipCallLeg::fallbackVcoSet(bool sdpHasSinfo, bool audioSupported, bool * callInfoChanged)
{
	// SDP is missing the SINFO private attribute to determine whether vco is enabled or not
	// enable it if audio is supported
	if (!sdpHasSinfo && audioSupported && !m_bSInfoRecvd)
	{
		auto sipCall = m_sipCallWeak.lock ();
		auto call = sipCall->CstiCallGet();

		// Only set if it's not already set
		if (call->RemoteIsVcoActiveGet() != estiTRUE)
		{
			// Enable by default (guided by "simplified vco")
			call->RemoteIsVcoActiveSet(estiTRUE);

			// Set to use one line vco since there are no preferences
			call->RemoteVcoTypeSet(estiVCO_ONE_LINE);

			// If the caller wants to know if media capabilities have changed
			// notify them.
			if (callInfoChanged)
			{
				*callInfoChanged = true;
			}
		}
	}
}


/*!
 * \brief Helper method to sdpParse for parsing Rtcp Feedback attributes
 *        and mapping payload id's to supported feedback mechanisms
 */
bool CstiSipCallLeg::rtcpFbAttrParse (
	RvSdpAttribute *attribute,
	RvSdpMediaDescr *mediaDescriptor,
	std::map<int8_t, std::list<RtcpFeedbackType>> *rtcpFeedbackMap,
	EstiSDPRole role)
{
	auto sipCall = m_sipCallWeak.lock ();
	auto *attrValue = rvSdpAttributeGetValue (attribute);
	const int minRtcpFbSize = 2;
	// Do nothing if the attribute is invalid
	if (!attrValue)
	{
		return false;
	}

	// Split the attribute value into space-delimited strings
	auto valueTokens = splitString (attrValue, ' ');
	
	if (valueTokens.size() < minRtcpFbSize)
	{
		stiASSERTMSG (false, "Unknown rtcp-fb attribute: %s", attrValue);
		return false;
	}
	
	const std::string &payloadIdStr = valueTokens[0];  // (*|<int8_t>)
	const std::string &fbCategory   = valueTokens[1];  // (ccm|ack|nack)
	const std::string &fbType       = valueTokens.size() > minRtcpFbSize ? valueTokens[2] : "";  // (tmmbr|fir|pli)

	RtcpFeedbackType feedbackType = RtcpFeedbackType::UNKNOWN;

	// Set the feedback type, only if that mechanism isn't configured to be disabled
	if (sipCall->m_stConferenceParams.rtcpFeedbackTmmbrEnabled !=  SignalingSupport::Disabled &&
		fbCategory == "ccm" && fbType == "tmmbr")
	{
		feedbackType = RtcpFeedbackType::TMMBR;
		m_tmmbrOffered = (role == estiOffer);
	}
	else if (sipCall->m_stConferenceParams.rtcpFeedbackFirEnabled &&
			 fbCategory == "ccm"  && fbType == "fir")
	{
		feedbackType = RtcpFeedbackType::FIR;
		m_firOffered = (role == estiOffer);
	}
	else if (sipCall->m_stConferenceParams.rtcpFeedbackPliEnabled &&
			 fbCategory == "nack" && fbType == "pli")
	{
		feedbackType = RtcpFeedbackType::PLI;
		m_pliOffered = (role == estiOffer);
	}
	else if (sipCall->m_stConferenceParams.rtcpFeedbackNackRtxSupport != SignalingSupport::Disabled &&
			 fbCategory == "nack" && fbType.empty())
	{
		feedbackType = RtcpFeedbackType::NACKRTX;
		m_nackOffered = (role == estiOffer);
	}
	else
	{
		return false;
	}

	if (feedbackType != RtcpFeedbackType::UNKNOWN)
	{
		// * means we should enable this feedback mechanism for all payload types
		if (payloadIdStr == "*")
		{
			int payloadCount = rvSdpMediaDescrGetNumOfPayloads (mediaDescriptor);
			for (int i = 0; i < payloadCount; i++)
			{
				int payloadId = rvSdpMediaDescrGetPayload (mediaDescriptor, i);
				(*rtcpFeedbackMap)[payloadId].push_back (feedbackType);
			}
		}
		// Add feedback support for specific payload type
		else
		{
			auto payloadId = static_cast<int8_t>(strtol(payloadIdStr.c_str(), nullptr, 10));
			(*rtcpFeedbackMap)[payloadId].push_back (feedbackType);
		}
	}

	return true;
}


#define MAX_VERSIONID_DIGITS 10
/*!\brief Add the message scope SDP information to the SDP message object passed in.
 *
 * \param pSdp The SDP message object to which the SDP header is added.
 *
 * \returns Returns success or failure in an stiHResult.
 */
stiHResult CstiSipCallLeg::SdpHeaderAdd (
	CstiSdp *pSdp)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char szVersionId[MAX_VERSIONID_DIGITS + 1];
	std::string EncryptedSInfo;
	std::stringstream displaySizeStream;
	unsigned int preferredDisplayWidth = 0;
	unsigned int preferredDisplayHeight = 0;
	auto sipCall = m_sipCallWeak.lock ();

	sprintf (szVersionId, "%u", m_un32SdpVersionId);

	//
	// Get the Public IP.  If we are tunneling then retrieve the public
	// IP from the tunneling client
	//
	std::string PublicIpAddr;
	RvSdpAddrType eRvSdpAddrType = RV_SDPADDRTYPE_IP4;
	if (sipCall->SipManagerGet ()->TunnelActive ())
	{
		PublicIpAddr.assign (sipCall->m_stConferenceParams.TunneledIpAddr);
		eRvSdpAddrType = IPv4AddressValidate (PublicIpAddr.c_str ()) ? RV_SDPADDRTYPE_IP4 : RV_SDPADDRTYPE_IP6;
	}
	else
	{
		// Only try using an IPv6 if we do not have an IPv4 address.
		// We will check if the IPv6 address we have is a NAT64 address
		// and use the IPv4 address if it converts to one. Fixes Bug# 25960.
		if (!sipCall->m_stConferenceParams.PublicIPv4.empty () && !sipCall->IsBridgedCall ())
		{
			PublicIpAddr.assign (sipCall->m_stConferenceParams.PublicIPv4);
			eRvSdpAddrType = RV_SDPADDRTYPE_IP4;
		}
		else if (sipCall->m_eIpAddressType == estiTYPE_IPV6 &&
				 !sipCall->m_stConferenceParams.PublicIPv6.empty () && !sipCall->IsBridgedCall ())
		{
			std::string mappedIpAddress;
			std::string localIPv6Address = sipCall->m_stConferenceParams.PublicIPv6;
			
			stiMapIPv6AddressToIPv4 (localIPv6Address, &mappedIpAddress);
			
			if (!mappedIpAddress.empty ())
			{
				PublicIpAddr.assign (mappedIpAddress);
				eRvSdpAddrType = RV_SDPADDRTYPE_IP4;
			}
			else
			{
				PublicIpAddr.assign (sipCall->m_stConferenceParams.PublicIPv6);
				eRvSdpAddrType = RV_SDPADDRTYPE_IP6;
			}
		}
		else
		{
			PublicIpAddr = sipCall->localIpAddressGet();
			stiTESTCOND (!PublicIpAddr.empty(), stiRESULT_ERROR);

			eRvSdpAddrType = RV_SDPADDRTYPE_IP4;
		}
	}


	hResult = pSdp->originSet ("-", sipCall->m_szSessionId,
			szVersionId, RV_SDPNETTYPE_IN,
			eRvSdpAddrType,
			(RvChar*)PublicIpAddr.c_str ());
	stiTESTRESULT ();

	// Add session name.  Use "-" for SIP; see rfc 3264 section 5
	hResult = pSdp->sessionNameSet ("-");
	stiTESTRESULT ();

	// Connection information
	hResult = pSdp->connectionSet (RV_SDPNETTYPE_IN,
			eRvSdpAddrType,
			(RvChar*)PublicIpAddr.c_str ());
	stiTESTRESULT ();

	// Add the session time.  Use "0 0" for SIP;  see rfc 3264 section 5
	pSdp->sessionTimeAdd (0, 0);

	//
	// Add the bandwidth.
	// If we have a video playback object get the maximum band width from it.  Otherwise, use the max receive speed settings.
	//
	if (sipCall->m_videoPlayback)
	{
		pSdp->bandwidthAdd ("AS", sipCall->m_videoPlayback->FlowControlRateGet () / 1000);
	}
	else
	{
		pSdp->bandwidthAdd ("AS", sipCall->m_stConferenceParams.GetMaxRecvSpeed(estiSIP) / 1000);
	}

	//
	// Add ICE message attributes
	//
	if (m_IceSdpAttributes.pMsgAttributes != nullptr)
	{
		RvSdpListIter ListIterator;
		RvSdpAttribute *pAttribute = nullptr;
		const char *pAttrName = nullptr;
		const char *pAttrValue = nullptr;

		//
		// Add the attributes from the ICE media attributes to the SDP message.
		//
		pAttribute = rvSdpMsgGetFirstAttribute (m_IceSdpAttributes.pMsgAttributes, &ListIterator);

		if (pAttribute)
		{
			for (;;)
			{
				pAttrName = rvSdpAttributeGetName (pAttribute);
				pAttrValue = rvSdpAttributeGetValue (pAttribute);

				pSdp->attributeAdd (pAttrName, pAttrValue);

				//
				// Get the next attribute
				//
				pAttribute = rvSdpMsgGetNextAttribute (&ListIterator);

				if (pAttribute == nullptr)
				{
					break;
				}
			}
		}
	}
	if (sipCall->m_poSipManager->productGet () == ProductType::MediaServer)
	{
		std::string IntendedDialString;
		sipCall->CstiCallGet ()->IntendedDialStringGet (&IntendedDialString);
		if (IntendedDialString[0] == '+')
		{
			IntendedDialString = IntendedDialString.substr (1);
		}
		EncryptString (IntendedDialString, &EncryptedSInfo);
		pSdp->attributeAdd (g_szSDPSVRSMailPrivateHeader, EncryptedSInfo);
	}
	else if (!sipCall->IsBridgedCall())
	{
		std::string systemInfo;

		//
		// Add SINFO private header
		//
		hResult = SystemInfoSerialize (&systemInfo, sipCall,
			sipCall->SipManagerGet ()->m_bDefaultProviderAgreementSigned,
			sipCall->SipManagerGet ()->m_eLocalInterfaceMode,
			sipCall->m_stConferenceParams.AutoDetectedPublicIp.c_str (),
			sipCall->m_stConferenceParams.ePreferredVCOType,
			sipCall->m_stConferenceParams.vcoNumber.c_str (),
			sipCall->m_stConferenceParams.bVerifyAddress,
			sipCall->SipManagerGet ()->productNameGet ().c_str (),
			sipCall->SipManagerGet ()->m_version.c_str (),
			g_nSVRSSIPVersion,
			sipCall->m_stConferenceParams.bBlockCallerID,
			sipCall->m_stConferenceParams.eAutoSpeedSetting);
		stiTESTRESULT ();

		EncryptString (systemInfo, &EncryptedSInfo);

		pSdp->attributeAdd (g_szSDPSVRSSinfoPrivateHeader, EncryptedSInfo);

		IstiVideoOutput::InstanceGet ()->PreferredDisplaySizeGet (
			&preferredDisplayWidth,
			&preferredDisplayHeight);

		if (preferredDisplayWidth != 0
			&& preferredDisplayHeight != 0)
		{
			sipCall->CstiCallGet ()->InitialPreferredDisplaySizeLog ();
			displaySizeStream << preferredDisplayWidth << ":";
			displaySizeStream << preferredDisplayHeight;
			pSdp->attributeAdd (g_svrsDisplaySize, displaySizeStream.str ());
		}
	}

STI_BAIL:

	return hResult;
}
#undef MAX_VERSIONID_DIGITS


/*!\brief Read SInfo from the private header
 *
 * \param hInboundMsg The message with the private header to read
 */
stiHResult CstiSipCallLeg::SInfoHeaderRead (
	const vpe::sip::Message &inboundMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();

	//
	// Check to see if there is an SInfo private header
	//
	std::string encryptedSInfo;
	std::string decryptedSInfo;
	bool present {false};

	std::tie(encryptedSInfo, present) = inboundMessage.headerValueGet(g_szSVRSSInfoPrivateHeader);

	if (present)
	{
		if (encryptedSInfo.rfind(stiINFO_HEADER_CSTR, 0) == 0)
		{
			decryptedSInfo = encryptedSInfo;
		}
		else
		{
			DecryptString((unsigned char*)encryptedSInfo.c_str(), encryptedSInfo.size(), &decryptedSInfo);
		}
		
		if (decryptedSInfo != m_SInfo)
		{
			m_SInfo = decryptedSInfo;
			SystemInfoDeserialize (shared_from_this(), m_SInfo.c_str ());
		}
	}

	if (!m_bSInfoRecvd && !m_SInfo.empty())
	{
		// Indicate that we no longer need to wait for SInfo to arrive before displaying the incoming call.
		sipCall->AlertUserCountDownDecr();

		m_bSInfoRecvd = true;
		
		sipCall->SipManagerGet ()->callInformationChangedSignal.Emit (sipCall->CstiCallGet ());
	}

	return hResult;
}

stiHResult CstiSipCallLeg::SInfoHeaderRead (
	const RvSdpAttribute *pAttribute,
	bool *pbCallInfoChanged)
{
	std::string EncryptedSInfo = rvSdpAttributeGetValue(const_cast<RvSdpAttribute *>(pAttribute));
	std::string SInfo;

	stiHResult hResult = stiRESULT_SUCCESS;

	if (EncryptedSInfo.rfind(stiINFO_HEADER_CSTR, 0) == 0)
	{
		SInfo = EncryptedSInfo;
	}
	else
	{
		hResult = DecryptString((unsigned char*)EncryptedSInfo.c_str(), EncryptedSInfo.size(), &SInfo);
	}

	if (!stiIS_ERROR(hResult))
	{
		//
		// If the string is different from the last SInfo then deserialize it.
		//
		if (SInfo != m_SInfo)
		{
			m_SInfo = SInfo;

			hResult = SystemInfoDeserialize(shared_from_this(), m_SInfo.c_str());

			// If we have a pointer to the pbCallInfoChanged parameter, update it.
			if (pbCallInfoChanged)
			{
				*pbCallInfoChanged = true;
			}
		}
	}
	return hResult;
}

stiHResult CstiSipCallLeg::SInfoHeaderRead (const CstiSdp &sdp)
{
	stiHResult hResult = stiRESULT_VALUE_NOT_FOUND;
	for (size_t i = 0; i < sdp.numberOfAttributesGet(); i++)
	{
		auto pAttribute = sdp.attributeGet(i);
		if (strcmp(pAttribute->iAttrName, g_szSDPSVRSSinfoPrivateHeader) == 0)
		{
			hResult = SInfoHeaderRead (pAttribute, nullptr);
			break;
		}
	}
	return hResult;
}

/*!\brief Add the SInfo private header to the call leg's out bound message
 *
 */
stiHResult CstiSipCallLeg::SInfoHeaderAdd ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet();

	if (!m_bSInfoSent)
	{
		std::string systemInfo;

		hResult = SystemInfoSerialize (&systemInfo, sipCall,
			sipCall->SipManagerGet ()->m_bDefaultProviderAgreementSigned,
			sipCall->SipManagerGet ()->m_eLocalInterfaceMode,
			sipCall->m_stConferenceParams.AutoDetectedPublicIp.c_str (),
			sipCall->m_stConferenceParams.ePreferredVCOType,
			sipCall->m_stConferenceParams.vcoNumber.c_str (),
			sipCall->m_stConferenceParams.bVerifyAddress,
			sipCall->SipManagerGet ()->productNameGet().c_str(),
			sipCall->SipManagerGet ()->m_version.c_str(),
			g_nSVRSSIPVersion,
			sipCall->m_stConferenceParams.bBlockCallerID,
			sipCall->m_stConferenceParams.eAutoSpeedSetting);
		stiTESTRESULT ();

		hResult = outboundMessageGet().sinfoHeaderAdd(systemInfo);
		stiTESTRESULT ();

		m_bSInfoSent = true;
	}

STI_BAIL:

	return hResult;
} // end CstiSipManager::SInfoHeaderAdd


/*!\brief Retrieves the remote product name.
 *
 * \param pRemoteProductName Contains the remote product name upon return
 */
void CstiSipCallLeg::RemoteProductNameGet (
	std::string *pRemoteProductName) const
{
	*pRemoteProductName = m_RemoteProductName;
}


/*!\brief Sets the remote product name.
 *
 * \param pszRemoteProductName The remote product name to set
 */
void CstiSipCallLeg::RemoteProductNameSet (
	const char *pszRemoteProductName)
{
	m_RemoteProductName = pszRemoteProductName;
}


/*!\brief Retrieves the remote product version
 *
 * \param pRemoteProductVersion Contains the remote product version upon return
 */
void CstiSipCallLeg::RemoteProductVersionGet (
	std::string *pRemoteProductVersion) const
{
	*pRemoteProductVersion = m_RemoteProductVersion;
};


/*!\brief Sets the remote product version.
 *
 * \param pszRemoteProductVersion The remote product version to set
 */
void CstiSipCallLeg::RemoteProductVersionSet (
	const char *pszRemoteProductVersion)
{
	m_RemoteProductVersion = pszRemoteProductVersion;
};


/*!\brief Sets the remote SIP version.
 *
 * \param nRemoteSIPVersion The remote SIP version to set
 */
void CstiSipCallLeg::RemoteSIPVersionSet (
	int nRemoteSIPVersion)
{
	m_nRemoteSIPVersion = nRemoteSIPVersion;
}


/*!\brief Retrieves the remote auto speed setting
 *
 * \param peAutoSpeedSetting contains the remote auto speed setting upon return
 */
void CstiSipCallLeg::RemoteAutoSpeedSettingGet (
	EstiAutoSpeedMode *peAutoSpeedSetting) const
{
	*peAutoSpeedSetting = m_eRemoteAutoSpeedSetting;
};


/*!\brief Sets the remote auto speed setting.
 *
 * \param eRemoteAutoSpeedSetting The remote auto speed setting to set
 */
void CstiSipCallLeg::RemoteAutoSpeedSettingSet (
	EstiAutoSpeedMode eRemoteAutoSpeedSetting)
{
	m_eRemoteAutoSpeedSetting = eRemoteAutoSpeedSetting;
}


/*!\brief Sets the remote UA name that is found in the inbound messsage
 *
 * \param inboundMessage The inbound message containing the user agent
 */
void CstiSipCallLeg::remoteUANameSet(
	const vpe::sip::Message &inboundMessage)
{
	auto userAgent = inboundMessage.userAgentGet ();
	auto remoteProductName = std::get<0> (userAgent);
	auto remoteProductVersion = std::get<1> (userAgent);
	auto remoteSIPVersion = std::get<2> (userAgent);
	auto userAgentHeader = std::get<3> (userAgent);
	
	if (userAgentHeader)
	{
		if (!remoteProductName.empty ())
		{
			RemoteProductNameSet (remoteProductName.c_str ());
		}
		if (!remoteProductVersion.empty ())
		{
			RemoteProductVersionSet (remoteProductVersion.c_str ());
		}
		if (remoteSIPVersion)
		{
			RemoteSIPVersionSet (remoteSIPVersion);
		}
	}
}

/*!\brief Is the remote device type of the type passed in the parameter list?
 *
 */
bool CstiSipCallLeg::RemoteDeviceTypeIs (
	int nDeviceType) const
{
	EstiBool bCheck = estiFALSE;

	std::string RemoteProductName;

	RemoteProductNameGet (&RemoteProductName);

	if (!RemoteProductName.empty ())
	{
		switch (nDeviceType)
		{
			case estiDEVICE_TYPE_SVRS_VIDEOPHONE:
				if (0 == stiStrNICmp (
					szPRODUCT_FAMILY_NAME,
					RemoteProductName.c_str (),
					sizeof (szPRODUCT_FAMILY_NAME) - 1))
				{
					bCheck = estiTRUE;
				}

				break;

			case estiDEVICE_TYPE_SVRS_HOLD_SERVER:

				if (RemoteProductName == g_szHoldServer)
				{
					bCheck = estiTRUE;
				}

				break;

			case estiDEVICE_TYPE_SVRS_DEVICE:
				if (0 == strncmp (g_szSVRS_DEVICE, RemoteProductName.c_str (), strlen (g_szSVRS_DEVICE)))
				{
					bCheck = estiTRUE;
				}

				break;

	/*			case estiDEVICE_TYPE_SVRS_VIDEO_SERVER:

					if (RemoteProductName == g_szVideoServer)
					{
						bCheck = estiTRUE;
					}

					break;*/
			case estiDEVICE_TYPE_SVRS_VP200:

				if (0 == stiStrICmp ("Sorenson Videophone V2", RemoteProductName.c_str ()))
				{
					bCheck = estiTRUE;
				}

				break;
		}
	}

	return bCheck;
}


stiHResult CstiSipCallLeg::SdpProcess (
	CstiSdp *pSdp,
	EstiOfferAnswerMessage eMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (m_bAnswerPending)
	{
		//
		// Call SDPAnswerProcess in case there is SDP attached to this provisional response.
		//
		hResult = SdpAnswerProcess (*pSdp, eMessage);
	}
	else
	{
		SdpOfferProcess (pSdp, eMessage);
	}

	return hResult;
}


stiHResult CstiSipCallLeg::SdpOfferProcess (
	CstiSdp *pSdp,
	EstiOfferAnswerMessage eMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();
	RvStatus rvStatus = RV_OK;
	bool bRestartICE = false;
	bool bCallInfoChanged = false;

	if (pSdp->empty ())
	{
		//
		// There is no SDP.  This is allowed only in an INVITE request.
		//
		stiTESTCOND (eMessage == estiOfferAnswerINVITE, stiRESULT_ERROR);

		switch (sipCall->CstiCallGet ()->StateGet ())
		{
			case esmiCS_IDLE:
			case esmiCS_CONNECTING:
			{
				//
				// We received an empty INVITE (no SDP).
				// We must generate an offer and send it back in our first reliable provisional response (preferable)
				// or let this endpoint ring and send the offer back when the user answers (not the best option).
				//
				RvSipTransaction100RelStatus eStatus;
				RvSipCallLegGet100RelStatus (m_hCallLeg, &eStatus);
				if (RVSIP_TRANSC_100_REL_UNDEFINED != eStatus)
				{
					//
					// The remote endpoint supports reliable provisional responses.  We can
					// generate an offer and send it in a reliable provisional response (such as a 180 or 183)
					//

					//
					// Only do ICE if transport is not UDP and we are not tunneling.
					//
					if (sipCall->UseIceForCalls ())
					{
						// Attach a SDP media info to the outgoing INVITE

						if (m_pIceSession)
						{
							m_pIceSession->SessionEnd ();
							m_pIceSession = nullptr;
						}

						hResult = sipCall->InitialSdpOfferCreate (&m_sdp, false);
						stiTESTRESULT ();

						m_eNextICEMessage = estiOfferAnswerReliableResponse;

						hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleOfferer, m_sdp, nullptr, shared_from_this());
						stiTESTRESULT ();
					}
					else
					{
						stiASSERT (estiFALSE); // Need to implement this

						//
						// If we are using NAT Traversal (and not Tunneling) then we will
						// gather our server reflexive and TURN allocations before responding.
						//
						if (sipCall->SipManagerGet ()->m_stConferenceParams.eNatTraversalSIP != estiSIPNATDisabled
						 && sipCall->UseIceForCalls ())
						{

						}
						else
						{
							CstiSdp SdpAnswer;

							// Attach a SDP media info to the outgoing INVITE
							hResult = sipCall->InitialSdpOfferCreate (&SdpAnswer, true);
							stiTESTRESULT ();

							hResult = sipCall->SipManagerGet ()->InviteSend (sipCall, &SdpAnswer);
							stiTESTRESULT ();
						}
					}
				}
				else
				{
					//
					// The remote endpoint doesn't support reliable provisional responses.  We will
					// have to let this endpoint ring and send the offer in the 200 OK.
					//

					//
					// If we are using NAT Traversal (and not Tunneling) then we will
					// gather our server reflexive and TURN allocations before responding.
					//
					if (sipCall->SipManagerGet ()->m_stConferenceParams.eNatTraversalSIP != estiSIPNATDisabled
					 && sipCall->UseIceForCalls ())
					{
						hResult = sipCall->InitialSdpOfferCreate (&m_sdp, false);
						stiTESTRESULT ();

						//
						// If the remote endpoint supports reliable provisional responses then
						// send the answer containing the ICE candidates in a reliable provisional response
						// Otherwise, send the offer in the final resposne to the INVITE request.
						//
						RvSipTransaction100RelStatus eStatus;
						RvSipCallLegGet100RelStatus (m_hCallLeg, &eStatus);
						if (RVSIP_TRANSC_100_REL_UNDEFINED != eStatus)
						{
							m_eNextICEMessage = estiOfferAnswerReliableResponse;
						}
						else
						{
							m_eNextICEMessage = estiOfferAnswerINVITEFinalResponse;
						}

						hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleGatherer, m_sdp, nullptr, shared_from_this());
						stiTESTRESULT ();
					}
					else
					{
						RvSipCallLegProvisionalResponse (m_hCallLeg, SIPRESP_RINGING);
						sipCall->AlertLocalUser ();
					}
				}

				break;
			}

			case esmiCS_CONNECTED:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			{
				//
				// We received an empty re-invite (no SDP)
				// We must restart ICE negotiations.
				//
				if (sipCall->UseIceForCalls ())
				{
					if (m_pIceSession)
					{
						m_pIceSession->SessionEnd ();
						m_pIceSession = nullptr;
					}

					hResult = SdpCreate (&m_sdp, estiOffer);
					stiTESTRESULT ();

					m_eNextICEMessage = estiOfferAnswerINVITEFinalResponse;

					hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleOfferer, m_sdp, nullptr, shared_from_this());
					stiTESTRESULT ();
				}
				else
				{
					//
					// Get the outbound message and add the SDP
					//
					hResult = SdpCreate (&m_sdp, estiOffer);
					stiTESTRESULT ();

					hResult = outboundMessageGet().bodySet(m_sdp);
					stiTESTRESULT ();

					rvStatus = RvSipCallLegAccept (m_hCallLeg);
					stiTESTRVSTATUS ();

					m_bAnswerPending = true;
				}

				break;
			}

			case esmiCS_UNKNOWN:
			case esmiCS_DISCONNECTING:
			case esmiCS_DISCONNECTED:
			case esmiCS_CRITICAL_ERROR:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:
				DBG_MSG ("Unable to accept re-invite in current state");
				RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
				break;
		}
	}
	else
	{
		//
		// We received a new offer so our offer/answer transaction is no longer complete.
		//
		m_bOfferAnswerComplete = false;

		//
		// Parse the message they gave us
		//
		hResult = SdpParse (*pSdp, estiOffer, &bRestartICE, &bCallInfoChanged);
		stiTESTRESULT ();

		switch (sipCall->CstiCallGet ()->StateGet ())
		{
			case esmiCS_IDLE:
			case esmiCS_CONNECTING:

				//
				// The call is being setup.
				//

				if (eMessage == estiOfferAnswerINVITE)
				{
					//
					// This is the initial invite with SDP
					//
					int nAlertUserCount = 0;
					bool bSendRingingResponse = false;

					if (sipCall->UseIceForCalls ())
					{
						//
						// We need to make sure we don't pass into the ICE session an SDP that contains streams that we don't support.
						// Therefore, ask the call leg which streams are supported in the offer.  We will disable each one that is not
						// supported before passing the SDP to the ICE session.
						//
						UnsupportedStreamsDisable(pSdp);

						//
						// If the remote endpoint supports reliable provisional responses then
						// send the answer containing the ICE candidates in a reliable provisional response
						// Otherwise, send the offer in the final resposne to the INVITE request.
						//
						RvSipTransaction100RelStatus eStatus;
						RvSipCallLegGet100RelStatus (m_hCallLeg, &eStatus);
						if (RVSIP_TRANSC_100_REL_UNDEFINED != eStatus)
						{
							m_eNextICEMessage = estiOfferAnswerReliableResponse;
						}
						else
						{
							m_eNextICEMessage = estiOfferAnswerINVITEFinalResponse;
						}

						hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleAnswerer, *pSdp, &m_remoteIceSupport, shared_from_this());

						// Call should be rejected since it will most likely fail when pushed to an interpreter.
						if (stiRESULT_CODE (hResult) == stiRESULT_INVALID_MEDIA)
						{
							// Propagate the error so that the call will be ended.
							stiTESTRESULT ();
						}
						else if (stiIS_ERROR (hResult) || m_remoteIceSupport != IceSupport::Supported)
						{
							//
							// Failure means that the remote endpoint does not support ICE (at least not correctly).
							//
							hResult = stiRESULT_SUCCESS;

							if (!sipCall->SipManagerGet ()->TunnelActive ())
							{
								//
								// A non-ICE client called us and we are not using Tunneling.  We need to allocate ports on the TURN server
								// and/or find out our server reflexive ports.  To do this we are going to start an ICE session
								// as if we are the caller just so that we can grab the transports.
								//
								hResult = sipCall->SipManagerGet ()->TransportsGather (shared_from_this());

								if (stiIS_ERROR (hResult))
								{
									hResult = stiRESULT_SUCCESS;

									bSendRingingResponse = true;
								}
								else
								{
									// We need to increment our counter to delay displaying the incoming call until we are ready.
									// When ICE gathering is complete we will decrement the counter.
									++nAlertUserCount;
								}
							}
							else
							{
								//
								// A non-ICE client called us and we are using Tunneling.  Just respond with ringing.
								//
								bSendRingingResponse = true;
							}
						}
						else
						{
							m_bRemoteIsICE = true;

							//
							// If the remote is sorenson sip version 3 then
							// we need to wait for nominations to complete before
							// alerting the user.
							//
							if (m_nRemoteSIPVersion == 3)
							{
								++nAlertUserCount;
							}
							
							// Increment only for Sorenson devices, otherwise incoming calls from 3rd party devices that support ICE and
							// reliable povisional responses (100rel) will never ring.
							if (RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE))
							{
								//
								// Increment the counter to delay alerting the user of the incoming call until we have received SInfo data.
								//
								++nAlertUserCount;
							}
						}
					}
					else
					{
						bSendRingingResponse = true;
					}

					if (bSendRingingResponse)
					{
						//
						// Indicate that we are ringing.
						//

						//
						// If the remote endpoint supports reliable provisional responses
						// and it is a Sorenson endpoint then send SInfo with the ringing since it is reliable.
						//
						RvSipTransaction100RelStatus eStatus;
						RvSipCallLegGet100RelStatus (m_hCallLeg, &eStatus);
						if (RVSIP_TRANSC_100_REL_UNDEFINED != eStatus
						 && RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE)
						 && m_nRemoteSIPVersion >= 2)
						{
							hResult = SInfoHeaderAdd ();
							stiTESTRESULT ();

							hResult = outboundMessageGet ().inviteRequirementsAdd ();
							stiTESTRESULT ();

							ReliableResponseSend (SIPRESP_RINGING, nullptr);

							// If the remote device is a Sorenson endpoint we increment this counter by one.
							// This is to delay alerting the user of the incoming call until we have received SInfo data.
							++nAlertUserCount;
						}
						else
						{
							RvSipCallLegProvisionalResponse (m_hCallLeg, SIPRESP_RINGING);
							sipCall->AlertLocalUser ();
						}
					}

					//
					// If the "Alert Local User" boolean is set then we don't want to use the count down
					// as we will alert the local user on receipt of the PRACK to our RINGING response.
					//
					if (!sipCall->AlertLocalUserGet ())
					{
						sipCall->AlertUserCountDownSet (nAlertUserCount);
					}
				}
				else
				{
					//
					// The offer, during call setup, has arrived in a non-Invite message (a PRACK or an UPDATE request)
					//

					//
					// If we need to restart ICE then do so.
					//
					if (bRestartICE &&
					  sipCall->UseIceForCalls ())
					{
						//
						// We need to make sure we don't pass into the ICE session an SDP that contains streams that we don't support.
						// Therefore, ask the call leg which streams are supported in the offer.  We will disable each one that is not
						// supported before passing the SDP to the ICE session.
						//
						UnsupportedStreamsDisable(pSdp);

						//
						// Handle the offer based on the type of message it arrived in.
						//
						switch (eMessage)
						{
							case estiOfferAnswerReliableResponse:
							{
								m_eNextICEMessage = estiOfferAnswerPRACK;

								break;
							}

							case estiOfferAnswerPRACK:
							{
								m_eNextICEMessage = estiOfferAnswerPRACKFinalResponse;

								break;
							}

							case estiOfferAnswerINVITEFinalResponse:
							{
								m_eNextICEMessage = estiOfferAnswerACK;

								break;
							}

							case estiOfferAnswerNone:
							case estiOfferAnswerINVITE:
							case estiOfferAnswerACK:
							case estiOfferAnswerPRACKFinalResponse:
							case estiOfferAnswerUPDATE:
							case estiOfferAnswerUPDATEFinalResponse:

								stiASSERT (estiFALSE);

								m_eNextICEMessage = estiOfferAnswerNone;

								break;
						}
						
						if (m_pIceSession)
						{
							m_pIceSession->SdpReceived (*pSdp, &m_remoteIceSupport);
						}
						else
						{
							hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleAnswerer, *pSdp, &m_remoteIceSupport, shared_from_this());
							
							if (!stiIS_ERROR (hResult) && m_remoteIceSupport == IceSupport::Supported)
							{
								m_bRemoteIsICE = true;
							}
						}
					}
					else
					{
						//
						// We don't need to restart ICE.
						//
						// If we have an ICE session then pass the SDP to it.
						//
						if (m_pIceSession)
						{
							m_pIceSession->SdpReceived (*pSdp, &m_remoteIceSupport);

#if 0
							sipCall->m_bICEUpdateReceived = true;
#endif
						}

						//
						// Handle the offer based on the type of message it arrived in.
						//
						switch (eMessage)
						{
							case estiOfferAnswerPRACK:
							{
								//
								// We received an offer in a PRACK.  According to RFC 3262 we must respond with an answer in
								// the 200 OK.
								//

								//
								// Get the outbound message handle, add SDP to it and respond to the
								// PRACK with a 200 OK.
								//
								SdpResponseAdd ();

								m_bOfferAnswerComplete = true;

								RvSipCallLegSendPrackResponse (m_hCallLeg, SIPRESP_SUCCESS);

								break;
							}

							case estiOfferAnswerReliableResponse:
							{
								//
								// We received an offer in a Reliable Response.  According to RFC 3262 we MAY respond with an answer in
								// the PRACK.
								//

								//
								// Get the outbound message handle, add SDP to it and respond to the
								// provisional reliable response with a PRACK.
								//
								SdpResponseAdd ();

								m_bOfferAnswerComplete = true;

								RvSipCallLegSendPrack (m_hCallLeg);

								break;
							}

							case estiOfferAnswerINVITEFinalResponse:
							{
								//
								// We did not send an offer so this must be an offer sent to us.
								// We need to send our answer in the ACK.
								//
								SdpResponseAdd ();

								m_bOfferAnswerComplete = true;

								RvSipCallLegAck (m_hCallLeg);

								break;
							}

							case estiOfferAnswerUPDATE:
							case estiOfferAnswerNone:
							case estiOfferAnswerINVITE:
							case estiOfferAnswerACK:
							case estiOfferAnswerPRACKFinalResponse:
							case estiOfferAnswerUPDATEFinalResponse:
								stiASSERTMSG(false, "Offer recieved in invalid message: %d", eMessage);
						}
					}
				}

				break;

			case esmiCS_CONNECTED:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			{
				//
				// This is an established call
				//
				// The offer arrived in a re-invite or an update request
				//

				bool bSendAnswerNow = true;

				//
				// If we need to restart ICE then do so.
				//
				if (bRestartICE &&
				  sipCall->UseIceForCalls ())
				{
					//
					// We need to make sure we don't pass into the ICE session an SDP that contains streams that we don't support.
					// Therefore, ask the call leg which streams are supported in the offer.  We will disable each one that is not
					// supported before passing the SDP to the ICE session.
					//
					UnsupportedStreamsDisable(pSdp);

					if (eMessage == estiOfferAnswerINVITE)
					{
						m_eNextICEMessage = estiOfferAnswerINVITEFinalResponse;
					}
					else if (eMessage == estiOfferAnswerINVITEFinalResponse)
					{
						m_eNextICEMessage = estiOfferAnswerACK;
					}
					else
					{
						stiASSERT (estiFALSE);
					}
					
					if (m_pIceSession)
					{
						m_pIceSession->SdpReceived (*pSdp, &m_remoteIceSupport);
					}
					else
					{
						
						hResult = sipCall->SipManagerGet ()->IceSessionStart (CstiIceSession::estiICERoleAnswerer, *pSdp, &m_remoteIceSupport, shared_from_this());
						
						if (!stiIS_ERROR (hResult) && m_remoteIceSupport == IceSupport::Supported)
						{
							m_bRemoteIsICE = true;
						}
						else
						{
							
							//
							// If we are here we probably went from an ICE to non-ICE connection. We will gather new transports resolves bug 22361.
							//
							
							m_IceSdpAttributes.MediaAttributes.clear ();
							m_IceSdpAttributes.pMsgAttributes = nullptr;
							
							m_bRemoteIsICE = false;
							
							//
							// A non-ICE client called us and we are not using Tunneling.  We need to allocate ports on the TURN server
							// and/or find out our server reflexive ports.  To do this we are going to start an ICE session
							// as if we are the caller just so that we can grab the transports.
							//
							hResult = sipCall->SipManagerGet ()->TransportsGather (shared_from_this());
							
							// If we had a failure gathering our defualt transports we need to send an answer now.
							if (stiIS_ERROR (hResult))
							{
								bSendAnswerNow = true;
							}
						}
					}
				}

				if (bSendAnswerNow)
				{
					CstiSdp SdpAnswer;

					if (isDtlsNegotiated ())
					{
						sipCall->dtlsBegin (false);
					}
					else
					{
						// Modify our media channels
						hResult = sipCall->IncomingMediaUpdate ();
						stiASSERTRESULTMSG (hResult, "Playback media open FAILED");

						if (m_pIceSession && m_pIceSession->m_eState == CstiIceSession::estiNominating)
						{
							stiASSERTMSG (estiFALSE, "Received offer while still in nominating state");
						}

						hResult = sipCall->OutgoingMediaOpen (false);
						stiASSERTRESULTMSG (hResult, "Record media open FAILED");
					}

					// Send the OK response
					SIP_DBG_LESSER_EVENT_SEPARATORS("creating OK");

					hResult = SdpCreate (&SdpAnswer, estiAnswer);
					stiTESTRESULT ();

					if (m_pIceSession)
					{
						m_pIceSession->SdpUpdate (&SdpAnswer);
					}

					//
					// Get the outbound message and add the SDP
					//
					hResult = outboundMessageGet().inviteRequirementsAdd();
					stiTESTRESULT();

					hResult = outboundMessageGet().bodySet(SdpAnswer);
					stiTESTRESULT();

					rvStatus = RvSipCallLegAccept (m_hCallLeg);
					stiTESTRVSTATUS ();
				}

				// If media capabilities have changed send a message for the UI in case it needs to update.
				if (bCallInfoChanged)
				{
					sipCall->SipManagerGet ()->callInformationChangedSignal.Emit (sipCall->CstiCallGet ());
				}

				// If we have a record channel make sure it knows how we want to handle privacy.
				if (sipCall->m_videoRecord)
				{
					bool bSorensonPrivacy = sipCall->m_videoRecord->SorensonPrivacyGet ();

					if (bSorensonPrivacy != (RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE)))
					{
						sipCall->m_videoRecord->SorensonPrivacySet (RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE));
					}
				}

				break;
			}

			case esmiCS_UNKNOWN:
			case esmiCS_DISCONNECTING:
			case esmiCS_DISCONNECTED:
			case esmiCS_CRITICAL_ERROR:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:
				DBG_MSG ("Unable to accept re-invite in current state");
				RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
				break;
		} // end switch
	}
	
	if (m_pIceSession && !pSdp->empty ())
	{
		m_pIceSession->m_remoteSdp = *pSdp;
	}

	MediaServerDisconnect ();

STI_BAIL:

	return hResult;
}


stiHResult CstiSipCallLeg::SdpAnswerProcess (
	const CstiSdp &Sdp,
	EstiOfferAnswerMessage eMessage)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();
	bool bCallInfoChanged = false;

	//
	// If we are not waiting on an answer then throw an error.
	//
	stiTESTCOND (m_bAnswerPending, stiRESULT_ERROR);

	m_bAnswerPending = false;

	switch (sipCall->CstiCallGet ()->StateGet ())
	{
		case esmiCS_IDLE:
		case esmiCS_CONNECTING:
		{
			hResult = SdpParse (Sdp, estiAnswer, nullptr, nullptr);
			if (stiRESULT_CODE (hResult) == stiRESULT_SECURITY_INADEQUATE)
			{
				stiTHROW (stiRESULT_SECURITY_INADEQUATE);
			}

			if (m_pIceSession)
			{
				m_pIceSession->SdpReceived (Sdp, &m_remoteIceSupport);

				if (m_remoteIceSupport == IceSupport::Supported)
				{
					//
					// Determine if any of the media streams have been disabled.  If one is found
					// then disable it also in the SDP for the updated offer
					//
					int nNbrMedia = Sdp.numberOfMediaDescriptorsGet();

					for (int i = 0; i < nNbrMedia; i++) // for each 'm' line
					{
						const RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorGet(i);

						int nRtpPort = rvSdpMediaDescrGetPort (pMediaDescriptor);

						if (nRtpPort == 0)
						{
							//
							// Disable the corresponding media stream for the updated offer
							//
							RvSdpMediaDescr *pOfferMediaDescriptor = m_sdp.mediaDescriptorGet(i);

							if (pOfferMediaDescriptor)
							{
								rvSdpMediaDescrSetPort (pOfferMediaDescriptor, 0);
							}
							else
							{
								stiASSERTMSG (false, "CallState = %d AnswerMediaDescriptors = %d OfferMediaDescriptors = %d", sipCall->CstiCallGet ()->StateGet (), nNbrMedia, m_sdp.numberOfMediaDescriptorsGet ());
							}
						}
					}

					m_bRemoteIsICE = true;

					if (m_pIceSession->m_eState == CstiIceSession::estiGatheringComplete)
					{
						m_pIceSession->SessionProceed ();
					}
				}
			}

			switch (eMessage)
			{
				case estiOfferAnswerPRACK:
				{
					//
					// According to RFC 6337, if an answer arrives in a PRACK we cannot send a new offer in the 200 OK to the PRACK
					// so just send a 200 OK with no new offer.
					//
					RvSipCallLegSendPrackResponse (m_hCallLeg, SIPRESP_SUCCESS);

					break;
				}

				case estiOfferAnswerINVITEFinalResponse:
				{
					RvSipCallLegAck (m_hCallLeg);

					break;
				}

				case estiOfferAnswerReliableResponse:
				{
					//
					// If we are using ICE and the remote endpoint is using ICE
					// then wait until the nominations are complete before sending the 
					// PRACK so that we can send the SDP with it.
					//
#if 0
					if (m_pIceSession && m_bRemoteIsICE && (!RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE) || m_nRemoteSIPVersion >= 4))
					{
						m_bSendNewOfferInPRACK = true;
					}
					else
#endif
					{
						RvSipCallLegSendPrack (m_hCallLeg);
					}

					break;
				}

				case estiOfferAnswerPRACKFinalResponse:

					//
					// Nothing needs to be done in this case.
					//

					break;

				case estiOfferAnswerNone:
				case estiOfferAnswerINVITE:
				case estiOfferAnswerACK:
				case estiOfferAnswerUPDATE:
				case estiOfferAnswerUPDATEFinalResponse:

					stiASSERT (estiFALSE);

					break;

			}

			break;
		}

		case esmiCS_CONNECTED:
		case esmiCS_HOLD_LCL:
		case esmiCS_HOLD_RMT:
		case esmiCS_HOLD_BOTH:

			// Parse the message they gave us
			SdpParse (Sdp, estiAnswer, nullptr, &bCallInfoChanged);

			if (m_pIceSession)
			{
				m_pIceSession->SdpReceived (Sdp, &m_remoteIceSupport);

				if (m_remoteIceSupport == IceSupport::Supported)
				{
					//
					// Determine if any of the media streams have been disabled.  If one is found
					// then disable it also in the SDP for the updated offer
					//
					int nNbrMedia = Sdp.numberOfMediaDescriptorsGet();

					for (int i = 0; i < nNbrMedia; i++) // for each 'm' line
					{
						const RvSdpMediaDescr *pMediaDescriptor = Sdp.mediaDescriptorGet(i);

						int nRtpPort = rvSdpMediaDescrGetPort (pMediaDescriptor);

						if (nRtpPort == 0)
						{
							//
							// Disable the corresponding media stream for the updated offer
							//
							RvSdpMediaDescr *pOfferMediaDescriptor = m_sdp.mediaDescriptorGet(i);

							if (pOfferMediaDescriptor)
							{
								rvSdpMediaDescrSetPort (pOfferMediaDescriptor, 0);
							}
							else
							{
								stiASSERTMSG (false, "CallState = %d AnswerMediaDescriptors = %d OfferMediaDescriptors = %d", sipCall->CstiCallGet ()->StateGet (), nNbrMedia, m_sdp.numberOfMediaDescriptorsGet ());
							}
						}
					}

					m_bRemoteIsICE = true;

					if (m_pIceSession->m_eState == CstiIceSession::estiGatheringComplete)
					{
						m_pIceSession->SessionProceed ();
					}
				}
				else if (m_pIceSession->m_eState != CstiIceSession::estiComplete)
				{
					//
					// The remote endpoint did not indicate that they
					// support ICE so we will need to get the default transports
					// that were gathered during the initial ICE negotiations
					// and update our channels with those transports.
					//
					hResult = sipCall->DefaultTransportsApply (shared_from_this());
					stiTESTRESULT ();

					m_pIceSession->SessionEnd();
					m_pIceSession = nullptr;
				}
			}

			if (!m_pIceSession || m_pIceSession->m_eState != CstiIceSession::estiNominating)
			{
				// If we are negotiating a local HOLD, we can now fully transition to the hold state.
				sipCall->HoldStateComplete (true);


				if (isDtlsNegotiated ())
				{
					sipCall->dtlsBegin (true);
				}
				else
				{
					// Modify our media channels
					hResult = sipCall->mediaUpdate (true);
					stiASSERTRESULTMSG (hResult, "Media update FAILED");
				}
			}

			// If media capabilities have changed send a message for the UI in case it needs to update.
			if (bCallInfoChanged)
			{
				sipCall->SipManagerGet ()->callInformationChangedSignal.Emit (sipCall->CstiCallGet ());
			}

			// If we have a record channel make sure it knows how we want to handle privacy.
			if (sipCall->m_videoRecord)
			{
				bool bSorensonPrivacy = sipCall->m_videoRecord->SorensonPrivacyGet ();

				if (bSorensonPrivacy != (RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE)))
				{
					sipCall->m_videoRecord->SorensonPrivacySet (RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE));
				}
			}

			break;

		case esmiCS_UNKNOWN:
		case esmiCS_DISCONNECTING:
		case esmiCS_DISCONNECTED:
		case esmiCS_CRITICAL_ERROR:
		case esmiCS_INIT_TRANSFER:
		case esmiCS_TRANSFERRING:
			//todo: this needs to be relooked at...
			DBG_MSG ("Unable to accept re-invite in current state");
			RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
			m_sendNewOfferToSorenson = false;
			break;
	}

	MediaServerDisconnect ();
	
	if (m_sendNewOfferToSorenson && RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_DEVICE))
	{
		sipCall->SipManagerGet ()->ReInviteSend(sipCall->CstiCallGet (), 0, false);
		m_sendNewOfferToSorenson = false;
	}

STI_BAIL:

	return hResult;
}


/*!\brief This check is here to see if we have connected to a Media Server
*	If we have connected to a Media Server it ends the calls.
*/
void CstiSipCallLeg::MediaServerDisconnect ()
{
	std::string videoMailNumber;
	auto sipCall = m_sipCallWeak.lock ();

	sipCall->CstiCallGet ()->VideoMailServerNumberGet (&videoMailNumber);
	if (!videoMailNumber.empty ())
	{
		sipCall->CstiCallGet ()->ResultSet (estiRESULT_REMOTE_SYSTEM_REJECTED);
		sipCall->CallEnd (estiSUBSTATE_LOCAL_HANGUP, SIPRESP_DECLINE);
	}
}


stiHResult CstiSipCallLeg::SdpOfferSend ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	auto sipCall = m_sipCallWeak.lock ();

	EsmiCallState eState = sipCall->CstiCallGet ()->StateGet ();

	switch (eState)
	{
		case esmiCS_IDLE:
		case esmiCS_CONNECTING:
		{
			if (m_bSendNewOfferInPRACK)
			{
				if (m_pIceSession)
				{
					hResult = m_pIceSession->SdpUpdate (&m_sdp);
					stiTESTRESULT ();
				}

				hResult = outboundMessageGet().bodySet(m_sdp);
				stiTESTRESULT ();

				//
				// Todo: send the offer in the correct response.
				//
				RvSipCallLegSendPrack (m_hCallLeg);

				m_bAnswerPending = true;
			}
			else
			{
				m_bSendOfferWhenReady = true;
			}

			break;
		}

		case esmiCS_CONNECTED:
		case esmiCS_HOLD_LCL:
		case esmiCS_HOLD_RMT:
		case esmiCS_HOLD_BOTH:
		{
			// Create a new message
			CstiSdp Sdp;
			RvSipCallLegInviteHandle hReInvite;

			rvStatus = RvSipCallLegReInviteCreate (m_hCallLeg, nullptr, &hReInvite);
			stiTESTRVSTATUS ();

			// If geolocation is needed (emergency call), add it.
			if (sipCall->CstiCallGet ()->geolocationRequestedGet ())
			{
				hResult = outboundMessageGet().geolocationAdd ();
				stiTESTRESULT ();
			}

			// Set up the message contents
			hResult = SdpCreate (&Sdp, estiOffer);
			stiTESTRESULT ();

			if (m_pIceSession)
			{
				m_pIceSession->SdpUpdate (&Sdp);
			}

			hResult = outboundMessageGet().bodySet(Sdp);
			stiTESTRESULT ();

			// Send the message
			rvStatus = RvSipCallLegReInviteRequest (m_hCallLeg, hReInvite);
			stiTESTRVSTATUS ();

			m_bAnswerPending = true;

			break;
		}

		case esmiCS_UNKNOWN:
		case esmiCS_DISCONNECTING:
		case esmiCS_DISCONNECTED:
		case esmiCS_CRITICAL_ERROR:
		case esmiCS_INIT_TRANSFER:
		case esmiCS_TRANSFERRING:
			DBG_MSG ("Unable to accept re-invite in current state");
			RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
			break;
	}

STI_BAIL:

	return hResult;
}


/*!\brief Handles the "ICE Gathering Completed" message by updating the SDP and sending the invite.
 *
 * \param pEvent The event object containing the attributes of the event.
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::IceGatheringComplete ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	RvStatus rvStatus = RV_OK;
	CstiRoutingAddress DialString;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();

	call->collectTrace (formattedString ("%s: role %d", __func__, m_pIceSession ? m_pIceSession->m_eRole : -1));

	if (m_pIceSession && m_pIceSession->m_eRole == CstiIceSession::estiICERoleAnswerer)
	{
		//
		// We are the answerer.
		//

		//
		// Get the local addresses that the ICE session has already acquired
		// so that we can build an SDP response
		//
		RvAddress AudioRtp;
		RvAddress AudioRtcp;
		RvAddress TextRtp;
		RvAddress TextRtcp;
		RvAddress VideoRtp;
		RvAddress VideoRtcp;

		hResult = m_pIceSession->LocalAddressesGet (
			&AudioRtp, &AudioRtcp,
			&TextRtp, &TextRtcp,
			&VideoRtp, &VideoRtcp);
		stiTESTRESULT ();

		sipCall->DefaultAddressesSet (
			&AudioRtp, &AudioRtcp,
			&TextRtp, &TextRtcp,
			&VideoRtp, &VideoRtp);

		//
		// Create a response SDP and have the ICE manager update it.
		//
		hResult = SdpCreate (&m_sdp, estiAnswer);
		stiTESTRESULT ();

		hResult = m_pIceSession->SdpUpdate (&m_sdp);

		if (stiIS_ERROR (hResult))
		{
			//
			// There was something in the SDP that the ICE stack didn't like.  Fall
			// back to using non-ICE.
			//
			hResult = stiRESULT_SUCCESS;

			//
			// Cancel the ICE session.
			//
			m_pIceSession->SessionEnd ();
			m_pIceSession = nullptr;

			//
			// Indicate that we are ringing.
			//
			hResult = outboundMessageGet().inviteRequirementsAdd();
			stiTESTRESULT ();

			if (RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE)
			 && m_nRemoteSIPVersion >= 2)
			{
				SInfoHeaderAdd ();
			}

			ReliableResponseSend (SIPRESP_RINGING, nullptr);

			sipCall->AlertUserCountDownDecr ();
		}
		else
		{
			hResult = outboundMessageGet().inviteRequirementsAdd();
			stiTESTRESULT ();

			switch (sipCall->CstiCallGet ()->StateGet ())
			{
				case esmiCS_IDLE:
				case esmiCS_CONNECTING:
				{
					if (m_eNextICEMessage == estiOfferAnswerINVITEFinalResponse)
					{
						//
						// We are going to wait until the call is answered to send our SDP.
						//
						RvSipCallLegProvisionalResponse (m_hCallLeg, SIPRESP_RINGING);
						sipCall->AlertLocalUser ();
					}
					else
					{
						if (RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE)
						 && m_nRemoteSIPVersion >= 2)
						{
							SInfoHeaderAdd ();
						}

						hResult = outboundMessageGet().bodySet(m_sdp);
						stiTESTRESULT ();

						switch (m_eNextICEMessage)
						{
							case estiOfferAnswerPRACKFinalResponse:

								RvSipCallLegSendPrackResponse (m_hCallLeg, SIPRESP_SUCCESS);

								break;

							case estiOfferAnswerPRACK:

								RvSipCallLegSendPrack (m_hCallLeg);

								break;

							case estiOfferAnswerReliableResponse:

								ReliableResponseSend (SIPRESP_RINGING, nullptr);
								if (m_nRemoteSIPVersion >= 4)
								{
									sipCall->AlertLocalUser ();
								}
								else if (m_nRemoteSIPVersion == 3)
								{
									//
									// We know that version 3 endpoints will send
									// an updated offer with nominated candidates
									// in an update request.  We need to wait for this
									// update transaction to complete before answering the call.
									//
									m_bPendingOfferUpdate = true;
								}

								break;

							case estiOfferAnswerACK:

								RvSipCallLegAck (m_hCallLeg);

								break;

							case estiOfferAnswerNone:
							case estiOfferAnswerINVITE:
							case estiOfferAnswerUPDATE:
							case estiOfferAnswerUPDATEFinalResponse:
							case estiOfferAnswerINVITEFinalResponse:

								stiASSERT (estiFALSE);

								break;
						}

						m_bOfferAnswerComplete = true;

						hResult = m_pIceSession->SessionProceed ();
						stiTESTRESULT ();
					}

					break;
				}

				case esmiCS_CONNECTED:
				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
				{
					hResult = outboundMessageGet().bodySet (m_sdp);
					stiTESTRESULT ();

					if (m_eNextICEMessage == estiOfferAnswerINVITEFinalResponse)
					{
						rvStatus = RvSipCallLegAccept (m_hCallLeg);
						stiTESTRVSTATUS ();
					}
					else if (m_eNextICEMessage == estiOfferAnswerACK)
					{
						RvSipCallLegAck (m_hCallLeg);
					}
					else
					{
						stiASSERT (estiFALSE);
					}

					hResult = m_pIceSession->SessionProceed ();
					stiTESTRESULT ();

					break;
				}

				case esmiCS_UNKNOWN:
				case esmiCS_DISCONNECTING:
				case esmiCS_DISCONNECTED:
				case esmiCS_CRITICAL_ERROR:
				case esmiCS_INIT_TRANSFER:
				case esmiCS_TRANSFERRING:
					stiASSERTMSG (estiFALSE, "Unable to accept re-invite in current state");
					RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
					break;
			} // end switch
		}
	}
	else if (m_pIceSession && m_pIceSession->m_eRole == CstiIceSession::estiICERoleOfferer)
	{
		//
		// We are the offerer.
		//
		hResult = m_pIceSession->SdpUpdate (&m_sdp);
		stiTESTRESULT ();

		switch (sipCall->CstiCallGet ()->StateGet ())
		{
			case esmiCS_IDLE:
			case esmiCS_CONNECTING:
			{
				switch (m_eNextICEMessage)
				{
					case estiOfferAnswerINVITE:

						hResult = sipCall->SipManagerGet ()->InviteSend (sipCall, &m_sdp);
						stiTESTRESULT ();

						m_bAnswerPending = true;

						break;

					case estiOfferAnswerReliableResponse:
					{
						hResult = outboundMessageGet().bodySet (m_sdp);
						stiTESTRESULT ();

						ReliableResponseSend (SIPRESP_RINGING, nullptr);
						sipCall->AlertLocalUser ();

						m_bAnswerPending = true;

						break;
					}

					case estiOfferAnswerNone:
					case estiOfferAnswerACK:
					case estiOfferAnswerPRACKFinalResponse:
					case estiOfferAnswerPRACK:
					case estiOfferAnswerUPDATE:
					case estiOfferAnswerUPDATEFinalResponse:
					case estiOfferAnswerINVITEFinalResponse:

						stiASSERT (estiFALSE);

						break;
				}

				break;
			}

			case esmiCS_CONNECTED:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			{
				hResult = outboundMessageGet().bodySet (m_sdp);
				stiTESTRESULT ();

				m_bAnswerPending = true;

				rvStatus = RvSipCallLegAccept (m_hCallLeg);
				stiTESTRVSTATUS ();

				break;
			}

			case esmiCS_UNKNOWN:
			case esmiCS_DISCONNECTING:
			case esmiCS_DISCONNECTED:
			case esmiCS_CRITICAL_ERROR:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:
				stiASSERTMSG (estiFALSE, "Unable to accept re-invite in current state");
				RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
				break;
		} // end switch
	}
	else if (m_pIceSession && m_pIceSession->m_eRole == CstiIceSession::estiICERoleGatherer)
	{
		//
		// If we got here then we must have started an ICE session simply to
		// get some transports.
		//

		hResult = m_pIceSession->SdpUpdate (&m_sdp);
		stiTESTRESULT ();

		hResult = sipCall->DefaultTransportsApply (shared_from_this());
		stiTESTRESULT ();

		// If we need to open channels do so now. Fixes bug #23640.
		if (isDtlsNegotiated ())
		{
			sipCall->dtlsBegin (true);
		}
		else
		{
			hResult = ConfigureMediaChannels (true);
			stiTESTRESULT ();
		}

		m_pIceSession->SessionEnd ();
		m_pIceSession = nullptr;

		switch (sipCall->CstiCallGet ()->StateGet ())
		{
			case esmiCS_IDLE:
			case esmiCS_CONNECTING:
			{
				switch (m_eNextICEMessage)
				{
					case estiOfferAnswerReliableResponse:
					{
						//
						// Indicate that we are ringing using a reliable provisional message.
						//
						hResult = outboundMessageGet().inviteRequirementsAdd ();
						stiTESTRESULT ();

						if (RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE)
							 && m_nRemoteSIPVersion >= 2)
						{
							SInfoHeaderAdd ();
						}

						ReliableResponseSend (SIPRESP_RINGING, nullptr);

						sipCall->AlertUserCountDownDecr ();

						break;
					}

					case estiOfferAnswerINVITEFinalResponse:
					{
						//
						// Indicate that we are ringing useing a regular provisional response.
						//
						RvSipCallLegProvisionalResponse (m_hCallLeg, SIPRESP_RINGING);
						sipCall->AlertLocalUser ();

						break;
					}

					case estiOfferAnswerINVITE:
					case estiOfferAnswerNone:
					case estiOfferAnswerACK:
					case estiOfferAnswerPRACKFinalResponse:
					case estiOfferAnswerPRACK:
					case estiOfferAnswerUPDATE:
					case estiOfferAnswerUPDATEFinalResponse:
						stiASSERT (estiFALSE);
						break;
				}

				break;
			}
			case esmiCS_CONNECTED:
			case esmiCS_HOLD_LCL:
			case esmiCS_HOLD_RMT:
			case esmiCS_HOLD_BOTH:
			{
				// If we have an active call we will send a response with our updated SDP.
				// Fixes bug 22361

				hResult = outboundMessageGet().bodySet (m_sdp);
				stiTESTRESULT ();

				rvStatus = RvSipCallLegAccept (m_hCallLeg);
				stiTESTRVSTATUS ();

				break;
			}
			case esmiCS_UNKNOWN:
			case esmiCS_DISCONNECTING:
			case esmiCS_DISCONNECTED:
			case esmiCS_CRITICAL_ERROR:
			case esmiCS_INIT_TRANSFER:
			case esmiCS_TRANSFERRING:
				stiASSERTMSG (estiFALSE, "Unable to accept re-invite in current state");
				RvSipCallLegReject (m_hCallLeg, SIPRESP_TEMPORARILY_UNAVAILABLE);
				break;
		}
	}
	else
	{
		stiASSERT (estiFALSE);
	}

STI_BAIL:

	call->collectTrace (formattedString ("%s: result %d", __func__, hResult));

	return hResult;
}


/*!\brief Iterates through the media streams looking for the correct type to apply the ICE nominated addresses
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::MediaStreamNominationsSet (
	EstiMediaType eMediaType, ///< The type of media (audio, text, or video) to search for
	const CstiMediaAddresses &mediaAddresses)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	if (mediaAddresses.isValid())
	{
		for (const auto &stream: m_SdpStreams)
		{
			if (stream->eType == eMediaType)
			{
				RvAddressCopy (mediaAddresses.RtpAddressGet (), &stream->RtpAddress);
				RvAddressCopy (mediaAddresses.RtcpAddressGet (), &stream->RtcpAddress);
			}
		}
	}

//STI_BAIL:

	return hResult;
}


/*!\brief Applies the nominated addresses and ports to the media streams.
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::IceNominationsApply ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiMediaTransports audioTransports;
	CstiMediaTransports textTransports;
	CstiMediaTransports videoTransports;
	CstiMediaAddresses remoteAudioAddresses;
	CstiMediaAddresses remoteTextAddresses;
	CstiMediaAddresses remoteVideoAddresses;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();

	call->collectTrace (formattedString ("%s", __func__));

	hResult = m_pIceSession->NominationsGet (
		&audioTransports,
		&textTransports,
		&videoTransports,
		&remoteAudioAddresses,
		&remoteTextAddresses,
		&remoteVideoAddresses,
		&m_IceSdpAttributes);
	stiTESTRESULT ();

	sipCall->IncomingTransportsSet (
		audioTransports,
		textTransports,
		videoTransports);

	MediaStreamNominationsSet (estiMEDIA_TYPE_AUDIO, remoteAudioAddresses);
	MediaStreamNominationsSet (estiMEDIA_TYPE_TEXT, remoteTextAddresses);
	MediaStreamNominationsSet (estiMEDIA_TYPE_VIDEO, remoteVideoAddresses);

	m_pIceSession->m_nominationsApplied = true;

STI_BAIL:

	call->collectTrace (formattedString ("%s: result %d", __func__, hResult));

	return hResult;
}


/*!\brief Handles the "ICE Nominations Completed" message by updating the SDP and sending the re-invite.
 *
 * \param pEvent The event object containing the attributes of the event.
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::IceNominationsComplete (
	bool bSucceeded)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet();
	CstiMediaTransports audioTransports;
	CstiMediaTransports textTransports;
	CstiMediaTransports videoTransports;

	call->collectTrace (formattedString ("%s: %d", __func__, bSucceeded));

	if (m_pIceSession)
	{
		//Setting ICE state to complete here resolves bug #23307.
		m_pIceSession->m_eState = CstiIceSession::estiComplete;
		m_eCallLegIceState = estiSucceeded;
		
		if (m_pIceSession->m_eRole == CstiIceSession::estiICERoleAnswerer)
		{
			if (bSucceeded)
			{
				hResult = IceNominationsApply ();
				stiTESTRESULT ();
			}
			else
			{
				//
				// The remote endpoint did not indicate that they
				// support ICE so we will need to get the default transports
				// that were gathered during the initial ICE negotiations
				// and update our channels with those transports.
				//
				hResult = sipCall->DefaultTransportsApply (shared_from_this());
				stiTESTRESULT ();
			}

			if (isDtlsNegotiated ())
			{
				sipCall->dtlsBegin (false);
			}
			else
			{
				hResult = ConfigureMediaChannels (false);
				stiTESTRESULT ();
			}
		}

		else if (m_pIceSession->m_eRole == CstiIceSession::estiICERoleOfferer)
		{
			switch (sipCall->CstiCallGet ()->StateGet ())
			{
				case esmiCS_IDLE:
				case esmiCS_CONNECTING:
				{
					if (bSucceeded)
					{
						SdpOfferSend ();
					}
					else if (m_nRemoteSIPVersion < 4)
					{
						m_eCallLegIceState = estiFailed;

						// If we are talking to a pre 4.5 endpoint and ICE has failed for all call legs
						// we need to hang up the call which fixes Bug #22502.
						if (sipCall->AllLegsFailedICENominations ())
						{
							call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
							sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_INTERNAL_ERROR);
						}
					}

					break;
				}

				case esmiCS_CONNECTED:
				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
				{
					if (bSucceeded)
					{
						hResult = IceNominationsApply ();
						stiTESTRESULT ();
					}
					else
					{
						hResult = sipCall->DefaultTransportsApply (shared_from_this());
						stiTESTRESULT ();
						
						m_pIceSession->SessionEnd();
						m_pIceSession = nullptr;
					}

					if (isDtlsNegotiated ())
					{
						sipCall->dtlsBegin (true);
					}
					else
					{
						hResult = ConfigureMediaChannels (true);
						stiASSERTRESULT (hResult);
					}

					SdpOfferSend ();

					break;
				}

				case esmiCS_UNKNOWN:
				case esmiCS_DISCONNECTING:
				case esmiCS_DISCONNECTED:
				case esmiCS_CRITICAL_ERROR:
				case esmiCS_INIT_TRANSFER:
				case esmiCS_TRANSFERRING:

					stiASSERT (estiFALSE);

					break;
			}
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

	if (!bSucceeded)
	{
		m_eCallLegIceState = estiFailed;
	}

STI_BAIL:

	//
	// If we had an error of any kind, end the call.
	//
	if (stiIS_ERROR (hResult))
	{
		call->ResultSet (estiRESULT_REMOTE_SYSTEM_UNREACHABLE);
		sipCall->CallEnd (estiSUBSTATE_UNREACHABLE, SIPRESP_INTERNAL_ERROR);
	}

	call->collectTrace (formattedString ("%s: result %d, role %d", __func__, hResult, m_pIceSession ? m_pIceSession->m_eRole : -1));

	return hResult;
}


/*!\brief Handles configuring media channels if it has not been done prior to ICE.
 *
 * \param bIsReply Indicates if this is a reply or not.
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::ConfigureMediaChannels (
	bool bIsReply)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	auto sipCall = m_sipCallWeak.lock ();
	auto call = sipCall->CstiCallGet ();
	CstiSipManager *poSipManager = sipCall->SipManagerGet ();

	// If we still have a call in the connected state and it is establishing we can now transition to the conferencing substate.
	if (call->StateValidate (esmiCS_CONNECTED) && call->SubstateValidate (estiSUBSTATE_ESTABLISHING))
	{
		poSipManager->NextStateSet (call, esmiCS_CONNECTED, estiSUBSTATE_CONFERENCING);

		//
		// ICE has completed and this call has been answered
		// so start sending and receiving media.
		//
		hResult = sipCall->mediaUpdate (bIsReply);
		stiTESTRESULT ();
	}
	else if (call->StateValidate (esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH))
	{
		hResult = sipCall->mediaUpdate (bIsReply);
		stiTESTRESULT ();
	}
	else
	{
		hResult = sipCall->IncomingMediaInitialize ();
		stiTESTRESULT ();
	}
	
STI_BAIL:
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CstiSipCallLeg::ReliableResponseSend
//
// Description: This function sends a reliable response if the remote
//	phone supports it, otherwise, unreliably.
//
// Abstract:
//
// Returns: Nothing
//
void CstiSipCallLeg::ReliableResponseSend (
	uint16_t un16SipResponse, ///< The provisional response status code.
	bool *pbSentReliably)
{

	// TODO: Probably do not need this.  Checking in for demo.
	// NOTE: For forked calls, must base prack off of the original call leg
	// not the forked leg, or else the cseq's could get duplicated.
	RvSipCallLegHandle hOriginalCallLeg;
	RvSipCallLegGetOriginalCallLeg (m_hCallLeg, &hOriginalCallLeg);
	{
		RvInt32 cSeq;
		RvSipCallLegGetCSeq (hOriginalCallLeg, &cSeq);
		RvSipCallLegSetCSeq (m_hCallLeg, cSeq);
		RvSipCallLegSetCSeq (hOriginalCallLeg, cSeq + 1 );
	}

	RvSipTransaction100RelStatus eStatus;
	RvSipCallLegGet100RelStatus (m_hCallLeg, &eStatus);
	if (RVSIP_TRANSC_100_REL_UNDEFINED != eStatus)
	{
		RvSipCallLegProvisionalResponseReliable (m_hCallLeg, un16SipResponse);
		m_bWaitingForPrackCompletion = true;

		if (pbSentReliably)
		{
			*pbSentReliably = true;
		}
	}
	else
	{
		RvSipCallLegProvisionalResponse (m_hCallLeg, un16SipResponse);

		if (pbSentReliably)
		{
			*pbSentReliably = false;
		}
	}
} // end CstiSipCallLeg::ReliableResponseSend


stiHResult CstiSipCallLeg::AllowCapabilitiesDetermine (
	const vpe::sip::Message &message)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	bool bAllowHeaderFound = false;
	bool bAllowMethodFound = false;
	bool bInfoPackageHeaderFound = false;
	bool bInfoPackageFound = false;

	MethodInAllowVerify(message, vpe::sip::Message::Method::Refer, &bAllowHeaderFound, &bAllowMethodFound);

	if (bAllowHeaderFound)
	{
		IsTransferableSet (bAllowMethodFound);
	}

	MethodInAllowVerify(message, vpe::sip::Message::Method::Update, &bAllowHeaderFound, &bAllowMethodFound);

	if (bAllowHeaderFound)
	{
		UpdateAllowedSet (bAllowMethodFound);
	}
	
	InfoPackageVerify (message, CONTACT_PACKAGE, &bInfoPackageHeaderFound, &bInfoPackageFound);

	if (bInfoPackageHeaderFound)
	{
		bInfoPackageFound = bInfoPackageFound && RemoteDeviceTypeIs (estiDEVICE_TYPE_SVRS_DEVICE);
		ShareContactSupportedSet (bInfoPackageFound);
	}

	return hResult;
}


/*!\brief Parses the display size requested by the remote side.
 *
 * \param remoteDisplaySize string containing display size requested ("width:height")
 * \param[out] width The width parsed from remoteDisplaySize can be 0.
 * \param[out] height The height parsed from remoteDisplaySize can be 0.
 *
 * \return Success or failure
 */
stiHResult RemoteDisplaySizeParse (
	const std::string& remoteDisplaySize,
	unsigned int *width,
	unsigned int *height)
{
	const int baseTen = 10;
	unsigned int parsedWidth = 0;
	unsigned int parsedHeight = 0;
	char *nonNumber = nullptr;
	std::string displaySize;
	stiHResult hResult = stiRESULT_SUCCESS;

	if (!remoteDisplaySize.empty ())
	{
		auto separatorPosition = remoteDisplaySize.find (":");
		stiTESTCONDMSG (separatorPosition != std::string::npos, stiRESULT_INVALID_PARAMETER,
				"CstiSipCallLeg::RemoteDisplaySizeParse error parsing DisplaySize = %s", remoteDisplaySize.c_str ());

		displaySize = remoteDisplaySize.substr (0, separatorPosition);
		parsedWidth = std::strtoul (displaySize.c_str (), &nonNumber, baseTen);

		if (*nonNumber != '\0'
				|| nonNumber == displaySize.data ())
		{
			stiTHROWMSG (stiRESULT_ERROR,
					"CstiSipCallLeg::RemoteDisplaySizeParse Failed to find valid width = %s", nonNumber);
		}

		displaySize = remoteDisplaySize.substr (separatorPosition + 1);
		parsedHeight = std::strtoul (displaySize.c_str (), &nonNumber, baseTen);

		if (*nonNumber != '\0'
				|| nonNumber == displaySize.data ())
		{
			stiTHROWMSG (stiRESULT_ERROR,
					"CstiSipCallLeg::RemoteDisplaySizeParse found Height = %d failed to find valid width in attribute = %s", parsedHeight, nonNumber);
		}

		*width = parsedWidth;
		*height = parsedHeight;
	}
	else
	{
		stiTHROWMSG (stiRESULT_ERROR,
				"CstiSipCallLeg::RemoteDisplaySizeParse no display size provided");
	}

STI_BAIL:

	return hResult;
}


/*!\brief Processes the display size requested by the remote side.
 *
 * \param remoteDisplaySize string containing display size requested ("width:height")
 *
 * \return Success or failure
 */
stiHResult CstiSipCallLeg::RemoteDisplaySizeProcess(const std::string& remoteDisplaySize)
{
	unsigned int width = 0;
	unsigned int height = 0;
	auto sipCall = m_sipCallWeak.lock ();
	CstiCallSP call = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;
		
	stiTESTCONDMSG (!remoteDisplaySize.empty (), stiRESULT_INVALID_PARAMETER,
					"CstiSipCallLeg::RemoteDisplaySizeProcess no display size provided");
	
	hResult = RemoteDisplaySizeParse(remoteDisplaySize, &width, &height);
	stiTESTRESULT ();
	
	stiTESTCONDMSG (width && height, stiRESULT_ERROR,
					"CstiSipCallLeg::RemoteDisplaySizeProcess Invalid parameters Width = %u Height = %u", width, height);
	
	call = sipCall->CstiCallGet ();
	stiTESTCONDMSG (call, stiRESULT_INVALID_PARAMETER,
					"CstiSipCallLeg::RemoteDisplaySizeProcess Called with no call");
	
	RemoteDisplaySizeSet (width, height);
	
	// Don't update the video record channel when this call isn't the active call.
	if (sipCall->m_videoRecord && call->StateValidate (esmiCS_CONNECTED))
	{
		sipCall->m_videoRecord->RemoteDisplaySizeSet (width, height);
	}

STI_BAIL:

	return hResult;
}


/*! \brief helper to access CstiSipCall::IsBridgedCall
 */
bool CstiSipCallLeg::IsBridgedCall ()
{
	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	return sipCall && sipCall->IsBridgedCall ();
}


/*!
 * \brief Sets if call leg has reached a conferenced state.
 */
void CstiSipCallLeg::ConferencedSet (bool conferenced)
{
	m_conferenced = conferenced;
}


/*!
 * \brief Indicates if the call leg reached a conferenced state.
 */bool CstiSipCallLeg::ConferencedGet () const
{
	return m_conferenced;
}


/*!
 * \brief Sets if the call leg has been logged to Splunk.
 */
void CstiSipCallLeg::callLoggedSet (bool logged)
{
	m_callLogged = logged;
}


/*!
 * \brief Returns if the call leg has been logged to Splunk.
 */
bool CstiSipCallLeg::callLoggedGet () const
{
	return m_callLogged;
}


/*!
 * \brief Returns true if the passed in timer has a duration.
 *
 */
bool isTimeSet (const std::chrono::steady_clock::time_point& timer)
{
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds> (timer.time_since_epoch());

	return duration.count();
}


/*!
 * \brief Sets the time we start connecting.
 *
 */
void CstiSipCallLeg::connectingTimeStart ()
{
	if (!isTimeSet (m_connectingStartTime))
	{
		m_connectingStartTime = std::chrono::steady_clock::now ();
	}
}


/*!
 * \brief Sets the time we stop connecting.
 *
 */
void CstiSipCallLeg::connectingTimeStop ()
{
	if (!isTimeSet (m_connectingStopTime))
	{
		m_connectingStopTime = std::chrono::steady_clock::now ();
	}
}


/*!
 * \brief Returns the time time spent connecting the all in milliseconds.
 *
 */
std::chrono::milliseconds CstiSipCallLeg::connectingDurationGet () const
{
	std::chrono::milliseconds duration = {};
	
	if (isTimeSet (m_connectingStartTime))
	{
		if (!isTimeSet (m_connectingStopTime))
		{
			duration = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - m_connectingStartTime);
		}
		else
		{
			duration = std::chrono::duration_cast<std::chrono::milliseconds> (m_connectingStopTime - m_connectingStartTime);
		}
	}
	
	return duration;
}


/*!
 * \brief Sets the time we become connected.
 *
 */
void CstiSipCallLeg::connectedTimeStart ()
{
	if (!isTimeSet (m_connectedStartTime))
	{
		m_connectedStartTime = std::chrono::steady_clock::now ();
	}
}


/*!
 * \brief Sets the time we are ending the call.
 *
 */
void CstiSipCallLeg::connectedTimeStop ()
{
	if (!isTimeSet (m_connectedStopTime))
	{
		m_connectedStopTime = std::chrono::steady_clock::now ();
	}
}


/*!
 * \brief Returns the call duration in milliseconds.
 *
 */
std::chrono::milliseconds  CstiSipCallLeg::connectedDurationGet () const
{
	std::chrono::milliseconds duration = {};
	
	if (isTimeSet (m_connectedStartTime))
	{
		if (!isTimeSet (m_connectedStopTime))
		{
			duration = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - m_connectedStartTime);
		}
		else
		{
			duration = std::chrono::duration_cast<std::chrono::milliseconds> (m_connectedStopTime - m_connectedStartTime);
		}
	}
	
	return duration;
}


/*!
 * \brief Returns time since the call was ended in seconds.
 *
 */
std::chrono::seconds CstiSipCallLeg::SecondsSinceCallEndedGet () const
{
	std::chrono::seconds timeSinceEnded = {};
	
	if (isTimeSet (m_connectedStopTime))
	{
		timeSinceEnded = std::chrono::duration_cast<std::chrono::seconds> (std::chrono::steady_clock::now () - m_connectedStopTime);
	}
	
	return timeSinceEnded;
}


/*!
 * \brief Returns the ICE nomintation result for call leg.
 *
 */
EstiICENominationsResult CstiSipCallLeg::iceNominationResultGet ()
{
	auto iceNomintationResult = eICENominationsUnknown;
	
	if (m_bRemoteIsICE)
	{
		if (m_eCallLegIceState == estiFailed)
		{
			iceNomintationResult = eICENominationsFailed;
		}
		else if (m_eCallLegIceState == estiSucceeded)
		{
			iceNomintationResult = eICENominationsSucceeded;
		}
	}
	else
	{
		iceNomintationResult = eRemoteNotICE;
	}
	
	return iceNomintationResult;
}


/*!
 * \brief Static method to generate a new unique key to represent the call leg
 *        This is distinctly different than the handle, which is just a pointer
 *        and is not guaranteed to be unique between delete and the next new
 */
size_t CstiSipCallLeg::uniqueKeyGenerate ()
{
	std::lock_guard<std::recursive_mutex> lock (m_uniqueKeyIncrementorMutex);
	return ++m_uniqueKeyIncrementor;
}


/*!
 * \brief Returns the rPort for this call leg if not set UNDEFINED is returned.
 *
 */
int CstiSipCallLeg::rportGet () const
{
	return m_rPort;
}


/*!
 * \brief Sets the rPort for this call leg if rPort is not UNDEFINED.
 *
 */
void CstiSipCallLeg::rPortSet (const int rPort)
{
	if (rPort != UNDEFINED)
	{
		m_rPort = rPort;
	}
}


/*!
 * \brief Returns the received address for this call leg if not set an empty string returned.
 *
 */
std::string CstiSipCallLeg::receivedAddrGet () const
{
	return m_receivedAddr;
}


/*!
 * \brief Sets the received address for this call leg if receviedAddr is valid IP address.
 *
 */
void CstiSipCallLeg::receivedAddrSet (const std::string& receivedAddr)
{
	if (IPAddressValidate (receivedAddr.c_str ()))
	{
		m_receivedAddr = receivedAddr;
	}
}


/*!
 * \brief clear the local sdp streams
 *
 * should really only be done when populating the streams again
 */
void CstiSipCallLeg::localSdpStreamsClear ()
{
	m_localSdpStreams.clear();
}

bool CstiSipCallLeg::localSdpStreamsAreEmpty () const
{
	return m_localSdpStreams.empty();
}

/*!
 * \brief Gets the local sdp stream model for a media type.  Creates it if it doesn't exist.
 *
 * \return reference to stream model and a boolean indicating if a new stream model was created.
 */
const SstiSdpStreamUP& CstiSipCallLeg::localSdpStreamGet(
	EstiMediaType mediaType)
{
	auto result = std::find_if (m_localSdpStreams.begin (), m_localSdpStreams.end (), 
		[mediaType](SstiSdpStreamUP& stream) {
			return stream->eType == mediaType;
		});

	if (result != m_localSdpStreams.end ())
	{
		return *result;
	}

	auto localStream = sci::make_unique<SstiSdpStream> (
		mediaType,
		"",
		RV_SDPPROTOCOL_UNKNOWN,
		"");
	m_localSdpStreams.push_back (std::move (localStream));
	return m_localSdpStreams.back ();
}


/*!
 * \brief supported SDES key suites
 *
 * TODO: can modify this at runtime somehow if needs be
 */
std::vector<vpe::SDESKey::Suite> CstiSipCallLeg::supportedSDESKeySuitesGet()
{
	// Supported SDES key suites, in the preferred order
	// NOTE: supporting several keys requires a larger SIP/SDP max buffer size
	// TODO: but, will that require that the old endpoints require a larger buffer as well? OUCH!
	// TODO: where should this go?
	// TODO: what is the preferred order?
	return {
		vpe::SDESKey::Suite::AES_256_CM_HMAC_SHA1_80,
		vpe::SDESKey::Suite::AES_CM_128_HMAC_SHA1_80,
		};
}

/*!
 * \brief Generate SDES keys for this call leg
 *
 * \return SDES keys, empty if encryption disabled for this call leg
 */
std::vector<vpe::SDESKey> CstiSipCallLeg::SDESKeysGenerate(const std::vector<vpe::SDESKey::Suite> &suites)
{
	std::vector<vpe::SDESKey> keys;

	for (const auto& suite : suites)
	{
		keys.emplace_back (suite);
	}

	return keys;
}

/*!
 * \brief Add any additional parsed streams
 *
 * \return true on success
 */
void CstiSipCallLeg::localStreamsFromRemoteStreamsUpdate()
{
	for (auto& remoteStream : m_SdpStreams)
	{
		nonstd::observer_ptr<vpe::RtpSession> session; // Use observer pointer, no need to increment ref count
		vpe::SDESKey::Suite selectedSuite;

		auto mediaType = remoteStream->eType;
		
		auto& localStream = localSdpStreamGet (mediaType);

		auto sipCall = m_sipCallWeak.lock ();

		if (sipCall != nullptr)
		{
			switch (mediaType)
			{
				case estiMEDIA_TYPE_VIDEO:
					session = nonstd::make_observer (sipCall->m_videoSession.get ());
					break;

				case estiMEDIA_TYPE_AUDIO:
					session = nonstd::make_observer (sipCall->m_audioSession.get ());
					break;

				case estiMEDIA_TYPE_TEXT:
					session = nonstd::make_observer (sipCall->m_textSession.get ());
					break;

				default:
					stiASSERT (false);
					break;
			}
		}

		if (session != nullptr)
		{
			bool disableEncryption{ true };
			if (selectDtls (*remoteStream))
			{
				// Clear SDES parameters
				localStream->sdesKeys.clear ();

				localStream->Fingerprint = dtlsContextGet ()->localFingerprintGet ();
				localStream->FingerprintHashFunction = dtlsContextGet ()->localFingerprintHashFunctionGet ();
				localStream->SetupRole = vpe::DtlsContext::setupRoleFromRemote (remoteStream->SetupRole);
				session->dtlsContextSet (dtlsContextGet ());
				disableEncryption = false;
			}
			else if (selectSDESKeySuite (*remoteStream, selectedSuite))
			{
				// Clear DTLS parameters
				localStream->Fingerprint.clear ();
				localStream->FingerprintHashFunction = vpe::HashFuncEnum::Undefined;
				localStream->SetupRole = RV_SDPSETUP_NOTSET;
				session->dtlsContextSet (nullptr);

				auto result = std::find_if (localStream->sdesKeys.begin (), localStream->sdesKeys.end (),
					[selectedSuite](vpe::SDESKey& key) {
						return key.suiteGet () == selectedSuite;
					});

				if (result == localStream->sdesKeys.end())
				{
					localStream->sdesKeys = SDESKeysGenerate ({selectedSuite});
				}

				vpe::SDESKey encryptKey, decryptKey;
				if (SDESMediaKeysGet (*remoteStream, *localStream, encryptKey, decryptKey))
				{
					// If the remote key has changed we should reset key and SSRC to ensure there is no encryption issues.
					if (session->decryptKeyChanged (decryptKey))
					{
						session->createNewSsrc ();
						localStream->sdesKeys = SDESKeysGenerate ({selectedSuite});
						SDESMediaKeysGet (*remoteStream, *localStream, encryptKey, decryptKey); // need to get our new key.
					}

					session->SDESKeysSet (encryptKey, decryptKey);
					disableEncryption = false;
				}
			}
			
			if (disableEncryption)
			{
				// Clear all encryption parameters
				localStream->Fingerprint.clear ();
				localStream->FingerprintHashFunction = vpe::HashFuncEnum::Undefined;
				localStream->SetupRole = RV_SDPSETUP_NOTSET;
				localStream->sdesKeys.clear ();
				
				session->encryptionDisabled ();
			}
		}
	}
}

bool CstiSipCallLeg::selectSDESKeySuite(const SstiSdpStream &sdpEntry, vpe::SDESKey::Suite &suite)
{
	// If they've signalled to do SRTP disregard our secure call mode and attempt to do
	// encryption
	if (sdpEntry.MediaProtocol != RV_SDPPROTOCOL_RTP_SAVP)
	{
		CstiSipCallSP sipCall (m_sipCallWeak.lock ());
		// Make sure we could lock the weak pointer
		if (sipCall && !sipCall->includeEncryptionAttributes ())
		{
			return false;
		}
	}

	// If we have parsed the key, then we know what it is and support it
	// TODO: that relies on something we are able to parse
	// If that ever changes, this will need to be updated
	if(!sdpEntry.sdesKeys.empty())
	{
		// Look for each of our supported suites
		for(const auto &s : supportedSDESKeySuitesGet())
		{
			for(const auto &key : sdpEntry.sdesKeys)
			{
				if(s == key.suiteGet())
				{
					// Yes, multiple returns, but, it's a double for loop... :/
					suite = s;
					return true;
				}
			}
		}
	}

	return false;
}

bool CstiSipCallLeg::SDESMediaKeysGet(const SstiSdpStream& remoteStream, const SstiSdpStream& localStream, vpe::SDESKey &encryptKey, vpe::SDESKey &decryptKey)
{
	stiASSERT (remoteStream.eType == localStream.eType);

	// Try to find matching key types for both sides
	for (auto& localKey : localStream.sdesKeys)
	{
		for (auto& remoteKey : remoteStream.sdesKeys)
		{
			if (localKey.suiteGet () == remoteKey.suiteGet ())
			{
				encryptKey = localKey;
				decryptKey = remoteKey;
				return true;
			}
		}
	}

	return false;
}

void CstiSipCallLeg::dtlsParametersAdd (RvSdpMediaDescr* sdpMediaDescriptor, const SstiSdpStream& sdpStream)
{
	if (!sdpStream.Fingerprint.empty() && sdpStream.FingerprintHashFunction != vpe::HashFuncEnum::Undefined)
	{
		RvSdpStatus sdpStatus{};

		dtlsContextGet ()->localKeyExchangeModeSet (vpe::DtlsContext::setupRoleToKeyExchangeMode (sdpStream.SetupRole));

		sdpStatus = rvSdpMediaDescrSetSetupRole (sdpMediaDescriptor->iSdpMsg, sdpMediaDescriptor, sdpStream.SetupRole);
		stiASSERT (sdpStatus == RV_SDPSTATUS_OK);

		sdpStatus = rvSdpMediaSetFingerprint (
			sdpMediaDescriptor,
			RvHashFuncSha1,
			reinterpret_cast<RvChar*>(dtlsContextGet ()->localFingerprintGet ().data ()),
			static_cast<RvInt>(dtlsContextGet ()->localFingerprintGet ().size ())
		);
		stiASSERT (sdpStatus == RV_SDPSTATUS_OK);
	}
}


bool CstiSipCallLeg::selectDtls (const SstiSdpStream& sdpEntry)
{
	auto result = true;

	// If they've signalled to do SRTP disregard our secure call mode and attempt to do 
	// encryption
	if (sdpEntry.MediaProtocol != RV_SDPPROTOCOL_RTP_SAVP)
	{
		CstiSipCallSP sipCall (m_sipCallWeak.lock ());

		// Make sure we could lock the weak pointer
		if (sipCall)
		{
			if (!sipCall->includeEncryptionAttributes ())
			{
				result = false;
			}
		}
	}

	// A fingerprint, hash function and setup role must be supplied
	if (sdpEntry.Fingerprint.empty() ||
		sdpEntry.FingerprintHashFunction == vpe::HashFuncEnum::Undefined ||
		sdpEntry.SetupRole == RV_SDPSETUP_NOTSET ||
		sdpEntry.SetupRole == RV_SDPSETUP_UNKNOWN)
	{
		result = false;
	}
	
	if (RvAddressGetIpPort(&sdpEntry.RtpAddress) == 0)
	{
		result = false;
	}

	return result;
}

bool CstiSipCallLeg::encryptionRequiredGet ()
{
	bool result = true;

	CstiSipCallSP sipCall (m_sipCallWeak.lock ());

	// Make sure we could lock the weak pointer
	if (sipCall)
	{
		// Look up conference params
		EstiSecureCallMode secureCallMode (sipCall->m_stConferenceParams.eSecureCallMode);

		std::string dialString;
		sipCall->CstiCallGet ()->OriginalDialStringGet (&dialString);

		if  (secureCallMode != estiSECURE_CALL_REQUIRED || sipCall->RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_HOLD_SERVER) || dialString == "911")
		{
			result = false;
		}
	}

	return result;
}

vpe::EncryptionState CstiSipCallLeg::encryptionStateVideoGet () const
{
	auto ret = vpe::EncryptionState::NONE;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_videoSession != nullptr)
	{
		ret = sipCall->m_videoSession->encryptionStateGet ();
	}
	return ret;
}

vpe::EncryptionState CstiSipCallLeg::encryptionStateAudioGet () const
{
	auto ret = vpe::EncryptionState::NONE;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_audioSession != nullptr)
	{
		ret = sipCall->m_audioSession->encryptionStateGet ();
	}
	return ret;
}

vpe::EncryptionState CstiSipCallLeg::encryptionStateTextGet () const
{
	auto ret = vpe::EncryptionState::NONE;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_textSession != nullptr)
	{
		ret = sipCall->m_textSession->encryptionStateGet ();
	}
	return ret;
}

vpe::KeyExchangeMethod CstiSipCallLeg::keyExchangeMethodVideoGet () const
{
	auto ret = vpe::KeyExchangeMethod::None;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_videoSession != nullptr)
	{
		ret = sipCall->m_videoSession->keyExchangeMethodGet();
	}
	return ret;
}

vpe::KeyExchangeMethod CstiSipCallLeg::keyExchangeMethodAudioGet () const
{
	auto ret = vpe::KeyExchangeMethod::None;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_audioSession != nullptr)
	{
		ret = sipCall->m_audioSession->keyExchangeMethodGet();
	}
	return ret;
}

vpe::KeyExchangeMethod CstiSipCallLeg::keyExchangeMethodTextGet () const
{
	auto ret = vpe::KeyExchangeMethod::None;

	CstiSipCallSP sipCall = m_sipCallWeak.lock ();

	if (sipCall && sipCall->m_textSession != nullptr)
	{
		ret = sipCall->m_textSession->keyExchangeMethodGet();
	}
	return ret;
}

std::shared_ptr<vpe::DtlsContext> CstiSipCallLeg::dtlsContextGet()
{
	if (m_dtlsContext == nullptr)
	{
		m_dtlsContext = vpe::DtlsContext::get();
	}
	return m_dtlsContext;
}

void CstiSipCallLeg::dtlsContextSet(std::shared_ptr<vpe::DtlsContext> dtlsContext)
{
	m_dtlsContext = dtlsContext;
}

bool CstiSipCallLeg::isDtlsNegotiated ()
{
	return m_dtlsContext != nullptr && m_dtlsContext->isNegotiated ();
}


stiHResult CstiSipCallLeg::encryptionAttributesAdd (RvSdpMediaDescr *sdpMediaDescr, EstiMediaType mediaType)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	const SstiSdpStreamUP& localStream = localSdpStreamGet (mediaType);
	auto sipCall = m_sipCallWeak.lock();
	
	localStream->sdesKeys = SDESKeysGenerate ();
	hResult = SDESKeysAdd (sdpMediaDescr, localStream->sdesKeys);
	stiTESTRESULT ();

	if (!sipCall->disableDtlsGet ())
	{
		localStream->FingerprintHashFunction = dtlsContextGet ()->localFingerprintHashFunctionGet ();
		localStream->Fingerprint = dtlsContextGet ()->localFingerprintGet ();
		localStream->SetupRole = RV_SDPSETUP_ACTIVE;
		dtlsParametersAdd (sdpMediaDescr, *localStream);
	}
	
STI_BAIL:
	
	return hResult;
}


/**
 * Gets the remote name from the To Header
 *
 * \param callLeg The sip call from which to extract the name from the header
 * \param RemoteName The string where to store the extracted name
 * \param bSvrsPrefixRequired If true, return the remote name _only_ if it has the SVRS: prefix on it
 * \return stiHResult
 */
#define SVRS_PREFIX "SVRS:"
#define SVRS_PREFIX_LENGTH (sizeof (SVRS_PREFIX) - 1)
stiHResult CstiSipCallLeg::toHeaderRemoteNameGet (
	std::string *RemoteName,
	bool bSvrsPrefixRequired)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	CstiSipCallSP sipCall = m_sipCallWeak.lock();
	const char *pszStrippedDisplayName;
	char *pszDisplayName = nullptr;
	unsigned int unLength = 0;
	RvSipPartyHeaderHandle hPartyHeader = nullptr;
	std::string DisplayName;

	auto inboundMessage = inboundMessageGet();
	stiTESTCOND(inboundMessage, stiRESULT_ERROR);

	// Get the to header
	stiTESTCOND (sipCall, stiRESULT_ERROR);

	hPartyHeader = inboundMessage.toHeaderGet();
	stiTESTCOND (hPartyHeader, stiRESULT_ERROR);

	// Get the display name
	unLength = RvSipPartyHeaderGetStringLength (hPartyHeader, RVSIP_PARTY_DISPLAY_NAME);
	if (unLength > SVRS_PREFIX_LENGTH)
	{
		pszDisplayName = new char [unLength + 1];
		stiTESTCOND (pszDisplayName, stiRESULT_ERROR);

		if (RV_OK == RvSipPartyHeaderGetDisplayName (hPartyHeader, pszDisplayName, unLength + 1, &unLength))
		{
			pszDisplayName[unLength - 1] = '\0'; // Make sure the string is terminated

			// Decode any sip escape characters
			UnescapeDisplayName (pszDisplayName, &DisplayName);

			// Make sure it has the prefix
			if (0 == strncmp (DisplayName.c_str(), SVRS_PREFIX, SVRS_PREFIX_LENGTH))
			{
				// Strip off any leading whitespace
				pszStrippedDisplayName = &DisplayName.c_str()[SVRS_PREFIX_LENGTH];
				while (isspace (*pszStrippedDisplayName))
				{
					pszStrippedDisplayName++;
				}
				if (*pszStrippedDisplayName)
				{
					// Set the value
					*RemoteName = pszStrippedDisplayName;

					// SVRS prefix indicates terp phone so set terp mode.
					sipCall->RemoteInterfaceModeSet(estiINTERPRETER_MODE);
				}
			}
			else if (!bSvrsPrefixRequired) // doesn't have the prefix, AND prefix not required
			{
				// Strip off any leading whitespace
				pszStrippedDisplayName = DisplayName.c_str();
				while (isspace (*pszStrippedDisplayName))
				{
					pszStrippedDisplayName++;
				}
				if (*pszStrippedDisplayName)
				{
					// Set the value
					*RemoteName = pszStrippedDisplayName;
				}
			}
		}
	}

STI_BAIL:

	if (pszDisplayName)
	{
		delete [] pszDisplayName;
		pszDisplayName = nullptr;
	}

	return hResult;
}
#undef SVRS_PREFIX
#undef SVRS_PREFIX_LENGTH
