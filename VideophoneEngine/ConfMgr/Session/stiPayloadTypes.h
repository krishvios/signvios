////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: stiPayloadTypes
//
//  File Name:  stiPayloadTypes.h
//
//  Abstract:
//	Provides standard values and types for working with RTP (and SDP) payload types.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef __STI_PAYLOAD_TYPES_H_
#define __STI_PAYLOAD_TYPES_H_

#include <list>
#include <map>
#include "stiSVX.h"


#define PAYLOAD_TYPE_DTMF 101 // Some vendors are erroneously expecting DTMF on payload type 101


const int8_t n8DYNAMIC_PAYLOAD_BEGIN = 96;
const int8_t n8DYNAMIC_PAYLOAD_END = 127;


struct AudioPayloadAttributes
{
	EstiAudioCodec eCodec;
	EstiPacketizationScheme ePacketizationScheme;
	uint32_t clockRate{0};
};

typedef std::map<int8_t,AudioPayloadAttributes> TAudioPayloadMap;

struct TextPayloadAttributes
{
	EstiTextCodec eCodec;
	EstiPacketizationScheme ePacketizationScheme;
};

typedef std::map<int8_t,TextPayloadAttributes> TTextPayloadMap;

struct VideoPayloadAttributes
{
	EstiVideoCodec eCodec {estiVIDEO_NONE};
	EstiVideoProfile eProfile {estiPROFILE_NONE};
	EstiPacketizationScheme ePacketizationScheme {estiPktUnknown};
	std::list<RtcpFeedbackType> rtcpFeedbackTypes;  // Supported RTCP feedback mechanisms
	int8_t rtxPayloadType {INVALID_DYNAMIC_PAYLOAD_TYPE};
};

typedef std::map<int8_t,VideoPayloadAttributes> TVideoPayloadMap;

int8_t NextDynamicPayloadTypeGet ();

#endif // __STI_PAYLOAD_TYPES_H_
