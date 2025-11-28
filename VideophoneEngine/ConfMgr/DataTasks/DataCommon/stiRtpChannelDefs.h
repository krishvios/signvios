/*!
 * \file stiRtpChannelDefs.h
 * \brief Transitional file to help hold RtpChannel definitions while working
 *        towards collapsing all of the channel objects into data tasks
 */
#pragma once

#include <string>
#include <vector>
#include <list>
#include <map>
#include "nonstd/observer_ptr.h"
#include "SDESKey.h"
#include "stiSVX.h"
#include "rvsdpenums.h"
#include "rvnettypes.h"

#define RTP_TIMEOUT_SECONDS 45 // RFC 6263 section 7 says 15 seconds, our playback default is 28 seconds, and our sip keepalive is 45-85 seconds

const uint32_t un32AUDIO_RATE = 64000;
const uint32_t unT140_RATE = 1000;
const int nEXCEEDINGLY_HIGH_DATA_RATE = 999999999;	// This is a value higher than we expect to ever use for

enum ECodec
{
	estiCODEC_UNKNOWN,
	estiH263_VIDEO,
	estiH263_1998_VIDEO,
	estiH264_VIDEO,
	estiH265_VIDEO,
	estiG711_ALAW_56K_AUDIO,
	estiG711_ALAW_64K_AUDIO,
	estiG711_ULAW_56K_AUDIO,
	estiG711_ULAW_64K_AUDIO,
	estiG722_48K_AUDIO,
	estiG722_56K_AUDIO,
	estiG722_64K_AUDIO,
	estiT140_TEXT,
	estiT140_RED_TEXT,
	estiTELEPHONE_EVENT,
	estiRTX
};

enum EHoldLocation
{
	estiHOLD_LOCAL,
	estiHOLD_REMOTE,
	estiHOLD_DHV // We are holding the media for DHV call
};

struct SstiMediaPayload
{
	SstiMediaPayload (
		int8_t n8PayloadNbr,
		const std::string &name,
		ECodec eCodec,
		int bw,
		std::map<std::string,std::string> Attr,
		const std::list<RtcpFeedbackType> &rtcpFbTypes,
		EstiVideoProfile eProfile,
		EstiPacketizationScheme ePktScheme)
	:
		n8PayloadTypeNbr (n8PayloadNbr),
		payloadName (name),
		eCodec (eCodec),
		bandwidth (bw),
		mAttributes (Attr),
		rtcpFeedbackTypes (rtcpFbTypes),
		eVideoProfile (eProfile),
		ePacketizationScheme (ePktScheme)
	{ }

	int8_t n8PayloadTypeNbr {0};
	int8_t rtxPayloadType {INVALID_DYNAMIC_PAYLOAD_TYPE};
	std::string payloadName;
	ECodec eCodec {estiCODEC_UNKNOWN};
	int bandwidth {0};
	std::map<std::string,std::string> mAttributes;
	std::list<RtcpFeedbackType> rtcpFeedbackTypes;  // Supported RTCP-FB mechanisms
	EstiVideoProfile eVideoProfile {estiPROFILE_NONE};
	EstiPacketizationScheme ePacketizationScheme {estiPktUnknown};

	// ids of other payloads in this sdpstream that are related to this payload
	// types that act as additional features to whatever the media is (ex. DTMF)
	std::vector<int8_t> FeatureMediaOffersIds;
};

// NOTE: boost::optional<&> was considered, but std::optional does not allow reference types
// ("assignment rebind" arguments within the c++ committee)
using SstiMediaPayloadP = nonstd::observer_ptr<SstiMediaPayload>;
// Sometimes want a const version
using SstiMediaPayloadConstP = nonstd::observer_ptr<const SstiMediaPayload>;

struct SstiSdpStream
{
	SstiSdpStream (
		EstiMediaType eMediaType,
		const std::string &mediaTypeString,
		RvSdpProtocol mediaProtocol,
		const std::string &mediaProtocolString)
	:
		eType (eMediaType),
		MediaTypeString (mediaTypeString),
		MediaProtocol (mediaProtocol),
		MediaProtocolString (mediaProtocolString)
	{
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &RtpAddress);
		RvAddressConstruct (RV_ADDRESS_TYPE_IPV4, &RtcpAddress);
	}

	SstiMediaPayloadConstP mediaPayloadGet (int8_t payloadId) const
	{
		SstiMediaPayloadConstP mediaPayload;

		// NOTE: would normally make this const, but don't want to
		// complicate the OpRef type (or add another const type)
		for (const auto &item : Payloads)
		{
			if (item.n8PayloadTypeNbr == payloadId)
			{
				mediaPayload = nonstd::make_observer(&item);
				break;
			}
		}

		return mediaPayload;
	}

	EstiMediaType eType;
	std::string MediaTypeString;
	RvSdpProtocol MediaProtocol;
	std::string MediaProtocolString;
	RvAddress RtpAddress{};				// Contains the address and the port
	RvAddress RtcpAddress{};				// Contains the address and the port
	int8_t n8DefaultPayloadTypeNbr = 0;
	RvSdpConnectionMode eOfferDirection{RV_SDPCONNECTMODE_NOTSET};
	unsigned int nPtime = 0;
	// TODO: use std::map
	std::list<SstiMediaPayload> Payloads; // parsed media payloads from "other side" (offer or answer)
	std::string IceUserFragment;
	std::string IcePassword;
	std::vector<vpe::SDESKey> sdesKeys;
	vpe::HashFuncEnum FingerprintHashFunction{};
	std::vector<uint8_t> Fingerprint;
	RvSdpConnSetupRole SetupRole{};
};

using SstiSdpStreamUP = std::unique_ptr<SstiSdpStream>;
using SstiSdpStreamP = nonstd::observer_ptr<SstiSdpStream>;

struct SstiCaptureSizeTable
{
	unsigned int unCols;
	unsigned int unRows;
	int nDataRateMin; // The minimum data rate required before dropping to a lower resolution
};
